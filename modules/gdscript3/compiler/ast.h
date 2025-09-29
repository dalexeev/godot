/**************************************************************************/
/*  ast.h                                                                 */
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

#include "cfg.h"
#include "diagnostic_list.h"

#include "core/object/method_info.h"

namespace gdscript {

class AST {
public:
	struct Node;

	struct AssignableNode;
	struct ExpressionNode;

	struct AnnotationNode;
	struct ArrayNode;
	struct AssertNode;
	struct AssignmentNode;
	struct AwaitNode;
	struct BinaryOperatorNode;
	struct BlockNode;
	struct BreakNode;
	struct BreakpointNode;
	struct CallNode;
	struct CastNode;
	struct ClassNode;
	struct ConstantNode;
	struct ContinueNode;
	struct DictionaryNode;
	struct EnumNode;
	struct ForNode;
	struct FunctionNode;
	struct GetNodeNode;
	struct IdentifierNode;
	struct IfNode;
	struct LambdaNode;
	struct LiteralNode;
	struct MatchNode;
	struct MatchBranchNode;
	struct MatchPatternNode;
	struct ParameterNode;
	struct ReturnNode;
	struct RootNode;
	struct SelfNode;
	struct SignalNode;
	struct SubscriptNode;
	struct SuperNode;
	struct TernaryOperatorNode;
	struct TypeNode;
	struct TypeAliasNode;
	struct TypeTestNode;
	struct UnaryOperatorNode;
	struct VariableNode;
	struct WhileNode;

	enum class DesiredStatus {
		UNRESOLVED,
		RESOLVED_INHERITANCE,
		RESOLVED_INTERFACE,
		RESOLVED_BODY,
	};

	class Context {
		ClassNode *current_class = nullptr;
		FunctionNode *current_function = nullptr;
		Node *current_loop = nullptr;
		BlockNode *current_block = nullptr;

	public:
		_FORCE_INLINE_ ClassNode *get_class() const { return current_class; }
		_FORCE_INLINE_ FunctionNode *get_function() const { return current_function; }
		_FORCE_INLINE_ Node *get_loop() const { return current_loop; }
		_FORCE_INLINE_ BlockNode *get_block() const { return current_block; }

		Context with_class(ClassNode *p_class) const;
		Context with_function(FunctionNode *p_function) const;
		Context with_loop(Node *p_loop) const;
		Context with_block(BlockNode *p_block) const;

		Context() {}
	};

	struct Symbol {
		// TODO: Add an `INHERITED_MEMBER` type?
		enum class Type {
			NONE,
			DECLARATION, // The symbol belongs to the current AST.
			EXTERNAL, // The symbol does not belong to the current AST.
		};

		Type type = Type::NONE;
		Node *declaration = nullptr;
		StringName external; // TODO: Rename to `name`? Reference external symbols with FQSN?

		Symbol() {}
	};

	struct AnnotationInfo {
		using Action = void (AST::*)(AnnotationNode *p_annotation, Node *p_target);

		enum class Target {
			NONE = 0,
			// clang-format off
			SCRIPT     = 1 << 0,
			TYPE_ALIAS = 1 << 1,
			CLASS      = 1 << 2,
			ENUM       = 1 << 3,
			CONSTANT   = 1 << 4,
			VARIABLE   = 1 << 5,
			FUNCTION   = 1 << 6,
			SIGNAL     = 1 << 7,
			STATEMENT  = 1 << 8,
			STANDALONE = 1 << 9,
			// clang-format on
			INDEPENDENT = SCRIPT | STANDALONE,
			CLASS_LEVEL = TYPE_ALIAS | CLASS | ENUM | CONSTANT | VARIABLE | FUNCTION | SIGNAL,
		};

		BitField<Target> targets = Target::NONE;
		Action action = nullptr;
		MethodInfo method_info;

		AnnotationInfo() {}
	};

