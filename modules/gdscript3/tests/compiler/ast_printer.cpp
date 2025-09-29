/**************************************************************************/
/*  ast_printer.cpp                                                       */
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

#include "ast_printer.h"

using namespace gdscript;

void ASTPrinter::push_text(const String &p_text) {
	if (pending_newline) {
		output += "\n";
		output += indent_string;
		pending_newline = false;
	}
	output += p_text.replace("\n", "\\n");
}

void ASTPrinter::push_newline() {
	pending_newline = true;
}

void ASTPrinter::start_block() {
	indent_level++;
	indent_string = String("|   ").repeat(indent_level);
	pending_newline = true;
}

void ASTPrinter::end_block() {
	ERR_FAIL_COND(indent_level <= 0);

	indent_level--;
	indent_string = String("|   ").repeat(indent_level);
	pending_newline = true;
}

void ASTPrinter::output_node(AST::Node *p_node) {
	ERR_FAIL_NULL(p_node);

	if (p_node->is_declaration()) {
		for (AST::AnnotationNode *annotation : p_node->annotations) {
			output_annotation(annotation);
		}
	}

	switch (p_node->get_type()) {
		using NodeType = AST::Node::Type;

		case NodeType::NONE:
			// Unreachable.
			break;
		case NodeType::ANNOTATION:
			output_annotation(static_cast<AST::AnnotationNode *>(p_node));
			break;
		case NodeType::ARRAY:
			output_array(static_cast<AST::ArrayNode *>(p_node));
			break;
		case NodeType::ASSERT:
			output_assert(static_cast<AST::AssertNode *>(p_node));
			break;
		case NodeType::ASSIGNMENT:
			output_assignment(static_cast<AST::AssignmentNode *>(p_node));
			break;
		case NodeType::AWAIT:
			output_await(static_cast<AST::AwaitNode *>(p_node));
			break;
		case NodeType::BINARY_OPERATOR:
			output_binary_operator(static_cast<AST::BinaryOperatorNode *>(p_node));
			break;
		case NodeType::BLOCK:
			output_block(static_cast<AST::BlockNode *>(p_node));
			break;
		case NodeType::BREAK:
			output_break(static_cast<AST::BreakNode *>(p_node));
			break;
		case NodeType::BREAKPOINT:
			output_breakpoint(static_cast<AST::BreakpointNode *>(p_node));
			break;
		case NodeType::CALL:
			output_call(static_cast<AST::CallNode *>(p_node));
			break;
		case NodeType::CAST:
			output_cast(static_cast<AST::CastNode *>(p_node));
			break;
		case NodeType::CLASS:
			output_class(static_cast<AST::ClassNode *>(p_node));
			break;
		case NodeType::CONSTANT:
			output_constant(static_cast<AST::ConstantNode *>(p_node));
			break;
		case NodeType::CONTINUE:
			output_continue(static_cast<AST::ContinueNode *>(p_node));
			break;
		case NodeType::DICTIONARY:
			output_dictionary(static_cast<AST::DictionaryNode *>(p_node));
			break;
		case NodeType::ENUM:
			output_enum(static_cast<AST::EnumNode *>(p_node));
			break;
		case NodeType::FOR:
			output_for(static_cast<AST::ForNode *>(p_node));
			break;
		case NodeType::FUNCTION:
			output_function(static_cast<AST::FunctionNode *>(p_node));
			break;
		case NodeType::GET_NODE:
			output_get_node(static_cast<AST::GetNodeNode *>(p_node));
			break;
		case NodeType::IDENTIFIER:
			output_identifier(static_cast<AST::IdentifierNode *>(p_node));
			break;
		case NodeType::IF:
			output_if(static_cast<AST::IfNode *>(p_node));
			break;
		case NodeType::LAMBDA:
			output_lambda(static_cast<AST::LambdaNode *>(p_node));
			break;
		case NodeType::LITERAL:
			output_literal(static_cast<AST::LiteralNode *>(p_node));
			break;
		case NodeType::MATCH:
			output_match(static_cast<AST::MatchNode *>(p_node));
			break;
		case NodeType::MATCH_BRANCH:
			output_match_branch(static_cast<AST::MatchBranchNode *>(p_node));
			break;
		case NodeType::MATCH_PATTERN:
			output_match_pattern(static_cast<AST::MatchPatternNode *>(p_node));
			break;
		case NodeType::PARAMETER:
			output_parameter(static_cast<AST::ParameterNode *>(p_node));
			break;
		case NodeType::RETURN:
			output_return(static_cast<AST::ReturnNode *>(p_node));
			break;
		case NodeType::ROOT:
			output_root(static_cast<AST::RootNode *>(p_node));
			break;
		case NodeType::SELF:
			output_self(static_cast<AST::SelfNode *>(p_node));
			break;
		case NodeType::SIGNAL:
			output_signal(static_cast<AST::SignalNode *>(p_node));
			break;
		case NodeType::SUBSCRIPT:
			output_subscript(static_cast<AST::SubscriptNode *>(p_node));
			break;
		case NodeType::SUPER:
			output_super(static_cast<AST::SuperNode *>(p_node));
			break;
		case NodeType::TERNARY_OPERATOR:
			output_ternary_operator(static_cast<AST::TernaryOperatorNode *>(p_node));
			break;
		case NodeType::TYPE:
			output_type(static_cast<AST::TypeNode *>(p_node));
			break;
		case NodeType::TYPE_ALIAS:
			output_type_alias(static_cast<AST::TypeAliasNode *>(p_node));
			break;
		case NodeType::TYPE_TEST:
			output_type_test(static_cast<AST::TypeTestNode *>(p_node));
			break;
		case NodeType::UNARY_OPERATOR:
			output_unary_operator(static_cast<AST::UnaryOperatorNode *>(p_node));
			break;
		case NodeType::VARIABLE:
			output_variable(static_cast<AST::VariableNode *>(p_node));
			break;
		case NodeType::WHILE:
			output_while(static_cast<AST::WhileNode *>(p_node));
			break;
	}
}

