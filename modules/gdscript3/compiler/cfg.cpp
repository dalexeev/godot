/**************************************************************************/
/*  cfg.cpp                                                               */
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

#include "cfg.h"

using namespace gdscript;

// ===== CFG::SymbolTable =====

uint32_t CFG::SymbolTable::add_global(const StringName &p_name, const DataType &p_datatype) {
	ERR_FAIL_COND_V(p_name.is_empty(), UINT32_MAX);

	uint32_t index = 0;
	if (data->global_indices.has(p_name)) {
		index = data->global_indices[p_name];
	} else {
		index = data->globals.size();
		data->globals.push_back(Symbol(p_name, p_datatype));
		data->global_indices[p_name] = index;
	}

	return index;
}

uint32_t CFG::SymbolTable::add_static_variable(const StringName &p_name, const DataType &p_datatype) {
	ERR_FAIL_COND_V(p_name.is_empty(), UINT32_MAX);

	uint32_t index = 0;
	if (data->static_variable_indices.has(p_name)) {
		index = data->static_variable_indices[p_name];
	} else {
		index = data->static_variables.size();
		data->static_variables.push_back(Symbol(p_name, p_datatype));
		data->static_variable_indices[p_name] = index;
	}

	return index;
}

uint32_t CFG::SymbolTable::add_member(const StringName &p_name, const DataType &p_datatype) {
	ERR_FAIL_COND_V(p_name.is_empty(), UINT32_MAX);

	uint32_t index = 0;
	if (data->member_indices.has(p_name)) {
		index = data->member_indices[p_name];
	} else {
		index = data->members.size();
		data->members.push_back(Symbol(p_name, p_datatype));
		data->member_indices[p_name] = index;
	}

	return index;
}

void CFG::SymbolTable::_clear() {
	if (data == nullptr) {
		return;
	}

	if (data->ref_count.unref()) {
		memdelete(data);
	}

	data = nullptr;
}

void CFG::SymbolTable::_share_from(const SymbolTable &p_other) {
	data = p_other.data;
	if (data != nullptr) {
		data->ref_count.ref();
	}
}

void CFG::SymbolTable::_move_from(SymbolTable &&p_other) {
	data = p_other.data;
	p_other.data = nullptr;
}

CFG::SymbolTable::SymbolTable(bool p_instantiate) {
	if (p_instantiate) {
		data = memnew(Data);
		data->ref_count.init();
	}
}

CFG::SymbolTable::SymbolTable(const SymbolTable &p_other) {
	_share_from(p_other);
}

CFG::SymbolTable::SymbolTable(SymbolTable &&p_other) {
	_move_from(std::move(p_other));
}

CFG::SymbolTable &CFG::SymbolTable::operator=(const SymbolTable &p_other) {
	if (this == &p_other) {
		return *this;
	}

	_clear();
	_share_from(p_other);

	return *this;
}

CFG::SymbolTable &CFG::SymbolTable::operator=(SymbolTable &&p_other) {
	if (data == p_other.data) {
		return *this;
	}

	_clear();
	_move_from(std::move(p_other));

	return *this;
}

CFG::SymbolTable::~SymbolTable() {
	_clear();
}

// ===== CFG::Instruction =====

CFG::Instruction CFG::Instruction::make_assign(Address p_destination, Address p_source) {
	Assign *data = memnew(Assign);
	data->destination = p_destination;
	data->source = p_source;

	return Instruction(Type::ASSIGN, data);
}

CFG::Instruction CFG::Instruction::make_type_test(Address p_destination, Address p_operand, const DataType &p_datatype) {
	TypeTest *data = memnew(TypeTest);
	data->destination = p_destination;
	data->operand = p_operand;
	data->datatype = p_datatype;

	return Instruction(Type::TYPE_TEST, data);
}

CFG::Instruction CFG::Instruction::make_cast(Address p_destination, Address p_operand, const DataType &p_datatype) {
	Cast *data = memnew(Cast);
	data->destination = p_destination;
	data->operand = p_operand;
	data->datatype = p_datatype;

	return Instruction(Type::CAST, data);
}

CFG::Instruction CFG::Instruction::make_await(Address p_destination, Address p_operand) {
	Await *data = memnew(Await);
	data->destination = p_destination;
	data->operand = p_operand;

	return Instruction(Type::AWAIT, data);
}

CFG::Instruction CFG::Instruction::make_set_attribute(Address p_base, const StringName &p_name, Address p_value) {
	SetAttribute *data = memnew(SetAttribute);
	data->base = p_base;
	data->name = p_name;
	data->value = p_value;

	return Instruction(Type::SET_ATTRIBUTE, data);
}