	struct Node {
		friend class AST;

		enum class Type {
			NONE,
			ANNOTATION,
			ARRAY,
			ASSERT,
			ASSIGNMENT,
			AWAIT,
			BINARY_OPERATOR,
			BLOCK,
			BREAK,
			BREAKPOINT,
			CALL,
			CAST,
			CLASS,
			CONSTANT,
			CONTINUE,
			DICTIONARY,
			ENUM,
			FOR,
			FUNCTION,
			GET_NODE,
			IDENTIFIER,
			IF,
			LAMBDA,
			LITERAL,
			MATCH,
			MATCH_BRANCH,
			MATCH_PATTERN,
			PARAMETER,
			RETURN,
			ROOT,
			SELF,
			SIGNAL,
			SUBSCRIPT,
			SUPER,
			TERNARY_OPERATOR,
			TYPE,
			TYPE_ALIAS,
			TYPE_TEST,
			UNARY_OPERATOR,
			VARIABLE,
			WHILE,
		};

		enum class Status {
			UNRESOLVED,
			RESOLVING_INHERITANCE,
			RESOLVED_INHERITANCE,
			RESOLVING_INTERFACE,
			RESOLVED_INTERFACE,
			RESOLVING_BODY,
			RESOLVED_BODY,
		};

	private:
		Node *prev_node = nullptr;

	protected:
		Type type = Type::NONE;

	public:
		Status status = Status::UNRESOLVED;
		SourceRegion source_region;
		LocalVector<AnnotationNode *> annotations;
		Node *parent_node = nullptr;

	protected:
		Node() {}

	public:
		_FORCE_INLINE_ Type get_type() const { return type; }

		virtual bool is_assignable() const { return false; }
		virtual bool is_expression() const { return false; }

		bool is_declaration() const;
		bool is_type_declaration() const;
		IdentifierNode *get_declaration_identifier() const;
		StringName get_declaration_name() const;

		_FORCE_INLINE_ bool is_loop() const { return type == Type::FOR || type == Type::WHILE; }

		_FORCE_INLINE_ bool is_resolving() const {
			return status == Status::RESOLVING_INHERITANCE ||
					status == Status::RESOLVING_INTERFACE ||
					status == Status::RESOLVING_BODY;
		}

		String get_node_raw_type_name() const;
		String get_node_type_name() const;
		AnnotationInfo::Target get_annotation_target() const;

		SourceRegion get_header_source_region() const;

		ClassNode *find_parent_class() const;
		FunctionNode *find_parent_function() const;
		Node *find_parent_loop() const;
		BlockNode *find_parent_block() const;

		Node(const Node &p_other) = delete;
		Node(Node &&p_other) = delete;
		Node &operator=(const Node &p_other) = delete;
		Node &operator=(Node &&p_other) = delete;
		virtual ~Node() {}
	};

	struct AssignableNode : public Node {
		IdentifierNode *identifier = nullptr;
		ExpressionNode *initializer = nullptr;
		TypeNode *type_specifier = nullptr;
		bool infer_datatype = false;

		DataType datatype;

		virtual bool is_assignable() const override { return true; }

	protected:
		AssignableNode() {}
	};

	struct ExpressionNode : public Node {
		enum class Constness {
			NONE, // The value is unknown at compile time.
			PARTIAL, // The value is known at compile time, but it is a reference type in a dynamic context.
			FULL, // The exact value is known at compile time.
		};

		DataType datatype;
		Symbol symbol;

		bool is_recovery = false;

		Constness constness = Constness::NONE;
		Variant constant_value;

		virtual bool is_expression() const override { return true; }

		_FORCE_INLINE_ void mark_as_partially_constant() { constness = Constness::PARTIAL; }
		_FORCE_INLINE_ void mark_as_fully_constant() { constness = Constness::FULL; }

