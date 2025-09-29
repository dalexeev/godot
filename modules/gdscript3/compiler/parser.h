/**************************************************************************/
/*  parser.h                                                              */
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
#include "tokenizer.h"

namespace gdscript {

class Parser {
	using Token = Tokenizer::Token;
	using AnnotationTarget = AST::AnnotationInfo::Target;

	enum class FunctionType {
		METHOD,
		LAMBDA,
		PROPERTY,
	};

	// Higher value means higher precedence (i.e. is evaluated first).
	enum class Precedence {
		NONE,
		ASSIGNMENT,
		CAST,
		TERNARY,
		LOGIC_OR,
		LOGIC_AND,
		LOGIC_NOT,
		CONTENT_TEST,
		COMPARISON,
		BIT_OR,
		BIT_XOR,
		BIT_AND,
		BIT_SHIFT,
		ADDITION,
		FACTOR,
		SIGN,
		BIT_NOT,
		POWER,
		TYPE_TEST,
		AWAIT,
		CALL,
		ATTRIBUTE,
		SUBSCRIPT,
		PRIMARY,
	};

	struct ParseRule {
		typedef AST::ExpressionNode *(Parser::*Prefix)();
		typedef AST::ExpressionNode *(Parser::*Infix)(AST::ExpressionNode *p_previous_operand);

		Prefix prefix = nullptr;
		Infix infix = nullptr;
		Precedence precedence = Precedence::NONE;

		ParseRule(Prefix p_prefix, Infix p_infix, Precedence p_precedence) :
				prefix(p_prefix),
				infix(p_infix),
				precedence(p_precedence) {}
	};

	struct PendingError {
		SourceRegion source_region;
		String message;
		bool panic = false;

		PendingError() {}
		PendingError(const SourceRegion &p_source_region, const String &p_message, bool p_panic) :
				source_region(p_source_region),
				message(p_message),
				panic(p_panic) {}
	};

	Tokenizer tokenizer;
	Tokenizer::State previous_state;
	Token previous_token; // Only for `readvance()`!
	SourcePosition previous_token_end; // Except `DEDENT`.
	LocalVector<PendingError> pending_errors;
	Token current_token;
	Token next_token;
	bool lambda_ended = false;
	bool panic_mode = false;

	LocalVector<AST::Context> context_stack;
	AST::Context current_context;

	LocalVector<AST::Node *> node_stack;
	LocalVector<Pair<AST::Node *, bool>> container_stack;
	LocalVector<bool> multiline_stack;

	List<AST::AnnotationNode *> pending_annotations;

	AST ast;

	_FORCE_INLINE_ static constexpr Precedence get_higher_precedence(Precedence p_precedence) {
		return (Precedence)((int)p_precedence + 1);
	}

	static const ParseRule *get_rule(Token::Type p_token_type);

	static AST::AssignmentNode::Operation get_assignment_operation(Token::Type p_token_type);
	static AST::BinaryOperatorNode::Operation get_binary_operation(Token::Type p_token_type);
	static AST::UnaryOperatorNode::Operation get_unary_operation(Token::Type p_token_type);

	// ----- Utilities -----

	AST::IdentifierNode *make_recovery_identifier();
	AST::ExpressionNode *make_recovery_expression();
	AST::TypeNode *make_recovery_type();

	// TODO: Distinguish between the following types of errors?
	// 1. Hard - Cause the parser to panic.
	// 2. Soft - Do not cause panic, but essential information is not reflected in the AST.
	// 3. Safe - Do not cause panic, do not lose essential information.
	void push_error(const SourceRegion &p_origin, const String &p_message, bool p_panic = true);
	void push_error(AST::Node *p_origin, const String &p_message, bool p_panic = true);
	_FORCE_INLINE_ void push_error(const Token &p_origin, const String &p_message, bool p_panic = true) {
		push_error(p_origin.source_region, p_message, p_panic);
	}
	_FORCE_INLINE_ void push_error(const String &p_message, bool p_panic = true) {
		push_error(current_token.source_region, p_message, p_panic);
	}

	void push_warning(const SourceRegion &p_origin, WarningDB::Code p_code, const Array &p_symbols = {});

	_FORCE_INLINE_ SourceRegion get_previous_token_end_region() const {
		return SourceRegion(previous_token_end, previous_token_end);
	}

	_FORCE_INLINE_ bool is_at_end() const {
		return current_token.type == Token::Type::EOF_;
	}
	_FORCE_INLINE_ bool check(Token::Type p_token_type) const {
		return current_token.type == p_token_type;
	}

	bool check_next(Token::Type p_token_type);

	void advance();
	void readvance();

	bool match(Token::Type p_token_type);

	void synchronize_statement();
	void synchronize(const LocalVector<Token::Type> &p_token_types);
	void expect(
			const LocalVector<Token::Type> &p_token_types,
			const String &p_context,
			bool p_allow_end = false,
			bool p_suggest_end = true);
	void end_statement(const String &p_context);

