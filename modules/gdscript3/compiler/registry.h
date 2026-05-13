/**************************************************************************/
/*  registry.h                                                            */
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

#include "ast.h"

namespace gdscript {

// TODO: Add a mutex.
// TODO: Consider dependencies on native and other extension classes?
class Registry {
	struct ScriptInfo {
		// Canonical FQTN of the main class in the script. Keep in sync with `fqtn_to_path`.
		StringName fqtn;
		// AST may be missing for cached or invalidated scripts (i.e. `is_null()` will return `true`).
		AST ast;

		ScriptInfo() {}
	};

	static constexpr uint32_t CACHE_FILE_MAGIC = 0x43434447; // "GDCC".
	static constexpr uint32_t CACHE_FILE_VERSION = 1;

	static inline Registry *singleton = nullptr;

	int tab_size = DEFAULT_TAB_SIZE;

	// Canonical script path -> script info.
	HashMap<String, ScriptInfo> scripts;
	// Canonical FQTN of the main clas -> canonical script path. Keep in sync with `ScriptInfo::fqtn`.
	// If multiple scripts have the same FQTN, only the first script is added here.
	// An error about duplicate global names should be generated.
	HashMap<StringName, String> fqtn_to_path;

	HashMap<String, HashSet<String>> dependencies;
	HashMap<String, HashSet<String>> inverse_dependencies;

	void _add_dependency(const String &p_from_path, const String &p_to_path);
	void _remove_dependency(const String &p_from_path, const String &p_to_path);

	template <bool t_parse_body>
	void _parse_script(const String &p_path);

	template <bool t_allow_cached>
	void _scan_dir(const String &p_dir, const String &p_project_data_dir);

	void _invalidate_all_scripts();
	void _invalidate_scripts(const LocalVector<String> &p_paths);

	String _get_cache_path() const;
	void _save_cache() const;
	void _load_cache();

	Registry() {}

public:
	_FORCE_INLINE_ static Registry *get_singleton() { return singleton; }

	static void initialize();
	static void deinitialize();

	void set_tab_size(int p_tab_size);

	void clear();

	void scan_project_dir();
	_FORCE_INLINE_ void save_cache() const { _save_cache(); }

	void add_script(const String &p_path);
	void add_script_dir(const String &p_dir);

	void remove_script(const String &p_path);
	void remove_script_dir(const String &p_dir);

	_FORCE_INLINE_ bool has_script_by_path(const String &p_path) const { return scripts.has(p_path); }
	_FORCE_INLINE_ bool has_script_by_fqtn(const StringName &p_fqtn) const { return fqtn_to_path.has(p_fqtn); }

	AST get_ast_by_path(const String &p_path, AST::DesiredStatus p_status);
	AST get_ast_by_fqtn(const StringName &p_fqtn, AST::DesiredStatus p_status);

	void add_dependency(const String &p_from_path, const String &p_to_path);

	AST get_ast_for_editor(const String &p_path, const String &p_source, AST::DesiredStatus p_status) const;
};

} // namespace gdscript
