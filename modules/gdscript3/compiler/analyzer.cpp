/**************************************************************************/
/*  analyzer.cpp                                                          */
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

#include "analyzer.h"

#include "registry.h"

using namespace gdscript;

// ===== Analyzer::Context =====

Analyzer::Context Analyzer::Context::with_class(AST::ClassNode *p_class) const {
	DEV_ASSERT(p_class != nullptr);

	Context context;
	context.current_class = p_class;

	return context;
}

Analyzer::Context Analyzer::Context::with_function(AST::FunctionNode *p_function) const {
	DEV_ASSERT(p_function != nullptr);

	if (unlikely(current_class == nullptr)) {
		ERR_PRINT("GDScript bug: The function context is set, but the class context is not set.");
	}

	Context context;
	context.current_class = current_class;
	context.current_function = p_function;

	return context;
}

Analyzer::Context Analyzer::Context::with_loop(AST::Node *p_loop) const {
	DEV_ASSERT(p_loop != nullptr && p_loop->is_loop());

	if (unlikely(current_class == nullptr)) {
		ERR_PRINT("GDScript bug: The loop context is set, but the class context is not set.");
	}
	if (unlikely(current_function == nullptr)) {
		ERR_PRINT("GDScript bug: The loop context is set, but the function context is not set.");
	}

	Context context = *this;
	context.current_loop = p_loop;

	return context;
}

Analyzer::Context Analyzer::Context::with_block(AST::BlockNode *p_block) const {
	DEV_ASSERT(p_block != nullptr);

	if (unlikely(current_class == nullptr)) {
		ERR_PRINT("GDScript bug: The block context is set, but the class context is not set.");
	}
	if (unlikely(current_function == nullptr)) {
		ERR_PRINT("GDScript bug: The block context is set, but the function context is not set.");
	}

	Context context = *this;
	context.current_block = p_block;

	return context;
}

Analyzer::Context Analyzer::Context::with_expected_type(const DataType &p_expected_type) const {
	Context context = *this;
	context.expected_type = p_expected_type;
	return context;
}

Analyzer::Context Analyzer::Context::with_constant(bool p_constant) const {
	Context context = *this;
	context._is_constant = p_constant;
	return context;
}

Analyzer::Context Analyzer::Context::with_static(bool p_static) const {
	Context context = *this;
	context._is_static = p_static;
	return context;
}

// ===== Analyzer =====

// ----- Utilities -----

// TODO: Non-existent dependencies, inner classes?
// TODO: Add external scripts by path?
AST Analyzer::get_dependency_by_path(const String &p_path, AST::DesiredStatus p_status) {
	const AST dependency = Registry::get_singleton()->get_ast_by_path(p_path, p_status);

	if (!dependency.is_null() && !is_for_editor) {
		Registry::get_singleton()->add_dependency(ast.get_script_path(), p_path);
	}

	return dependency;
}

// TODO: Non-existent dependencies, inner classes?
AST Analyzer::get_dependency_by_fqtn(const StringName &p_fqtn, AST::DesiredStatus p_status) {
	const AST dependency = Registry::get_singleton()->get_ast_by_fqtn(p_fqtn, p_status);

	if (!dependency.is_null() && !is_for_editor) {
		Registry::get_singleton()->add_dependency(ast.get_script_path(), p_fqtn);
	}

	return dependency;
}

bool Analyzer::check_type_compatibility(AST::ExpressionNode *p_expression, const Context &p_context) {
	const DataType &expected_type = p_context.get_expected_type();
	const DataType &actual_type = p_expression->datatype;

	switch (expected_type.can_accept(actual_type, p_context.is_constant())) {
		case Trilean::FALSE: {
			return false;
		} break;
		case Trilean::UNKNOWN: {
			mark_node_unsafe(p_expression);
		} break;
		case Trilean::TRUE: {
			// TODO: Expand to other int/float types, like `Vector2i` and `Vector2`?
			if (expected_type.is_builtin(Variant::INT) && actual_type.is_builtin(Variant::FLOAT)) {
				push_warning(p_expression, WarningDB::Code::NARROWING_CONVERSION);
			}
			// TODO: Improve enum and bitfield warnings. // TODO: underlying type for enums - remame warning.
			if (expected_type.is_enum() && (actual_type.is_builtin(Variant::INT) || actual_type.is_bitfield())) {
				push_warning(p_expression, WarningDB::Code::INT_AS_ENUM_WITHOUT_CAST);
			}
		} break;
	}

	return true;
}