		_FORCE_INLINE_ bool is_any_degree_constant() const { return constness != Constness::NONE; }
		_FORCE_INLINE_ bool is_partially_constant() const { return constness == Constness::PARTIAL; }
		_FORCE_INLINE_ bool is_fully_constant() const { return constness == Constness::FULL; }

	protected:
		ExpressionNode() {}
	};

	struct AnnotationNode : public Node {
		StringName name;
		LocalVector<ExpressionNode *> arguments;
		LocalVector<Variant> resolved_arguments;
		AnnotationInfo *info = nullptr;
		bool is_applied = false;

		bool applies_to(BitField<AnnotationInfo::Target> p_targets) const;
		void apply(AST &p_ast, Node *p_target);

		AnnotationNode() { type = Type::ANNOTATION; }
	};

	struct ArrayNode : public ExpressionNode {
		LocalVector<ExpressionNode *> elements;

		ArrayNode() { type = Type::ARRAY; }
	};

	struct AssertNode : public Node {
		ExpressionNode *condition = nullptr;
		ExpressionNode *message = nullptr;

		AssertNode() { type = Type::ASSERT; }
	};

	// Assignment is not really an expression but it's easier to parse as if it were.
	struct AssignmentNode : public ExpressionNode {
		enum class Operation {
			NONE,
			ADDITION,
			SUBTRACTION,
			MULTIPLICATION,
			DIVISION,
			MODULO,
			POWER,
			BIT_SHIFT_LEFT,
			BIT_SHIFT_RIGHT,
			BIT_AND,
			BIT_OR,
			BIT_XOR,
		};

		Operation operation = Operation::NONE;
		ExpressionNode *assignee = nullptr;
		ExpressionNode *assigned_value = nullptr;

		String get_operator_name() const;
		Variant::Operator get_variant_operator() const;

		AssignmentNode() { type = Type::ASSIGNMENT; }
	};

	struct AwaitNode : public ExpressionNode {
		ExpressionNode *operand = nullptr;

		AwaitNode() { type = Type::AWAIT; }
	};

	struct BinaryOperatorNode : public ExpressionNode {
		enum class Operation {
			ADDITION,
			SUBTRACTION,
			MULTIPLICATION,
			DIVISION,
			MODULO,
			POWER,
			BIT_LEFT_SHIFT,
			BIT_RIGHT_SHIFT,
			BIT_AND,
			BIT_OR,
			BIT_XOR,
			LOGIC_AND,
			LOGIC_OR,
			CONTENT_TEST,
			COMP_EQUAL,
			COMP_NOT_EQUAL,
			COMP_LESS,
			COMP_LESS_EQUAL,
			COMP_GREATER,
			COMP_GREATER_EQUAL,
		};

		Operation operation = Operation::ADDITION;
		ExpressionNode *left_operand = nullptr;
		ExpressionNode *right_operand = nullptr;

		String get_operator_name() const;
		Variant::Operator get_variant_operator() const;

		BinaryOperatorNode() { type = Type::BINARY_OPERATOR; }
	};

	struct BlockNode : public Node {
		LocalVector<Node *> statements;

		BlockNode() { type = Type::BLOCK; }
	};

	struct BreakNode : public Node {
		BreakNode() { type = Type::BREAK; }
	};

	struct BreakpointNode : public Node {
		BreakpointNode() { type = Type::BREAKPOINT; }
	};

	struct CallNode : public ExpressionNode {
		ExpressionNode *callee = nullptr;
		LocalVector<ExpressionNode *> arguments;

		CallNode() { type = Type::CALL; }
	};

	struct CastNode : public ExpressionNode {
		ExpressionNode *operand = nullptr;
		TypeNode *cast_type = nullptr;

		CastNode() { type = Type::CAST; }
	};

	struct ClassNode : public Node {
		IdentifierNode *identifier = nullptr;
		TypeNode *extends_type = nullptr;
		LocalVector<Node *> members;

