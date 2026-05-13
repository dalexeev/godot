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

class TestCommandParser {
	String command;
	HashMap<String, String> options;
	int max_option_length = 0;

	String path;
	String source;
	HashSet<String> provided_options;

	void print_invalid_command() const {
		print_line(vformat(R"(Invalid command. Expected "<godot> --test %s [<options>] <script path>".)", command));
		print_line(vformat(R"(Enter "<godot> --test %s --help" for assistance.)", command));
	}

	void print_help() const {
		print_line(vformat(R"(Usage: "<godot> --test %s [<options>] <script path>".)", command));
		print_line("\nOptions:\n");
		if (options.is_empty()) {
			print_line("  (None.)");
		} else {
			const String format = vformat("  %%-%ds | %%s", max_option_length);
			for (const KeyValue<String, String> &kv : options) {
				print_line(vformat(format, kv.key, kv.value));
			}
		}
	}

public:
	void add_option(const String &p_option, const String &p_description) {
		ERR_FAIL_COND(p_option.is_empty());
		ERR_FAIL_COND(options.has(p_option));
		ERR_FAIL_COND(p_description.is_empty());

		options[p_option] = p_description;
		max_option_length = MAX(max_option_length, p_option.length());
	}

	// Expected:
	// <godot> --test <command> --help
	// <godot> --test <command> [<options>] <script path>
	bool parse_cmdline_args() {
		const List<String> args = OS::get_singleton()->get_cmdline_args();

		if (args.size() < 4) {
			print_invalid_command();
			return false;
		}

		int arg_index = 0;
		for (const String &arg : args) {
			if (arg_index == 2) {
				ERR_FAIL_COND_V(arg != command, false);
			}
			if (arg_index == 3 && arg == "--help") {
				print_help();
				return false;
			}
			if (arg_index >= args.size() - 1) {
				break;
			}

			if (arg_index >= 3) {
				if (!options.has(arg) || provided_options.has(arg)) {
					print_invalid_command();
					return false;
				}

				provided_options.insert(arg);
			}

			arg_index++;
		}

		const String raw_path = args.back()->get();
		const Ref<FileAccess> file = FileAccess::open(raw_path, FileAccess::READ);
		ERR_FAIL_COND_V_MSG(file.is_null(), false, vformat(R"(Could not open file "%s".)", raw_path));

		path = file->get_path_absolute();
		source = file->get_as_text();

		return true;
	}

	_FORCE_INLINE_ const String &get_path() const { return path; }
	_FORCE_INLINE_ const String &get_source() const { return source; }
	_FORCE_INLINE_ bool has_provided_option(const String &p_option) const { return provided_options.has(p_option); }

	TestCommandParser(const String &p_command) :
			command(p_command) {}
};

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
	TestCommandParser tcp("gdscript3-tokenizer");
	if (!tcp.parse_cmdline_args()) {
		return;
	}

	const String &source = tcp.get_source();
	const Vector<String> lines = source.split("\n");

	const String primary_separator = String::chr('=').repeat(80);
	const String secondary_separator = String::chr('-').repeat(80);
	const String indent = String::chr(' ').repeat(4);

	Tokenizer tokenizer(source);

	Tokenizer::Token token;
	do {
		token = tokenizer.scan();

		print_line(primary_separator);
		print_source_region(token.source_region, lines);

		String token_info = " ===> " + token.get_raw_name();
		switch (token.type) {
			case Tokenizer::Token::Type::ANNOTATION:
			case Tokenizer::Token::Type::IDENTIFIER:
			case Tokenizer::Token::Type::LITERAL:
			case Tokenizer::Token::Type::ERROR: {
				token_info += "(" + token.data.get_construct_string() + ")";
			} break;
			case Tokenizer::Token::Type::INDENTATION: {
				const Tokenizer::Indentation indentation = Tokenizer::Indentation::from_variant(token.data);

				String type;
				switch (indentation.type) {
					case Tokenizer::Indentation::Type::EMPTY:
						type = "EMPTY";
						break;
					case Tokenizer::Indentation::Type::SPACES:
						type = "SPACES";
						break;
					case Tokenizer::Indentation::Type::TABS:
						type = "TABS";
						break;
					case Tokenizer::Indentation::Type::MIXED:
						type = "MIXED";
						break;
				}

				token_info += vformat("(type = %s, column_width = %d)", type, indentation.column_width);
			} break;
			default: {
				if (unlikely(token.data.get_type() != Variant::NIL)) {
					print_line_rich("[color=red] !!!> Invalid token data.[/color]");
					token_info += "(" + token.data.get_construct_string() + ")";
				}
			} break;
		}

		print_line(token_info);

		for (const Tokenizer::Token::InnerError &inner_error : token.inner_errors) {
			print_line(secondary_separator);
			print_source_region(inner_error.source_region, lines);
			print_line(vformat(" ---> InnerError: %s", inner_error.message));
		}
	} while (token.type != Tokenizer::Token::Type::TK_EOF);
}

void test_command_parser() {
	TestCommandParser tcp("gdscript3-parser");
	tcp.add_option("--print-source-regions", "Print the source region of each node after the AST representation.");
	if (!tcp.parse_cmdline_args()) {
		return;
	}

	const String &path = tcp.get_path();
	const String &source = tcp.get_source();
	const Vector<String> lines = source.split("\n");
	const bool print_source_regions = tcp.has_provided_option("--print-source-regions");

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
	TestCommandParser tcp("gdscript3-analyzer");
	tcp.add_option("--print-extended-info", "Print extended information about each node.");
	if (!tcp.parse_cmdline_args()) {
		return;
	}

	const String &path = tcp.get_path();
	const String &source = tcp.get_source();
	const Vector<String> lines = source.split("\n");
	const bool print_extended_info = tcp.has_provided_option("--print-extended-info");

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
				} else if (p_node->get_type() == AST::Node::Type::ENUM) {
					AST::EnumNode *enum_node = static_cast<AST::EnumNode *>(p_node);
					node_info += " | underlying_datatype = " + enum_node->underlying_datatype.to_debug_string();
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
