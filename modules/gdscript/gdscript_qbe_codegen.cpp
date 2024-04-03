/**************************************************************************/
/*  gdscript_qbe_codegen.cpp                                              */
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

#include "gdscript_qbe_codegen.h"

#include "core/string/char_utils.h"
#include "modules/gdscript/gdscript.h"

HashMap<StringName, int> GDScriptQBECodeGenerator::gdscript_utility_indices;

// ========== Utilities ==========

// Modified `String::uri_encode()`.
String GDScriptQBECodeGenerator::make_ascii_identifier(const String &p_string) {
	const CharString temp = p_string.utf8();
	String res;
	for (int i = 0; i < temp.length(); ++i) {
		uint8_t ord = temp[i];
		if (is_ascii_alphanumeric_char(ord)) {
			res += ord;
		} else {
			char p[4] = { '_', 0, 0, 0 };
			static const char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
			p[1] = hex[ord >> 4];
			p[2] = hex[ord & 0xF];
			res += p;
		}
	}
	return res;
}

void GDScriptQBECodeGenerator::add_header(const String &p_func_name, const String &p_func_symbol) {
	const String args = "&L %ret, &L %error, &L %args, w %argcount, &L %members, &L %stack";
	qbe_function += vformat("# func %s()\n", p_func_name);
	qbe_function += vformat("export function $%s(%s) {\n", p_func_symbol, args.replace("&L", ptr_size_letter));
	qbe_function += "@start\n";
	qbe_function += "\t#%self =&L TODO(%stack 0)\n";
	qbe_function += "\t#%class =&L TODO(%stack 1)\n";
	qbe_function += "\t#%nil =&L TODO(%stack 2)\n";
	last_line_type = LineType::INSTR;
}

void GDScriptQBECodeGenerator::add_text(const String &p_line) {
	qbe_function += p_line + "\n";
}

void GDScriptQBECodeGenerator::add_label(const String &p_line) {
	DEV_ASSERT(p_line.begins_with("@"));
	qbe_function += ((last_line_type == LineType::LABEL) ? "" : "\n") + p_line + "\n";
	last_line_type = LineType::LABEL;
}

void GDScriptQBECodeGenerator::add_instr(const String &p_line) {
	DEV_ASSERT(!p_line.begins_with("jmp") && !p_line.begins_with("jnz") && !p_line.begins_with("ret") && !p_line.begins_with("hlt"));
	if (last_line_type == LineType::JUMP) {
		qbe_function += vformat("\n@unnamed_%d\n", unnamed_block_count);
		unnamed_block_count++;
	}
	qbe_function += "\t" + p_line.replace("&L", ptr_size_letter).replace("&8", ptr_size_digit) + "\n";
	last_line_type = LineType::INSTR;
}

void GDScriptQBECodeGenerator::add_jump(const String &p_line) {
	DEV_ASSERT(p_line.begins_with("jmp") || p_line.begins_with("jnz") || p_line.begins_with("ret") || p_line.begins_with("hlt"));
	if (last_line_type == LineType::JUMP) {
		qbe_function += vformat("\n@unnamed_%d\n", unnamed_block_count);
		unnamed_block_count++;
	}
	qbe_function += "\t" + p_line.replace("&L", ptr_size_letter).replace("&8", ptr_size_digit) + "\n";
	last_line_type = LineType::JUMP;
}

String GDScriptQBECodeGenerator::addr(const Address &p_address) const {
	switch (p_address.mode) {
		case Address::SELF:
			return "%self";
		case Address::CLASS:
			return "%class";
		case Address::MEMBER:
			return "<TODO>"; // TODO
		case Address::CONSTANT:
			return "<TODO>"; // TODO
		case Address::LOCAL_VARIABLE:
		case Address::FUNCTION_PARAMETER:
			return locals[p_address.address];
		case Address::TEMPORARY:
			return "<TODO>"; // TODO
		case Address::NIL:
			return "%nil";
	}
	return String(); // Unreachable.
}

// ========== Address Management ==========

uint32_t GDScriptQBECodeGenerator::add_parameter(const StringName &p_name, bool p_is_optional, const GDScriptDataType &p_type) {
	gds_function->_argument_count++;
	gds_function->argument_types.push_back(p_type);
	if (p_is_optional) {
		gds_function->_default_arg_count++;
	}

	uint32_t local = add_local(p_name, p_type);
	parameters.push_back(local);
	return local;
}

