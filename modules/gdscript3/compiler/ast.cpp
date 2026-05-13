/**************************************************************************/
/*  ast.cpp                                                               */
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

#include "ast.h"

using namespace gdscript;

// ===== AST::Context =====

AST::Context AST::Context::with_class(ClassNode *p_class) const {
	DEV_ASSERT(p_class != nullptr);

	Context context;
	context.current_class = p_class;

	return context;
}

AST::Context AST::Context::with_function(FunctionNode *p_function) const {
	DEV_ASSERT(p_function != nullptr);

	if (unlikely(current_class == nullptr)) {
		ERR_PRINT("GDScript bug: The function context is set, but the class context is not set.");
	}

	Context context;
	context.current_class = current_class;
	context.current_function = p_function;

	return context;
}

AST::Context AST::Context::with_loop(Node *p_loop) const {
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

AST::Context AST::Context::with_block(BlockNode *p_block) const {
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

// ===== AST::Node =====

bool AST::Node::is_declaration() const {
	switch (type) {
		case Type::TYPE_ALIAS:
		case Type::CLASS:
		case Type::ENUM:
		case Type::CONSTANT:
		case Type::VARIABLE:
		case Type::FUNCTION:
		case Type::SIGNAL:
			return true;
		default:
			return false;
	}
}

bool AST::Node::is_type_declaration() const {
	switch (type) {
		case Type::TYPE_ALIAS:
		case Type::CLASS:
		case Type::ENUM:
			return true;
		default:
			return false;
	}
}

AST::IdentifierNode *AST::Node::get_declaration_identifier() const {
	switch (type) {
		case Type::TYPE_ALIAS:
			return static_cast<const TypeAliasNode *>(this)->identifier;
		case Type::CLASS:
			return static_cast<const ClassNode *>(this)->identifier;
		case Type::ENUM:
			return static_cast<const EnumNode *>(this)->identifier;
		case Type::CONSTANT:
			return static_cast<const ConstantNode *>(this)->identifier;
		case Type::VARIABLE:
			return static_cast<const VariableNode *>(this)->identifier;
		case Type::FUNCTION:
			return static_cast<const FunctionNode *>(this)->identifier;
		case Type::SIGNAL:
			return static_cast<const SignalNode *>(this)->identifier;
		default:
			ERR_FAIL_V(nullptr);
	}
}

StringName AST::Node::get_declaration_name() const {
	IdentifierNode *identifier = get_declaration_identifier();
	return identifier ? identifier->name : StringName();
}

String AST::Node::get_node_raw_type_name() const {
#define HANDLE_NODE_TYPE(m_node_type, m_type) \
	case Type::m_node_type: \
		return ((void)sizeof(m_type), #m_type)

	// clang-format off
	switch (type) {
		case Type::NONE:
			break; // Unreachable.

		HANDLE_NODE_TYPE( ANNOTATION,       AnnotationNode      );
		HANDLE_NODE_TYPE( ARRAY,            ArrayNode           );
		HANDLE_NODE_TYPE( ASSERT,           AssertNode          );
		HANDLE_NODE_TYPE( ASSIGNMENT,       AssignmentNode      );
		HANDLE_NODE_TYPE( AWAIT,            AwaitNode           );
		HANDLE_NODE_TYPE( BINARY_OPERATOR,  BinaryOperatorNode  );
		HANDLE_NODE_TYPE( BLOCK,            BlockNode           );
		HANDLE_NODE_TYPE( BREAK,            BreakNode           );
		HANDLE_NODE_TYPE( BREAKPOINT,       BreakpointNode      );
		HANDLE_NODE_TYPE( CALL,             CallNode            );
		HANDLE_NODE_TYPE( CAST,             CastNode            );
		HANDLE_NODE_TYPE( CLASS,            ClassNode           );
		HANDLE_NODE_TYPE( CONSTANT,         ConstantNode        );
		HANDLE_NODE_TYPE( CONTINUE,         ContinueNode        );
		HANDLE_NODE_TYPE( DICTIONARY,       DictionaryNode      );
		HANDLE_NODE_TYPE( ENUM,             EnumNode            );
		HANDLE_NODE_TYPE( FOR,              ForNode             );
		HANDLE_NODE_TYPE( FUNCTION,         FunctionNode        );
		HANDLE_NODE_TYPE( GET_NODE,         GetNodeNode         );
		HANDLE_NODE_TYPE( IDENTIFIER,       IdentifierNode      );
		HANDLE_NODE_TYPE( IF,               IfNode              );
		HANDLE_NODE_TYPE( LAMBDA,           LambdaNode          );
		HANDLE_NODE_TYPE( LITERAL,          LiteralNode         );
		HANDLE_NODE_TYPE( MATCH,            MatchNode           );
		HANDLE_NODE_TYPE( MATCH_BRANCH,     MatchBranchNode     );
		HANDLE_NODE_TYPE( MATCH_PATTERN,    MatchPatternNode    );
		HANDLE_NODE_TYPE( PARAMETER,        ParameterNode       );
		HANDLE_NODE_TYPE( RETURN,           ReturnNode          );
		HANDLE_NODE_TYPE( ROOT,             RootNode            );
		HANDLE_NODE_TYPE( SELF,             SelfNode            );
		HANDLE_NODE_TYPE( SIGNAL,           SignalNode          );
		HANDLE_NODE_TYPE( SUBSCRIPT,        SubscriptNode       );
		HANDLE_NODE_TYPE( SUPER,            SuperNode           );
		HANDLE_NODE_TYPE( TERNARY_OPERATOR, TernaryOperatorNode );
		HANDLE_NODE_TYPE( TYPE,             TypeNode            );
		HANDLE_NODE_TYPE( TYPE_ALIAS,       TypeAliasNode       );
		HANDLE_NODE_TYPE( TYPE_TEST,        TypeTestNode        );
		HANDLE_NODE_TYPE( UNARY_OPERATOR,   UnaryOperatorNode   );
		HANDLE_NODE_TYPE( VARIABLE,         VariableNode        );
		HANDLE_NODE_TYPE( WHILE,            WhileNode           );
	}
	// clang-format on

	ERR_FAIL_V(String());

#undef HANDLE_NODE_TYPE
}

String AST::Node::get_node_type_name() const {
	if (type == Type::GET_NODE) {
		return R"*("get_node()" shorthand)*";
	}
	if (type == Type::ROOT) {
		return "script";
	}
	return get_node_raw_type_name().trim_suffix("Node").to_snake_case().replace_char('_', ' ');
}

AST::AnnotationInfo::Target AST::Node::get_annotation_target() const {
	using Target = AnnotationInfo::Target;

	if (type == Type::ROOT) {
		return Target::SCRIPT;
	}

	ERR_FAIL_NULL_V(parent_node, Target::NONE);

	if (parent_node->type == Type::BLOCK) {
		return Target::STATEMENT;
	}
	if (type == Type::MATCH_BRANCH && parent_node->type == Type::MATCH) {
		return Target::STATEMENT;
	}

	switch (type) {
		case Type::TYPE_ALIAS:
			return Target::TYPE_ALIAS;
		case Type::CLASS:
			return Target::CLASS;
		case Type::ENUM:
			return Target::ENUM;
		case Type::CONSTANT:
			return Target::CONSTANT;
		case Type::VARIABLE:
			return Target::VARIABLE;
		case Type::FUNCTION:
			return Target::FUNCTION;
		case Type::SIGNAL:
			return Target::SIGNAL;
		default:
			ERR_FAIL_V(Target::NONE);
	}
}

SourceRegion AST::Node::get_header_source_region() const {
	SourceRegion result = source_region;

#define CHECK_NODE(m_node) \
	if (m_node != nullptr) { \
		result.end = m_node->source_region.end; \
		break; \
	}

#define CHECK_NODE_VECTOR(m_vector) \
	if (!m_vector.is_empty()) { \
		result.end = m_vector[m_vector.size() - 1]->source_region.end; \
		break; \
	}

#define HANDLE_DEFAULT(m_token_length) \
	result.end = result.start; \
	result.end.position += m_token_length; \
	result.end.column += m_token_length; \
	result.end.raw_column += m_token_length

	switch (type) {
		case Type::CLASS: {
			// TODO: Root class vs inner classes.
			const ClassNode *class_node = static_cast<const ClassNode *>(this);
			CHECK_NODE(class_node->extends_type); // TODO: Ignore if nested for inner classes.
			CHECK_NODE(class_node->identifier);
			HANDLE_DEFAULT(5); // len("class") == 5
		} break;

		case Type::ENUM: {
			const EnumNode *enum_node = static_cast<const EnumNode *>(this);
			CHECK_NODE(enum_node->identifier);
			HANDLE_DEFAULT(4); // len("enum") == 4
		} break;

		case Type::VARIABLE: {
			const VariableNode *variable = static_cast<const VariableNode *>(this);
			CHECK_NODE(variable->initializer);
			CHECK_NODE(variable->type_specifier);
			CHECK_NODE(variable->identifier);
			HANDLE_DEFAULT(3); // len("var") == 3
		} break;

		case Type::FUNCTION: {
			const FunctionNode *function = static_cast<const FunctionNode *>(this);
			if (function->header_end.position > 0) {
				result.end = function->header_end;
				break;
			}
			CHECK_NODE(function->return_type);
			CHECK_NODE_VECTOR(function->parameters);
			CHECK_NODE(function->identifier);
			HANDLE_DEFAULT(4); // len("func") == 4
		} break;

		case Type::IF: {
			const IfNode *if_node = static_cast<const IfNode *>(this);
			CHECK_NODE(if_node->condition);
			HANDLE_DEFAULT(2); // len("if") == 2
		} break;

		case Type::MATCH: {
			const MatchNode *match = static_cast<const MatchNode *>(this);
			CHECK_NODE(match->test);
			HANDLE_DEFAULT(5); // len("match") == 5
		} break;

		case Type::MATCH_BRANCH: {
			const MatchBranchNode *branch = static_cast<const MatchBranchNode *>(this);
			CHECK_NODE(branch->pattern_guard);
			CHECK_NODE_VECTOR(branch->patterns);
			result.end = result.start; // Unlikely, but just in case.
		} break;

		case Type::FOR: {
			const ForNode *for_node = static_cast<const ForNode *>(this);
			CHECK_NODE(for_node->iterable);
			CHECK_NODE(for_node->iterator_type);
			CHECK_NODE(for_node->iterator);
			HANDLE_DEFAULT(3); // len("for") == 3
		} break;

		case Type::WHILE: {
			const WhileNode *while_node = static_cast<const WhileNode *>(this);
			CHECK_NODE(while_node->condition);
			HANDLE_DEFAULT(5); // len("while") == 5
		} break;

		default: {
			// Nothing to do.
		} break;
	}

#undef CHECK_NODE
#undef CHECK_NODE_VECTOR
#undef HANDLE_DEFAULT

	return result;
}

AST::ClassNode *AST::Node::find_parent_class() const {
	Node *node = parent_node;
	while (node != nullptr) {
		if (node->type == Type::CLASS) {
			return static_cast<ClassNode *>(node);
		}
		node = node->parent_node;
	}
	return nullptr;
}

AST::FunctionNode *AST::Node::find_parent_function() const {
	Node *node = parent_node;
	while (node != nullptr) {
		if (node->type == Type::FUNCTION) {
			return static_cast<FunctionNode *>(node);
		}
		node = node->parent_node;
	}
	return nullptr;
}

AST::Node *AST::Node::find_parent_loop() const {
	Node *node = parent_node;
	while (node != nullptr) {
		if (node->type == Type::FUNCTION) {
			return nullptr;
		}
		if (node->is_loop()) {
			return node;
		}
		node = node->parent_node;
	}
	return nullptr;
}

AST::BlockNode *AST::Node::find_parent_block() const {
	Node *node = parent_node;
	while (node != nullptr) {
		if (node->type == Type::FUNCTION) {
			return nullptr;
		}
		if (node->type == Type::BLOCK) {
			return static_cast<BlockNode *>(node);
		}
		node = node->parent_node;
	}
	return nullptr;
}

// ===== AST::AnnotationNode =====

bool AST::AnnotationNode::applies_to(BitField<AnnotationInfo::Target> p_targets) const {
	if (info == nullptr) {
		return false;
	}

	return !info->targets.get_shared(p_targets).is_empty();
}

void AST::AnnotationNode::apply(AST &p_ast, Node *p_target) {
	ERR_FAIL_NULL(info);
	ERR_FAIL_COND(is_applied);

	is_applied = true;
	(p_ast.*(info->action))(this, p_target);
}

// ===== AST::AssignmentNode =====

String AST::AssignmentNode::get_operator_name() const {
	switch (operation) {
		case Operation::NONE:
			return "=";
		case Operation::ADDITION:
			return "+=";
		case Operation::SUBTRACTION:
			return "-=";
		case Operation::MULTIPLICATION:
			return "*=";
		case Operation::DIVISION:
			return "/=";
		case Operation::MODULO:
			return "%=";
		case Operation::POWER:
			return "**=";
		case Operation::BIT_SHIFT_LEFT:
			return "<<=";
		case Operation::BIT_SHIFT_RIGHT:
			return ">>=";
		case Operation::BIT_AND:
			return "&=";
		case Operation::BIT_OR:
			return "|=";
		case Operation::BIT_XOR:
			return "^=";
	}

	ERR_FAIL_V(String());
}

Variant::Operator AST::AssignmentNode::get_variant_operator() const {
	switch (operation) {
		case Operation::NONE:
			return Variant::OP_MAX;
		case Operation::ADDITION:
			return Variant::OP_ADD;
		case Operation::SUBTRACTION:
			return Variant::OP_SUBTRACT;
		case Operation::MULTIPLICATION:
			return Variant::OP_MULTIPLY;
		case Operation::DIVISION:
			return Variant::OP_DIVIDE;
		case Operation::MODULO:
			return Variant::OP_MODULE;
		case Operation::POWER:
			return Variant::OP_POWER;
		case Operation::BIT_SHIFT_LEFT:
			return Variant::OP_SHIFT_LEFT;
		case Operation::BIT_SHIFT_RIGHT:
			return Variant::OP_SHIFT_RIGHT;
		case Operation::BIT_AND:
			return Variant::OP_BIT_AND;
		case Operation::BIT_OR:
			return Variant::OP_BIT_OR;
		case Operation::BIT_XOR:
			return Variant::OP_BIT_XOR;
	}

	ERR_FAIL_V(Variant::OP_MAX);
}

// ===== AST::BinaryOperatorNode =====

String AST::BinaryOperatorNode::get_operator_name() const {
	switch (operation) {
		case Operation::ADDITION:
			return "+";
		case Operation::SUBTRACTION:
			return "-";
		case Operation::MULTIPLICATION:
			return "*";
		case Operation::DIVISION:
			return "/";
		case Operation::MODULO:
			return "%";
		case Operation::POWER:
			return "**";
		case Operation::BIT_LEFT_SHIFT:
			return "<<";
		case Operation::BIT_RIGHT_SHIFT:
			return ">>";
		case Operation::BIT_AND:
			return "&";
		case Operation::BIT_OR:
			return "|";
		case Operation::BIT_XOR:
			return "^";
		case Operation::LOGIC_AND:
			return "and";
		case Operation::LOGIC_OR:
			return "or";
		case Operation::CONTENT_TEST:
			return "in";
		case Operation::COMP_EQUAL:
			return "==";
		case Operation::COMP_NOT_EQUAL:
			return "!=";
		case Operation::COMP_LESS:
			return "<";
		case Operation::COMP_LESS_EQUAL:
			return "<=";
		case Operation::COMP_GREATER:
			return ">";
		case Operation::COMP_GREATER_EQUAL:
			return ">=";
	}

	ERR_FAIL_V(String());
}

Variant::Operator AST::BinaryOperatorNode::get_variant_operator() const {
	switch (operation) {
		case Operation::ADDITION:
			return Variant::OP_ADD;
		case Operation::SUBTRACTION:
			return Variant::OP_SUBTRACT;
		case Operation::MULTIPLICATION:
			return Variant::OP_MULTIPLY;
		case Operation::DIVISION:
			return Variant::OP_DIVIDE;
		case Operation::MODULO:
			return Variant::OP_MODULE;
		case Operation::POWER:
			return Variant::OP_POWER;
		case Operation::BIT_LEFT_SHIFT:
			return Variant::OP_SHIFT_LEFT;
		case Operation::BIT_RIGHT_SHIFT:
			return Variant::OP_SHIFT_RIGHT;
		case Operation::BIT_AND:
			return Variant::OP_BIT_AND;
		case Operation::BIT_OR:
			return Variant::OP_BIT_OR;
		case Operation::BIT_XOR:
			return Variant::OP_BIT_XOR;
		case Operation::LOGIC_AND:
			return Variant::OP_AND;
		case Operation::LOGIC_OR:
			return Variant::OP_OR;
		case Operation::CONTENT_TEST:
			return Variant::OP_IN;
		case Operation::COMP_EQUAL:
			return Variant::OP_EQUAL;
		case Operation::COMP_NOT_EQUAL:
			return Variant::OP_NOT_EQUAL;
		case Operation::COMP_LESS:
			return Variant::OP_LESS;
		case Operation::COMP_LESS_EQUAL:
			return Variant::OP_LESS_EQUAL;
		case Operation::COMP_GREATER:
			return Variant::OP_GREATER;
		case Operation::COMP_GREATER_EQUAL:
			return Variant::OP_GREATER_EQUAL;
	}

	ERR_FAIL_V(Variant::OP_MAX);
}

// ===== AST::UnaryOperatorNode =====

String AST::UnaryOperatorNode::get_operator_name() const {
	switch (operation) {
		case Operation::POSITIVE:
			return "+";
		case Operation::NEGATIVE:
			return "-";
		case Operation::COMPLEMENT:
			return "~";
		case Operation::LOGIC_NOT:
			return "not";
	}

	ERR_FAIL_V(String());
}

Variant::Operator AST::UnaryOperatorNode::get_variant_operator() const {
	switch (operation) {
		case Operation::POSITIVE:
			return Variant::OP_POSITIVE;
		case Operation::NEGATIVE:
			return Variant::OP_NEGATE;
		case Operation::COMPLEMENT:
			return Variant::OP_BIT_NEGATE;
		case Operation::LOGIC_NOT:
			return Variant::OP_NOT;
	}

	ERR_FAIL_V(Variant::OP_MAX);
}

// ===== AST =====

void AST::initialize() {
	register_annotations();
}

void AST::deinitialize() {
	unregister_annotations();
}

void AST::set_root(RootNode *p_root) {
	ERR_FAIL_COND(data->root != nullptr);
	ERR_FAIL_NULL(p_root);

	data->root = p_root;
}

void AST::_clear() {
	if (data == nullptr) {
		return;
	}

	if (data->ref_count.unref()) {
		Node *node = data->last_node;
		while (node != nullptr) {
			Node *prev_node = node->prev_node;
			memdelete(node);
			node = prev_node;
		}

		data->root = nullptr;
		data->last_node = nullptr;

		memdelete(data);
	}

	data = nullptr;
}

void AST::_share_from(const AST &p_other) {
	data = p_other.data;
	if (data != nullptr) {
		data->ref_count.ref();
	}
}

void AST::_move_from(AST &&p_other) {
	data = p_other.data;
	p_other.data = nullptr;
}

AST::AST(const String &p_script_path, const String &p_source) {
	data = memnew(Data);
	data->ref_count.init();

	data->script_path = p_script_path;
	data->source = p_source;
	data->diagnostic_list = DiagnosticList(p_script_path);
}

AST::AST(const AST &p_other) {
	_share_from(p_other);
}

AST::AST(AST &&p_other) {
	_move_from(std::move(p_other));
}

AST &AST::operator=(const AST &p_other) {
	if (this == &p_other) {
		return *this;
	}

	_clear();
	_share_from(p_other);

	return *this;
}

AST &AST::operator=(AST &&p_other) {
	if (data == p_other.data) {
		return *this;
	}

	_clear();
	_move_from(std::move(p_other));

	return *this;
}

AST::~AST() {
	_clear();
}

// ----- Annotations -----

HashMap<StringName, AST::AnnotationInfo> AST::annotations;

void AST::register_annotation(
		const String &p_name,
		BitField<AnnotationInfo::Target> p_targets,
		AnnotationInfo::Action p_action) {
	ERR_FAIL_COND(annotations.has(p_name));

	AnnotationInfo info;
	info.action = p_action;
	info.targets = p_targets;
	info.method_info = MethodInfo(p_name);

	annotations[p_name] = info;
}

void AST::register_annotation_arguments(
		const String &p_name,
		const Vector<PropertyInfo> &p_arguments,
		const Vector<Variant> &p_default_arguments,
		bool p_is_vararg) {
	ERR_FAIL_COND(!annotations.has(p_name));

	MethodInfo &method_info = annotations[p_name].method_info;
	method_info.arguments = p_arguments;
	method_info.default_arguments = p_default_arguments;
	if (p_is_vararg) {
		method_info.flags |= METHOD_FLAG_VARARG;
	}
}

void AST::register_annotations() {
#define REG register_annotation
#define TG1(m_a) BitField<AnnotationInfo::Target>(AnnotationInfo::Target::m_a)
#define TG2(m_a, m_b) TG1(m_a).get_combined(TG1(m_b))
#define BASIC(m_name) &AST::m_name##_annotation
#define EXPORT(m_hint, m_type) &AST::export_annotation<PROPERTY_HINT_##m_hint, Variant::m_type>
#define EXPORT_GROUP(m_usage) &AST::export_group_annotation<PROPERTY_USAGE_##m_usage>

	// clang-format off
	// --- Main annotations ---
	REG( "@tool",                         TG1(SCRIPT),                  BASIC(tool)                                  );
	REG( "@icon",                         TG1(CLASS),                   BASIC(icon)                                  );
	REG( "@abstract",                     TG2(CLASS, FUNCTION),         BASIC(abstract)                              );
	REG( "@static",                       TG2(VARIABLE, FUNCTION),      BASIC(static)                                );
	REG( "@onready",                      TG1(VARIABLE),                BASIC(onready)                               );
	// --- Export annotations ---
	REG( "@export",                       TG1(VARIABLE),                EXPORT(NONE, NIL)                            );
	REG( "@export_enum",                  TG1(VARIABLE),                EXPORT(ENUM, NIL)                            );
	REG( "@export_file",                  TG1(VARIABLE),                EXPORT(FILE, STRING)                         );
	REG( "@export_file_path",             TG1(VARIABLE),                EXPORT(FILE_PATH, STRING)                    );
	REG( "@export_dir",                   TG1(VARIABLE),                EXPORT(DIR, STRING)                          );
	REG( "@export_global_file",           TG1(VARIABLE),                EXPORT(GLOBAL_FILE, STRING)                  );
	REG( "@export_global_dir",            TG1(VARIABLE),                EXPORT(GLOBAL_DIR, STRING)                   );
	REG( "@export_multiline",             TG1(VARIABLE),                EXPORT(MULTILINE_TEXT, STRING)               );
	REG( "@export_placeholder",           TG1(VARIABLE),                EXPORT(PLACEHOLDER_TEXT, STRING)             );
	REG( "@export_range",                 TG1(VARIABLE),                EXPORT(RANGE, FLOAT)                         );
	REG( "@export_exp_easing",            TG1(VARIABLE),                EXPORT(EXP_EASING, FLOAT)                    );
	REG( "@export_color_no_alpha",        TG1(VARIABLE),                EXPORT(COLOR_NO_ALPHA, COLOR)                );
	REG( "@export_node_path",             TG1(VARIABLE),                EXPORT(NODE_PATH_VALID_TYPES, NODE_PATH)     );
	REG( "@export_flags",                 TG1(VARIABLE),                EXPORT(FLAGS, INT)                           );
	REG( "@export_flags_2d_render",       TG1(VARIABLE),                EXPORT(LAYERS_2D_RENDER, INT)                );
	REG( "@export_flags_2d_physics",      TG1(VARIABLE),                EXPORT(LAYERS_2D_PHYSICS, INT)               );
	REG( "@export_flags_2d_navigation",   TG1(VARIABLE),                EXPORT(LAYERS_2D_NAVIGATION, INT)            );
	REG( "@export_flags_3d_render",       TG1(VARIABLE),                EXPORT(LAYERS_3D_RENDER, INT)                );
	REG( "@export_flags_3d_physics",      TG1(VARIABLE),                EXPORT(LAYERS_3D_PHYSICS, INT)               );
	REG( "@export_flags_3d_navigation",   TG1(VARIABLE),                EXPORT(LAYERS_3D_NAVIGATION,INT)             );
	REG( "@export_flags_avoidance",       TG1(VARIABLE),                EXPORT(LAYERS_AVOIDANCE, INT)                );
	REG( "@export_storage",               TG1(VARIABLE),                BASIC(export_storage)                        );
	REG( "@export_custom",                TG1(VARIABLE),                BASIC(export_custom)                         );
	REG( "@export_tool_button",           TG1(VARIABLE),                BASIC(export_tool_button)                    );
	// --- Export grouping annotations ---
	REG( "@export_category",              TG1(STANDALONE),              EXPORT_GROUP(CATEGORY)                       );
	REG( "@export_group",                 TG1(STANDALONE),              EXPORT_GROUP(GROUP)                          );
	REG( "@export_subgroup",              TG1(STANDALONE),              EXPORT_GROUP(SUBGROUP)                       );
	// --- Warning annotations ---
	REG( "@warning_ignore",               TG2(CLASS_LEVEL, STATEMENT),  BASIC(warning_ignore)                        );
	REG( "@warning_ignore_start",         TG1(STANDALONE),              BASIC(warning_ignore_region)                 );
	REG( "@warning_ignore_restore",       TG1(STANDALONE),              BASIC(warning_ignore_region)                 );
	// --- Networking annotations ---
	REG( "@rpc",                          TG1(FUNCTION),                BASIC(rpc)                                   );
	// clang-format on

#undef REG
#undef TG1
#undef TG2
#undef BASIC
#undef EXPORT
#undef EXPORT_GROUP

#define REG register_annotation_arguments
#define ARG_INT(m_name) PropertyInfo(Variant::INT, m_name)
#define ARG_FLOAT(m_name) PropertyInfo(Variant::FLOAT, m_name)
#define ARG_STR(m_name) PropertyInfo(Variant::STRING, m_name)

	const Vector<PropertyInfo> range_args = {
		ARG_FLOAT("min"),
		ARG_FLOAT("max"),
		ARG_FLOAT("step"),
		ARG_STR("extra_hints"),
	};

	const Vector<PropertyInfo> export_custom_args = {
		PropertyInfo(Variant::INT, "hint", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CLASS_IS_ENUM, "PropertyHint"),
		ARG_STR("hint_string"),
		PropertyInfo(Variant::INT, "usage", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CLASS_IS_BITFIELD, "PropertyUsageFlags"),
	};
	const Vector<Variant> export_custom_def_args = varray(PROPERTY_USAGE_DEFAULT);

	const Vector<PropertyInfo> rpc_args = {
		ARG_STR("mode"),
		ARG_STR("sync"),
		ARG_STR("transfer_mode"),
		ARG_INT("transfer_channel"),
	};
	// Keep in sync with `rpc_annotation()` and `SceneRPCInterface::_parse_rpc_config()`.
	const Vector<Variant> rpc_def_args = varray("authority", "call_remote", "reliable", 0);

	// clang-format off
	// --- Main annotations ---
	REG( "@icon",                         { ARG_STR("icon_path")                  }, varray(),                 false );
	// --- Export annotations ---
	REG( "@export_enum",                  { ARG_STR("names")                      }, varray(),                 true  );
	REG( "@export_file",                  { ARG_STR("filter")                     }, varray(""),               true  );
	REG( "@export_file_path",             { ARG_STR("filter")                     }, varray(""),               true  );
	REG( "@export_global_file",           { ARG_STR("filter")                     }, varray(""),               true  );
	REG( "@export_multiline",             { ARG_STR("hint")                       }, varray(""),               true  );
	REG( "@export_placeholder",           { ARG_STR("placeholder")                }, varray(),                 false );
	REG( "@export_range",                 range_args,                                varray(1.0, ""),          true  );
	REG( "@export_exp_easing",            { ARG_STR("hints")                      }, varray(""),               true  );
	REG( "@export_node_path",             { ARG_STR("type")                       }, varray(""),               true  );
	REG( "@export_flags",                 { ARG_STR("names")                      }, varray(),                 true  );
	REG( "@export_custom",                export_custom_args,                        export_custom_def_args,   false );
	REG( "@export_tool_button",           { ARG_STR("text"), ARG_STR("icon")      }, varray(""),               false );
	// --- Export grouping annotations ---
	REG( "@export_category",              { ARG_STR("name")                       }, varray(),                 false );
	REG( "@export_group",                 { ARG_STR("name"), ARG_STR("prefix")    }, varray(""),               false );
	REG( "@export_subgroup",              { ARG_STR("name"), ARG_STR("prefix")    }, varray(""),               false );
	// --- Warning annotations ---
	REG( "@warning_ignore",               { ARG_STR("warning")                    }, varray(),                 true  );
	REG( "@warning_ignore_start",         { ARG_STR("warning")                    }, varray(),                 true  );
	REG( "@warning_ignore_restore",       { ARG_STR("warning")                    }, varray(),                 true  );
	// --- Networking annotations ---
	REG( "@rpc",                          { rpc_args                              }, rpc_def_args,             false );
	// clang-format on

#undef REG
#undef ARG_INT
#undef ARG_FLOAT
#undef ARG_STR
}

void AST::unregister_annotations() {
	annotations.clear();
}

bool AST::is_export_grouping_annotation(const StringName &p_name) {
	return p_name == SNAME("@export_category") || p_name == SNAME("@export_group") || p_name == SNAME("@export_subgroup");
}

bool AST::is_warning_region_annotation(const StringName &p_name) {
	return p_name == SNAME("@warning_ignore_start") || p_name == SNAME("@warning_ignore_restore");
}

void AST::tool_annotation(AnnotationNode *p_annotation, Node *p_target) {
	ERR_FAIL_COND(p_target->type != Node::Type::ROOT);

	RootNode *root = static_cast<RootNode *>(p_target);
	if (root->is_tool) {
		push_error(p_annotation, R"(Annotation "@tool" can only be used once per script.)");
		return;
	}

	root->is_tool = true;
}

void AST::icon_annotation(AnnotationNode *p_annotation, Node *p_target) {
	ERR_FAIL_COND(p_target->type != Node::Type::CLASS);
	ERR_FAIL_COND(p_annotation->resolved_arguments.is_empty());

	ClassNode *class_node = static_cast<ClassNode *>(p_target);
	if (class_node->has_icon) {
		push_error(p_annotation, R"(Annotation "@icon" can only be used once per class.)");
		return;
	}

	class_node->has_icon = true;

	const String path = p_annotation->resolved_arguments[0];
	if (path.is_empty()) {
		push_error(p_annotation->arguments[0], "The icon path must not be empty.");
		return;
	}

	if (path.is_absolute_path()) {
		class_node->simplified_icon_path = path.simplify_path();
	} else if (path.is_relative_path()) {
		class_node->simplified_icon_path = data->script_path.get_base_dir().path_join(path).simplify_path();
	} else {
		class_node->simplified_icon_path = path;
	}
}

void AST::abstract_annotation(AnnotationNode *p_annotation, Node *p_target) {
	if (p_target->type == Node::Type::CLASS) {
		ClassNode *class_node = static_cast<ClassNode *>(p_target);
		if (class_node->is_abstract) {
			push_error(p_annotation, R"(Annotation "@abstract" can only be used once per class.)");
			return;
		}

		class_node->is_abstract = true;
		return;
	}

	if (p_target->type == Node::Type::FUNCTION) {
		FunctionNode *function_node = static_cast<FunctionNode *>(p_target);
		if (function_node->is_abstract) {
			push_error(p_annotation, R"(Annotation "@abstract" can only be used once per function.)");
			return;
		}
		if (function_node->is_static) {
			push_error(p_annotation, R"(Annotation "@abstract" cannot be applied to static functions.)");
			return;
		}

		function_node->is_abstract = true;
		return;
	}

	ERR_FAIL_MSG(R"(GDScript bug: Invalid target for annotation "@abstract".)");
}

void AST::static_annotation(AnnotationNode *p_annotation, Node *p_target) {
	if (p_target->type == Node::Type::VARIABLE) {
		VariableNode *variable_node = static_cast<VariableNode *>(p_target);
		if (variable_node->is_static) {
			push_error(p_annotation, R"(Annotation "@static" can only be used once per variable.)");
			return;
		}

		variable_node->is_static = true;
		return;
	}

	if (p_target->type == Node::Type::FUNCTION) {
		FunctionNode *function_node = static_cast<FunctionNode *>(p_target);
		if (function_node->is_static) {
			push_error(p_annotation, R"(Annotation "@static" can only be used once per function.)");
			return;
		}
		if (function_node->is_abstract) {
			push_error(p_annotation, R"(Annotation "@static" cannot be applied to abstract functions.)");
			return;
		}

		function_node->is_static = true;
		return;
	}

	ERR_FAIL_MSG(R"(GDScript bug: Invalid target for annotation "@static".)");
}

void AST::onready_annotation(AnnotationNode *p_annotation, Node *p_target) {
	// TODO
}

void AST::export_annotation(AnnotationNode *p_annotation, Node *p_target, PropertyHint p_hint, Variant::Type p_type) {
	// TODO
}

void AST::export_storage_annotation(AnnotationNode *p_annotation, Node *p_target) {
	// TODO
}

void AST::export_custom_annotation(AnnotationNode *p_annotation, Node *p_target) {
	// TODO
}

void AST::export_tool_button_annotation(AnnotationNode *p_annotation, Node *p_target) {
	// TODO
}

void AST::export_group_annotation(AnnotationNode *p_annotation, Node *p_target, PropertyUsageFlags p_usage) {
	// TODO
}

void AST::warning_ignore_annotation(AnnotationNode *p_annotation, Node *p_target) {
	const SourceRegion source_region = p_target->get_header_source_region();

	for (unsigned i = 0; i < p_annotation->resolved_arguments.size(); i++) {
		const Variant &warning_name = p_annotation->resolved_arguments[i];
		const WarningDB::Code warning_code = WarningDB::get_code_from_name(String(warning_name).to_upper());

		if (warning_code == WarningDB::Code::MAX) {
			push_error(p_annotation->arguments[i], vformat(R"(Invalid warning name: "%s".)", warning_name));
			continue;
		}

		data->diagnostic_list.mark_warning_ignored_region(source_region, warning_code);
	}
}

void AST::warning_ignore_region_annotation(AnnotationNode *p_annotation, Node *p_target) {
	const bool is_start = p_annotation->name == SNAME("@warning_ignore_start");
	for (unsigned i = 0; i < p_annotation->resolved_arguments.size(); i++) {
		const Variant &warning_name = p_annotation->resolved_arguments[i];
		const WarningDB::Code warning_code = WarningDB::get_code_from_name(String(warning_name).to_upper());

		if (warning_code == WarningDB::Code::MAX) {
			push_error(p_annotation->arguments[i], vformat(R"(Invalid warning name: "%s".)", warning_name));
			continue;
		}

		if (is_start) {
			if (data->open_warning_ignore_regions.has(warning_code)) {
				const String msg = vformat(
						R"(Warning "%s" is already being ignored by "@warning_ignore_start" at line %d.)",
						String(warning_name).to_upper(),
						data->open_warning_ignore_regions[warning_code]->source_region.start.line);
				push_error(p_annotation->arguments[i], msg);
				continue;
			}

			data->open_warning_ignore_regions[warning_code] = p_annotation;
		} else {
			if (!data->open_warning_ignore_regions.has(warning_code)) {
				const String msg = vformat(
						R"(Warning "%s" is not being ignored by "@warning_ignore_start".)",
						String(warning_name).to_upper());
				push_error(p_annotation->arguments[i], msg);
				continue;
			}

			const SourcePosition start = data->open_warning_ignore_regions[warning_code]->source_region.start;
			const SourcePosition end = p_annotation->source_region.end;
			data->diagnostic_list.mark_warning_ignored_region(SourceRegion(start, end), warning_code);

			data->open_warning_ignore_regions.erase(warning_code);
		}
	}
}

void AST::close_warning_regions(const SourcePosition &p_end) {
	for (const KeyValue<WarningDB::Code, AnnotationNode *> &kv : data->open_warning_ignore_regions) {
		const SourcePosition start = kv.value->source_region.start;
		data->diagnostic_list.mark_warning_ignored_region(SourceRegion(start, p_end), kv.key);
	}

	data->open_warning_ignore_regions.clear();
}

void AST::rpc_annotation(AnnotationNode *p_annotation, Node *p_target) {
	// TODO
	// Default values should match the annotation registration defaults and `SceneRPCInterface::_parse_rpc_config()`.
}
