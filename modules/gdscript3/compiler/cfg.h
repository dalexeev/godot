/**************************************************************************/
/*  cfg.h                                                                 */
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

#include "data_type.h"

#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/variant/variant.h"

namespace gdscript {

class CFG {
public:
	struct StackCell {
		StringName name;
		uint32_t ssa_index = 0;
		DataType datatype;

		StackCell() {}
		StackCell(const StringName &p_name, uint32_t p_ssa_index, const DataType &p_datatype) :
				name(p_name),
				ssa_index(p_ssa_index),
				datatype(p_datatype) {}
	};

	struct Symbol {
		StringName name;
		DataType datatype;

		Symbol() {}
		Symbol(const StringName &p_name, const DataType &p_datatype) :
				name(p_name),
				datatype(p_datatype) {}
	};

	// CFGs of the same class share a symbol table to save memory.
	class SymbolTable {
		struct Data {
			SafeRefCount ref_count;

			LocalVector<Symbol> globals;
			HashMap<StringName, uint32_t> global_indices;

			LocalVector<Symbol> static_variables;
			HashMap<StringName, uint32_t> static_variable_indices;

			LocalVector<Symbol> members;
			HashMap<StringName, uint32_t> member_indices;

			Data() {}
		};

		Data *data = nullptr;

		void _clear();
		void _share_from(const SymbolTable &p_other);
		void _move_from(SymbolTable &&p_other);

	public:
		_FORCE_INLINE_ bool is_null() const { return data == nullptr; }

		uint32_t add_global(const StringName &p_name, const DataType &p_datatype);
		uint32_t add_static_variable(const StringName &p_name, const DataType &p_datatype);
		uint32_t add_member(const StringName &p_name, const DataType &p_datatype);

		_FORCE_INLINE_ const LocalVector<Symbol> &get_globals() const { return data->globals; }
		_FORCE_INLINE_ const LocalVector<Symbol> &get_static_variables() const { return data->static_variables; }
		_FORCE_INLINE_ const LocalVector<Symbol> &get_members() const { return data->members; }

		SymbolTable(bool p_instantiate = false);

		SymbolTable(const SymbolTable &p_other);
		SymbolTable(SymbolTable &&p_other);
		SymbolTable &operator=(const SymbolTable &p_other);
		SymbolTable &operator=(SymbolTable &&p_other);
		~SymbolTable();
	};

	struct BlockID {
		uint32_t id = 0; // NOTE: `0` represents an invalid block.

		_ALWAYS_INLINE_ uint32_t hash() const { return HashMapHasherDefault::hash(id); }

		_ALWAYS_INLINE_ bool operator==(const BlockID &p_other) const { return id == p_other.id; }
		_ALWAYS_INLINE_ bool operator!=(const BlockID &p_other) const { return id != p_other.id; }
		_ALWAYS_INLINE_ bool operator<(const BlockID &p_other) const { return id < p_other.id; }

		BlockID() {}
	};

	static_assert(sizeof(BlockID) <= sizeof(void *));
	static_assert(std::is_trivially_copyable_v<BlockID>);

	struct Address {
		enum class Type : uint8_t {
			NONE,
			SELF,
			STACK_CELL,
			CONSTANT,
			GLOBAL,
			STATIC_VARIABLE,
			MEMBER,
		};

		Type type = Type::NONE;
		uint32_t index : 24;

		static constexpr Address self() { return Address(Type::SELF, 0); }

		_FORCE_INLINE_ bool is_assignable() const {
			return type == Type::STACK_CELL || type == Type::STATIC_VARIABLE || type == Type::MEMBER;
		}

		constexpr Address() :
				index(0) {}
		constexpr Address(Type p_type, uint32_t p_index) :
				type(p_type),
				index(p_index) {}
	};

	static_assert(sizeof(Address) <= sizeof(void *));
	static_assert(std::is_trivially_copyable_v<Address>);

	struct PhiStatement {
		Address destination;
		LocalVector<Pair<BlockID, Address>> sources;

