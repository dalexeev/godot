/**************************************************************************/
/*  analyzer.h                                                            */
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

#include "ast.h"

namespace gdscript {

class Analyzer {
	// Extended version of `AST::Context`.
	class Context {
		AST::ClassNode *current_class = nullptr;
		AST::FunctionNode *current_function = nullptr;
		AST::Node *current_loop = nullptr;
		AST::BlockNode *current_block = nullptr;

		DataType expected_type;
		bool _is_constant = false;
		bool _is_static = false;

	public:
		_FORCE_INLINE_ AST::ClassNode *get_class() const { return current_class; }
		_FORCE_INLINE_ AST::FunctionNode *get_function() const { return current_function; }
		_FORCE_INLINE_ AST::Node *get_loop() const { return current_loop; }
		_FORCE_INLINE_ AST::BlockNode *get_block() const { return current_block; }

		_FORCE_INLINE_ const DataType &get_expected_type() const { return expected_type; }
		_FORCE_INLINE_ bool is_constant() const { return _is_constant; }
		_FORCE_INLINE_ bool is_static() const { return _is_static; }

		Context with_class(AST::ClassNode *p_class) const;
		Context with_function(AST::FunctionNode *p_function) const;
		Context with_loop(AST::Node *p_loop) const;
		Context with_block(AST::BlockNode *p_block) const;

		Context with_expected_type(const DataType &p_expected_type) const;
		Context with_constant(bool p_constant) const;
		Context with_static(bool p_static) const;

		Context() {}
	};

	AST ast;
	bool is_for_editor = false;

	// ----- Utilities -----

	_FORCE_INLINE_ void push_error(const AST::Node *p_origin, const String &p_message) {
		ast.get_diagnostic_list().push_error(p_origin->source_region, p_message);
	}
	_FORCE_INLINE_ void push_warning(const AST::Node *p_origin, WarningDB::Code p_code, const Array &p_symbols = {}) {
		ast.get_diagnostic_list().push_warning(p_origin->source_region, p_code, p_symbols);
	}
	_FORCE_INLINE_ void mark_node_unsafe(const AST::Node *p_origin) {
		ast.get_diagnostic_list().mark_unsafe_lines(p_origin->source_region);
	}

	AST get_dependency_by_path(const String &p_path, AST::DesiredStatus p_status);
	AST get_dependency_by_fqtn(const StringName &p_fqtn, AST::DesiredStatus p_status);

	// NOTE: If this method returns `false`, you should call `push_error()`.
	[[nodiscard]] bool check_type_compatibility(AST::ExpressionNode *p_expression, const Context &p_context);

	void adjust_constant_value_from_datatype(AST::ExpressionNode *p_expression);
	void adjust_constant_value_from_context(AST::ExpressionNode *p_expression, const Context &p_context);

	// ----- Main -----

	void resolve_class_inheritance(AST::ClassNode *p_class);
	void resolve_class_interface(AST::ClassNode *p_class);
	void resolve_class_body(AST::ClassNode *p_class);
	void resolve_class_member(AST::Node *p_member, const Context &p_context);

	void resolve_block(AST::BlockNode *p_block, const Context &p_context);
	void resolve_statement(AST::Node *p_statement, const Context &p_context);

	void resolve_annotation(AST::AnnotationNode *p_annotation, const Context &p_context);
	void resolve_type(AST::TypeNode *p_type, const Context &p_context);

	// ----- Declarations -----

	void resolve_type_alias(AST::TypeAliasNode *p_type_alias, const Context &p_context);
	void resolve_class(AST::ClassNode *p_class, const Context &p_context);
	void resolve_enum(AST::EnumNode *p_enum, const Context &p_context);
	void resolve_assignable(AST::AssignableNode *p_assignable, const Context &p_context);
	void resolve_constant(AST::ConstantNode *p_constant, const Context &p_context);
	void resolve_variable(AST::VariableNode *p_variable, const Context &p_context);
	void resolve_parameter(AST::ParameterNode *p_parameter, const Context &p_context);
	void resolve_function(AST::FunctionNode *p_function, const Context &p_context);
	void resolve_signal(AST::SignalNode *p_signal, const Context &p_context);

	// ----- Control Flow -----

	void resolve_if(AST::IfNode *p_if, const Context &p_context);
	void resolve_match(AST::MatchNode *p_match, const Context &p_context);
	void resolve_match_branch(AST::MatchBranchNode *p_match_branch, const Context &p_context);
	void resolve_match_pattern(AST::MatchPatternNode *p_match_pattern, const Context &p_context);
	void resolve_for(AST::ForNode *p_for, const Context &p_context);
	void resolve_while(AST::WhileNode *p_while, const Context &p_context);
	void resolve_break(AST::BreakNode *p_break, const Context &p_context);
	void resolve_continue(AST::ContinueNode *p_continue, const Context &p_context);
	void resolve_return(AST::ReturnNode *p_return, const Context &p_context);
	void resolve_assert(AST::AssertNode *p_assert, const Context &p_context);
	void resolve_breakpoint(AST::BreakpointNode *p_breakpoint, const Context &p_context);

	// ----- Expressions -----

	enum class ReductionOption {
		NORMAL,
		ALLOW_VOID,
		ALLOW_META,
	};

	void reduce_expression(
			AST::ExpressionNode *p_expression,
			const Context &p_context,
			ReductionOption p_option = ReductionOption::NORMAL);

	void reduce_array(AST::ArrayNode *p_array, const Context &p_context);
	void reduce_assignment(AST::AssignmentNode *p_assignment, const Context &p_context);
	void reduce_await(AST::AwaitNode *p_await, const Context &p_context);
	void reduce_binary_operator(AST::BinaryOperatorNode *p_binary_operator, const Context &p_context);
	void reduce_call(AST::CallNode *p_call, const Context &p_context);
	void reduce_cast(AST::CastNode *p_cast, const Context &p_context);
	void reduce_dictionary(AST::DictionaryNode *p_dictionary, const Context &p_context);
	void reduce_get_node(AST::GetNodeNode *p_get_node, const Context &p_context);
	void reduce_identifier(AST::IdentifierNode *p_identifier, const Context &p_context);
	void reduce_lambda(AST::LambdaNode *p_lambda, const Context &p_context);
	void reduce_literal(AST::LiteralNode *p_literal, const Context &p_context);
	void reduce_self(AST::SelfNode *p_self, const Context &p_context);
	void reduce_subscript(AST::SubscriptNode *p_subscript, const Context &p_context);
	void reduce_super(AST::SuperNode *p_super, const Context &p_context);
	void reduce_ternary_operator(AST::TernaryOperatorNode *p_ternary_operator, const Context &p_context);
	void reduce_type_test(AST::TypeTestNode *p_type_test, const Context &p_context);
	void reduce_unary_operator(AST::UnaryOperatorNode *p_unary_operator, const Context &p_context);

public:
	void resolve_inheritance();
	void resolve_interface();
	void resolve_body();

	Analyzer(const AST &p_ast, bool p_is_for_editor) :
			ast(p_ast),
			is_for_editor(p_is_for_editor) {}
};

} // namespace gdscript