void Analyzer::adjust_constant_value_from_datatype(AST::ExpressionNode *p_expression) {
	p_expression->datatype.adjust_constant_value(p_expression->constant_value);
	// TODO: INT_AS_ENUM_WITHOUT_MATCH
}

void Analyzer::adjust_constant_value_from_context(AST::ExpressionNode *p_expression, const Context &p_context) {
	p_context.get_expected_type().adjust_constant_value(p_expression->constant_value);
	p_expression->datatype = DataType::from_constant_value(p_expression->constant_value);
	// TODO: INT_AS_ENUM_WITHOUT_MATCH
}

// ----- Main -----

void Analyzer::resolve_class_inheritance(AST::ClassNode *p_class) {
	// TODO
}

void Analyzer::resolve_class_interface(AST::ClassNode *p_class) {
	// TODO: require resolve_class_inheritance()
	// TODO
}

void Analyzer::resolve_class_body(AST::ClassNode *p_class) {
	// TODO: require resolve_class_interface()
	// TODO
}

void Analyzer::resolve_class_member(AST::Node *p_member, const Context &p_context) {
	using Status = AST::Node::Status;

	ERR_FAIL_NULL(p_member);

	if (p_member->status >= Status::RESOLVED_INTERFACE) {
		return;
	}

	if (p_member->status == Status::RESOLVING_INTERFACE) {
		const StringName member_name = p_member->get_declaration_name();
		push_error(p_member, vformat(R"(Could not resolve member "%s": Cyclic reference.)", member_name));
		return;
	}

	p_member->status = Status::RESOLVING_INTERFACE;

	switch (p_member->get_type()) {
		using NodeType = AST::Node::Type;

		case NodeType::TYPE_ALIAS:
			resolve_type_alias(static_cast<AST::TypeAliasNode *>(p_member), p_context);
			break;
		case NodeType::CLASS:
			resolve_class(static_cast<AST::ClassNode *>(p_member), p_context);
			break;
		case NodeType::ENUM:
			resolve_enum(static_cast<AST::EnumNode *>(p_member), p_context);
			break;
		case NodeType::CONSTANT:
			resolve_constant(static_cast<AST::ConstantNode *>(p_member), p_context);
			break;
		case NodeType::VARIABLE:
			resolve_variable(static_cast<AST::VariableNode *>(p_member), p_context);
			break;
		case NodeType::FUNCTION:
			resolve_function(static_cast<AST::FunctionNode *>(p_member), p_context);
			break;
		case NodeType::SIGNAL:
			resolve_signal(static_cast<AST::SignalNode *>(p_member), p_context);
			break;

		case NodeType::ANNOTATION:
			resolve_annotation(static_cast<AST::AnnotationNode *>(p_member), p_context);
			break;

		default:
			ERR_PRINT("GDScript bug: Unhandled node type.");
			break;
	}

	p_member->status = Status::RESOLVED_INTERFACE;

	if (p_member->is_declaration()) {
		AST::IdentifierNode *identifier = p_member->get_declaration_identifier();
		if (identifier != nullptr && !identifier->is_recovery) {
			// TODO: Check for conflicts, add to resolved members.
		}
	}
}

void Analyzer::resolve_block(AST::BlockNode *p_block, const Context &p_context) {
	const Context statement_context = p_context.with_block(p_block);
	for (AST::Node *statement : p_block->statements) {
		for (AST::AnnotationNode *annotation : statement->annotations) {
			resolve_annotation(annotation, p_context);
			annotation->apply(ast, statement);
		}
		resolve_statement(statement, statement_context);
		//resolve_pending_lambda_bodies(); // TODO
	}
}