void ASTPrinter::output_expression(AST::ExpressionNode *p_expression) {
	if (p_expression == nullptr) {
		push_text("<invalid expression>");
		return;
	}

	output_node(p_expression);
}

void ASTPrinter::output_annotation(AST::AnnotationNode *p_annotation) {
	ERR_FAIL_NULL(p_annotation);

	push_text(p_annotation->name.is_empty() ? "<empty annotation>" : p_annotation->name);
	if (!p_annotation->arguments.is_empty()) {
		push_text("(");
		start_block();
		for (AST::ExpressionNode *argument : p_annotation->arguments) {
			output_expression(argument);
			push_text(",");
			push_newline();
		}
		end_block();
		push_text(")");
	}
	push_newline();
}

void ASTPrinter::output_array(AST::ArrayNode *p_array) {
	if (p_array->elements.is_empty()) {
		push_text("[]");
	} else {
		push_text("[");
		start_block();
		for (AST::ExpressionNode *element : p_array->elements) {
			output_expression(element);
			push_text(",");
			push_newline();
		}
		end_block();
		push_text("]");
	}
}

void ASTPrinter::output_assert(AST::AssertNode *p_assert) {
	push_text("assert(");
	output_expression(p_assert->condition);
	if (p_assert->message != nullptr) {
		push_text(", ");
		output_expression(p_assert->message);
	}
	push_text(")");
	push_newline();
}

void ASTPrinter::output_assignment(AST::AssignmentNode *p_assignment) {
	bool is_statement = false;
	if (p_assignment->parent_node != nullptr) {
		is_statement = p_assignment->parent_node->get_type() == AST::Node::Type::BLOCK;
	}

	if (!is_statement) {
		push_text("(");
	}

	output_expression(p_assignment->assignee);
	push_text(" ");
	push_text(p_assignment->get_operator_name());
	push_text(" ");
	output_expression(p_assignment->assigned_value);

	if (is_statement) {
		push_newline();
	} else {
		push_text(")");
	}
}

void ASTPrinter::output_await(AST::AwaitNode *p_await) {
	push_text("(await ");
	output_expression(p_await->operand);
	push_text(")");
}

void ASTPrinter::output_binary_operator(AST::BinaryOperatorNode *p_binary_operator) {
	push_text("(");
	output_expression(p_binary_operator->left_operand);
	push_text(" ");
	push_text(p_binary_operator->get_operator_name());
	push_text(" ");
	output_expression(p_binary_operator->right_operand);
	push_text(")");
}