		PhiStatement() {}
		PhiStatement(Address p_destination, const LocalVector<Pair<BlockID, Address>> &p_sources) :
				destination(p_destination),
				sources(p_sources) {}
	};

	class Instruction {
	public:
		enum class Type {
			NONE,
			ASSIGN,
			TYPE_TEST,
			CAST,
			AWAIT,
			SET_ATTRIBUTE,
			GET_ATTRIBUTE,
			SET_INDEX,
			GET_INDEX,
			CALL_GLOBAL_FUNCTION,
			CALL_CONSTRUCTOR,
			CALL_OPERATOR,
			CALL_METHOD,
			CREATE_LAMBDA,
			// TODO: `ASSERT`, `BREAKPOINT`, `LINE`.
			INTRODUCE_ARGUMENT,
			KEEP_ALIVE,
		};

		struct Assign {
			Address destination;
			Address source;
		};

		struct TypeTest {
			Address destination;
			Address operand;
			DataType datatype;
		};

		struct Cast {
			Address destination;
			Address operand;
			DataType datatype;
		};

		struct Await {
			Address destination;
			Address operand;
		};

		struct SetAttribute {
			StringName name;
			Address base;
			Address value;
		};

		struct GetAttribute {
			StringName name;
			Address destination;
			Address base;
		};

		struct SetIndex {
			Address base;
			Address index;
			Address value;
		};

		struct GetIndex {
			Address destination;
			Address base;
			Address index;
		};

		struct CallGlobalFunction {
			Address destination;
			StringName function_name;
			LocalVector<Address> arguments;
		};

		struct CallConstructor {
			Address destination;
			DataType datatype;
			LocalVector<Address> arguments;
		};

		struct CallOperator {
			Variant::Operator variant_operator = Variant::OP_MAX;
			Address destination;
			Address left_operand;
			Address right_operand;
		};

		struct CallMethod {
			Address destination;
			Address base;
			StringName method_name;
			LocalVector<Address> arguments;
		};

		struct CreateLambda {
			Address destination;
			CFG *function = nullptr;
			LocalVector<Address> captures;
		};

		struct IntroduceArgument {
			Address address;
		};

		struct KeepAlive {
			Address address;
		};

	private:
		Type type = Type::NONE;
		void *data = nullptr;

		void _clear();
		void _copy_from(const Instruction &p_other);
		void _move_from(Instruction &&p_other);

		template <typename T, Type t_type>
		const T *_get_data() const {
			DEV_ASSERT(type == t_type);
			return static_cast<const T *>(data);
		}

		Instruction(Type p_type, void *p_data) :
				type(p_type),
				data(p_data) {}

	public:
		static Instruction make_assign(Address p_destination, Address p_source);
		static Instruction make_type_test(Address p_destination, Address p_operand, const DataType &p_datatype);
		static Instruction make_cast(Address p_destination, Address p_operand, const DataType &p_datatype);
		static Instruction make_await(Address p_destination, Address p_operand);
		static Instruction make_set_attribute(Address p_base, const StringName &p_name, Address p_value);
		static Instruction make_get_attribute(Address p_destination, Address p_base, const StringName &p_name);
		static Instruction make_set_index(Address p_base, Address p_index, Address p_value);
		static Instruction make_get_index(Address p_destination, Address p_base, Address p_index);
		static Instruction make_call_global_function(
				Address p_destination,
				const StringName &p_function_name,
				const LocalVector<Address> &p_arguments);
		static Instruction make_call_constructor(
				Address p_destination,
				const DataType &p_datatype,
				const LocalVector<Address> &p_arguments);
		static Instruction make_call_operator(
				Address p_destination,
				Variant::Operator p_variant_operator,
				Address p_left_operand,
				Address p_right_operand);
		static Instruction make_call_method(
				Address p_destination,
				Address p_base,
				const StringName &p_method_name,
				const LocalVector<Address> &p_arguments);
		static Instruction make_create_lambda(
				Address p_destination,
				CFG *p_function,
				const LocalVector<Address> &p_captures);
		static Instruction make_introduce_argument(Address p_address);
		static Instruction make_keep_alive(Address p_address);

