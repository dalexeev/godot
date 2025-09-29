/**************************************************************************/
/*  gdscript_gdextension_manager.cpp                                      */
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

#include "gdscript_gdextension_manager.h"

#include "core/extension/gdextension.h"
#include "core/extension/gdextension_manager.h"

namespace gdscript_gdextension {

static const String path = "libgdscript://gdscript.gdextension";

extern "C" {

void _initialize(void *p_userdata, GDExtensionInitializationLevel p_level) {
}

void _deinitialize(void *p_userdata, GDExtensionInitializationLevel p_level) {
}

GDExtensionBool init(
		GDExtensionInterfaceGetProcAddress p_get_proc_address,
		GDExtensionClassLibraryPtr p_library,
		GDExtensionInitialization *r_initialization) {
	r_initialization->initialize = _initialize;
	r_initialization->deinitialize = _deinitialize;
	r_initialization->userdata = nullptr;
	r_initialization->minimum_initialization_level = GDEXTENSION_INITIALIZATION_SCENE;

	return true;
}

} // extern "C"

} // namespace gdscript_gdextension

// ===== GDScriptGDExtensionLoader =====

Error GDScriptGDExtensionLoader::open_library(const String &p_path) {
	ERR_FAIL_COND_V(p_path != gdscript_gdextension::path, ERR_FILE_NOT_FOUND);
	ERR_FAIL_COND_V(_is_library_open, ERR_FILE_ALREADY_IN_USE);
	_is_library_open = true;
	return OK;
}

Error GDScriptGDExtensionLoader::initialize(
		GDExtensionInterfaceGetProcAddress p_get_proc_address,
		const Ref<GDExtension> &p_extension,
		GDExtensionInitialization *r_initialization) {
	const GDExtensionBool ret = gdscript_gdextension::init(p_get_proc_address, p_extension.ptr(), r_initialization);
	return ret ? OK : FAILED;
}

void GDScriptGDExtensionLoader::close_library() {
	ERR_FAIL_COND(!_is_library_open);
	_is_library_open = false;
}

bool GDScriptGDExtensionLoader::is_library_open() const {
	return _is_library_open;
}

bool GDScriptGDExtensionLoader::has_library_changed() const {
	return false; // TODO
}

bool GDScriptGDExtensionLoader::library_exists() const {
	return true;
}

// ===== GDScriptGDExtensionManager =====

void GDScriptGDExtensionManager::initialize() {
	ERR_FAIL_COND(singleton != nullptr);

	singleton = memnew(GDScriptGDExtensionManager);
	singleton->loader.instantiate();

	GDExtensionManager::get_singleton()->load_extension_with_loader(gdscript_gdextension::path, singleton->loader);
}

void GDScriptGDExtensionManager::deinitialize() {
	ERR_FAIL_NULL(singleton);

	GDExtensionManager::get_singleton()->unload_extension(gdscript_gdextension::path);

	memdelete(singleton);
	singleton = nullptr;
}