void ASTPrinter::output_block(AST::BlockNode *p_block) {
	start_block();
	if (p_block == nullptr) {
		push_text("<invalid block>");
	} else {
		if (p_block->statements.is_empty()) {
			push_text("pass");
		} else {
			for (AST::Node *statement : p_block->statements) {
				ERR_CONTINUE(statement == nullptr);
				for (AST::AnnotationNode *annotation : statement->annotations) {
					output_annotation(annotation);
				}
				output_node(statement);
				if (statement->is_expression()) {
					push_newline();
				}
			}
		}
	}
	end_block();
}

void ASTPrinter::output_break(AST::BreakNode *p_break) {
	push_text("break");
	push_newline();
}

void ASTPrinter::output_breakpoint(AST::BreakpointNode *p_breakpoint) {
	push_text("breakpoint");
	push_newline();
}

void ASTPrinter::output_call(AST::CallNode *p_call) {
	output_expression(p_call->callee);
	if (p_call->arguments.is_empty()) {
		push_text("()");
	} else {
		push_text("(");
		start_block();
		for (AST::ExpressionNode *argument : p_call->arguments) {
			output_expression(argument);
			push_text(",");
			push_newline();
		}
		end_block();
		push_text(")");
	}
}

void ASTPrinter::output_cast(AST::CastNode *p_cast) {
	push_text("(");
	output_expression(p_cast->operand);
	push_text(" as ");
	output_type(p_cast->cast_type);
	push_text(")");
}

void ASTPrinter::output_class(AST::ClassNode *p_class) {
	ERR_FAIL_NULL(p_class);

	push_text("class");
	if (p_class->identifier != nullptr) {
		push_text(" ");
		output_identifier(p_class->identifier);
	}
	if (p_class->extends_type != nullptr) {
		push_text(" extends ");
		output_type(p_class->extends_type);
	}
	push_text(":");

	start_block();
	if (p_class->members.is_empty()) {
		push_text("pass");
	} else {
		for (AST::Node *member : p_class->members) {
			output_node(member);
		}
	}
	end_block();
}

void ASTPrinter::output_constant(AST::ConstantNode *p_constant) {
	push_text("const ");
	output_identifier(p_constant->identifier);
	if (use_deduced_types) {
		push_text(": ");
		push_text(p_constant->datatype.to_debug_string());
		if (p_constant->initializer != nullptr) {
			push_text(" = ");
			output_expression(p_constant->initializer);
		}
	} else {
		if (p_constant->type_specifier != nullptr) {
			push_text(": ");
			output_type(p_constant->type_specifier);
		}
		if (p_constant->initializer != nullptr) {
			push_text(p_constant->infer_datatype ? " := " : " = ");
			output_expression(p_constant->initializer);
		}
	}
	push_newline();
}

void ASTPrinter::output_continue(AST::ContinueNode *p_continue) {
	push_text("continue");
	push_newline();
}

void ASTPrinter::output_dictionary(AST::DictionaryNode *p_dictionary) {
	if (p_dictionary->elements.is_empty()) {
		push_text("{}");
	} else {
		String separator;
		switch (p_dictionary->style) {
			case AST::DictionaryNode::Style::PYTHON_DICT:
				separator = ": ";
				break;
			case AST::DictionaryNode::Style::LUA_TABLE:
				separator = " = ";
				break;
		}

		push_text("{");
		start_block();
		for (const AST::DictionaryNode::Pair &pair : p_dictionary->elements) {
			output_expression(pair.key);
			push_text(separator);
			output_expression(pair.value);
			push_text(",");
			push_newline();
		}
		end_block();
		push_text("}");
	};
}

void ASTPrinter::output_enum(AST::EnumNode *p_enum) {
	push_text("enum");
	if (p_enum->identifier != nullptr) {
		push_text(" ");
		output_identifier(p_enum->identifier);
	}

	if (use_deduced_types) {
		push_text(": ");
		push_text(p_enum->underlying_datatype.to_debug_string());
	} else {
		if (p_enum->underlying_type != nullptr) {
			push_text(": ");
			output_type(p_enum->underlying_type);
		}
	}

	if (p_enum->items.is_empty()) {
		push_text(" {}");
	} else {
		push_text(" {");
		start_block();
		for (const AST::EnumNode::Item &item : p_enum->items) {
			output_identifier(item.identifier);
			if (item.custom_value != nullptr) {
				push_text(" = ");
				output_expression(item.custom_value);
			}
			push_text(",");
			push_newline();
		}
		end_block();
		push_text("}");
	}
	push_newline();
}