void Analyzer::resolve_statement(AST::Node *p_statement, const Context &p_context) {
	using Status = AST::Node::Status;

	ERR_FAIL_NULL(p_statement);

	if (p_statement->status == Status::RESOLVED_BODY) {
		return;
	}

	ERR_FAIL_COND(p_statement->status != Status::UNRESOLVED);

	p_statement->status = Status::RESOLVING_BODY;

	switch (p_statement->get_type()) {
		using NodeType = AST::Node::Type;

		case NodeType::TYPE_ALIAS:
			resolve_type_alias(static_cast<AST::TypeAliasNode *>(p_statement), p_context);
			break;
		case NodeType::CONSTANT:
			resolve_constant(static_cast<AST::ConstantNode *>(p_statement), p_context);
			break;
		case NodeType::VARIABLE:
			resolve_variable(static_cast<AST::VariableNode *>(p_statement), p_context);
			break;

		case NodeType::IF:
			resolve_if(static_cast<AST::IfNode *>(p_statement), p_context);
			break;
		case NodeType::MATCH:
			resolve_match(static_cast<AST::MatchNode *>(p_statement), p_context);
			break;
		case NodeType::FOR:
			resolve_for(static_cast<AST::ForNode *>(p_statement), p_context);
			break;
		case NodeType::WHILE:
			resolve_while(static_cast<AST::WhileNode *>(p_statement), p_context);
			break;
		case NodeType::BREAK:
			resolve_break(static_cast<AST::BreakNode *>(p_statement), p_context);
			break;
		case NodeType::CONTINUE:
			resolve_continue(static_cast<AST::ContinueNode *>(p_statement), p_context);
			break;
		case NodeType::RETURN:
			resolve_return(static_cast<AST::ReturnNode *>(p_statement), p_context);
			break;
		case NodeType::ASSERT:
			resolve_assert(static_cast<AST::AssertNode *>(p_statement), p_context);
			break;
		case NodeType::BREAKPOINT:
			resolve_breakpoint(static_cast<AST::BreakpointNode *>(p_statement), p_context);
			break;

		default:
			if (p_statement->is_expression()) {
				reduce_expression(
						static_cast<AST::ExpressionNode *>(p_statement),
						p_context,
						ReductionOption::ALLOW_VOID);
				// TODO: Warnings.
			} else {
				ERR_PRINT("GDScript bug: Unhandled node type.");
			}
			break;
	}

	p_statement->status = Status::RESOLVED_BODY;

	// TODO: Check for conflicts, add to locals.
}

void Analyzer::resolve_annotation(AST::AnnotationNode *p_annotation, const Context &p_context) {
	// TODO
}

void Analyzer::resolve_type(AST::TypeNode *p_type, const Context &p_context) {
	// TODO
}

// ----- Declarations -----

void Analyzer::resolve_type_alias(AST::TypeAliasNode *p_type_alias, const Context &p_context) {
	resolve_type(p_type_alias->source_type, p_context);
	// TODO
}

void Analyzer::resolve_class(AST::ClassNode *p_class, const Context &p_context) {
	// TODO
}

void Analyzer::resolve_enum(AST::EnumNode *p_enum, const Context &p_context) {
	// TODO
}

void Analyzer::resolve_assignable(AST::AssignableNode *p_assignable, const Context &p_context) {
	const bool is_constant = p_assignable->get_type() == AST::Node::Type::CONSTANT;
	//const bool is_parameter = p_assignable->type == AST::Node::Type::PARAMETER;

	const bool has_specified_type = p_assignable->type_specifier != nullptr;
	if (has_specified_type) {
		resolve_type(p_assignable->type_specifier, p_context);
		p_assignable->datatype = p_assignable->type_specifier->denoted_datatype;
	} else {
		p_assignable->datatype = DataType::make_variant();
	}

	if (p_assignable->initializer == nullptr) {
		if (is_constant) {
			push_error(p_assignable, "A constant must have an initializer.");
		}
	} else {
		const Context context = p_context.with_expected_type(p_assignable->datatype).with_constant(is_constant);

		reduce_expression(p_assignable->initializer, context);

		const String node_type_name = p_assignable->get_node_type_name();

		if (is_constant && !p_assignable->initializer->is_fully_constant()) {
			const String msg = vformat(
					R"(Assigned value for %s "%s" isn't a constant expression.)",
					node_type_name,
					p_assignable->identifier->name);
			push_error(p_assignable->initializer, msg);
		}

		const DataType &initializer_type = p_assignable->initializer->datatype;
		if (p_assignable->infer_datatype) {
			if (initializer_type.is_unknown() || initializer_type.is_union()) { // TODO: Or `erase_union()` instead?
				const String msg = vformat(
						R"(Cannot infer the type of %s "%s" because the initializer doesn't have a set type.)",
						node_type_name,
						p_assignable->identifier->name);
				push_error(p_assignable->initializer, msg);
			} else if (initializer_type.is_builtin(Variant::NIL)) {
				const String msg = vformat(
						R"(Cannot infer the type of %s "%s" because the initializer is "null".)",
						node_type_name,
						p_assignable->identifier->name);
				push_error(p_assignable->initializer, msg);
			} else {
				p_assignable->datatype = initializer_type;
				if (initializer_type.is_variant()) {
					push_warning(p_assignable, WarningDB::Code::INFERENCE_ON_VARIANT, { node_type_name });
				}
			}
		} else {
			if (!check_type_compatibility(p_assignable->initializer, context)) {
				const String msg = vformat(
						R"(Cannot assign a value of type "%s" to %s "%s" with specified type "%s".)",
						initializer_type.to_string(),
						node_type_name,
						p_assignable->identifier->name,
						p_assignable->datatype.to_string());
				push_error(p_assignable->initializer, msg);
			}
		}
	}

	// TODO: Warnings: UNTYPED_DECLARATION, INFERRED_DECLARATION, ENUM_VARIABLE_WITHOUT_DEFAULT.
	// TODO: CONFUSABLE_LOCAL_DECLARATION - remove or rework?
}

