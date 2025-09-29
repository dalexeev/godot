#pragma once

#include "../../compiler/cfg.h"
#include "cfg_printer.h"

#include "tests/test_macros.h"

namespace gdscript {

TEST_CASE("[Modules][GDScript3][Compiler][CFG] Temp test") {
	CFG cfg;

	const CFG::BlockID block1 = cfg.add_block();
	const CFG::BlockID block2 = cfg.add_block();
	const CFG::BlockID block3 = cfg.add_block();

	const CFG::Address a0_addr = cfg.add_local("a", DataType::make_builtin(Variant::BOOL));
	const CFG::Address a1_addr = cfg.add_local("a", DataType::make_builtin(Variant::BOOL));
	const CFG::Address a2_addr = cfg.add_local("a", DataType::make_builtin(Variant::BOOL));

	cfg.block_get_instructions(block1).push_back(CFG::Instruction::make_introduce_argument(a0_addr));
	cfg.block_set_action(block1, CFG::Action::make_jump_if_not(a0_addr, block3, block2));

	cfg.block_get_instructions(block2).push_back(CFG::Instruction::make_assign(a1_addr, cfg.add_constant(false)));
	cfg.block_set_action(block2, CFG::Action::make_next(block3));

	cfg.block_add_phi_statement(block3, CFG::PhiStatement(a2_addr, { { block1, a0_addr }, { block2, a1_addr } }));
	cfg.block_set_action(block3, CFG::Action::make_return(cfg.add_constant(Variant())));

	CFGPrinter cfg_printer;
	cfg_printer.print_cfg(cfg);
}

} // namespace gdscript