void ASTPrinter::output_for(AST::ForNode *p_for) {
	push_text("for ");
	output_identifier(p_for->iterator);
	if (use_deduced_types) {
		push_text(": ");
		push_text(p_for->iterator_datatype.to_debug_string());
	} else {
		if (p_for->iterator_type != nullptr) {
			push_text(": ");
			output_type(p_for->iterator_type);
		}
	}
	push_text(" in ");
	output_expression(p_for->iterable);
	push_text(":");
	output_block(p_for->loop);
}

void ASTPrinter::output_function(AST::FunctionNode *p_function, bool p_is_property) {
	if (p_is_property) {
		output_identifier(p_function->identifier);
	} else {
		push_text("func ");
		if (p_function->identifier != nullptr) {
			output_identifier(p_function->identifier);
		}
	}

	push_text("(");
	if (!p_function->parameters.is_empty()) {
		start_block();
		for (AST::ParameterNode *parameter : p_function->parameters) {
			output_parameter(parameter);
			push_text(",");
			push_newline();
		}
		end_block();
	}
	push_text(")");

	if (use_deduced_types) {
		push_text(" -> ");
		push_text(p_function->return_datatype.to_debug_string());
	} else {
		if (p_function->return_type != nullptr) {
			push_text(" -> ");
			output_type(p_function->return_type);
		}
	}

	if (p_function->body == nullptr) {
		push_newline();
	} else {
		push_text(":");
		output_block(p_function->body);
	}
}

void ASTPrinter::output_get_node(AST::GetNodeNode *p_get_node) {
	push_text(p_get_node->use_dollar ? "$" : "%");
	push_text(p_get_node->full_path.c_escape().quote());
}

void ASTPrinter::output_identifier(AST::IdentifierNode *p_identifier) {
	if (p_identifier == nullptr) {
		push_text("<invalid identifier>");
	} else {
		push_text(p_identifier->name.is_empty() ? "<empty identifier>" : p_identifier->name);
	}
}

void ASTPrinter::output_if(AST::IfNode *p_if) {
	push_text("if ");
	output_expression(p_if->condition);
	push_text(":");
	output_block(p_if->true_block);
	if (p_if->false_block) {
		push_text("else:");
		output_block(p_if->false_block);
	}
}

void ASTPrinter::output_lambda(AST::LambdaNode *p_lambda) {
	if (p_lambda->function == nullptr) {
		push_text("<invalid lambda>");
	} else {
		push_text("(");
		output_function(p_lambda->function);
		push_text(")");
	}
}

void ASTPrinter::output_literal(AST::LiteralNode *p_literal) {
	if (p_literal == nullptr) {
		push_text("<invalid literal>");
		return;
	}

	if (p_literal->value.get_type() == Variant::STRING) {
		push_text(p_literal->value.operator String().c_escape().quote());
	} else {
		push_text(p_literal->value.get_construct_string());
	}
}

void ASTPrinter::output_match(AST::MatchNode *p_match) {
	push_text("match ");
	output_expression(p_match->test);
	push_text(":");
	start_block();
	if (p_match->branches.is_empty()) {
		push_text("pass");
	} else {
		for (AST::MatchBranchNode *branch : p_match->branches) {
			output_match_branch(branch);
		}
	}
	end_block();
}

void ASTPrinter::output_match_branch(AST::MatchBranchNode *p_match_branch) {
	ERR_FAIL_NULL(p_match_branch);

	for (unsigned i = 0; i < p_match_branch->patterns.size(); i++) {
		AST::MatchPatternNode *pattern = p_match_branch->patterns[i];
		if (i > 0) {
			push_text(", ");
		}
		output_match_pattern(pattern);
	}
	if (p_match_branch->pattern_guard != nullptr) {
		push_text(" when ");
		output_expression(p_match_branch->pattern_guard);
	}
	push_text(":");
	output_block(p_match_branch->block);
}