uint32_t GDScriptQBECodeGenerator::add_local(const StringName &p_name, const GDScriptDataType &p_type) {
	int index = locals.size();
	locals.push_back(vformat("%%local_%s", make_ascii_identifier(p_name)));
	return index;
}

uint32_t GDScriptQBECodeGenerator::add_local_constant(const StringName &p_name, const Variant &p_constant) {
	return 0; // TODO
}

uint32_t GDScriptQBECodeGenerator::add_or_get_constant(const Variant &p_constant) {
	return 0; // TODO
}

uint32_t GDScriptQBECodeGenerator::add_or_get_name(const StringName &p_name) {
	return 0; // TODO
}

uint32_t GDScriptQBECodeGenerator::add_temporary(const GDScriptDataType &p_type) {
	return 0; // TODO
}

void GDScriptQBECodeGenerator::pop_temporary() {
}

void GDScriptQBECodeGenerator::clean_temporaries() {
}

void GDScriptQBECodeGenerator::start_block() {
}

void GDScriptQBECodeGenerator::end_block() {
}

// ========== Function Structure ==========

void GDScriptQBECodeGenerator::write_start(GDScript *p_script, const StringName &p_function_name, bool p_static, Variant p_rpc_config, const GDScriptDataType &p_return_type) {
	if (gds_function != nullptr) {
		memdelete(gds_function);
	}
	parameters.clear();
	locals.clear();

	qbe_function = StringBuilder();
	//last_line_type = LineType::LABEL; // Will be overwritten in `add_header()` anyway.
	unnamed_block_count = 0;
	_if_block_count = 0;
	_if_block_stack.clear();

	script_source_code = p_script->get_source_code(); // TODO: Per script.

	gds_function = memnew(GDScriptQBEFunction);
	//debug_stack = EngineDebugger::is_active();

	gds_function->name = p_function_name;
	gds_function->_script = p_script;
	gds_function->source = p_script->get_script_path();

#ifdef DEBUG_ENABLED
	gds_function->func_cname = (String(gds_function->source) + " - " + String(p_function_name)).utf8();
	gds_function->_func_cname = gds_function->func_cname.get_data();
#endif

	gds_function->_static = p_static;
	gds_function->return_type = p_return_type;
	gds_function->rpc_config = p_rpc_config;
	gds_function->_argument_count = 0;

	gds_function->symbol = vformat("gds_fn_%s__%s", make_ascii_identifier(p_script->get_fully_qualified_name()), make_ascii_identifier(p_function_name));

	add_header(p_function_name, gds_function->symbol);
}

void GDScriptQBECodeGenerator::start_parameters() {
}

void GDScriptQBECodeGenerator::write_assign_default_parameter(const Address &p_dst, const Address &p_src, bool p_use_conversion) {
	// TODO !!!
}

void GDScriptQBECodeGenerator::end_parameters() {
	const int mandatory_argc = parameters.size() - gds_function->_default_arg_count;

	for (int i = 0; i < gds_function->_default_arg_count; i++) {
		const int arg_index = mandatory_argc + i;
		add_label(vformat("@start_check_argc_%d", arg_index));
		add_instr(vformat("%%argc_check =w ceqw %%argcount, %d", arg_index));
		if (i == gds_function->_default_arg_count - 1) {
			add_jump(vformat("jnz %%argc_check, @start_assign_default_arg_%d, @start_body", arg_index));
		} else {
			add_jump(vformat("jnz %%argc_check, @start_assign_default_arg_%d, @start_check_argc_%d", arg_index, arg_index + 1));
		}
	}

	for (int i = 0; i < gds_function->_default_arg_count; i++) {
		const int arg_index = mandatory_argc + i;
		add_label(vformat("@start_assign_default_arg_%d", arg_index));
		// TODO: write assignment.
	}

	add_label("@start_body"); // TODO
}

#ifdef DEBUG_ENABLED
void GDScriptQBECodeGenerator::set_signature(const String &p_signature) {
}
#endif

void GDScriptQBECodeGenerator::set_initial_line(int p_line) {
	gds_function->_initial_line = p_line;
}

GDScriptFunction *GDScriptQBECodeGenerator::write_end() {
	ERR_FAIL_COND_V_MSG(!_if_block_stack.is_empty(), nullptr, "Unclosed \"if\".");

	add_jump("ret");
	add_text("}");

	print_line(qbe_function.as_string());

	GDScriptFunction *func = gds_function;
	gds_function = nullptr;
	return func;
}

// ========== Store and Load ==========

void GDScriptQBECodeGenerator::write_assign(const Address &p_target, const Address &p_source) {
}