		_FORCE_INLINE_ Type get_type() const { return type; }

#define DEFINE_CAST_HELPER(m_class, m_method, m_type) \
	_FORCE_INLINE_ const m_class *m_method() const { return _get_data<m_class, Type::m_type>(); }

		// clang-format off
		DEFINE_CAST_HELPER( Assign,             as_assign,               ASSIGN               )
		DEFINE_CAST_HELPER( TypeTest,           as_type_test,            TYPE_TEST            )
		DEFINE_CAST_HELPER( Cast,               as_cast,                 CAST                 )
		DEFINE_CAST_HELPER( Await,              as_await,                AWAIT                )
		DEFINE_CAST_HELPER( SetAttribute,       as_set_attribute,        SET_ATTRIBUTE        )
		DEFINE_CAST_HELPER( GetAttribute,       as_get_attribute,        GET_ATTRIBUTE        )
		DEFINE_CAST_HELPER( SetIndex,           as_set_index,            SET_INDEX            )
		DEFINE_CAST_HELPER( GetIndex,           as_get_index,            GET_INDEX            )
		DEFINE_CAST_HELPER( CallGlobalFunction, as_call_global_function, CALL_GLOBAL_FUNCTION )
		DEFINE_CAST_HELPER( CallConstructor,    as_call_constructor,     CALL_CONSTRUCTOR     )
		DEFINE_CAST_HELPER( CallOperator,       as_call_operator,        CALL_OPERATOR        )
		DEFINE_CAST_HELPER( CallMethod,         as_call_method,          CALL_METHOD          )
		DEFINE_CAST_HELPER( CreateLambda,       as_create_lambda,        CREATE_LAMBDA        )
		DEFINE_CAST_HELPER( IntroduceArgument,  as_introduce_argument,   INTRODUCE_ARGUMENT   )
		DEFINE_CAST_HELPER( KeepAlive,          as_keep_alive,           KEEP_ALIVE           )
		// clang-format on

#undef DEFINE_CAST_HELPER

		Instruction() {}

		Instruction(const Instruction &p_other);
		Instruction(Instruction &&p_other);
		Instruction &operator=(const Instruction &p_other);
		Instruction &operator=(Instruction &&p_other);
		~Instruction();
	};

	class Action {
	public:
		enum class Type {
			NONE,
			NEXT,
			JUMP,
			JUMP_IF,
			JUMP_IF_NOT,
			JUMP_TABLE,
			RETURN,
		};

		struct Next {
			BlockID next_block;
		};

		struct Jump {
			BlockID jump_block;
		};

		struct JumpIf {
			Address condition;
			BlockID jump_block;
			BlockID next_block;
		};

		struct JumpIfNot {
			Address condition;
			BlockID jump_block;
			BlockID next_block;
		};

		struct JumpTable {
			Address test_value;
			BlockID default_block;
			HashMap<Variant, BlockID> table; // TODO: Comparator.
		};

		struct Return {
			Address value;
		};

	private:
		Type type = Type::NONE;
		void *data = nullptr;

		void _clear();
		void _copy_from(const Action &p_other);
		void _move_from(Action &&p_other);

		template <typename T, Type t_type>
		const T *_get_data() const {
			DEV_ASSERT(type == t_type);
			return static_cast<const T *>(data);
		}

		Action(Type p_type, void *p_data) :
				type(p_type),
				data(p_data) {}

	public:
		static Action make_next(BlockID p_next_block);
		static Action make_jump(BlockID p_jump_block);
		static Action make_jump_if(Address p_condition, BlockID p_jump_block, BlockID p_next_block);
		static Action make_jump_if_not(Address p_condition, BlockID p_jump_block, BlockID p_next_block);
		static Action make_jump_table(
				Address p_test_value,
				const HashMap<Variant, BlockID> &p_table,
				BlockID p_default_block);
		static Action make_return(Address p_value);

		_FORCE_INLINE_ Type get_type() const { return type; }

#define DEFINE_CAST_HELPER(m_class, m_method, m_type) \
	_FORCE_INLINE_ const m_class *m_method() const { return _get_data<m_class, Type::m_type>(); }