		CFG::SymbolTable symbol_table;
		DataType self_datatype;
		DataType base_datatype;
		String simplified_icon_path;
		bool is_abstract = false;
		bool has_icon = false;

		ClassNode() { type = Type::CLASS; }
	};

	struct ConstantNode : public AssignableNode {
		ConstantNode() { type = Type::CONSTANT; }
	};

	struct ContinueNode : public Node {
		ContinueNode() { type = Type::CONTINUE; }
	};

	struct DictionaryNode : public ExpressionNode {
		enum class Style {
			PYTHON_DICT,
			LUA_TABLE,
		};

		struct Pair {
			ExpressionNode *key = nullptr;
			ExpressionNode *value = nullptr;

			Pair() {}
		};

		Style style = Style::PYTHON_DICT;
		LocalVector<Pair> elements;

		DictionaryNode() { type = Type::DICTIONARY; }
	};

	struct EnumNode : public Node {
		struct Item {
			IdentifierNode *identifier = nullptr;
			ExpressionNode *custom_value = nullptr;

			Item() {}
		};

		IdentifierNode *identifier = nullptr;
		TypeNode *underlying_type = nullptr;
		LocalVector<Item> items;

		DataType underlying_datatype;

		EnumNode() { type = Type::ENUM; }
	};

	struct ForNode : public Node {
		IdentifierNode *iterator = nullptr;
		TypeNode *iterator_type = nullptr;
		ExpressionNode *iterable = nullptr;
		BlockNode *loop = nullptr;

		DataType iterator_datatype;

		ForNode() { type = Type::FOR; }
	};

	struct FunctionNode : public Node {
		IdentifierNode *identifier = nullptr;
		LocalVector<ParameterNode *> parameters;
		TypeNode *return_type = nullptr;
		SourcePosition header_end;
		BlockNode *body = nullptr;

		DataType return_datatype;
		bool is_abstract = false;
		bool is_static = false;
		bool rest_used = false;

		CFG cfg;

		_FORCE_INLINE_ bool is_variadic() const { return rest_used; }

		FunctionNode() { type = Type::FUNCTION; }
	};

	struct GetNodeNode : public ExpressionNode {
		String full_path;
		bool use_dollar = true;

		GetNodeNode() { type = Type::GET_NODE; }
	};

	struct IdentifierNode : public ExpressionNode {
		StringName name;

		IdentifierNode() { type = Type::IDENTIFIER; }
	};

	struct IfNode : public Node {
		ExpressionNode *condition = nullptr;
		BlockNode *true_block = nullptr;
		BlockNode *false_block = nullptr;

		IfNode() { type = Type::IF; }
	};

	struct LambdaNode : public ExpressionNode {
		FunctionNode *function = nullptr;

		LambdaNode() { type = Type::LAMBDA; }
	};

	struct LiteralNode : public ExpressionNode {
		// NOTE: `value` is not necessarily equal to `constant_value`. In a typed context,
		// `constant_value` can be converted to the expected type (like `const X: float = 1`).
		Variant value;

		LiteralNode() { type = Type::LITERAL; }
	};

	struct MatchNode : public Node {
		ExpressionNode *test = nullptr;
		LocalVector<MatchBranchNode *> branches;

		MatchNode() { type = Type::MATCH; }
	};

	struct MatchBranchNode : public Node {
		LocalVector<MatchPatternNode *> patterns;
		ExpressionNode *pattern_guard = nullptr;
		BlockNode *block = nullptr;

		MatchBranchNode() { type = Type::MATCH_BRANCH; }
	};

	struct MatchPatternNode : public Node {
		enum class PatternType {
			EXPRESSION,
			BIND,
			ARRAY,
			DICTIONARY,
			WILDCARD,
		};

		// TODO: Add Lua style support for dictionary patterns?
		struct Element {
			bool is_rest = false;
			ExpressionNode *key = nullptr; // For dictionary patterns.
			MatchPatternNode *value = nullptr;