void Analyzer::resolve_constant(AST::ConstantNode *p_constant, const Context &p_context) {
	resolve_assignable(p_constant, p_context);
}

void Analyzer::resolve_variable(AST::VariableNode *p_variable, const Context &p_context) {
	resolve_assignable(p_variable, p_context);
}

void Analyzer::resolve_parameter(AST::ParameterNode *p_parameter, const Context &p_context) {
	resolve_assignable(p_parameter, p_context);
}

void Analyzer::resolve_function(AST::FunctionNode *p_function, const Context &p_context) {
	// TODO
}

void Analyzer::resolve_signal(AST::SignalNode *p_signal, const Context &p_context) {
	// TODO
}

// ----- Control Flow -----

void Analyzer::resolve_if(AST::IfNode *p_if, const Context &p_context) {
	reduce_expression(p_if->condition, p_context.with_expected_type(DataType::make_builtin(Variant::BOOL)));
	resolve_block(p_if->true_block, p_context);
	resolve_block(p_if->false_block, p_context);
}

void Analyzer::resolve_match(AST::MatchNode *p_match, const Context &p_context) {
	// TODO
}

void Analyzer::resolve_match_branch(AST::MatchBranchNode *p_match_branch, const Context &p_context) {
	// TODO
}

void Analyzer::resolve_match_pattern(AST::MatchPatternNode *p_match_pattern, const Context &p_context) {
	// TODO
}

void Analyzer::resolve_for(AST::ForNode *p_for, const Context &p_context) {
	// TODO
}

void Analyzer::resolve_while(AST::WhileNode *p_while, const Context &p_context) {
	reduce_expression(p_while->condition, p_context.with_expected_type(DataType::make_builtin(Variant::BOOL)));
	resolve_block(p_while->loop, p_context.with_loop(p_while));
}

void Analyzer::resolve_break(AST::BreakNode *p_break, const Context &p_context) {
	if (p_context.get_loop() == nullptr) {
		push_error(p_break, R"(Cannot use "break" outside of a loop.)");
	}
}

void Analyzer::resolve_continue(AST::ContinueNode *p_continue, const Context &p_context) {
	if (p_context.get_loop() == nullptr) {
		push_error(p_continue, R"(Cannot use "continue" outside of a loop.)");
	}
}

void Analyzer::resolve_return(AST::ReturnNode *p_return, const Context &p_context) {
	ERR_FAIL_NULL(p_context.get_function());

	const DataType expected_type = p_context.get_function()->return_datatype;

	DataType return_type;
	if (p_return->return_value == nullptr) {
		return_type = DataType::make_marked_void();
	} else {
		reduce_expression(
				p_return->return_value,
				p_context.with_expected_type(expected_type),
				ReductionOption::ALLOW_VOID);
		return_type = p_return->return_value->datatype;
	}

	// TODO: Check void and compatibility.

	// TODO
}

void Analyzer::resolve_assert(AST::AssertNode *p_assert, const Context &p_context) {
	reduce_expression(p_assert->condition, p_context.with_expected_type(DataType::make_builtin(Variant::BOOL)));
	if (p_assert->message != nullptr) {
		reduce_expression(p_assert->message, p_context.with_expected_type(DataType::make_builtin(Variant::STRING)));
	}
}