void GDScriptQBECodeGenerator::write_assign_with_conversion(const Address &p_target, const Address &p_source) {
}

void GDScriptQBECodeGenerator::write_assign_true(const Address &p_target) {
}

void GDScriptQBECodeGenerator::write_assign_false(const Address &p_target) {
}

void GDScriptQBECodeGenerator::write_store_global(const Address &p_dst, int p_global_index) {
}

void GDScriptQBECodeGenerator::write_store_named_global(const Address &p_dst, const StringName &p_global) {
}

void GDScriptQBECodeGenerator::write_set(const Address &p_target, const Address &p_index, const Address &p_source) {
}

void GDScriptQBECodeGenerator::write_get(const Address &p_target, const Address &p_index, const Address &p_source) {
}

void GDScriptQBECodeGenerator::write_set_named(const Address &p_target, const StringName &p_name, const Address &p_source) {
}

void GDScriptQBECodeGenerator::write_get_named(const Address &p_target, const StringName &p_name, const Address &p_source) {
}

void GDScriptQBECodeGenerator::write_set_member(const Address &p_value, const StringName &p_name) {
}

void GDScriptQBECodeGenerator::write_get_member(const Address &p_target, const StringName &p_name) {
}

void GDScriptQBECodeGenerator::write_set_static_variable(const Address &p_value, const Address &p_class, int p_index) {
}

void GDScriptQBECodeGenerator::write_get_static_variable(const Address &p_target, const Address &p_class, int p_index) {
}

// ========== Calls ==========