			Element() {}
		};

		PatternType pattern_type = PatternType::EXPRESSION;
		union {
			ExpressionNode *expression = nullptr;
			IdentifierNode *bind;
		};
		LocalVector<Element> elements; // For array and dictionary patterns.

		bool rest_used = false; // For array and dictionary patterns.
		//bool has_binds = false; // TODO

		MatchPatternNode() { type = Type::MATCH_PATTERN; }
	};

	struct ParameterNode : public AssignableNode {
		bool is_rest = false; // For variadic functions.

		ParameterNode() { type = Type::PARAMETER; }
	};

	struct ReturnNode : public Node {
		ExpressionNode *return_value = nullptr;

		ReturnNode() { type = Type::RETURN; }
	};

	struct RootNode : public Node {
		ClassNode *main_class = nullptr;
		HashMap<int, Comment> comments;
		bool is_tool = false;

		RootNode() { type = Type::ROOT; }
	};

	struct SelfNode : public ExpressionNode {
		SelfNode() { type = Type::SELF; }
	};

	struct SignalNode : public Node {
		IdentifierNode *identifier = nullptr;
		LocalVector<ParameterNode *> parameters;

		SignalNode() { type = Type::SIGNAL; }
	};

	struct SubscriptNode : public ExpressionNode {
		ExpressionNode *base = nullptr;
		bool is_attribute = false;
		union {
			ExpressionNode *index = nullptr;
			IdentifierNode *attribute;
		};

		SubscriptNode() { type = Type::SUBSCRIPT; }
	};

	struct SuperNode : public ExpressionNode {
		IdentifierNode *identifier = nullptr;

		SuperNode() { type = Type::SUPER; }
	};

	struct TernaryOperatorNode : public ExpressionNode {
		// Only one ternary operation exists, so no abstraction here.
		ExpressionNode *condition = nullptr;
		ExpressionNode *true_expr = nullptr;
		ExpressionNode *false_expr = nullptr;

		TernaryOperatorNode() { type = Type::TERNARY_OPERATOR; }
	};

	struct TypeNode : public Node {
		TypeNode *outer_type = nullptr;
		bool is_recovery = false;
		bool is_file_path = false;
		union {
			IdentifierNode *identifier = nullptr;
			LiteralNode *file_path;
		};
		LocalVector<TypeNode *> parameters;

		DataType denoted_datatype;

		TypeNode() { type = Type::TYPE; }
	};

	struct TypeAliasNode : public Node {
		IdentifierNode *identifier = nullptr;
		TypeNode *source_type = nullptr;

		TypeAliasNode() { type = Type::TYPE_ALIAS; }
	};

	struct TypeTestNode : public ExpressionNode {
		ExpressionNode *operand = nullptr;
		TypeNode *test_type = nullptr;

		TypeTestNode() { type = Type::TYPE_TEST; }
	};

	struct UnaryOperatorNode : public ExpressionNode {
		enum class Operation {
			POSITIVE,
			NEGATIVE,
			COMPLEMENT,
			LOGIC_NOT,
		};

		Operation operation = Operation::POSITIVE;
		ExpressionNode *operand = nullptr;

		String get_operator_name() const;
		Variant::Operator get_variant_operator() const;

		UnaryOperatorNode() { type = Type::UNARY_OPERATOR; }
	};

	struct VariableNode : public AssignableNode {
		enum class PropertyStyle {
			NONE,
			INLINE,
			POINTER,
		};

		struct Property {
			IdentifierNode *pointer_key = nullptr;
			union {
				FunctionNode *inline_function = nullptr;
				IdentifierNode *pointer_value;
			};

			Property() {}
		};

		PropertyStyle property_style = PropertyStyle::NONE;
		LocalVector<Property> properties;
		bool is_static = false;

		VariableNode() { type = Type::VARIABLE; }
	};