void Analyzer::resolve_breakpoint(AST::BreakpointNode *p_breakpoint, const Context &p_context) {
	// Nothing to do.
}

// ----- Expressions -----

// TODO: Check `void`.
void Analyzer::reduce_expression(
		AST::ExpressionNode *p_expression,
		const Context &p_context,
		ReductionOption p_option) {
	using Status = AST::Node::Status;

	ERR_FAIL_NULL(p_expression);

	if (p_expression->status == Status::RESOLVED_BODY) {
		return;
	}

	ERR_FAIL_COND(p_expression->status != Status::UNRESOLVED);

	p_expression->status = Status::RESOLVING_BODY;

	switch (p_expression->get_type()) {
		using NodeType = AST::Node::Type;

		case NodeType::ARRAY:
			reduce_array(static_cast<AST::ArrayNode *>(p_expression), p_context);
			break;
		case NodeType::ASSIGNMENT:
			reduce_assignment(static_cast<AST::AssignmentNode *>(p_expression), p_context);
			break;
		case NodeType::AWAIT:
			reduce_await(static_cast<AST::AwaitNode *>(p_expression), p_context);
			break;
		case NodeType::BINARY_OPERATOR:
			reduce_binary_operator(static_cast<AST::BinaryOperatorNode *>(p_expression), p_context);
			break;
		case NodeType::CALL:
			reduce_call(static_cast<AST::CallNode *>(p_expression), p_context);
			break;
		case NodeType::CAST:
			reduce_cast(static_cast<AST::CastNode *>(p_expression), p_context);
			break;
		case NodeType::DICTIONARY:
			reduce_dictionary(static_cast<AST::DictionaryNode *>(p_expression), p_context);
			break;
		case NodeType::GET_NODE:
			reduce_get_node(static_cast<AST::GetNodeNode *>(p_expression), p_context);
			break;
		case NodeType::IDENTIFIER:
			reduce_identifier(static_cast<AST::IdentifierNode *>(p_expression), p_context);
			break;
		case NodeType::LAMBDA:
			reduce_lambda(static_cast<AST::LambdaNode *>(p_expression), p_context);
			break;
		case NodeType::LITERAL:
			reduce_literal(static_cast<AST::LiteralNode *>(p_expression), p_context);
			break;
		case NodeType::SELF:
			reduce_self(static_cast<AST::SelfNode *>(p_expression), p_context);
			break;
		case NodeType::SUBSCRIPT:
			reduce_subscript(static_cast<AST::SubscriptNode *>(p_expression), p_context);
			break;
		case NodeType::SUPER:
			reduce_super(static_cast<AST::SuperNode *>(p_expression), p_context);
			break;
		case NodeType::TERNARY_OPERATOR:
			reduce_ternary_operator(static_cast<AST::TernaryOperatorNode *>(p_expression), p_context);
			break;
		case NodeType::TYPE_TEST:
			reduce_type_test(static_cast<AST::TypeTestNode *>(p_expression), p_context);
			break;
		case NodeType::UNARY_OPERATOR:
			reduce_unary_operator(static_cast<AST::UnaryOperatorNode *>(p_expression), p_context);
			break;

		default:
			ERR_PRINT("GDScript bug: Unhandled node type.");
			break;
	}

	p_expression->status = Status::RESOLVED_BODY;

	if (p_option != ReductionOption::ALLOW_VOID && p_expression->datatype.is_marked_void()) {
		push_error(p_expression, R"(Cannot get return value of a "void" function.)");
		p_expression->datatype = DataType::make_builtin(Variant::NIL);
	}
	if (p_option != ReductionOption::ALLOW_META && p_expression->datatype.is_meta()) {
		const String type_name = p_expression->datatype.get_meta_wrapped_type().to_string();
		push_error(p_expression, vformat(R"(Type "%s" cannot be used on its own.)", type_name));
		p_expression->datatype = DataType::make_unknown();
	}
}