CFG::Instruction CFG::Instruction::make_get_attribute(Address p_destination, Address p_base, const StringName &p_name) {
	GetAttribute *data = memnew(GetAttribute);
	data->destination = p_destination;
	data->base = p_base;
	data->name = p_name;

	return Instruction(Type::GET_ATTRIBUTE, data);
}

CFG::Instruction CFG::Instruction::make_set_index(Address p_base, Address p_index, Address p_value) {
	SetIndex *data = memnew(SetIndex);
	data->base = p_base;
	data->index = p_index;
	data->value = p_value;

	return Instruction(Type::SET_INDEX, data);
}

CFG::Instruction CFG::Instruction::make_get_index(Address p_destination, Address p_base, Address p_index) {
	GetIndex *data = memnew(GetIndex);
	data->destination = p_destination;
	data->base = p_base;
	data->index = p_index;

	return Instruction(Type::GET_INDEX, data);
}

CFG::Instruction CFG::Instruction::make_call_global_function(
		Address p_destination,
		const StringName &p_function_name,
		const LocalVector<Address> &p_arguments) {
	CallGlobalFunction *data = memnew(CallGlobalFunction);
	data->destination = p_destination;
	data->function_name = p_function_name;
	data->arguments = p_arguments;

	return Instruction(Type::CALL_GLOBAL_FUNCTION, data);
}

CFG::Instruction CFG::Instruction::make_call_constructor(
		Address p_destination,
		const DataType &p_datatype,
		const LocalVector<Address> &p_arguments) {
	CallConstructor *data = memnew(CallConstructor);
	data->destination = p_destination;
	data->datatype = p_datatype;
	data->arguments = p_arguments;

	return Instruction(Type::CALL_CONSTRUCTOR, data);
}

CFG::Instruction CFG::Instruction::make_call_operator(
		Address p_destination,
		Variant::Operator p_variant_operator,
		Address p_left_operand,
		Address p_right_operand) {
	CallOperator *data = memnew(CallOperator);
	data->destination = p_destination;
	data->variant_operator = p_variant_operator;
	data->left_operand = p_left_operand;
	data->right_operand = p_right_operand;

	return Instruction(Type::CALL_OPERATOR, data);
}

CFG::Instruction CFG::Instruction::make_call_method(
		Address p_destination,
		Address p_base,
		const StringName &p_method_name,
		const LocalVector<Address> &p_arguments) {
	CallMethod *data = memnew(CallMethod);
	data->destination = p_destination;
	data->base = p_base;
	data->method_name = p_method_name;
	data->arguments = p_arguments;

	return Instruction(Type::CALL_METHOD, data);
}

CFG::Instruction CFG::Instruction::make_create_lambda(
		Address p_destination,
		CFG *p_function,
		const LocalVector<Address> &p_captures) {
	ERR_FAIL_NULL_V(p_function, Instruction());

	CreateLambda *data = memnew(CreateLambda);
	data->destination = p_destination;
	data->function = p_function;
	data->captures = p_captures;

	return Instruction(Type::CREATE_LAMBDA, data);
}

CFG::Instruction CFG::Instruction::make_introduce_argument(Address p_address) {
	IntroduceArgument *data = memnew(IntroduceArgument);
	data->address = p_address;

	return Instruction(Type::INTRODUCE_ARGUMENT, data);
}

CFG::Instruction CFG::Instruction::make_keep_alive(Address p_address) {
	KeepAlive *data = memnew(KeepAlive);
	data->address = p_address;

	return Instruction(Type::KEEP_ALIVE, data);
}

void CFG::Instruction::_clear() {
#define CLEAR_DATA(m_type, m_class) \
	case Type::m_type: \
		memdelete(static_cast<m_class *>(data)); \
		break

	// clang-format off
	switch (type) {
		case Type::NONE:
			return; // Already clean.

		CLEAR_DATA( ASSIGN,                Assign             );
		CLEAR_DATA( TYPE_TEST,             TypeTest           );
		CLEAR_DATA( CAST,                  Cast               );
		CLEAR_DATA( AWAIT,                 Await              );
		CLEAR_DATA( SET_ATTRIBUTE,         SetAttribute       );
		CLEAR_DATA( GET_ATTRIBUTE,         GetAttribute       );
		CLEAR_DATA( SET_INDEX,             SetIndex           );
		CLEAR_DATA( GET_INDEX,             GetIndex           );
		CLEAR_DATA( CALL_GLOBAL_FUNCTION,  CallGlobalFunction );
		CLEAR_DATA( CALL_CONSTRUCTOR,      CallConstructor    );
		CLEAR_DATA( CALL_OPERATOR,         CallOperator       );
		CLEAR_DATA( CALL_METHOD,           CallMethod         );
		CLEAR_DATA( CREATE_LAMBDA,         CreateLambda       );
		CLEAR_DATA( INTRODUCE_ARGUMENT,    IntroduceArgument  );
		CLEAR_DATA( KEEP_ALIVE,            KeepAlive          );
	}
	// clang-format on

	type = Type::NONE;
	data = nullptr;

#undef CLEAR_DATA
}