void ASTPrinter::output_match_pattern(AST::MatchPatternNode *p_match_pattern) {
	ERR_FAIL_NULL(p_match_pattern);

	switch (p_match_pattern->pattern_type) {
		case AST::MatchPatternNode::PatternType::EXPRESSION:
			output_expression(p_match_pattern->expression);
			break;
		case AST::MatchPatternNode::PatternType::BIND:
			push_text("var ");
			output_identifier(p_match_pattern->bind);
			break;
		case AST::MatchPatternNode::PatternType::ARRAY:
			if (p_match_pattern->elements.is_empty()) {
				push_text("[]");
			} else {
				push_text("[");
				start_block();
				for (const AST::MatchPatternNode::Element &element : p_match_pattern->elements) {
					if (element.is_rest) {
						push_text("..");
					} else {
						output_match_pattern(element.value);
					}
					push_text(",");
					push_newline();
				}
				end_block();
				push_text("]");
			}
			break;
		case AST::MatchPatternNode::PatternType::DICTIONARY:
			if (p_match_pattern->elements.is_empty()) {
				push_text("{}");
			} else {
				push_text("{");
				start_block();
				for (const AST::MatchPatternNode::Element &element : p_match_pattern->elements) {
					if (element.is_rest) {
						push_text("..");
					} else {
						output_expression(element.key);
						push_text(": ");
						output_match_pattern(element.value);
					}
					push_text(",");
					push_newline();
				}
				end_block();
				push_text("}");
			}
			break;
		case AST::MatchPatternNode::PatternType::WILDCARD:
			push_text("_");
			break;
	}
}

void ASTPrinter::output_parameter(AST::ParameterNode *p_parameter) {
	ERR_FAIL_NULL(p_parameter);

	if (p_parameter->is_rest) {
		push_text("...");
	}
	output_identifier(p_parameter->identifier);

	if (use_deduced_types) {
		push_text(": ");
		push_text(p_parameter->datatype.to_debug_string());
		if (p_parameter->initializer != nullptr) {
			push_text(" = ");
			output_expression(p_parameter->initializer);
		}
	} else {
		if (p_parameter->type_specifier != nullptr) {
			push_text(": ");
			output_type(p_parameter->type_specifier);
		}
		if (p_parameter->initializer != nullptr) {
			push_text(p_parameter->infer_datatype ? " := " : " = ");
			output_expression(p_parameter->initializer);
		}
	}
}

void ASTPrinter::output_return(AST::ReturnNode *p_return) {
	push_text("return");
	if (p_return->return_value != nullptr) {
		push_text(" ");
		output_expression(p_return->return_value);
	}
	push_newline();
}

void ASTPrinter::output_root(AST::RootNode *p_root) {
	if (!p_root->annotations.is_empty()) {
		print_line("Script annotations:");
		for (AST::AnnotationNode *annotation : p_root->annotations) {
			output_annotation(annotation);
			print_line(vformat("Line %d: %s", annotation->source_region.start.line, output.as_string()));
			clear_output();
		}
		print_line(String());
	}
	output_class(p_root->main_class);
}

void ASTPrinter::output_self(AST::SelfNode *p_self) {
	push_text("self");
}

void ASTPrinter::output_signal(AST::SignalNode *p_signal) {
	push_text("signal ");
	output_identifier(p_signal->identifier);
	if (p_signal->parameters.is_empty()) {
		push_text("()");
	} else {
		push_text("(");
		start_block();
		for (AST::ParameterNode *parameter : p_signal->parameters) {
			output_parameter(parameter);
			push_text(",");
			push_newline();
		}
		end_block();
		push_text(")");
	}
	push_newline();
}

void ASTPrinter::output_subscript(AST::SubscriptNode *p_subscript) {
	push_text("(");
	output_expression(p_subscript->base);
	if (p_subscript->is_attribute) {
		push_text(".");
		output_identifier(p_subscript->attribute);
	} else {
		push_text("[");
		output_expression(p_subscript->index);
		push_text("]");
	}
	push_text(")");
}

