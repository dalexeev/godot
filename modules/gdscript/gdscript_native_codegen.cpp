/**************************************************************************/
/*  gdscript_native_codegen.cpp                                           */
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

#include "gdscript_native_codegen.h"

#include "core/string/char_utils.h"
#include "modules/gdscript/gdscript.h"

HashMap<StringName, int> GDScriptNativeCodeGenerator::gdscript_utility_indices;

// ========== Utilities ==========

// Modified `String::uri_encode()`.
String GDScriptNativeCodeGenerator::make_ascii_identifier(const String &p_string) {
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

String GDScriptNativeCodeGenerator::add_call_args(const Vector<Address> &p_addresses) {
	if (p_addresses.is_empty()) {
		return "NULL";
	}

	const String name = vformat("call_args_%d", call_args_last_id);
	call_args_last_id++;

	String list;
	for (int i = 0; i < p_addresses.size(); i++) {
		if (i > 0) {
			list += ", ";
		}
		list += addr(p_addresses[i]);
	}

	add_line(vformat("const Variant *%s[%d] = { %s };", name, p_addresses.size(), list));
	return name;
}

String GDScriptNativeCodeGenerator::addr(const Address &p_address) const {
	switch (p_address.mode) {
		case Address::SELF:
			return vformat("(stack+%d)", GDScriptFunction::ADDR_STACK_SELF);
		case Address::CLASS:
			return vformat("(stack+%d)", GDScriptFunction::ADDR_STACK_CLASS);
		case Address::MEMBER:
			return vformat("(members+%d)", p_address.address);
		case Address::CONSTANT:
			return vformat("(constants+%d)", p_address.address);
		case Address::LOCAL_VARIABLE:
		case Address::FUNCTION_PARAMETER:
			return vformat("(stack+%d)", GDScriptFunction::FIXED_ADDRESSES_MAX + p_address.address);
		case Address::TEMPORARY:
			return vformat("(<temp_%d>)", p_address.address);
		case Address::NIL:
			return vformat("(stack+%d)", GDScriptFunction::ADDR_STACK_NIL);
	}
	return String(); // Unreachable.
}

// ========== Address Management ==========

uint32_t GDScriptNativeCodeGenerator::add_parameter(const StringName &p_name, bool p_is_optional, const GDScriptDataType &p_type) {
	gds_function->_argument_count++;
	gds_function->argument_types.push_back(p_type);

	if (p_is_optional) {
		gds_function->_default_arg_count++;
		if (first_optional_param_index < 0) {
			first_optional_param_index = parameter_count;
			current_optional_param_index = parameter_count;
		}
	}

	uint32_t local = add_local(p_name, p_type);
	parameter_count++;
	return local;
}

uint32_t GDScriptNativeCodeGenerator::add_local(const StringName &p_name, const GDScriptDataType &p_type) {
	int index = current_local_count;
	current_local_count++;
	return index;
}

uint32_t GDScriptNativeCodeGenerator::add_local_constant(const StringName &p_name, const Variant &p_constant) {
	return 0; // TODO
}

uint32_t GDScriptNativeCodeGenerator::add_or_get_constant(const Variant &p_constant) {
	return 0; // TODO
}

uint32_t GDScriptNativeCodeGenerator::add_or_get_name(const StringName &p_name) {
	return 0; // TODO
}

uint32_t GDScriptNativeCodeGenerator::add_temporary(const GDScriptDataType &p_type) {
	return 0; // TODO
}

void GDScriptNativeCodeGenerator::pop_temporary() {
}

void GDScriptNativeCodeGenerator::clean_temporaries() {
}

void GDScriptNativeCodeGenerator::start_block() {
}

void GDScriptNativeCodeGenerator::end_block() {
}

// ========== Function Structure ==========

void GDScriptNativeCodeGenerator::write_start(GDScript *p_script, const StringName &p_function_name, bool p_static, Variant p_rpc_config, const GDScriptDataType &p_return_type) {
	if (gds_function != nullptr) {
		memdelete(gds_function);
	}
	parameter_count = 0;
	first_optional_param_index = -1;
	current_optional_param_index = -1;
	current_local_count = 0;
	call_args_last_id = 0;
	_while_last_id = 0;
	_while_stack.clear();
	loop_stack.clear();

	c_function = StringBuilder();

	script_source_code = p_script->get_source_code(); // TODO: Per script.

	gds_function = memnew(GDScriptFunction);
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

	gds_function->native_symbol = vformat("gdsc_fn_%s__%s", make_ascii_identifier(p_script->get_fully_qualified_name()), make_ascii_identifier(p_function_name));
	gds_function->_call = &GDScriptFunction::_call_native; // TODO
}

void GDScriptNativeCodeGenerator::start_parameters() {
}

void GDScriptNativeCodeGenerator::write_assign_default_parameter(const Address &p_dst, const Address &p_src, bool p_use_conversion) {
	add_line(vformat("assign_default_parameter_%d:;", current_optional_param_index));
	current_optional_param_index++;
	if (p_use_conversion) {
		write_assign_with_conversion(p_dst, p_src);
	} else {
		write_assign(p_dst, p_src);
	}
}

void GDScriptNativeCodeGenerator::end_parameters() {
	//const int mandatory_argc = parameters.size() - gds_function->_default_arg_count;

	for (int i = 0; i < gds_function->_default_arg_count; i++) {
		//const int arg_index = mandatory_argc + i;
		//add_label(vformat("@start_check_argc_%d", arg_index));
		//add_instr(vformat("%%argc_check =w ceqw %%argcount, %d", arg_index));
		if (i == gds_function->_default_arg_count - 1) {
			//add_jump(vformat("jnz %%argc_check, @start_assign_default_arg_%d, @start_body", arg_index));
		} else {
			//add_jump(vformat("jnz %%argc_check, @start_assign_default_arg_%d, @start_check_argc_%d", arg_index, arg_index + 1));
		}
	}

	for (int i = 0; i < gds_function->_default_arg_count; i++) {
		//const int arg_index = mandatory_argc + i;
		//add_label(vformat("@start_assign_default_arg_%d", arg_index));
		// TODO: write assignment.
	}

	//add_label("@start_body"); // TODO
}

#ifdef DEBUG_ENABLED
void GDScriptNativeCodeGenerator::set_signature(const String &p_signature) {
}
#endif

void GDScriptNativeCodeGenerator::set_initial_line(int p_line) {
	gds_function->_initial_line = p_line;
}

GDScriptFunction *GDScriptNativeCodeGenerator::write_end() {
	String c_func = vformat("// func %s()\n", gds_function->name);
	c_func += vformat("extern void %s(int argcount, Variant *stack, Variant *constants, Variant *members, Variant *ret, CallError *error, int *line, int *resume) {\n", gds_function->native_symbol);
	// TODO: add resume and argcount jumps.
	c_func += c_function.as_string();
	c_func += "}";

	print_line(c_func);

	GDScriptFunction *gds_func = gds_function;
	gds_function = nullptr;
	return gds_func;
}

// ========== Store and Load ==========

void GDScriptNativeCodeGenerator::write_assign(const Address &p_target, const Address &p_source) {
	if (p_target.type.kind == GDScriptDataType::BUILTIN && p_target.type.builtin_type == Variant::ARRAY && p_target.type.has_container_element_type(0)) {
		//const GDScriptDataType &element_type = p_target.type.get_container_element_type(0);
		// TODO
	} else if (p_target.type.kind == GDScriptDataType::BUILTIN && p_source.type.kind == GDScriptDataType::BUILTIN && p_target.type.builtin_type != p_source.type.builtin_type) {
		// Need conversion.
		// TODO
	} else {
		add_line(vformat("variant_assign(%s, %s);", addr(p_target), addr(p_source)));
	}
}

void GDScriptNativeCodeGenerator::write_assign_with_conversion(const Address &p_target, const Address &p_source) {
}

void GDScriptNativeCodeGenerator::write_assign_true(const Address &p_target) {
}

void GDScriptNativeCodeGenerator::write_assign_false(const Address &p_target) {
}

void GDScriptNativeCodeGenerator::write_store_global(const Address &p_dst, int p_global_index) {
}

void GDScriptNativeCodeGenerator::write_store_named_global(const Address &p_dst, const StringName &p_global) {
}

void GDScriptNativeCodeGenerator::write_set(const Address &p_target, const Address &p_index, const Address &p_source) {
}

void GDScriptNativeCodeGenerator::write_get(const Address &p_target, const Address &p_index, const Address &p_source) {
}

void GDScriptNativeCodeGenerator::write_set_named(const Address &p_target, const StringName &p_name, const Address &p_source) {
	// TODO Variant::ValidatedSetter
	add_line(vformat("variant_set_named(%s, %s, %s);", addr(p_target), "TODO", addr(p_source))); // TODO: Add StringName.
}

void GDScriptNativeCodeGenerator::write_get_named(const Address &p_target, const StringName &p_name, const Address &p_source) {
}

void GDScriptNativeCodeGenerator::write_set_member(const Address &p_value, const StringName &p_name) {
}

void GDScriptNativeCodeGenerator::write_get_member(const Address &p_target, const StringName &p_name) {
}

void GDScriptNativeCodeGenerator::write_set_static_variable(const Address &p_value, const Address &p_class, int p_index) {
}

void GDScriptNativeCodeGenerator::write_get_static_variable(const Address &p_target, const Address &p_class, int p_index) {
}

// ========== Calls ==========

void GDScriptNativeCodeGenerator::write_call(const Address &p_target, const Address &p_base, const StringName &p_function_name, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_call_async(const Address &p_target, const Address &p_base, const StringName &p_function_name, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_call_self(const Address &p_target, const StringName &p_function_name, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_call_self_async(const Address &p_target, const StringName &p_function_name, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_super_call(const Address &p_target, const StringName &p_function_name, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_call_builtin_type(const Address &p_target, const Address &p_base, Variant::Type p_type, const StringName &p_method, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_call_builtin_type_static(const Address &p_target, Variant::Type p_type, const StringName &p_method, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_call_native_static(const Address &p_target, const StringName &p_class, const StringName &p_method, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_call_method_bind(const Address &p_target, const Address &p_base, MethodBind *p_method, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_call_method_bind_validated(const Address &p_target, const Address &p_base, MethodBind *p_method, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_call_script_function(const Address &p_target, const Address &p_base, const StringName &p_function_name, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_call_utility(const Address &p_target, const StringName &p_function, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_call_gdscript_utility(const Address &p_target, const StringName &p_function, const Vector<Address> &p_arguments) {
	const int func_index = gdscript_utility_indices[p_function];
	const String args = add_call_args(p_arguments);
	add_line(vformat("gdscript_utility(%d, %s, %s, %d, error);", func_index, addr(p_target), args, p_arguments.size()));
	add_return_if_error();
}

// ========== Construction ==========

void GDScriptNativeCodeGenerator::write_construct(const Address &p_target, Variant::Type p_type, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_construct_array(const Address &p_target, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_construct_typed_array(const Address &p_target, const GDScriptDataType &p_element_type, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_construct_dictionary(const Address &p_target, const Vector<Address> &p_arguments) {
}

void GDScriptNativeCodeGenerator::write_lambda(const Address &p_target, GDScriptFunction *p_function, const Vector<Address> &p_captures, bool p_use_self) {
}

// ========== Operators ==========

void GDScriptNativeCodeGenerator::write_unary_operator(const Address &p_target, Variant::Operator p_operator, const Address &p_left_operand) {
}

void GDScriptNativeCodeGenerator::write_binary_operator(const Address &p_target, Variant::Operator p_operator, const Address &p_left_operand, const Address &p_right_operand) {
}

void GDScriptNativeCodeGenerator::write_type_test(const Address &p_target, const Address &p_source, const GDScriptDataType &p_type) {
}

void GDScriptNativeCodeGenerator::write_cast(const Address &p_target, const Address &p_source, const GDScriptDataType &p_type) {
}

void GDScriptNativeCodeGenerator::write_type_adjust(const Address &p_target, Variant::Type p_new_type) {
}

// ========== Blocks ==========

void GDScriptNativeCodeGenerator::write_and_left_operand(const Address &p_left_operand) {
}

void GDScriptNativeCodeGenerator::write_and_right_operand(const Address &p_right_operand) {
}

void GDScriptNativeCodeGenerator::write_end_and(const Address &p_target) {
}

void GDScriptNativeCodeGenerator::write_or_left_operand(const Address &p_left_operand) {
}

void GDScriptNativeCodeGenerator::write_or_right_operand(const Address &p_right_operand) {
}

void GDScriptNativeCodeGenerator::write_end_or(const Address &p_target) {
}

void GDScriptNativeCodeGenerator::write_start_ternary(const Address &p_target) {
}

void GDScriptNativeCodeGenerator::write_ternary_condition(const Address &p_condition) {
}

void GDScriptNativeCodeGenerator::write_ternary_true_expr(const Address &p_expr) {
}

void GDScriptNativeCodeGenerator::write_ternary_false_expr(const Address &p_expr) {
}

void GDScriptNativeCodeGenerator::write_end_ternary() {
}

void GDScriptNativeCodeGenerator::write_if(const Address &p_condition) {
	add_line(vformat("if (variant_booleanize(%s)) {", addr(p_condition)));
}

void GDScriptNativeCodeGenerator::write_else() {
	add_line("} else {");
}

void GDScriptNativeCodeGenerator::write_endif() {
	add_line("} // endif");
}

void GDScriptNativeCodeGenerator::write_jump_if_shared(const Address &p_value) {
	add_line(vformat("if (!variant_is_shared(%s)) {", addr(p_value)));
}

void GDScriptNativeCodeGenerator::write_end_jump_if_shared() {
	add_line("} // end_jump_if_shared");
}

void GDScriptNativeCodeGenerator::start_for(const GDScriptDataType &p_iterator_type, const GDScriptDataType &p_list_type) {
}

void GDScriptNativeCodeGenerator::write_for_assignment(const Address &p_list) {
}

void GDScriptNativeCodeGenerator::write_for(const Address &p_variable, bool p_use_conversion) {
}

void GDScriptNativeCodeGenerator::write_endfor() {
}

void GDScriptNativeCodeGenerator::start_while_condition() {
	const int id = _while_last_id;
	_while_last_id++;
	_while_stack.push_back(id);
	loop_stack.push_back({ vformat("while_start_%d:", id), vformat("while_end_%d:", id) });
	add_line(vformat("while_start_%d:;", id));
}

void GDScriptNativeCodeGenerator::write_while(const Address &p_condition) {
	ERR_FAIL_COND(_while_stack.is_empty());
	const int id = _while_stack.back()->get();
	add_line(vformat("if (!variant_booleanize(%s)) { goto while_end_%d; }", addr(p_condition), id));
}

void GDScriptNativeCodeGenerator::write_endwhile() {
	ERR_FAIL_COND(_while_stack.is_empty());
	const int id = _while_stack.back()->get();
	add_line(vformat("while_end_%d:;", id));
	_while_stack.pop_back();
	loop_stack.pop_back();
}

// ========== Jumps and Other ==========

void GDScriptNativeCodeGenerator::write_break() {
	ERR_FAIL_COND(loop_stack.is_empty());
	const Pair<String, String> &loop = loop_stack.back()->get();
	add_line(vformat("goto %s;", loop.second));
}

void GDScriptNativeCodeGenerator::write_continue() {
	ERR_FAIL_COND(loop_stack.is_empty());
	const Pair<String, String> &loop = loop_stack.back()->get();
	add_line(vformat("goto %s;", loop.first));
}

void GDScriptNativeCodeGenerator::write_return(const Address &p_return_value) {
	// TODO check type (see bytecodegen), do not assign if void return (null is by default).
	add_line(vformat("variant_assign(ret, %s);", addr(p_return_value)));
	add_line("return;"); // Need reset resume. TODO: return 0, replace resume from int* to int, add return 0 at end.
}

void GDScriptNativeCodeGenerator::write_await(const Address &p_target, const Address &p_operand) {
}

void GDScriptNativeCodeGenerator::write_assert(const Address &p_test, const Address &p_message) {
}

void GDScriptNativeCodeGenerator::write_breakpoint() {
}

void GDScriptNativeCodeGenerator::write_newline(int p_line) {
	add_line(vformat("*line = %d; // %s", p_line, script_source_code.get_slice("\n", p_line - 1).strip_edges()));
}

GDScriptNativeCodeGenerator::GDScriptNativeCodeGenerator() {
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

GDScriptNativeCodeGenerator::~GDScriptNativeCodeGenerator() {
	if (gds_function != nullptr) {
		memdelete(gds_function);
	}
}