void CFG::Instruction::_copy_from(const Instruction &p_other) {
#define COPY_DATA(m_type, m_class) \
	case Type::m_type: { \
		m_class *copy = memnew(m_class); \
		*copy = *static_cast<m_class *>(p_other.data); \
		data = copy; \
	} break

	// clang-format off
	switch (p_other.type) {
		case Type::NONE:
			return; // Nothing to copy.

		COPY_DATA( ASSIGN,                Assign             );
		COPY_DATA( TYPE_TEST,             TypeTest           );
		COPY_DATA( CAST,                  Cast               );
		COPY_DATA( AWAIT,                 Await              );
		COPY_DATA( SET_ATTRIBUTE,         SetAttribute       );
		COPY_DATA( GET_ATTRIBUTE,         GetAttribute       );
		COPY_DATA( SET_INDEX,             SetIndex           );
		COPY_DATA( GET_INDEX,             GetIndex           );
		COPY_DATA( CALL_GLOBAL_FUNCTION,  CallGlobalFunction );
		COPY_DATA( CALL_CONSTRUCTOR,      CallConstructor    );
		COPY_DATA( CALL_OPERATOR,         CallOperator       );
		COPY_DATA( CALL_METHOD,           CallMethod         );
		COPY_DATA( CREATE_LAMBDA,         CreateLambda       );
		COPY_DATA( INTRODUCE_ARGUMENT,    IntroduceArgument  );
		COPY_DATA( KEEP_ALIVE,            KeepAlive          );
	}
	// clang-format on

	type = p_other.type;

#undef COPY_DATA
}

void CFG::Instruction::_move_from(Instruction &&p_other) {
	type = p_other.type;
	data = p_other.data;

	p_other.type = Type::NONE;
	p_other.data = nullptr;
}

CFG::Instruction::Instruction(const Instruction &p_other) {
	_copy_from(p_other);
}

CFG::Instruction::Instruction(Instruction &&p_other) {
	_move_from(std::move(p_other));
}

CFG::Instruction &CFG::Instruction::operator=(const Instruction &p_other) {
	if (this == &p_other) {
		return *this;
	}

	_clear();
	_copy_from(p_other);

	return *this;
}

CFG::Instruction &CFG::Instruction::operator=(Instruction &&p_other) {
	if (data == p_other.data) {
		return *this;
	}

	_clear();
	_move_from(std::move(p_other));

	return *this;
}

CFG::Instruction::~Instruction() {
	_clear();
}

// ===== CFG::Action =====

CFG::Action CFG::Action::make_next(BlockID p_next_block) {
	Next *data = memnew(Next);
	data->next_block = p_next_block;

	return Action(Type::NEXT, data);
}

CFG::Action CFG::Action::make_jump(BlockID p_jump_block) {
	Jump *data = memnew(Jump);
	data->jump_block = p_jump_block;

	return Action(Type::JUMP, data);
}

CFG::Action CFG::Action::make_jump_if(Address p_condition, BlockID p_jump_block, BlockID p_next_block) {
	JumpIf *data = memnew(JumpIf);
	data->condition = p_condition;
	data->jump_block = p_jump_block;
	data->next_block = p_next_block;

	return Action(Type::JUMP_IF, data);
}

CFG::Action CFG::Action::make_jump_if_not(Address p_condition, BlockID p_jump_block, BlockID p_next_block) {
	JumpIfNot *data = memnew(JumpIfNot);
	data->condition = p_condition;
	data->jump_block = p_jump_block;
	data->next_block = p_next_block;

	return Action(Type::JUMP_IF_NOT, data);
}

CFG::Action CFG::Action::make_jump_table(
		Address p_test_value,
		const HashMap<Variant, BlockID> &p_table,
		BlockID p_default_block) {
	JumpTable *data = memnew(JumpTable);
	data->test_value = p_test_value;
	data->table = p_table;
	data->default_block = p_default_block;

	return Action(Type::JUMP_TABLE, data);
}

CFG::Action CFG::Action::make_return(Address p_value) {
	Return *data = memnew(Return);
	data->value = p_value;

	return Action(Type::RETURN, data);
}

