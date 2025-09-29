/**************************************************************************/
/*  cfg_printer.cpp                                                       */
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

#include "cfg_printer.h"

using namespace gdscript;

void CFGPrinter::push_text(const String &p_text) {
	if (pending_newline) {
		output += "\n";
		output += indent_string;
		pending_newline = false;
	}
	output += p_text.replace("\n", "\\n");
}

void CFGPrinter::push_newline() {
	pending_newline = true;
}

void CFGPrinter::start_block() {
	indent_level++;
	indent_string = String("    ").repeat(indent_level);
	pending_newline = true;
}

void CFGPrinter::end_block() {
	ERR_FAIL_COND(indent_level <= 0);

	indent_level--;
	indent_string = String("    ").repeat(indent_level);
	pending_newline = true;
}

void CFGPrinter::validate_next_block(CFG::BlockID p_actual, CFG::BlockID p_expected) {
	if (p_actual != p_expected) {
		push_text(vformat("# ERROR: Invalid next block: %ud != %ud.", p_actual.id, p_expected.id));
		push_newline();
	}
}

void CFGPrinter::output_block_label(CFG::BlockID p_block) {
	push_text(vformat("@%ud", p_block.id));
}

void CFGPrinter::output_identifier(const StringName &p_name) {
	push_text(vformat("`%s`", p_name));
}

void CFGPrinter::output_datatype(const DataType &p_datatype) {
	push_text(p_datatype.to_debug_string());
}

void CFGPrinter::output_address(const CFG &p_cfg, CFG::Address p_address, bool p_with_datatype) {
	switch (p_address.type) {
		case CFG::Address::Type::NONE: {
			push_text("<none>");
		} break;
		case CFG::Address::Type::SELF: {
			push_text("self");
		} break;
		case CFG::Address::Type::STACK_CELL: {
			const LocalVector<CFG::StackCell> &stack_cells = p_cfg.get_stack_cells();
			if (p_address.index < stack_cells.size()) {
				const CFG::StackCell &stack_cell = stack_cells[p_address.index];
				push_text(vformat("%s.%ud", stack_cell.name, stack_cell.ssa_index));
				if (p_with_datatype) {
					push_text(": ");
					output_datatype(stack_cell.datatype);
				}
			} else {
				push_text("<invalid stack cell>");
			}
		} break;
		case CFG::Address::Type::CONSTANT: {
			const LocalVector<Variant> &constants = p_cfg.get_constants();
			if (p_address.index < constants.size()) {
				push_text(vformat("constant(%s)", constants[p_address.index].get_construct_string()));
			} else {
				push_text("<invalid constant>");
			}
		} break;
		case CFG::Address::Type::GLOBAL: {
			const LocalVector<CFG::Symbol> &globals = p_cfg.get_globals();
			if (p_address.index < globals.size()) {
				const CFG::Symbol &global = globals[p_address.index];
				push_text(vformat("global(%s)", global.name));
			} else {
				push_text("<invalid global>");
			}
		} break;
		case CFG::Address::Type::STATIC_VARIABLE: {
			const LocalVector<CFG::Symbol> &static_variables = p_cfg.get_static_variables();
			if (p_address.index < static_variables.size()) {
				const CFG::Symbol &global = static_variables[p_address.index];
				push_text(vformat("static_variable(%s)", global.name));
			} else {
				push_text("<invalid static variable>");
			}
		} break;
		case CFG::Address::Type::MEMBER: {
			const LocalVector<CFG::Symbol> &members = p_cfg.get_members();
			if (p_address.index < members.size()) {
				const CFG::Symbol &member = members[p_address.index];
				push_text(vformat("member(%s)", member.name));
			} else {
				push_text("<invalid member>");
			}
		} break;
	}
}

void CFGPrinter::output_arguments(const CFG &p_cfg, const LocalVector<CFG::Address> &p_arguments) {
	if (p_arguments.is_empty()) {
		push_text("()");
	} else {
		push_text("(");
		start_block();
		for (CFG::Address argument : p_arguments) {
			output_address(p_cfg, argument);
			push_text(",");
			push_newline();
		}
		end_block();
		push_text(")");
	}
}

void CFGPrinter::output_phi_statement(const CFG &p_cfg, const CFG::PhiStatement &p_phi_statement) {
	push_text("PHI ");
	output_address(p_cfg, p_phi_statement.destination, true);
	push_text(":");
	if (p_phi_statement.sources.is_empty()) {
		push_text(" # ERROR: Empty phi statement source list.");
		push_newline();
	} else {
		start_block();
		for (const Pair<CFG::BlockID, CFG::Address> &source : p_phi_statement.sources) {
			output_block_label(source.first);
			push_text(" -> ");
			output_address(p_cfg, source.second);
			push_newline();
		}
		end_block();
	}
}