// TODO: Check invalid elements.
void Analyzer::reduce_array(AST::ArrayNode *p_array, const Context &p_context) {
	const DataType &expected_type = p_context.get_expected_type();

	Context element_context;
	if (expected_type.is_builtin(Variant::ARRAY)) {
		p_array->datatype = expected_type;
		element_context = p_context.with_expected_type(expected_type.get_array_element_type());
	} else if (expected_type.is_packed_array()) {
		p_array->datatype = expected_type;
		element_context = p_context.with_expected_type(expected_type.get_packed_array_element_type());
	} else {
		p_array->datatype = DataType::make_builtin(Variant::ARRAY);
		element_context = p_context.with_expected_type(DataType::make_variant());
	}

	using Constness = AST::ExpressionNode::Constness;
	Constness min_constness = Constness::NONE;
	if (!expected_type.is_packed_array()) {
		min_constness = p_context.is_constant() ? Constness::FULL : Constness::PARTIAL;
	}

	for (AST::ExpressionNode *element : p_array->elements) {
		reduce_expression(element, element_context);
		min_constness = MIN(min_constness, element->constness);
	}

	if (min_constness > Constness::NONE) {
		Array array;
		if (!p_array->elements.is_empty()) {
			array.resize(p_array->elements.size());
			for (unsigned i = 0; i < p_array->elements.size(); i++) {
				array[i] = p_array->elements[i]->constant_value;
			}
		}

		p_array->constness = min_constness;
		p_array->constant_value = array;
		adjust_constant_value_from_datatype(p_array);
	}
}

void Analyzer::reduce_assignment(AST::AssignmentNode *p_assignment, const Context &p_context) {
	// TODO
}

void Analyzer::reduce_await(AST::AwaitNode *p_await, const Context &p_context) {
	// TODO
}

void Analyzer::reduce_binary_operator(AST::BinaryOperatorNode *p_binary_operator, const Context &p_context) {
	AST::ExpressionNode *left_operand = p_binary_operator->left_operand;
	AST::ExpressionNode *right_operand = p_binary_operator->right_operand;

	const Context operand_context = p_context.with_expected_type(DataType::make_variant());
	reduce_expression(left_operand, operand_context);
	reduce_expression(right_operand, operand_context);

	const Variant::Operator variant_operator = p_binary_operator->get_variant_operator();

	if (variant_operator == Variant::OP_AND || variant_operator == Variant::OP_OR) {
		// Those work for any type of operand and always return a boolean.
		// They don't use the `Variant` operator since they have short-circuit semantics.
		p_binary_operator->datatype = DataType::make_builtin(Variant::BOOL);
	} else {
		const Trilean valid = DataType::get_binary_operator_type(
				p_binary_operator->datatype,
				variant_operator,
				left_operand->datatype,
				right_operand->datatype);
		if (valid == Trilean::UNKNOWN) {
			mark_node_unsafe(p_binary_operator);
			return;
		}
		if (valid == Trilean::FALSE) {
			const String msg = vformat(
					R"(Invalid operands "%s" and "%s" for "%s" operator.)",
					left_operand->datatype.to_string(),
					right_operand->datatype.to_string(),
					Variant::get_operator_name(variant_operator));
			push_error(p_binary_operator, msg);
			return;
		}
	}

	if (left_operand->is_any_degree_constant() && right_operand->is_any_degree_constant()) {
		bool valid = false;
		Variant::evaluate(
				variant_operator,
				left_operand->constant_value,
				right_operand->constant_value,
				p_binary_operator->constant_value,
				valid);
		ERR_FAIL_COND_MSG(!valid, "GDScript bug: Failed to evaluate constant value.");

		adjust_constant_value_from_context(p_binary_operator, p_context);

		if (left_operand->is_fully_constant() && right_operand->is_fully_constant()) {
			const bool left_is_shared = left_operand->constant_value.is_shared();
			const bool right_is_shared = right_operand->constant_value.is_shared();
			if (!p_context.is_constant() && (left_is_shared || right_is_shared)) {
				p_binary_operator->mark_as_partially_constant();
			} else {
				p_binary_operator->mark_as_fully_constant();
			}
		} else {
			p_binary_operator->mark_as_partially_constant();
		}
	}
}

void Analyzer::reduce_call(AST::CallNode *p_call, const Context &p_context) {
	reduce_expression(
			p_call->callee,
			p_context.with_expected_type(DataType::make_variant()),
			ReductionOption::ALLOW_META);
	// TODO: Check metatype.

	// TODO: Reduce arguments with correct expected parameter types, if known.

	// TODO
}

void Analyzer::reduce_cast(AST::CastNode *p_cast, const Context &p_context) {
	resolve_type(p_cast->cast_type, p_context.with_expected_type(DataType::make_unknown()));
	reduce_expression(p_cast->operand, p_context.with_expected_type(p_cast->cast_type->denoted_datatype));

	// TODO
}

