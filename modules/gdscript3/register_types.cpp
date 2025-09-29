/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#include "runtime/gdscript_gdextension_manager.h"

#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"

// ----- Compiler -----

#ifdef GDSCRIPT_COMPILER_ENABLED

#include "compiler/ast.h"
#include "compiler/diagnostic_list.h"
#include "compiler/registry.h"
#include "compiler/tokenizer.h"

#endif // GDSCRIPT_COMPILER_ENABLED

// ----- Editor -----

#ifdef TOOLS_ENABLED

#include "core/config/engine.h"

#endif // TOOLS_ENABLED

// ----- Tests -----

#if defined(TESTS_ENABLED) && defined(GDSCRIPT_COMPILER_ENABLED)

#include "./tests/compiler/commands.h"

#include "tests/test_macros.h"

#endif // defined(TESTS_ENABLED) && defined(GDSCRIPT_COMPILER_ENABLED)

namespace gdscript {

static void define_project_settings() {
#ifdef GDSCRIPT_COMPILER_ENABLED
	DiagnosticList::define_project_settings();
#endif // GDSCRIPT_COMPILER_ENABLED
}

static void update_project_settings() {
#ifdef GDSCRIPT_COMPILER_ENABLED
	DiagnosticList::update_project_settings();
#endif // GDSCRIPT_COMPILER_ENABLED
}

} // namespace gdscript

void initialize_gdscript3_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
#ifdef GDSCRIPT_COMPILER_ENABLED
		gdscript::Tokenizer::initialize();
		gdscript::DataType::initialize();
		gdscript::AST::initialize();
		gdscript::Registry::initialize();
#endif // GDSCRIPT_COMPILER_ENABLED

		GDScriptGDExtensionManager::initialize();

		static bool project_settings_defined = false;
		if (!project_settings_defined) {
			project_settings_defined = true;
			gdscript::define_project_settings();
		}

		gdscript::update_project_settings();
		ProjectSettings::get_singleton()->connect(
				"settings_changed",
				callable_mp_static(&gdscript::update_project_settings));
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		if (Engine::get_singleton()->is_editor_hint() && !Engine::get_singleton()->is_project_manager_hint()) {
			// TODO: Use `EditorProgress`? Move to `./editor/`?
			gdscript::Registry::get_singleton()->scan_project_dir();
		}
	}
#endif // TOOLS_ENABLED
}

void uninitialize_gdscript3_module(ModuleInitializationLevel p_level) {
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		if (Engine::get_singleton()->is_editor_hint() && !Engine::get_singleton()->is_project_manager_hint()) {
			// TODO: Use `EditorProgress`? Move to `./editor/`?
			gdscript::Registry::get_singleton()->save_cache();
		}
	}
#endif // TOOLS_ENABLED

	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		ProjectSettings::get_singleton()->disconnect(
				"settings_changed",
				callable_mp_static(&gdscript::update_project_settings));

		GDScriptGDExtensionManager::deinitialize();

#ifdef GDSCRIPT_COMPILER_ENABLED
		gdscript::Registry::deinitialize();
		gdscript::AST::deinitialize();
		gdscript::DataType::deinitialize();
		gdscript::Tokenizer::deinitialize();

		gdscript::DiagnosticList::clear_static_data();
#endif // GDSCRIPT_COMPILER_ENABLED
	}
}

#if defined(TESTS_ENABLED) && defined(GDSCRIPT_COMPILER_ENABLED)
REGISTER_TEST_COMMAND("gdscript3-tokenizer", &gdscript::test_command_tokenizer);
REGISTER_TEST_COMMAND("gdscript3-parser", &gdscript::test_command_parser);
REGISTER_TEST_COMMAND("gdscript3-analyzer", &gdscript::test_command_analyzer);
#endif // defined(TESTS_ENABLED) && defined(GDSCRIPT_COMPILER_ENABLED)