		// clang-format off
		DEFINE_CAST_HELPER( Next,      as_next,        NEXT        )
		DEFINE_CAST_HELPER( Jump,      as_jump,        JUMP        )
		DEFINE_CAST_HELPER( JumpIf,    as_jump_if,     JUMP_IF     )
		DEFINE_CAST_HELPER( JumpIfNot, as_jump_if_not, JUMP_IF_NOT )
		DEFINE_CAST_HELPER( JumpTable, as_jump_table,  JUMP_TABLE  )
		DEFINE_CAST_HELPER( Return,    as_return,      RETURN      )
		// clang-format on

#undef DEFINE_CAST_HELPER

		Action() {}

		Action(const Action &p_other);
		Action(Action &&p_other);
		Action &operator=(const Action &p_other);
		Action &operator=(Action &&p_other);
		~Action();
	};

private:
	struct Block {
		BlockID id;
		LocalVector<BlockID> predecessors;
		LocalVector<BlockID> successors;
		LocalVector<PhiStatement> phi_statements;
		LocalVector<Instruction> instructions;
		Action action;

		Block() {}
	};

	LocalVector<Block> blocks;
	HashMap<BlockID, uint32_t> block_indices;
	BlockID max_block_id;

	LocalVector<StackCell> stack_cells;
	HashMap<StringName, LocalVector<uint32_t>> stack_cell_indices;

	LocalVector<Variant> constants;
	HashMap<Variant, uint32_t> constant_indices; // TODO: Comparator.

	SymbolTable symbols;

	_ALWAYS_INLINE_ bool _has_block(BlockID p_id) const { return block_indices.has(p_id); }
	_ALWAYS_INLINE_ const Block &_get_block(BlockID p_id) const { return blocks[block_indices[p_id]]; }
	_ALWAYS_INLINE_ Block &_get_block(BlockID p_id) { return blocks[block_indices[p_id]]; }

	uint32_t _add_stack_cell(const StringName &p_name, const DataType &p_datatype);

public:
	Address add_local(const StringName &p_name, const DataType &p_datatype);
	Address add_temporary(const DataType &p_datatype);
	Address add_constant(const Variant &p_value);
	Address add_global(const StringName &p_name, const DataType &p_datatype);
	Address add_static_variable(const StringName &p_name, const DataType &p_datatype);
	Address add_member(const StringName &p_name, const DataType &p_datatype);

	_FORCE_INLINE_ const LocalVector<StackCell> &get_stack_cells() const { return stack_cells; }
	_FORCE_INLINE_ const LocalVector<Variant> &get_constants() const { return constants; }
	_FORCE_INLINE_ const LocalVector<Symbol> &get_globals() const { return symbols.get_globals(); }
	_FORCE_INLINE_ const LocalVector<Symbol> &get_static_variables() const { return symbols.get_static_variables(); }
	_FORCE_INLINE_ const LocalVector<Symbol> &get_members() const { return symbols.get_members(); }

	void set_symbol_table(const SymbolTable &p_symbol_table);

	BlockID add_block();
	void remove_block(BlockID p_id);

	_FORCE_INLINE_ uint32_t get_block_count() const { return blocks.size(); }
	BlockID get_block(uint32_t p_index) const;

	BlockID get_previous_block(BlockID p_id) const;
	BlockID get_next_block(BlockID p_id) const;

	const LocalVector<BlockID> &block_get_predecessors(BlockID p_id) const;
	const LocalVector<BlockID> &block_get_successors(BlockID p_id) const;

	void block_add_phi_statement(BlockID p_id, const PhiStatement &p_phi_statement);
	void block_remove_phi_statement(BlockID p_id, uint32_t p_index);
	const LocalVector<PhiStatement> &block_get_phi_statements(BlockID p_id) const;

	const LocalVector<Instruction> &block_get_instructions(BlockID p_id) const;
	LocalVector<Instruction> &block_get_instructions(BlockID p_id);

	void block_set_action(BlockID p_id, const Action &p_action);
	const Action &block_get_action(BlockID p_id) const;

	CFG() {}
};

} // namespace gdscript