void GDScriptQBECodeGenerator::write_call(const Address &p_target, const Address &p_base, const StringName &p_function_name, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_call_async(const Address &p_target, const Address &p_base, const StringName &p_function_name, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_call_self(const Address &p_target, const StringName &p_function_name, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_call_self_async(const Address &p_target, const StringName &p_function_name, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_super_call(const Address &p_target, const StringName &p_function_name, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_call_builtin_type(const Address &p_target, const Address &p_base, Variant::Type p_type, const StringName &p_method, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_call_builtin_type_static(const Address &p_target, Variant::Type p_type, const StringName &p_method, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_call_native_static(const Address &p_target, const StringName &p_class, const StringName &p_method, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_call_method_bind(const Address &p_target, const Address &p_base, MethodBind *p_method, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_call_method_bind_validated(const Address &p_target, const Address &p_base, MethodBind *p_method, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_call_script_function(const Address &p_target, const Address &p_base, const StringName &p_function_name, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_call_utility(const Address &p_target, const StringName &p_function, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_call_gdscript_utility(const Address &p_target, const StringName &p_function, const Vector<Address> &p_arguments) {
	const int func_index = gdscript_utility_indices[p_function];
	add_instr(vformat("call $gds_gdscript_utility(w %d, &L %s, &L %s, w %d, &L %%error)", func_index, addr(p_target), "<TODO>", p_arguments.size()));
	// TODO: return if error
}

// ========== Construction ==========

void GDScriptQBECodeGenerator::write_construct(const Address &p_target, Variant::Type p_type, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_construct_array(const Address &p_target, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_construct_typed_array(const Address &p_target, const GDScriptDataType &p_element_type, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_construct_dictionary(const Address &p_target, const Vector<Address> &p_arguments) {
}

void GDScriptQBECodeGenerator::write_lambda(const Address &p_target, GDScriptFunction *p_function, const Vector<Address> &p_captures, bool p_use_self) {
}

// ========== Operators ==========

void GDScriptQBECodeGenerator::write_unary_operator(const Address &p_target, Variant::Operator p_operator, const Address &p_left_operand) {
}

void GDScriptQBECodeGenerator::write_binary_operator(const Address &p_target, Variant::Operator p_operator, const Address &p_left_operand, const Address &p_right_operand) {
}

void GDScriptQBECodeGenerator::write_type_test(const Address &p_target, const Address &p_source, const GDScriptDataType &p_type) {
}

void GDScriptQBECodeGenerator::write_cast(const Address &p_target, const Address &p_source, const GDScriptDataType &p_type) {
}

void GDScriptQBECodeGenerator::write_type_adjust(const Address &p_target, Variant::Type p_new_type) {
}

// ========== Blocks ==========

void GDScriptQBECodeGenerator::write_and_left_operand(const Address &p_left_operand) {
}

void GDScriptQBECodeGenerator::write_and_right_operand(const Address &p_right_operand) {
}

void GDScriptQBECodeGenerator::write_end_and(const Address &p_target) {
}

void GDScriptQBECodeGenerator::write_or_left_operand(const Address &p_left_operand) {
}

void GDScriptQBECodeGenerator::write_or_right_operand(const Address &p_right_operand) {
}

void GDScriptQBECodeGenerator::write_end_or(const Address &p_target) {
}

void GDScriptQBECodeGenerator::write_start_ternary(const Address &p_target) {
}

void GDScriptQBECodeGenerator::write_ternary_condition(const Address &p_condition) {
}

void GDScriptQBECodeGenerator::write_ternary_true_expr(const Address &p_expr) {
}

void GDScriptQBECodeGenerator::write_ternary_false_expr(const Address &p_expr) {
}

void GDScriptQBECodeGenerator::write_end_ternary() {
}

void GDScriptQBECodeGenerator::write_if(const Address &p_condition) {
	const int id = _if_block_count;
	_if_block_count++;
	_if_block_stack.push_back({ id, false });
	// TODO: Boolenize cond.
	add_jump(vformat("jnz %s @if_true_%d @if_false_%d", "<TODO>", id, id));
	add_label(vformat("@if_true_%d", id));
}

void GDScriptQBECodeGenerator::write_else() {
	ERR_FAIL_COND_MSG(_if_block_stack.is_empty(), "Unexpected \"else\".");
	Pair<int, bool> &block = _if_block_stack.back()->get();
	block.second = true;
	add_jump(vformat("jmp @if_end_%d", block.first));
	add_label(vformat("@if_false_%d", block.first));
}

void GDScriptQBECodeGenerator::write_endif() {
	ERR_FAIL_COND_MSG(_if_block_stack.is_empty(), "Unexpected \"endif\".");
	const Pair<int, bool> &block = _if_block_stack.back()->get();
	add_label(vformat(block.second ? "@if_end_%d" : "@if_false_%d", block.first));
	_if_block_stack.pop_back();
}

void GDScriptQBECodeGenerator::write_jump_if_shared(const Address &p_value) {
	const int id = _if_block_count;
	_if_block_count++;
	_if_block_stack.push_back({ id, false });
	// TODO: Boolenize cond (p_value), handle opcode.
	add_jump(vformat("jnz %s @if_true_%d @if_false_%d", "<TODO>", id, id));
	add_label(vformat("@if_true_%d", id));
}

void GDScriptQBECodeGenerator::write_end_jump_if_shared() {
	write_endif();
}

void GDScriptQBECodeGenerator::start_for(const GDScriptDataType &p_iterator_type, const GDScriptDataType &p_list_type) {
}

void GDScriptQBECodeGenerator::write_for_assignment(const Address &p_list) {
}

void GDScriptQBECodeGenerator::write_for(const Address &p_variable, bool p_use_conversion) {
}

void GDScriptQBECodeGenerator::write_endfor() {
}

void GDScriptQBECodeGenerator::start_while_condition() {
}

void GDScriptQBECodeGenerator::write_while(const Address &p_condition) {
}

void GDScriptQBECodeGenerator::write_endwhile() {
}

// ========== Jumps and Other ==========

void GDScriptQBECodeGenerator::write_break() {
}

void GDScriptQBECodeGenerator::write_continue() {
}

void GDScriptQBECodeGenerator::write_return(const Address &p_return_value) {
	// TODO: Add return value (`%ret =&L ...`).
	add_jump("ret");
}

void GDScriptQBECodeGenerator::write_await(const Address &p_target, const Address &p_operand) {
}

void GDScriptQBECodeGenerator::write_assert(const Address &p_test, const Address &p_message) {
}

void GDScriptQBECodeGenerator::write_breakpoint() {
}

void GDScriptQBECodeGenerator::write_newline(int p_line) {
	add_text(((last_line_type == LineType::LABEL) ? "" : "\n") + vformat("\t# %04d | %s", p_line, script_source_code.get_slice("\n", p_line - 1).strip_edges()));
}

GDScriptQBECodeGenerator::GDScriptQBECodeGenerator() {
	if (unlikely(gdscript_utility_indices.is_empty())) {
		List<StringName> functions;
		GDScriptUtilityFunctions::get_function_list(&functions);
		int i = 0;
		for (const StringName &func : functions) {
			gdscript_utility_indices[func] = i;
			i++;
		}
	}
}

GDScriptQBECodeGenerator::~GDScriptQBECodeGenerator() {
	if (gds_function != nullptr) {
		memdelete(gds_function);
	}
}