void CFG::Action::_clear() {
#define CLEAR_DATA(m_type, m_class) \
	case Type::m_type: \
		memdelete(static_cast<m_class *>(data)); \
		break

	// clang-format off
	switch (type) {
		case Type::NONE:
			return; // Already clean.

		CLEAR_DATA( NEXT,        Next      );
		CLEAR_DATA( JUMP,        Jump      );
		CLEAR_DATA( JUMP_IF,     JumpIf    );
		CLEAR_DATA( JUMP_IF_NOT, JumpIfNot );
		CLEAR_DATA( JUMP_TABLE,  JumpTable );
		CLEAR_DATA( RETURN,      Return    );
	}
	// clang-format on

	type = Type::NONE;
	data = nullptr;

#undef CLEAR_DATA
}

void CFG::Action::_copy_from(const Action &p_other) {
#define COPY_DATA(m_type, m_class) \
	case Type::m_type: { \
		m_class *copy = memnew(m_class); \
		*copy = *static_cast<m_class *>(p_other.data); \
		data = copy; \
	} break

	// clang-format off
	switch (p_other.type) {
		case Type::NONE:
			return; // Nothing to copy.

		COPY_DATA( NEXT,        Next      );
		COPY_DATA( JUMP,        Jump      );
		COPY_DATA( JUMP_IF,     JumpIf    );
		COPY_DATA( JUMP_IF_NOT, JumpIfNot );
		COPY_DATA( JUMP_TABLE,  JumpTable );
		COPY_DATA( RETURN,      Return    );
	}
	// clang-format on

	type = p_other.type;

#undef COPY_DATA
}

void CFG::Action::_move_from(Action &&p_other) {
	type = p_other.type;
	data = p_other.data;

	p_other.type = Type::NONE;
	p_other.data = nullptr;
}

CFG::Action::Action(const Action &p_other) {
	_copy_from(p_other);
}

CFG::Action::Action(Action &&p_other) {
	_move_from(std::move(p_other));
}

CFG::Action &CFG::Action::operator=(const Action &p_other) {
	if (this == &p_other) {
		return *this;
	}

	_clear();
	_copy_from(p_other);

	return *this;
}

CFG::Action &CFG::Action::operator=(Action &&p_other) {
	if (data == p_other.data) {
		return *this;
	}

	_clear();
	_move_from(std::move(p_other));

	return *this;
}

CFG::Action::~Action() {
	_clear();
}

// ===== CFG =====

uint32_t CFG::_add_stack_cell(const StringName &p_name, const DataType &p_datatype) {
	if (!stack_cell_indices.has(p_name)) {
		stack_cell_indices[p_name] = LocalVector<uint32_t>();
	}

	const uint32_t address_index = stack_cells.size();
	const uint32_t ssa_index = stack_cell_indices[p_name].size();

	stack_cells.push_back(StackCell(p_name, ssa_index, p_datatype));
	stack_cell_indices[p_name].push_back(address_index);

	return address_index;
}

CFG::Address CFG::add_local(const StringName &p_name, const DataType &p_datatype) {
	ERR_FAIL_COND_V(p_name.is_empty(), Address());

	return Address(Address::Type::STACK_CELL, _add_stack_cell(p_name, p_datatype));
}

CFG::Address CFG::add_temporary(const DataType &p_datatype) {
	return Address(Address::Type::STACK_CELL, _add_stack_cell(SNAME("%temp"), p_datatype));
}

CFG::Address CFG::add_constant(const Variant &p_value) {
	uint32_t index = 0;
	if (constant_indices.has(p_value)) {
		index = constant_indices[p_value];
	} else {
		index = constants.size();
		constants.push_back(p_value);
		constant_indices[p_value] = index;
	}

	return Address(Address::Type::CONSTANT, index);
}

CFG::Address CFG::add_global(const StringName &p_name, const DataType &p_datatype) {
	return Address(Address::Type::GLOBAL, symbols.add_global(p_name, p_datatype));
}

CFG::Address CFG::add_static_variable(const StringName &p_name, const DataType &p_datatype) {
	return Address(Address::Type::STATIC_VARIABLE, symbols.add_static_variable(p_name, p_datatype));
}

CFG::Address CFG::add_member(const StringName &p_name, const DataType &p_datatype) {
	return Address(Address::Type::MEMBER, symbols.add_member(p_name, p_datatype));
}

void CFG::set_symbol_table(const SymbolTable &p_symbol_table) {
	ERR_FAIL_COND(!blocks.is_empty()); // Symbol table can only be set when CFG is empty.

	symbols = p_symbol_table;
}

