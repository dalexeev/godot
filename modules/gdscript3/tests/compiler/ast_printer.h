/**************************************************************************/
/*  ast_printer.h                                                         */
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

#pragma once

#include "../../compiler/ast.h"

#include "core/string/string_builder.h"

namespace gdscript {

class ASTPrinter {
	bool use_deduced_types = false;

	StringBuilder output;
	int indent_level = 0;
	String indent_string;
	bool pending_newline = false;

	void push_text(const String &p_text);
	void push_newline();
	void start_block();
	void end_block();

	void output_node(AST::Node *p_node);
	void output_expression(AST::ExpressionNode *p_expression);

	void output_annotation(AST::AnnotationNode *p_annotation);
	void output_array(AST::ArrayNode *p_array);
	void output_assert(AST::AssertNode *p_assert);
	void output_assignment(AST::AssignmentNode *p_assignment);
	void output_await(AST::AwaitNode *p_await);
	void output_binary_operator(AST::BinaryOperatorNode *p_binary_operator);
	void output_block(AST::BlockNode *p_block);
	void output_break(AST::BreakNode *p_break);
	void output_breakpoint(AST::BreakpointNode *p_breakpoint);
	void output_call(AST::CallNode *p_call);
	void output_cast(AST::CastNode *p_cast);
	void output_class(AST::ClassNode *p_class);
	void output_constant(AST::ConstantNode *p_constant);
	void output_continue(AST::ContinueNode *p_continue);
	void output_dictionary(AST::DictionaryNode *p_dictionary);
	void output_enum(AST::EnumNode *p_enum);
	void output_for(AST::ForNode *p_for);
	void output_function(AST::FunctionNode *p_function, bool p_is_property = false);
	void output_get_node(AST::GetNodeNode *p_get_node);
	void output_identifier(AST::IdentifierNode *p_identifier);
	void output_if(AST::IfNode *p_if);
	void output_lambda(AST::LambdaNode *p_lambda);
	void output_literal(AST::LiteralNode *p_literal);
	void output_match(AST::MatchNode *p_match);
	void output_match_branch(AST::MatchBranchNode *p_match_branch);
	void output_match_pattern(AST::MatchPatternNode *p_match_pattern);
	void output_parameter(AST::ParameterNode *p_parameter);
	void output_return(AST::ReturnNode *p_return);
	void output_root(AST::RootNode *p_root);
	void output_self(AST::SelfNode *p_self);
	void output_signal(AST::SignalNode *p_signal);
	void output_subscript(AST::SubscriptNode *p_subscript);
	void output_super(AST::SuperNode *p_super);
	void output_ternary_operator(AST::TernaryOperatorNode *p_ternary_operator);
	void output_type(AST::TypeNode *p_type);
	void output_type_alias(AST::TypeAliasNode *p_type_alias);
	void output_type_test(AST::TypeTestNode *p_type_test);
	void output_unary_operator(AST::UnaryOperatorNode *p_unary_operator);
	void output_variable(AST::VariableNode *p_variable);
	void output_while(AST::WhileNode *p_while);

	void clear_output();

public:
	void print_node(AST::Node *p_node);
	_FORCE_INLINE_ void print_ast(const AST &p_ast) { print_node(p_ast.get_root()); }

	ASTPrinter(bool p_use_deduced_types) :
			use_deduced_types(p_use_deduced_types) {}
};

} // namespace gdscript
