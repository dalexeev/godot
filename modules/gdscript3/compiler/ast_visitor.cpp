/**************************************************************************/
/*  ast_visitor.cpp                                                       */
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

#include "ast_visitor.h"

using namespace gdscript;

void ASTVisitor::_visit_node(AST::Node *p_node, const AST::Context &p_context) {
	if (p_node == nullptr) {
		return;
	}

	if (!callback(true, p_node, p_context)) {
		return;
	}

	for (AST::AnnotationNode *annotation : p_node->annotations) {
		_visit_node(annotation, p_context);
	}

	switch (p_node->get_type()) {
		using NodeType = AST::Node::Type;

		case NodeType::NONE: {
			// Unreachable.
		} break;
		case NodeType::ANNOTATION: {
			AST::AnnotationNode *annotation = static_cast<AST::AnnotationNode *>(p_node);
			for (AST::ExpressionNode *argument : annotation->arguments) {
				_visit_node(argument, p_context);
			}
		} break;
		case NodeType::ARRAY: {
			AST::ArrayNode *array = static_cast<AST::ArrayNode *>(p_node);
			for (AST::ExpressionNode *element : array->elements) {
				_visit_node(element, p_context);
			}
		} break;
		case NodeType::ASSERT: {
			AST::AssertNode *assert = static_cast<AST::AssertNode *>(p_node);
			_visit_node(assert->condition, p_context);
			_visit_node(assert->message, p_context);
		} break;
		case NodeType::ASSIGNMENT: {
			AST::AssignmentNode *assignment = static_cast<AST::AssignmentNode *>(p_node);
			_visit_node(assignment->assignee, p_context);
			_visit_node(assignment->assigned_value, p_context);
		} break;
		case NodeType::AWAIT: {
			AST::AwaitNode *await = static_cast<AST::AwaitNode *>(p_node);
			_visit_node(await->operand, p_context);
		} break;
		case NodeType::BINARY_OPERATOR: {
			AST::BinaryOperatorNode *binary_operator = static_cast<AST::BinaryOperatorNode *>(p_node);
			_visit_node(binary_operator->left_operand, p_context);
			_visit_node(binary_operator->right_operand, p_context);
		} break;
		case NodeType::BLOCK: {
			AST::BlockNode *block = static_cast<AST::BlockNode *>(p_node);
			const AST::Context block_context = p_context.with_block(block);
			for (AST::Node *statement : block->statements) {
				_visit_node(statement, block_context);
			}
		} break;
		case NodeType::BREAK: {
			// Nothing to do.
		} break;
		case NodeType::BREAKPOINT: {
			// Nothing to do.
		} break;
		case NodeType::CALL: {
			AST::CallNode *call = static_cast<AST::CallNode *>(p_node);
			_visit_node(call->callee, p_context);
			for (AST::Node *argument : call->arguments) {
				_visit_node(argument, p_context);
			}
		} break;
		case NodeType::CAST: {
			AST::CastNode *cast = static_cast<AST::CastNode *>(p_node);
			_visit_node(cast->operand, p_context);
			_visit_node(cast->cast_type, p_context);
		} break;
		case NodeType::CLASS: {
			AST::ClassNode *class_node = static_cast<AST::ClassNode *>(p_node);
			_visit_node(class_node->identifier, p_context);
			_visit_node(class_node->extends_type, p_context);
			const AST::Context class_context = p_context.with_class(class_node);
			for (AST::Node *member : class_node->members) {
				_visit_node(member, class_context);
			}
		} break;
		case NodeType::CONSTANT: {
			AST::ConstantNode *constant = static_cast<AST::ConstantNode *>(p_node);
			_visit_node(constant->identifier, p_context);
			_visit_node(constant->type_specifier, p_context);
			_visit_node(constant->initializer, p_context);
		} break;
		case NodeType::CONTINUE: {
			// Nothing to do.
		} break;
		case NodeType::DICTIONARY: {
			AST::DictionaryNode *dictionary = static_cast<AST::DictionaryNode *>(p_node);
			for (const AST::DictionaryNode::Pair &pair : dictionary->elements) {
				_visit_node(pair.key, p_context);
				_visit_node(pair.value, p_context);
			}
		} break;
		case NodeType::ENUM: {
			AST::EnumNode *enum_node = static_cast<AST::EnumNode *>(p_node);
			_visit_node(enum_node->identifier, p_context);
			_visit_node(enum_node->underlying_type, p_context);
			for (const AST::EnumNode::Item &item : enum_node->items) {
				_visit_node(item.identifier, p_context);
				_visit_node(item.custom_value, p_context);
			}
		} break;
		case NodeType::FOR: {
			AST::ForNode *for_node = static_cast<AST::ForNode *>(p_node);
			_visit_node(for_node->iterator, p_context); // TODO: Should it be inside loop block?
			_visit_node(for_node->iterator_type, p_context);
			_visit_node(for_node->iterable, p_context);
			_visit_node(for_node->loop, p_context.with_loop(for_node));
		} break;
		case NodeType::FUNCTION: {
			AST::FunctionNode *function = static_cast<AST::FunctionNode *>(p_node);
			_visit_node(function->identifier, p_context);
			const AST::Context function_context = p_context.with_function(function);
			for (AST::ParameterNode *parameter : function->parameters) {
				_visit_node(parameter, function_context);
			}
			_visit_node(function->return_type, function_context);
			_visit_node(function->body, function_context);
		} break;
		case NodeType::GET_NODE: {
			// Nothing to do.
		} break;
		case NodeType::IDENTIFIER: {
			// Nothing to do.
		} break;
		case NodeType::IF: {
			AST::IfNode *if_node = static_cast<AST::IfNode *>(p_node);
			_visit_node(if_node->condition, p_context);
			_visit_node(if_node->true_block, p_context);
			_visit_node(if_node->false_block, p_context);
		} break;
		case NodeType::LAMBDA: {
			AST::LambdaNode *lambda = static_cast<AST::LambdaNode *>(p_node);
			_visit_node(lambda->function, p_context);
		} break;
		case NodeType::LITERAL: {
			// Nothing to do.
		} break;
		case NodeType::MATCH: {
			AST::MatchNode *match = static_cast<AST::MatchNode *>(p_node);
			_visit_node(match->test, p_context);
			for (AST::MatchBranchNode *branch : match->branches) {
				_visit_node(branch, p_context);
			}
		} break;
		case NodeType::MATCH_BRANCH: {
			AST::MatchBranchNode *branch = static_cast<AST::MatchBranchNode *>(p_node);
			for (AST::MatchPatternNode *pattern : branch->patterns) {
				_visit_node(pattern, p_context);
			}
			_visit_node(branch->pattern_guard, p_context);
			_visit_node(branch->block, p_context);
		} break;
		case NodeType::MATCH_PATTERN: {
			AST::MatchPatternNode *pattern = static_cast<AST::MatchPatternNode *>(p_node);
			switch (pattern->pattern_type) {
				case AST::MatchPatternNode::PatternType::EXPRESSION:
					_visit_node(pattern->expression, p_context);
					break;
				case AST::MatchPatternNode::PatternType::BIND:
					_visit_node(pattern->bind, p_context);
					break;
				case AST::MatchPatternNode::PatternType::ARRAY:
					for (const AST::MatchPatternNode::Element &element : pattern->elements) {
						_visit_node(element.value, p_context);
					}
					break;
				case AST::MatchPatternNode::PatternType::DICTIONARY:
					for (const AST::MatchPatternNode::Element &element : pattern->elements) {
						_visit_node(element.key, p_context);
						_visit_node(element.value, p_context);
					}
					break;
				case AST::MatchPatternNode::PatternType::WILDCARD:
					break; // Nothing to do.
			}
		} break;
		case NodeType::PARAMETER: {
			AST::ParameterNode *parameter = static_cast<AST::ParameterNode *>(p_node);
			_visit_node(parameter->identifier, p_context);
			_visit_node(parameter->type_specifier, p_context);
			_visit_node(parameter->initializer, p_context);
		} break;
		case NodeType::RETURN: {
			AST::ReturnNode *return_node = static_cast<AST::ReturnNode *>(p_node);
			_visit_node(return_node->return_value, p_context);
		} break;
		case NodeType::ROOT: {
			AST::RootNode *root = static_cast<AST::RootNode *>(p_node);
			_visit_node(root->main_class, p_context);
		} break;
		case NodeType::SELF: {
			// Nothing to do.
		} break;
		case NodeType::SIGNAL: {
			AST::SignalNode *signal = static_cast<AST::SignalNode *>(p_node);
			_visit_node(signal->identifier, p_context);
			for (AST::ParameterNode *parameter : signal->parameters) {
				_visit_node(parameter, p_context);
			}
		} break;
		case NodeType::SUBSCRIPT: {
			AST::SubscriptNode *subscript = static_cast<AST::SubscriptNode *>(p_node);
			_visit_node(subscript->base, p_context);
			if (subscript->is_attribute) {
				_visit_node(subscript->attribute, p_context);
			} else {
				_visit_node(subscript->index, p_context);
			}
		} break;
		case NodeType::SUPER: {
			AST::SuperNode *super = static_cast<AST::SuperNode *>(p_node);
			_visit_node(super->identifier, p_context);
		} break;
		case NodeType::TERNARY_OPERATOR: {
			AST::TernaryOperatorNode *ternary_operator = static_cast<AST::TernaryOperatorNode *>(p_node);
			_visit_node(ternary_operator->condition, p_context);
			_visit_node(ternary_operator->true_expr, p_context);
			_visit_node(ternary_operator->false_expr, p_context);
		} break;
		case NodeType::TYPE: {
			AST::TypeNode *type = static_cast<AST::TypeNode *>(p_node);
			_visit_node(type->outer_type, p_context);
			if (type->is_file_path) {
				_visit_node(type->file_path, p_context);
			} else {
				_visit_node(type->identifier, p_context);
			}
			for (AST::TypeNode *parameter : type->parameters) {
				_visit_node(parameter, p_context);
			}
		} break;
		case NodeType::TYPE_ALIAS: {
			AST::TypeAliasNode *type_alias = static_cast<AST::TypeAliasNode *>(p_node);
			_visit_node(type_alias->identifier, p_context);
			_visit_node(type_alias->source_type, p_context);
		} break;
		case NodeType::TYPE_TEST: {
			AST::TypeTestNode *type_test = static_cast<AST::TypeTestNode *>(p_node);
			_visit_node(type_test->operand, p_context);
			_visit_node(type_test->test_type, p_context);
		} break;
		case NodeType::UNARY_OPERATOR: {
			AST::UnaryOperatorNode *unary_operator = static_cast<AST::UnaryOperatorNode *>(p_node);
			_visit_node(unary_operator->operand, p_context);
		} break;
		case NodeType::VARIABLE: {
			AST::VariableNode *variable = static_cast<AST::VariableNode *>(p_node);
			_visit_node(variable->identifier, p_context);
			_visit_node(variable->type_specifier, p_context);
			_visit_node(variable->initializer, p_context);
			switch (variable->property_style) {
				case AST::VariableNode::PropertyStyle::NONE:
					// Nothing to do.
					break;
				case AST::VariableNode::PropertyStyle::INLINE:
					for (const AST::VariableNode::Property &property : variable->properties) {
						_visit_node(property.inline_function, p_context);
					}
					break;
				case AST::VariableNode::PropertyStyle::POINTER:
					for (const AST::VariableNode::Property &property : variable->properties) {
						_visit_node(property.pointer_key, p_context);
						_visit_node(property.pointer_value, p_context);
					}
					break;
			}
		} break;
		case NodeType::WHILE: {
			AST::WhileNode *while_node = static_cast<AST::WhileNode *>(p_node);
			_visit_node(while_node->condition, p_context); // TODO: Should it be inside loop block?
			_visit_node(while_node->loop, p_context.with_loop(while_node));
		} break;
	}

	callback(false, p_node, p_context);
}

void ASTVisitor::visit_node(AST::Node *p_node, const AST::Context &p_context) {
	ERR_FAIL_NULL(callback);

	_visit_node(p_node, p_context);
}
