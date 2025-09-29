/**************************************************************************/
/*  cfg_printer.h                                                         */
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

#include "../../compiler/cfg.h"

#include "core/string/string_builder.h"

namespace gdscript {

class CFGPrinter {
	StringBuilder output;
	int indent_level = 0;
	String indent_string;
	bool pending_newline = false;

	void push_text(const String &p_text);
	void push_newline();
	void start_block();
	void end_block();

	void validate_next_block(CFG::BlockID p_actual, CFG::BlockID p_expected);

	void output_block_label(CFG::BlockID p_block);
	void output_identifier(const StringName &p_name);
	void output_datatype(const DataType &p_datatype);
	void output_address(const CFG &p_cfg, CFG::Address p_address, bool p_with_datatype = false);
	void output_arguments(const CFG &p_cfg, const LocalVector<CFG::Address> &p_arguments);

	void output_phi_statement(const CFG &p_cfg, const CFG::PhiStatement &p_phi_statement);
	void output_instruction(const CFG &p_cfg, const CFG::Instruction &p_instruction);
	void output_action(const CFG &p_cfg, CFG::BlockID p_block, const CFG::Action &p_action);
	void output_block(const CFG &p_cfg, CFG::BlockID p_block);

	void clear_output();

public:
	void print_block(const CFG &p_cfg, CFG::BlockID p_block);
	void print_cfg(const CFG &p_cfg);

	CFGPrinter() {}
};

} // namespace gdscript