void ASTPrinter::output_super(AST::SuperNode *p_super) {
	push_text("super");
	if (p_super->identifier != nullptr) {
		push_text(".");
		output_identifier(p_super->identifier);
	}
}

void ASTPrinter::output_ternary_operator(AST::TernaryOperatorNode *p_ternary_operator) {
	push_text("(");
	output_expression(p_ternary_operator->true_expr);
	push_text(" if ");
	output_expression(p_ternary_operator->condition);
	push_text(" else ");
	output_expression(p_ternary_operator->false_expr);
	push_text(")");
}

void ASTPrinter::output_type(AST::TypeNode *p_type) {
	if (p_type == nullptr) {
		push_text("<invalid type>");
		return;
	}

	if (use_deduced_types) {
		push_text(p_type->denoted_datatype.to_debug_string());
		return;
	}

	if (p_type->outer_type != nullptr) {
		push_text("(");
		output_type(p_type->outer_type);
		push_text(").");
	}

	if (p_type->is_file_path) {
		output_literal(p_type->file_path);
	} else {
		output_identifier(p_type->identifier);
	}

	if (!p_type->parameters.is_empty()) {
		push_text("[");
		for (unsigned i = 0; i < p_type->parameters.size(); i++) {
			AST::TypeNode *parameter = p_type->parameters[i];
			if (i > 0) {
				push_text(", ");
			}
			output_type(parameter);
		}
		push_text("]");
	}
}

void ASTPrinter::output_type_alias(AST::TypeAliasNode *p_type_alias) {
	push_text("using ");
	output_identifier(p_type_alias->identifier);
	push_text(" = ");
	output_type(p_type_alias->source_type);
	push_newline();
}

void ASTPrinter::output_type_test(AST::TypeTestNode *p_type_test) {
	push_text("(");
	output_expression(p_type_test->operand);
	push_text(" is ");
	output_type(p_type_test->test_type);
	push_text(")");
}

void ASTPrinter::output_unary_operator(AST::UnaryOperatorNode *p_unary_operator) {
	push_text("(");
	push_text(p_unary_operator->get_operator_name());
	push_text(" ");
	output_expression(p_unary_operator->operand);
	push_text(")");
}

void ASTPrinter::output_variable(AST::VariableNode *p_variable) {
	push_text("var ");
	output_identifier(p_variable->identifier);

	if (use_deduced_types) {
		push_text(": ");
		push_text(p_variable->datatype.to_debug_string());
		if (p_variable->initializer != nullptr) {
			push_text(" = ");
			output_expression(p_variable->initializer);
		}
	} else {
		if (p_variable->type_specifier != nullptr) {
			push_text(": ");
			output_type(p_variable->type_specifier);
		}
		if (p_variable->initializer != nullptr) {
			push_text(p_variable->infer_datatype ? " := " : " = ");
			output_expression(p_variable->initializer);
		}
	}

	using PropertyStyle = AST::VariableNode::PropertyStyle;
	if (p_variable->property_style == PropertyStyle::NONE) {
		push_newline();
	} else {
		push_text(":");
		start_block();
		switch (p_variable->property_style) {
			case PropertyStyle::NONE:
				break; // Unreachable.
			case PropertyStyle::INLINE:
				for (const AST::VariableNode::Property &property : p_variable->properties) {
					output_function(property.inline_function, true);
				}
				break;
			case PropertyStyle::POINTER:
				for (unsigned i = 0; i < p_variable->properties.size(); i++) {
					const AST::VariableNode::Property &property = p_variable->properties[i];
					if (i > 0) {
						push_text(", ");
					}
					output_identifier(property.pointer_key);
					push_text(" = ");
					output_identifier(property.pointer_value);
				}
				break;
		}
		end_block();
	}
}

void ASTPrinter::output_while(AST::WhileNode *p_while) {
	push_text("while ");
	output_expression(p_while->condition);
	push_text(":");
	output_block(p_while->loop);
}

void ASTPrinter::clear_output() {
	output = StringBuilder();
	indent_level = 0;
	indent_string = String();
	pending_newline = false;
}

void ASTPrinter::print_node(AST::Node *p_node) {
	output_node(p_node);

	const String result = output.as_string();
	if (!result.is_empty()) {
		print_line(result);
	}

	clear_output();
}