// TODO: Check invalid keys/values.
void Analyzer::reduce_dictionary(AST::DictionaryNode *p_dictionary, const Context &p_context) {
	const DataType &expected_type = p_context.get_expected_type();

	Context key_context;
	Context value_context;
	if (expected_type.is_builtin(Variant::DICTIONARY)) {
		p_dictionary->datatype = expected_type;
		key_context = p_context.with_expected_type(expected_type.get_dictionary_key_type());
		value_context = p_context.with_expected_type(expected_type.get_dictionary_value_type());
	} else {
		p_dictionary->datatype = DataType::make_builtin(Variant::DICTIONARY);
		const DataType variant_type = DataType::make_variant();
		key_context = p_context.with_expected_type(variant_type);
		value_context = p_context.with_expected_type(variant_type);
	}

	HashMap<Variant, int, HashMapHasherDefault, StringLikeVariantComparator> keys; // TODO: Check comparator.

	using Constness = AST::ExpressionNode::Constness;
	Constness min_constness = p_context.is_constant() ? Constness::FULL : Constness::PARTIAL;

	for (const AST::DictionaryNode::Pair &pair : p_dictionary->elements) {
		switch (p_dictionary->style) {
			case AST::DictionaryNode::Style::PYTHON_DICT:
				reduce_expression(pair.key, key_context);
				break;
			case AST::DictionaryNode::Style::LUA_TABLE:
				if (pair.key->get_type() == AST::Node::Type::IDENTIFIER) {
					// Custom reduction. The identifier here is a string, not a primary expression.
					pair.key->status = AST::Node::Status::RESOLVED_BODY;

					pair.key->mark_as_fully_constant();
					pair.key->constant_value = static_cast<AST::IdentifierNode *>(pair.key)->name;
					adjust_constant_value_from_context(pair.key, key_context);
				} else if (pair.key->get_type() == AST::Node::Type::LITERAL) {
					reduce_expression(pair.key, key_context);
				} else {
					ERR_PRINT("GDScript bug: Unexpected key type in Lua-style dictionary.");
				}
				break;
		}

		if (pair.key->is_any_degree_constant()) {
			if (keys.has(pair.key->constant_value)) {
				const String msg = vformat(
						R"(Key "%s" was already used in this dictionary (at line %d).)",
						pair.key->constant_value,
						keys[pair.key->constant_value]);
				push_error(pair.key, msg);
			} else {
				keys[pair.key->constant_value] = pair.key->source_region.start.line;
			}
		}

		reduce_expression(pair.value, value_context);

		min_constness = MIN(min_constness, MIN(pair.key->constness, pair.value->constness));
	}

	if (min_constness > Constness::NONE) {
		Dictionary dictionary;
		for (const AST::DictionaryNode::Pair &pair : p_dictionary->elements) {
			dictionary[pair.key->constant_value] = dictionary[pair.value->constant_value];
		}

		p_dictionary->constness = min_constness;
		p_dictionary->constant_value = dictionary;
		adjust_constant_value_from_datatype(p_dictionary);
	}
}

void Analyzer::reduce_get_node(AST::GetNodeNode *p_get_node, const Context &p_context) {
	// TODO: Check if current class extends Node.

	if (p_context.is_static()) {
		const String msg = vformat(
				R"*(Cannot use "get_node()" shorthand ("%c") in a static context.)*",
				p_get_node->use_dollar ? '$' : '%');
		push_error(p_get_node, msg);
		return;
	}

	// TODO: Mark lambda use self.

	p_get_node->datatype = DataType::make_native_class(SNAME("Node"));
}

void Analyzer::reduce_identifier(AST::IdentifierNode *p_identifier, const Context &p_context) {
	// TODO
}

void Analyzer::reduce_lambda(AST::LambdaNode *p_lambda, const Context &p_context) {
	resolve_function(p_lambda->function, p_context.with_expected_type(DataType::make_unknown()));

	p_lambda->datatype = DataType::make_builtin(Variant::CALLABLE);

	// TODO
	// resolve_function_signature // TODO?
	// pending_body_resolution_lambdas.push_back(p_lambda); // TODO?
}

void Analyzer::reduce_literal(AST::LiteralNode *p_literal, const Context &p_context) {
	p_literal->mark_as_fully_constant();
	p_literal->constant_value = p_literal->value;
	adjust_constant_value_from_context(p_literal, p_context);
}