	void push_context(const AST::Context &p_context);
	void pop_context();

	void push_node(AST::Node *p_node, const SourceRegion &p_initial_region);
	_FORCE_INLINE_ void push_node(AST::Node *p_node) { push_node(p_node, current_token.source_region); }
	void pop_node(AST::Node *p_node);

	void push_container(AST::Node *p_node, bool p_is_multiline);
	void pop_container(AST::Node *p_node);
	_FORCE_INLINE_ bool is_multiline_container() const {
		return container_stack.is_empty() ? true : container_stack[container_stack.size() - 1].second;
	}

	void push_multiline(bool p_state);
	void pop_multiline(Token::Type p_token_type, const String &p_context);
	_FORCE_INLINE_ void pop_multiline() { pop_multiline(Token::Type::MAX, String()); }
	_FORCE_INLINE_ bool is_multiline_mode() const {
		return multiline_stack.is_empty() ? false : multiline_stack[multiline_stack.size() - 1];
	}

	// ----- Main -----

	void parse_script();
	void parse_class_body();
	AST::BlockNode *parse_block(const String &p_context);

	AST::AnnotationNode *parse_annotation();
	void handle_annotation_default(AST::AnnotationNode *p_annotation);
	void push_applicable_annotations(AST::Node *p_target, List<AST::AnnotationNode *> &p_annotations);
	void clear_unused_annotations(List<AST::AnnotationNode *> &p_annotations);

	AST::TypeNode *parse_type(bool p_allow_file_path = false);

	// ----- Declarations -----

	void parse_class_name();
	void parse_extends();

	AST::TypeAliasNode *parse_type_alias();
	AST::ClassNode *parse_class();
	AST::EnumNode *parse_enum();
	AST::ConstantNode *parse_constant();
	AST::VariableNode *parse_variable(bool p_allow_property);
	AST::VariableNode *parse_property(AST::VariableNode *p_variable, bool p_need_indent);
	AST::ParameterNode *parse_parameter_or_null();
	AST::FunctionNode *parse_function(FunctionType p_function_type);
	AST::SignalNode *parse_signal();

	// ----- Control Flow -----

	AST::IfNode *parse_if();
	AST::MatchNode *parse_match();
	AST::MatchBranchNode *parse_match_branch();
	AST::MatchPatternNode *parse_match_pattern();
	AST::ForNode *parse_for();
	AST::WhileNode *parse_while();
	AST::BreakNode *parse_break();
	AST::ContinueNode *parse_continue();
	AST::ReturnNode *parse_return();
	AST::AssertNode *parse_assert();
	AST::BreakpointNode *parse_breakpoint();

	// ----- Expressions -----

	AST::ExpressionNode *parse_expression();
	AST::ExpressionNode *parse_expression_or_null();
	AST::ExpressionNode *parse_precedence(Precedence p_precedence);

	AST::IdentifierNode *parse_identifier();
	AST::LiteralNode *parse_literal();

	AST::ExpressionNode *_parse_array();
	AST::ExpressionNode *_parse_await();
	AST::ExpressionNode *_parse_dictionary();
	AST::ExpressionNode *_parse_get_node();
	AST::ExpressionNode *_parse_grouping();
	AST::ExpressionNode *_parse_identifier();
	AST::ExpressionNode *_parse_lambda();
	AST::ExpressionNode *_parse_literal();
	AST::ExpressionNode *_parse_self();
	AST::ExpressionNode *_parse_super();
	AST::ExpressionNode *_parse_unary_operator();
	AST::ExpressionNode *_parse_yield();

	AST::ExpressionNode *_parse_assignment(AST::ExpressionNode *p_previous_operand);
	AST::ExpressionNode *_parse_attribute(AST::ExpressionNode *p_previous_operand);
	AST::ExpressionNode *_parse_binary_operator(AST::ExpressionNode *p_previous_operand);
	AST::ExpressionNode *_parse_call(AST::ExpressionNode *p_previous_operand);
	AST::ExpressionNode *_parse_cast(AST::ExpressionNode *p_previous_operand);
	AST::ExpressionNode *_parse_not_in_operator(AST::ExpressionNode *p_previous_operand);
	AST::ExpressionNode *_parse_question_mark(AST::ExpressionNode *p_previous_operand);
	AST::ExpressionNode *_parse_subscript(AST::ExpressionNode *p_previous_operand);
	AST::ExpressionNode *_parse_ternary_operator(AST::ExpressionNode *p_previous_operand);
	AST::ExpressionNode *_parse_type_test(AST::ExpressionNode *p_previous_operand);

public:
	//static AST parse_header(const String &p_script_path, const String &p_source); // TODO

	_FORCE_INLINE_ void parse() { parse_script(); }
	_FORCE_INLINE_ const AST &get_ast() const { return ast; }

	Parser(const String &p_script_path, const String &p_source, int p_tab_size = DEFAULT_TAB_SIZE);
};

} // namespace gdscript