CFG::BlockID CFG::add_block() {
	Block block;

	// TODO: Handle block connections.

	max_block_id.id++;
	block.id = max_block_id;

	block_indices[block.id] = blocks.size();
	blocks.push_back(block);

	return block.id;
}

void CFG::remove_block(BlockID p_id) {
	ERR_FAIL_COND_MSG(!_has_block(p_id), "GDScript bug: CFG block not found.");

	const uint32_t index = block_indices[p_id];

	// TODO: Handle block connections.

	block_indices.erase(p_id);
	blocks.remove_at(index);

	for (uint32_t i = index; i < blocks.size(); i++) {
		block_indices[blocks[i].id] = i;
	}
}

CFG::BlockID CFG::get_block(uint32_t p_index) const {
	ERR_FAIL_UNSIGNED_INDEX_V(p_index, blocks.size(), BlockID());

	return blocks[p_index].id;
}

CFG::BlockID CFG::get_previous_block(BlockID p_id) const {
	ERR_FAIL_COND_V_MSG(!_has_block(p_id), BlockID(), "GDScript bug: CFG block not found.");

	const uint32_t index = block_indices[p_id];

	return (index > 0) ? blocks[index - 1].id : BlockID();
}

CFG::BlockID CFG::get_next_block(BlockID p_id) const {
	ERR_FAIL_COND_V_MSG(!_has_block(p_id), BlockID(), "GDScript bug: CFG block not found.");

	const uint32_t index = block_indices[p_id];

	return (index < blocks.size() - 1) ? blocks[index + 1].id : BlockID();
}

const LocalVector<CFG::BlockID> &CFG::block_get_predecessors(BlockID p_id) const {
	static const LocalVector<BlockID> dummy;
	ERR_FAIL_COND_V_MSG(!_has_block(p_id), dummy, "GDScript bug: CFG block not found.");

	return _get_block(p_id).predecessors;
}

const LocalVector<CFG::BlockID> &CFG::block_get_successors(BlockID p_id) const {
	static const LocalVector<BlockID> dummy;
	ERR_FAIL_COND_V_MSG(!_has_block(p_id), dummy, "GDScript bug: CFG block not found.");

	return _get_block(p_id).successors;
}

void CFG::block_add_phi_statement(BlockID p_id, const PhiStatement &p_phi_statement) {
	ERR_FAIL_COND_MSG(!_has_block(p_id), "GDScript bug: CFG block not found.");

	Block &block = _get_block(p_id);

	// TODO: Validate, handle block connections.

	block.phi_statements.push_back(p_phi_statement);
}

void CFG::block_remove_phi_statement(BlockID p_id, uint32_t p_index) {
	ERR_FAIL_COND_MSG(!_has_block(p_id), "GDScript bug: CFG block not found.");

	Block &block = _get_block(p_id);

	ERR_FAIL_UNSIGNED_INDEX(p_index, block.phi_statements.size());

	// TODO: Validate, handle block connections.

	block.phi_statements.remove_at(p_index);
}

const LocalVector<CFG::PhiStatement> &CFG::block_get_phi_statements(BlockID p_id) const {
	static const LocalVector<PhiStatement> dummy;
	ERR_FAIL_COND_V_MSG(!_has_block(p_id), dummy, "GDScript bug: CFG block not found.");

	return _get_block(p_id).phi_statements;
}

const LocalVector<CFG::Instruction> &CFG::block_get_instructions(BlockID p_id) const {
	static const LocalVector<Instruction> dummy;
	ERR_FAIL_COND_V_MSG(!_has_block(p_id), dummy, "GDScript bug: CFG block not found.");

	return _get_block(p_id).instructions;
}

LocalVector<CFG::Instruction> &CFG::block_get_instructions(BlockID p_id) {
	static LocalVector<Instruction> dummy;
	ERR_FAIL_COND_V_MSG(!_has_block(p_id), dummy, "GDScript bug: CFG block not found.");

	return _get_block(p_id).instructions;
}

void CFG::block_set_action(BlockID p_id, const Action &p_action) {
	ERR_FAIL_COND_MSG(!_has_block(p_id), "GDScript bug: CFG block not found.");

	Block &block = _get_block(p_id);

	// TODO: Validate new action, handle block connections.

	block.action = p_action;
}

const CFG::Action &CFG::block_get_action(BlockID p_id) const {
	static const Action dummy;
	ERR_FAIL_COND_V_MSG(!_has_block(p_id), dummy, "GDScript bug: CFG block not found.");

	return _get_block(p_id).action;
}