void Analyzer::reduce_self(AST::SelfNode *p_self, const Context &p_context) {
	if (p_context.is_static()) {
		push_error(p_self, R"(Cannot use "self" in a static context.)");
		return;
	}

	// TODO: Mark lambda use self.

	// TODO
}

void Analyzer::reduce_subscript(AST::SubscriptNode *p_subscript, const Context &p_context) {
	reduce_expression(
			p_subscript->base,
			p_context.with_expected_type(DataType::make_variant()),
			ReductionOption::ALLOW_META);
	// TODO: Check metatype.

	if (p_subscript->is_attribute) {
		// TODO
	} else {
		// TODO: Reduce index with correct expected type, if known.
	}

	// TODO
}

void Analyzer::reduce_super(AST::SuperNode *p_super, const Context &p_context) {
	// TODO
}

void Analyzer::reduce_ternary_operator(AST::TernaryOperatorNode *p_ternary_operator, const Context &p_context) {
	const DataType bool_type = DataType::make_builtin(Variant::BOOL);
	reduce_expression(p_ternary_operator->condition, p_context.with_expected_type(bool_type));

	// TODO: Allowed for compatibility. Remove this feature?
	const bool allow_void = p_ternary_operator->parent_node->get_type() == AST::Node::Type::BLOCK;
	const ReductionOption reduction_option = allow_void ? ReductionOption::ALLOW_VOID : ReductionOption::NORMAL;
	// Expected type propagates to branches.
	reduce_expression(p_ternary_operator->true_expr, p_context, reduction_option);
	reduce_expression(p_ternary_operator->false_expr, p_context, reduction_option);

	// TODO: Check void.
	// TODO: warn branch compatibility

	p_ternary_operator->datatype = DataType::make_union({
			p_ternary_operator->true_expr->datatype,
			p_ternary_operator->false_expr->datatype,
	});

	// TODO
}

void Analyzer::reduce_type_test(AST::TypeTestNode *p_type_test, const Context &p_context) {
	resolve_type(p_type_test->test_type, p_context.with_expected_type(DataType::make_unknown()));
	reduce_expression(p_type_test->operand, p_context.with_expected_type(DataType::make_variant()));

	// TODO
}

void Analyzer::reduce_unary_operator(AST::UnaryOperatorNode *p_unary_operator, const Context &p_context) {
	AST::ExpressionNode *operand = p_unary_operator->operand;

	reduce_expression(operand, p_context.with_expected_type(DataType::make_variant()));

	const Variant::Operator variant_operator = p_unary_operator->get_variant_operator();

	{
		const Trilean valid = DataType::get_unary_operator_type(
				p_unary_operator->datatype,
				variant_operator,
				operand->datatype);
		if (valid == Trilean::UNKNOWN) {
			mark_node_unsafe(p_unary_operator);
			return;
		}
		if (valid == Trilean::FALSE) {
			const String msg = vformat(
					R"(Invalid operand of type "%s" for unary operator "%s".)",
					operand->datatype.to_string(),
					Variant::get_operator_name(variant_operator));
			push_error(p_unary_operator, msg);
			return;
		}
	}

	if (operand->is_any_degree_constant()) {
		bool valid = false;
		Variant::evaluate(
				variant_operator,
				operand->constant_value,
				Variant(),
				p_unary_operator->constant_value,
				valid);
		ERR_FAIL_COND_MSG(!valid, "GDScript bug: Failed to evaluate constant value.");

		adjust_constant_value_from_context(p_unary_operator, p_context);

		if (operand->is_fully_constant()) {
			if (!p_context.is_constant() && operand->constant_value.is_shared()) {
				p_unary_operator->mark_as_partially_constant();
			} else {
				p_unary_operator->mark_as_fully_constant();
			}
		} else {
			p_unary_operator->mark_as_partially_constant();
		}
	}
}

// ----- Public methods -----

void Analyzer::resolve_inheritance() {
	resolve_class_inheritance(ast.get_root()->main_class);
}

void Analyzer::resolve_interface() {
	resolve_class_interface(ast.get_root()->main_class);
}

void Analyzer::resolve_body() {
	resolve_class_body(ast.get_root()->main_class);

	if (ast.get_root()->main_class->status == AST::Node::Status::RESOLVED_BODY) {
		ast.get_diagnostic_list().apply_pending_warnings();
	}
}