void CFGPrinter::output_instruction(const CFG &p_cfg, const CFG::Instruction &p_instruction) {
	switch (p_instruction.get_type()) {
		case CFG::Instruction::Type::NONE: {
			push_text("# ERROR: Invalid instruction.");
		} break;
		case CFG::Instruction::Type::ASSIGN: {
			push_text("ASSIGN ");
			output_address(p_cfg, p_instruction.as_assign()->destination, true);
			push_text(" = ");
			output_address(p_cfg, p_instruction.as_assign()->source);
			push_newline();
		} break;
		case CFG::Instruction::Type::TYPE_TEST: {
			push_text("TYPE_TEST ");
			output_address(p_cfg, p_instruction.as_type_test()->destination, true);
			push_text(" = ");
			output_address(p_cfg, p_instruction.as_type_test()->operand);
			push_text(" IS ");
			output_datatype(p_instruction.as_type_test()->datatype);
			push_newline();
		} break;
		case CFG::Instruction::Type::CAST: {
			push_text("CAST ");
			output_address(p_cfg, p_instruction.as_cast()->destination, true);
			push_text(" = ");
			output_address(p_cfg, p_instruction.as_cast()->operand);
			push_text(" AS ");
			output_datatype(p_instruction.as_cast()->datatype);
			push_newline();
		} break;
		case CFG::Instruction::Type::AWAIT:
			push_text("AWAIT ");
			output_address(p_cfg, p_instruction.as_await()->destination, true);
			push_text(" = await ");
			output_address(p_cfg, p_instruction.as_await()->operand);
			push_newline();
			break;
		case CFG::Instruction::Type::SET_ATTRIBUTE: {
			push_text("SET_ATTRIBUTE ");
			output_address(p_cfg, p_instruction.as_set_attribute()->base);
			push_text(" . ");
			output_identifier(p_instruction.as_set_attribute()->name);
			push_text(" = ");
			output_address(p_cfg, p_instruction.as_set_attribute()->value);
			push_newline();
		} break;
		case CFG::Instruction::Type::GET_ATTRIBUTE: {
			push_text("GET_ATTRIBUTE ");
			output_address(p_cfg, p_instruction.as_get_attribute()->destination, true);
			push_text(" = ");
			output_address(p_cfg, p_instruction.as_get_attribute()->base);
			push_text(" . ");
			output_identifier(p_instruction.as_get_attribute()->name);
			push_newline();
		} break;
		case CFG::Instruction::Type::SET_INDEX: {
			push_text("SET_INDEX ");
			output_address(p_cfg, p_instruction.as_set_index()->base);
			push_text(" [ ");
			output_address(p_cfg, p_instruction.as_set_index()->index);
			push_text(" ] = ");
			output_address(p_cfg, p_instruction.as_set_index()->value);
			push_newline();
		} break;
		case CFG::Instruction::Type::GET_INDEX: {
			push_text("GET_INDEX ");
			output_address(p_cfg, p_instruction.as_get_index()->destination, true);
			push_text(" = ");
			output_address(p_cfg, p_instruction.as_get_index()->base);
			push_text(" [ ");
			output_address(p_cfg, p_instruction.as_get_index()->index);
			push_text(" ]");
			push_newline();
		} break;
		case CFG::Instruction::Type::CALL_GLOBAL_FUNCTION: {
			push_text("CALL_GLOBAL_FUNCTION ");
			output_address(p_cfg, p_instruction.as_call_global_function()->destination, true);
			push_text(" = ");
			output_identifier(p_instruction.as_call_global_function()->function_name);
			push_text(" ");
			output_arguments(p_cfg, p_instruction.as_call_global_function()->arguments);
			push_newline();
		} break;
		case CFG::Instruction::Type::CALL_CONSTRUCTOR: {
			push_text("CALL_CONSTRUCTOR ");
			output_address(p_cfg, p_instruction.as_call_constructor()->destination, true);
			push_text(" = ");
			output_datatype(p_instruction.as_call_constructor()->datatype);
			push_text(" ");
			output_arguments(p_cfg, p_instruction.as_call_constructor()->arguments);
			push_newline();
		} break;
		case CFG::Instruction::Type::CALL_OPERATOR: {
			push_text("CALL_OPERATOR ");
			output_address(p_cfg, p_instruction.as_call_operator()->destination, true);
			const Variant::Operator variant_operator = p_instruction.as_call_operator()->variant_operator;
			push_text(vformat(" = operator %s ( ", Variant::get_operator_name(variant_operator)));
			output_address(p_cfg, p_instruction.as_call_operator()->left_operand);
			push_text(" , ");
			output_address(p_cfg, p_instruction.as_call_operator()->right_operand);
			push_text(" ) ");
			push_newline();
		} break;
		case CFG::Instruction::Type::CALL_METHOD: {
			push_text("CALL_METHOD ");
			output_address(p_cfg, p_instruction.as_call_method()->destination, true);
			push_text(" = ");
			output_address(p_cfg, p_instruction.as_call_method()->base);
			push_text(" . ");
			output_identifier(p_instruction.as_call_method()->method_name);
			push_text(" ");
			output_arguments(p_cfg, p_instruction.as_call_method()->arguments);
			push_newline();
		} break;
		case CFG::Instruction::Type::CREATE_LAMBDA: {
			push_text("CREATE_LAMBDA ");
			output_address(p_cfg, p_instruction.as_create_lambda()->destination, true);
			push_text(" = <lambda> captures "); // TODO: Add a debug name to CFG?
			output_arguments(p_cfg, p_instruction.as_create_lambda()->captures);
			push_newline();
		} break;
		case CFG::Instruction::Type::INTRODUCE_ARGUMENT: {
			push_text("INTRODUCE_ARGUMENT ");
			output_address(p_cfg, p_instruction.as_introduce_argument()->address, true);
			push_newline();
		} break;
		case CFG::Instruction::Type::KEEP_ALIVE: {
			push_text("KEEP_ALIVE ");
			output_address(p_cfg, p_instruction.as_keep_alive()->address);
			push_newline();
		} break;
	}
}