	struct WhileNode : public Node {
		ExpressionNode *condition = nullptr;
		BlockNode *loop = nullptr;

		WhileNode() { type = Type::WHILE; }
	};

private:
	struct Data {
		SafeRefCount ref_count;
		RootNode *root = nullptr;
		Node *last_node = nullptr;
		String script_path;
		String source;
		DiagnosticList diagnostic_list;
		HashMap<WarningDB::Code, AnnotationNode *> open_warning_ignore_regions;

		Data() {}
	};

	Data *data = nullptr;

	void _clear();
	void _share_from(const AST &p_other);
	void _move_from(AST &&p_other);

	_FORCE_INLINE_ void push_error(const Node *p_origin, const String &p_message) {
		data->diagnostic_list.push_error(p_origin->source_region, p_message);
	}

public:
	static void initialize();
	static void deinitialize();

	_FORCE_INLINE_ bool is_null() const { return data == nullptr; }

	template <typename T>
	T *alloc_node() {
		T *node = memnew(T);
		node->prev_node = data->last_node;
		data->last_node = node;
		return node;
	}

	void set_root(RootNode *p_root);

	_FORCE_INLINE_ RootNode *get_root() const { return data->root; }
	_FORCE_INLINE_ const String &get_script_path() const { return data->script_path; }
	_FORCE_INLINE_ const String &get_source() const { return data->source; }
	_FORCE_INLINE_ DiagnosticList &get_diagnostic_list() const { return data->diagnostic_list; }

	AST() {}
	AST(const String &p_script_path, const String &p_source);

	AST(const AST &p_other);
	AST(AST &&p_other);
	AST &operator=(const AST &p_other);
	AST &operator=(AST &&p_other);
	~AST();

	// ----- Annotations -----
private:
	static HashMap<StringName, AnnotationInfo> annotations;

	static void register_annotation(
			const String &p_name,
			BitField<AnnotationInfo::Target> p_targets,
			AnnotationInfo::Action p_action);
	static void register_annotation_arguments(
			const String &p_name,
			const Vector<PropertyInfo> &p_arguments,
			const Vector<Variant> &p_default_arguments,
			bool p_is_vararg);

	static void register_annotations();
	static void unregister_annotations();

	void tool_annotation(AnnotationNode *p_annotation, Node *p_target);
	void icon_annotation(AnnotationNode *p_annotation, Node *p_target);
	void abstract_annotation(AnnotationNode *p_annotation, Node *p_target);
	void static_annotation(AnnotationNode *p_annotation, Node *p_target);
	void onready_annotation(AnnotationNode *p_annotation, Node *p_target);
	template <PropertyHint t_hint, Variant::Type t_type>
	void export_annotation(AnnotationNode *p_annotation, Node *p_target);
	void export_storage_annotation(AnnotationNode *p_annotation, Node *p_target);
	void export_custom_annotation(AnnotationNode *p_annotation, Node *p_target);
	void export_tool_button_annotation(AnnotationNode *p_annotation, Node *p_target);
	template <PropertyUsageFlags t_usage>
	void export_group_annotation(AnnotationNode *p_annotation, Node *p_target);
	void warning_ignore_annotation(AnnotationNode *p_annotation, Node *p_target);
	void warning_ignore_region_annotation(AnnotationNode *p_annotation, Node *p_target);
	void rpc_annotation(AnnotationNode *p_annotation, Node *p_target);

public:
	_FORCE_INLINE_ static bool annotation_exists(const StringName &p_name) {
		return annotations.has(p_name);
	}

	_FORCE_INLINE_ static AnnotationInfo *get_annotation_info(const StringName &p_name) {
		return annotations.getptr(p_name);
	}

	static bool is_export_grouping_annotation(const StringName &p_name);
	static bool is_warning_region_annotation(const StringName &p_name);

	void close_warning_regions(const SourcePosition &p_end);
};

} // namespace gdscript
