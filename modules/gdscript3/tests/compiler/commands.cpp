/**************************************************************************/
/*  commands.cpp                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "commands.h"

#include "../../compiler/analyzer.h"
#include "../../compiler/ast_visitor.h"
#include "../../compiler/parser.h"
#include "../../compiler/tokenizer.h"
#include "ast_printer.h"
#include "print_utils.h"

#include "core/io/file_access.h"
#include "core/os/os.h"

namespace gdscript {

// Expected:
// <godot binary> --test gdscript3-<command> --help
// <godot binary> --test gdscript3-<command> [<options>] <script path>
static bool parse_test_cmdline_args(String &r_path, String &r_source, Vector<String> &r_options, bool &r_print_help) {
	const List<String> args = OS::get_singleton()->get_cmdline_args();

	if (args.size() < 4) {
		return false;
	}

	Vector<String> options;
	{
		int i = 0;
		for (const String &arg : args) {
			if (i >= 3) {
				if (i == 3 && arg == "--help") {
					r_print_help = true;
					return true;
				}
				if (i >= args.size() - 1) {
					break;
				}
				options.push_back(arg);
			}
			i++;
		}
	}

	const String path = args.back()->get();
	const Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(file.is_null(), false, vformat(R"(Could not open file "%s".)", path));

	r_path = file->get_path_absolute();
	r_source = file->get_as_text();
	r_options = options;
	return true;
}

static void print_dignostics(
		const DiagnosticList &p_diagnostic_list,
		const Vector<String> &p_source_lines,
		bool p_print_warnings) {
	const String separator = String::chr('-').repeat(80);

	const List<DiagnosticList::Error> &errors = p_diagnostic_list.get_errors();
	if (errors.is_empty()) {
		print_line(separator);
		print_line("No errors found.");
	} else {
		for (const DiagnosticList::Error &error : errors) {
			print_line(separator);
			print_source_region(error.source_region, p_source_lines, p_diagnostic_list.get_unsafe_lines());
			print_line(vformat(" ---> Error: %s", error.message));
		}
	}

	if (!p_print_warnings) {
		return;
	}

	const List<DiagnosticList::Warning> &warnings = p_diagnostic_list.get_warnings();
	if (warnings.is_empty()) {
		print_line(separator);
		print_line("No warnings found.");
	} else {
		for (const DiagnosticList::Warning &warning : warnings) {
			print_line(separator);
			print_source_region(warning.source_region, p_source_lines, p_diagnostic_list.get_unsafe_lines());
			print_line(vformat(" ---> Warning: %s", WarningDB::get_message(warning.code, warning.symbols)));
		}
	}
}

void test_command_tokenizer() {
	String path;
	String source;
	Vector<String> options;
	bool print_help = false;
	if (!parse_test_cmdline_args(path, source, options, print_help)) {
		print_line(R"(Invalid command. Expected "<godot binary> --test gdscript3-tokenizer [<options>] <script path>".)");
		print_line(R"(Enter "<godot binary> --test gdscript3-tokenizer --help" for assistance.)");
		return;
	}

	if (print_help) {
		print_line(R"(Expected format: "<godot binary> --test gdscript3-tokenizer [<options>] <script path>".)");
		print_line("\nOptions:\n");
		print_line("  --interactive    | Activate interactive mode.");
		print_line("  --print-comments | Print comments as they are found in the source code.");
		return;
	}

	const Vector<String> lines = source.split("\n");
	const bool interactive = options.has("--interactive");
	const bool print_comments = options.has("--print-comments");

	if (interactive) {
		print_line_rich(R"([color=blue]Interactive mode is active. Enter "h" for assistance.[/color])");
	}

	const String primary_separator = String::chr('=').repeat(80);
	const String secondary_separator = String::chr('-').repeat(80);
	const String indent = String::chr(' ').repeat(4);

	Tokenizer tokenizer(source);
	unsigned last_comment_count = 0;
	int last_comment_line = 0;

	Tokenizer::Token token;
	do {
		if (interactive) {
			print_raw("\u001b[90m >>>>\u001b[39m ");
			const String line = OS::get_singleton()->get_stdin_string().strip_edges();
			if (line.is_empty()) {
				// Scan next token.
			} else {
				if (line == "m") {
					if (tokenizer.is_multiline_mode()) {
						print_line_rich(indent, "[color=red]Multiline mode is already enabled.[/color]");
					} else {
						tokenizer.set_multiline_mode(true);
						print_line_rich(indent, "[color=green]Multiline mode is enabled.[/color]");
					}
				} else if (line == "s") {
					if (tokenizer.is_multiline_mode()) {
						tokenizer.set_multiline_mode(false);
						print_line_rich(indent, "[color=green]Multiline mode is disabled.[/color]");
					} else {
						print_line_rich(indent, "[color=red]Multiline mode is already disabled.[/color]");
					}
				} else if (line == "b+") {
					tokenizer.push_expression_indented_block();
					const int stack_size = tokenizer.get_expression_indented_block_depth();
					print_line_rich(indent, vformat("[color=green]The stack size is now %d.[/color]", stack_size));
				} else if (line == "b-") {
					if (tokenizer.get_expression_indented_block_depth() > 0) {
						tokenizer.pop_expression_indented_block();
						const int stack_size = tokenizer.get_expression_indented_block_depth();
						print_line_rich(indent, vformat("[color=green]The stack size is now %d.[/color]", stack_size));
					} else {
						print_line_rich(indent, "[color=red]The stack is empty.[/color]");
					}
				} else if (line == "h") {
					print_line_rich(indent, "[color=blue]   | Scan next token.[/color]");
					print_line_rich(indent, "[color=blue]m  | Enable multiline mode.[/color]");
					print_line_rich(indent, "[color=blue]s  | Disable multiline mode.[/color]");
					print_line_rich(indent, "[color=blue]b+ | Push indented block.[/color]");
					print_line_rich(indent, "[color=blue]b- | Pop indented block.[/color]");
					print_line_rich(indent, "[color=blue]h  | Print help information.[/color]");
				} else {
					print_line_rich(indent, R"([color=red]Invalid command. Enter "h" for assistance.[/color])");
				}
				continue;
			}
		}

		token = tokenizer.scan();

		const HashMap<int, Comment> &comments = tokenizer.get_comments();
		if (print_comments && comments.size() != last_comment_count) {
			for (const KeyValue<int, Comment> &kv : tokenizer.get_comments()) {
				if (kv.key <= last_comment_line) {
					continue;
				}

				print_line(primary_separator);
				print_source_region(kv.value.source_region, lines);
				print_line(vformat(" ===> Comment | is_inline = %s", kv.value.is_inline));

				if (interactive) {
					print_raw("\u001b[90mPress Enter...\u001b[39m");
					(void)OS::get_singleton()->get_stdin_string();
				}

				last_comment_line = kv.key;
			}
			last_comment_count = comments.size();
		}

		print_line(primary_separator);
		print_source_region(token.source_region, lines);

		String token_info = " ===> " + token.get_name();
		switch (token.type) {
			case Tokenizer::Token::Type::ANNOTATION:
			case Tokenizer::Token::Type::IDENTIFIER:
			case Tokenizer::Token::Type::LITERAL:
			case Tokenizer::Token::Type::ERROR:
				token_info += "(" + token.data.get_construct_string() + ")";
				break;
			default:
				break;
		}
		token_info += vformat(" | is_inline = %s", token.is_inline);

		print_line(token_info);

		for (const Tokenizer::Token::InnerError &inner_error : token.inner_errors) {
			if (interactive) {
				print_raw("\u001b[90mPress Enter...\u001b[39m");
				(void)OS::get_singleton()->get_stdin_string();
			}

			print_line(secondary_separator);
			print_source_region(inner_error.source_region, lines);
			print_line(vformat(" ---> InnerError: %s", inner_error.message));
		}
	} while (token.type != Tokenizer::Token::Type::EOF_);

	if (!print_comments && !tokenizer.get_comments().is_empty()) {
		print_line(String());
		print_line_rich(vformat(
				R"([color=blue]Found %d comment(s). Use "--print-comments" for details.[/color])",
				tokenizer.get_comments().size()));
	}
}

void test_command_parser() {
	String path;
	String source;
	Vector<String> options;
	bool print_help = false;
	if (!parse_test_cmdline_args(path, source, options, print_help)) {
		print_line(R"(Invalid command. Expected "<godot binary> --test gdscript3-parser [<options>] <script path>".)");
		print_line(R"(Enter "<godot binary> --test gdscript3-parser --help" for assistance.)");
		return;
	}

	if (print_help) {
		print_line(R"(Expected format: "<godot binary> --test gdscript3-parser [<options>] <script path>".)");
		print_line("\nOptions:\n");
		print_line("  --print-source-regions | Print the source region of each node after the AST representation.");
		return;
	}

	const Vector<String> lines = source.split("\n");
	const bool print_source_regions = options.has("--print-source-regions");

	const String separator = String::chr('-').repeat(80);

	Parser parser(path, source);
	parser.parse();

	const AST &ast = parser.get_ast();
	const DiagnosticList &diagnostic_list = ast.get_diagnostic_list();

	ASTPrinter ast_printer(false);
	ast_printer.print_ast(ast);

	LocalVector<AST::Node *> node_stack;
	int invalid_parent_count = 0;

	ASTVisitor::Callback callback = [&](bool p_enter_node, AST::Node *p_node, const AST::Context &p_context) {
		if (p_enter_node) {
			if (print_source_regions) {
				String node_info = p_node->get_node_raw_type_name();
				if (p_node->get_type() == AST::Node::Type::IDENTIFIER) {
					AST::IdentifierNode *id = static_cast<AST::IdentifierNode *>(p_node);
					node_info += "(" + Variant(id->name).get_construct_string() + ")";
				}

				print_line(separator);
				print_source_region(p_node->source_region, lines);
				print_line(" ---> " + node_info);
			}

			AST::Node *expected_parent = node_stack.is_empty() ? nullptr : node_stack[node_stack.size() - 1];
			if (p_node->parent_node != expected_parent) {
				invalid_parent_count++;
				if (print_source_regions) {
					print_line_rich("[color=red] !!!> Invalid parent node.[/color]");
				}
			}

			node_stack.push_back(p_node);
		} else {
			node_stack.resize(node_stack.size() - 1);
		}

		return true;
	};

	ASTVisitor ast_visitor(callback);
	ast_visitor.visit_ast(ast);

	print_dignostics(diagnostic_list, lines, false);

	if (!print_source_regions && invalid_parent_count > 0) {
		print_line(String());
		print_line_rich(vformat(
				R"([color=red]Found %d node(s) with invalid parent. Use "--print-source-regions" for details.[/color])",
				invalid_parent_count));
	}
}

void test_command_analyzer() {
	String path;
	String source;
	Vector<String> options;
	bool print_help = false;
	if (!parse_test_cmdline_args(path, source, options, print_help)) {
		print_line(R"(Invalid command. Expected "<godot binary> --test gdscript3-analyzer [<options>] <script path>".)");
		print_line(R"(Enter "<godot binary> --test gdscript3-analyzer --help" for assistance.)");
		return;
	}

	if (print_help) {
		print_line(R"(Expected format: "<godot binary> --test gdscript3-analyzer [<options>] <script path>".)");
		print_line("\nOptions:\n");
		print_line("  --print-extended-info | Print extended information about each node.");
		return;
	}

	const Vector<String> lines = source.split("\n");
	const bool print_extended_info = options.has("--print-extended-info");

	const String separator = String::chr('-').repeat(80);

	Parser parser(path, source);
	parser.parse();

	const AST &ast = parser.get_ast();
	const DiagnosticList &diagnostic_list = ast.get_diagnostic_list();

	if (!diagnostic_list.get_errors().is_empty()) {
		print_line(separator);
		print_line_rich("[color=yellow]NOTE: The script has parser errors.[/color]");
	}

	Analyzer analyzer(ast, false);
	analyzer.resolve_body();

	ASTPrinter ast_printer(true);
	ast_printer.print_ast(ast);

	if (print_extended_info) {
		ASTVisitor::Callback callback = [&](bool p_enter_node, AST::Node *p_node, const AST::Context &p_context) {
			if (p_enter_node) {
				String node_info = p_node->get_node_raw_type_name();
				if (p_node->get_type() == AST::Node::Type::IDENTIFIER) {
					AST::IdentifierNode *id = static_cast<AST::IdentifierNode *>(p_node);
					node_info += "(" + Variant(id->name).get_construct_string() + ")";
				}

				if (p_node->is_expression()) {
					AST::ExpressionNode *expression = static_cast<AST::ExpressionNode *>(p_node);
					node_info += " | datatype = " + expression->datatype.to_debug_string();
					if (expression->is_any_degree_constant()) {
						node_info += " | " + String(expression->is_fully_constant() ? "full" : "partial");
						node_info += " constant (" + expression->constant_value.get_construct_string() + ")";
					}
				} else if (p_node->is_assignable()) {
					AST::AssignableNode *assignable = static_cast<AST::AssignableNode *>(p_node);
					node_info += " | datatype = " + assignable->datatype.to_debug_string();
				} else if (p_node->get_type() == AST::Node::Type::TYPE) {
					AST::TypeNode *type = static_cast<AST::TypeNode *>(p_node);
					node_info += " | denoted_datatype = " + type->denoted_datatype.to_debug_string();
				} else if (p_node->get_type() == AST::Node::Type::FUNCTION) {
					AST::FunctionNode *function = static_cast<AST::FunctionNode *>(p_node);
					node_info += " | return_datatype = " + function->return_datatype.to_debug_string();
				} else if (p_node->get_type() == AST::Node::Type::FOR) {
					AST::ForNode *for_node = static_cast<AST::ForNode *>(p_node);
					node_info += " | iterator_datatype = " + for_node->iterator_datatype.to_debug_string();
				}

				print_line(separator);
				print_source_region(p_node->source_region, lines, diagnostic_list.get_unsafe_lines());
				print_line(" ---> " + node_info);
			}

			return true;
		};

		ASTVisitor ast_visitor(callback);
		ast_visitor.visit_ast(ast);
	}

	print_dignostics(diagnostic_list, lines, true);
}

} // namespace gdscript