void CFGPrinter::output_action(const CFG &p_cfg, CFG::BlockID p_block, const CFG::Action &p_action) {
	switch (p_action.get_type()) {
		case CFG::Action::Type::NONE:
			push_text("# ERROR: Invalid action.");
			push_newline();
			break;
		case CFG::Action::Type::NEXT:
			validate_next_block(p_action.as_next()->next_block, p_cfg.get_next_block(p_block));
			break;
		case CFG::Action::Type::JUMP:
			push_text("JUMP TO ");
			output_block_label(p_action.as_jump()->jump_block);
			push_newline();
			break;
		case CFG::Action::Type::JUMP_IF:
			push_text("JUMP_IF ");
			output_address(p_cfg, p_action.as_jump_if()->condition);
			push_text(" TO ");
			output_block_label(p_action.as_jump_if()->jump_block);
			push_newline();
			validate_next_block(p_action.as_jump_if()->next_block, p_cfg.get_next_block(p_block));
			break;
		case CFG::Action::Type::JUMP_IF_NOT:
			push_text("JUMP_IF_NOT ");
			output_address(p_cfg, p_action.as_jump_if_not()->condition);
			push_text(" TO ");
			output_block_label(p_action.as_jump_if_not()->jump_block);
			push_newline();
			validate_next_block(p_action.as_jump_if_not()->next_block, p_cfg.get_next_block(p_block));
			break;
		case CFG::Action::Type::JUMP_TABLE:
			push_text("JUMP_TABLE ");
			output_address(p_cfg, p_action.as_jump_table()->test_value);
			push_text(":");
			start_block();
			for (const KeyValue<Variant, CFG::BlockID> &kv : p_action.as_jump_table()->table) {
				push_text(vformat("%s -> ", kv.key.get_construct_string()));
				output_block_label(kv.value);
				push_newline();
			}
			push_text("default -> ");
			output_block_label(p_action.as_jump_table()->default_block);
			end_block();
			break;
		case CFG::Action::Type::RETURN:
			push_text("RETURN ");
			output_address(p_cfg, p_action.as_return()->value);
			push_newline();
			break;
	}
}

void CFGPrinter::output_block(const CFG &p_cfg, CFG::BlockID p_block) {
	if (output.get_string_length() > 0) {
		push_text("");
		push_newline();
	}

	output_block_label(p_block);
	push_text(":");

	const LocalVector<CFG::BlockID> &predecessors = p_cfg.block_get_predecessors(p_block);
	if (!predecessors.is_empty()) {
		push_text(" # ");
		bool is_first = true;
		for (CFG::BlockID predecessor : predecessors) {
			if (is_first) {
				is_first = false;
			} else {
				push_text(", ");
			}
			output_block_label(predecessor);
		}
	}

	start_block();
	for (const CFG::PhiStatement &phi_statement : p_cfg.block_get_phi_statements(p_block)) {
		output_phi_statement(p_cfg, phi_statement);
	}
	for (const CFG::Instruction &instruction : p_cfg.block_get_instructions(p_block)) {
		output_instruction(p_cfg, instruction);
	}
	output_action(p_cfg, p_block, p_cfg.block_get_action(p_block));
	end_block();
}

void CFGPrinter::clear_output() {
	output = StringBuilder();
	indent_level = 0;
	indent_string = String();
	pending_newline = false;
}

void CFGPrinter::print_block(const CFG &p_cfg, CFG::BlockID p_block) {
	output_block(p_cfg, p_block);

	const String result = output.as_string();
	if (!result.is_empty()) {
		print_line(result);
	}

	clear_output();
}

void CFGPrinter::print_cfg(const CFG &p_cfg) {
	for (uint32_t i = 0; i < p_cfg.get_block_count(); i++) {
		output_block(p_cfg, p_cfg.get_block(i));
	}

	const String result = output.as_string();
	if (!result.is_empty()) {
		print_line(result);
	}

	clear_output();
}
