/**************************************************************************/
/*  registry.cpp                                                          */
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

#include "registry.h"

#include "analyzer.h"
#include "parser.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"

using namespace gdscript;

void Registry::initialize() {
	ERR_FAIL_COND(singleton != nullptr);

	singleton = memnew(Registry);
}

void Registry::deinitialize() {
	ERR_FAIL_NULL(singleton);

	memdelete(singleton);
	singleton = nullptr;
}

// This method expects the passed paths to be valid.
void Registry::_add_dependency(const String &p_from_path, const String &p_to_path) {
	if (dependencies.has(p_from_path)) {
		dependencies[p_from_path].insert(p_to_path);
	} else {
		dependencies[p_from_path] = { p_to_path };
	}

	if (inverse_dependencies.has(p_to_path)) {
		inverse_dependencies[p_to_path].insert(p_from_path);
	} else {
		inverse_dependencies[p_to_path] = { p_from_path };
	}
}

// This method expects the passed paths to be valid.
void Registry::_remove_dependency(const String &p_from_path, const String &p_to_path) {
	if (dependencies.has(p_from_path)) {
		dependencies[p_from_path].erase(p_to_path);
		if (dependencies[p_from_path].is_empty()) {
			dependencies.erase(p_from_path);
		}
	}

	if (inverse_dependencies.has(p_to_path)) {
		inverse_dependencies[p_to_path].erase(p_from_path);
		if (inverse_dependencies[p_to_path].is_empty()) {
			inverse_dependencies.erase(p_to_path);
		}
	}
}

// This method expects the passed path to be valid.
template <bool t_is_cached>
void Registry::_parse_script(const String &p_path) {
	const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_MSG(file.is_null(), vformat(R"(Failed to open file "%s".)", p_path));

	Parser parser(p_path, file->get_as_text(), tab_size);
	parser.parse();

	if constexpr (t_is_cached) {
		ERR_FAIL_COND_MSG(!scripts.has(p_path), "GDScript bug: Cache mismatch detected.");
	} else {
		ERR_FAIL_COND_MSG(scripts.has(p_path), "GDScript bug: Cache mismatch detected.");
		scripts[p_path] = ScriptInfo();
	}

	scripts[p_path].ast = parser.get_ast();

	StringName fqtn;
	{
		AST::IdentifierNode *id = parser.get_ast().get_root()->main_class->identifier;
		if (id != nullptr && !id->is_recovery) {
			fqtn = id->name; // TODO: Add namespaces later.
		}

		// TODO: Check for a conflict with native classes and other extensions?
	}

	if constexpr (t_is_cached) {
		ERR_FAIL_COND_MSG(fqtn != scripts[p_path].fqtn, "GDScript bug: Cache mismatch detected.");
	} else {
		// An error about a duplicate global name will be generated in the analyzer.
		if (!fqtn.is_empty() && !fqtn_to_path.has(fqtn)) {
			scripts[p_path].fqtn = fqtn;
			fqtn_to_path[fqtn] = p_path;
		}
	}
}

template <bool t_allow_cached>
void Registry::_scan_dir(const String &p_dir, const String &p_project_data_dir) {
	const Ref<DirAccess> dir = DirAccess::open(p_dir);
	ERR_FAIL_COND_MSG(dir.is_null(), vformat(R"(Failed to open directory "%s".)", p_dir));

	if (unlikely(dir->get_current_dir() == p_project_data_dir)) {
		return; // Ignore `res://godot` for non-standard configuration.
	}

	if (dir->file_exists(".gdignore")) {
		return; // Ignore scripts in directories with a `.gdignore` file.
	}

	const Error err = dir->list_dir_begin();
	ERR_FAIL_COND_MSG(err != OK, vformat(R"(Failed to open directory "%s".)", p_dir));

	while (true) {
		const String current_name = dir->get_next();
		if (current_name.is_empty()) {
			break;
		}
		if (current_name.begins_with(".")) {
			continue;
		}

		const String current_path = p_dir.path_join(current_name);
		if (dir->current_is_dir()) {
			_scan_dir<t_allow_cached>(current_path, p_project_data_dir);
		} else {
			if (current_name.get_extension() == GD_FILE_EXTENSION) {
				if constexpr (t_allow_cached) {
					if (!scripts.has(current_path)) {
						_parse_script<false>(current_path);
					}
				} else {
					ERR_CONTINUE(scripts.has(current_path));
					_parse_script<false>(current_path);
				}
			}
		}
	}

	dir->list_dir_end();
}

void Registry::_invalidate_all_scripts() {
	dependencies.clear();
	inverse_dependencies.clear();

	fqtn_to_path.clear();

	for (const KeyValue<String, ScriptInfo> &kv : scripts) {
		scripts[kv.key] = ScriptInfo();
	}

	for (const KeyValue<String, ScriptInfo> &kv : scripts) {
		_parse_script<false>(kv.key);
	}
}

// This method expects all passed paths to be valid.
void Registry::_invalidate_scripts(const LocalVector<String> &p_paths) {
	HashSet<String> paths_to_invalidate;
	{
		LocalVector<String> paths_to_process = LocalVector<String>(p_paths);
		while (!paths_to_process.is_empty()) {
			const String current_path = paths_to_process[paths_to_process.size() - 1];
			paths_to_process.resize(paths_to_process.size() - 1);

			paths_to_invalidate.insert(current_path);

			if (inverse_dependencies.has(current_path)) {
				for (const String &path : inverse_dependencies[current_path]) {
					if (!paths_to_invalidate.has(path)) {
						paths_to_process.push_back(path);
					}
				}
			}

			if (dependencies.has(current_path)) {
				LocalVector<String> paths;
				paths.reserve(dependencies[current_path].size());
				for (const String &path : dependencies[current_path]) {
					paths.push_back(path);
				}
				for (const String &path : paths) {
					_remove_dependency(current_path, path);
				}
			}
		}
	}

	for (const String &path : paths_to_invalidate) {
		if (!scripts[path].fqtn.is_empty()) {
			fqtn_to_path.erase(scripts[path].fqtn);
		}

		scripts[path] = ScriptInfo();
	}

	for (const String &path : paths_to_invalidate) {
		_parse_script<false>(path);
	}
}

String Registry::_get_cache_path() const {
	return ProjectSettings::get_singleton()->get_project_data_path().path_join("gdscript/compiler_cache.bin");
}

void Registry::_save_cache() const {
	const String cache_path = _get_cache_path();
	const String cache_dir = cache_path.get_base_dir();

	print_verbose(vformat(R"(GDScript: Saving compiler cache to "%s"...)", cache_path));

	if (!DirAccess::exists(cache_dir)) {
		const Error err = DirAccess::make_dir_recursive_absolute(cache_dir);
		ERR_FAIL_COND_MSG(err != OK, vformat(R"(Failed to create directory "%s".)", cache_dir));
	}

	const Ref<FileAccess> file = FileAccess::open(cache_path, FileAccess::WRITE);
	ERR_FAIL_COND_MSG(file.is_null(), vformat(R"(Failed to open file "%s".)", cache_path));

	bool valid = true;

	valid = valid && file->store_32(CACHE_FILE_MAGIC);
	valid = valid && file->store_32(CACHE_FILE_VERSION);

#define ABORT_CACHE_IF(m_cond) \
	if (unlikely(m_cond)) { \
		ERR_PRINT(vformat(R"(Failed to create file "%s".)", cache_path)); \
		file->resize(0); \
		file->close(); \
		return; \
	} else \
		((void)0)

	HashMap<String, uint32_t> path_to_index;

	print_verbose(vformat(R"(GDScript: Storing %d script entries to compiler cache...)", scripts.size()));
	valid = valid && file->store_32(scripts.size());
	for (const KeyValue<String, ScriptInfo> &kv : scripts) {
		const String &path = kv.key;

		valid = valid && file->store_pascal_string(path);
		valid = valid && file->store_pascal_string(FileAccess::get_md5(path));
		valid = valid && file->store_pascal_string(scripts[path].fqtn);

		path_to_index[path] = path_to_index.size();
	}

	print_verbose(vformat(R"(GDScript: Storing %d dependency entries to compiler cache...)", dependencies.size()));
	valid = valid && file->store_32(dependencies.size());
	for (const KeyValue<String, HashSet<String>> &kv : dependencies) {
		ABORT_CACHE_IF(!path_to_index.has(kv.key));
		valid = valid && file->store_32(path_to_index[kv.key]);

		valid = valid && file->store_32(kv.value.size());
		for (const String &path : kv.value) {
			ABORT_CACHE_IF(!path_to_index.has(path));
			valid = valid && file->store_32(path_to_index[path]);
		}
	}

	ABORT_CACHE_IF(!valid);

	file->close();

#undef ABORT_CACHE_IF

	print_verbose("GDScript: Compiler cache saved.");
}

void Registry::_load_cache() {
	const String cache_path = _get_cache_path();

	print_verbose(vformat(R"(GDScript: Loading compiler cache from "%s"...)", cache_path));

	if (!FileAccess::exists(cache_path)) {
		print_verbose("GDScript: Compiler cache loading cancelled (file not found).");
		return;
	}

	const Ref<FileAccess> file = FileAccess::open(cache_path, FileAccess::READ);
	ERR_FAIL_COND_MSG(file.is_null(), vformat(R"(Failed to open file "%s".)", cache_path));

	if (file->get_32() != CACHE_FILE_MAGIC) {
		ERR_PRINT(vformat(R"(File "%s" is corrupted.)", cache_path));
		return;
	}
	if (file->get_32() != CACHE_FILE_VERSION) {
		print_verbose("GDScript: Compiler cache loading cancelled (invalid version).");
		return;
	}

#define REJECT_CACHE_IF(m_cond) \
	if (unlikely(m_cond)) { \
		ERR_PRINT(vformat(R"(File "%s" is corrupted.)", cache_path)); \
		clear(); \
		return; \
	} else \
		((void)0)

#define REJECT_CACHE_IF_PAST_EOF() \
	REJECT_CACHE_IF(file->get_position() >= file->get_length())

	LocalVector<String> paths_to_invalidate;

	const uint32_t path_count = file->get_32();
	print_verbose(vformat(R"(GDScript: Loading %d script entries from compiler cache...)", path_count));

	LocalVector<String> index_to_path;
	index_to_path.reserve(path_count);

	for (uint32_t i = 0; i < path_count; i++) {
		REJECT_CACHE_IF_PAST_EOF();

		const String path = file->get_pascal_string();
		const String md5 = file->get_pascal_string();
		const StringName fqtn = file->get_pascal_string();
		const StringName base_fqtn = file->get_pascal_string();

		REJECT_CACHE_IF(path.get_extension() != GD_FILE_EXTENSION || scripts.has(path));
		REJECT_CACHE_IF(fqtn_to_path.has(fqtn));

		if (path.begins_with("res://")) {
			if (md5.is_empty() || !FileAccess::exists(path) || md5 != FileAccess::get_md5(path)) {
				paths_to_invalidate.push_back(path);
			}
		} else {
			paths_to_invalidate.push_back(path);
		}

		index_to_path.push_back(path);

		scripts[path] = ScriptInfo();

		if (!fqtn.is_empty()) {
			scripts[path].fqtn = fqtn;
			fqtn_to_path[fqtn] = path;
		}
	}

	const uint32_t dependent_count = file->get_32();
	print_verbose(vformat(R"(GDScript: Loading %d dependency entries from compiler cache...)", dependent_count));

	for (uint32_t i = 0; i < dependent_count; i++) {
		REJECT_CACHE_IF_PAST_EOF();

		const uint32_t dependent_index = file->get_32();
		REJECT_CACHE_IF(dependent_index >= index_to_path.size());
		const String &dependent_path = index_to_path[dependent_index];

		const uint32_t dependency_count = file->get_32();
		for (uint32_t j = 0; j < dependency_count; j++) {
			REJECT_CACHE_IF_PAST_EOF();

			const uint32_t dependency_index = file->get_32();
			REJECT_CACHE_IF(dependency_index >= index_to_path.size());
			const String &dependency_path = index_to_path[dependency_index];

			REJECT_CACHE_IF(dependent_index == dependency_index);
			_add_dependency(dependent_path, dependency_path);
		}
	}

	REJECT_CACHE_IF(file->get_position() < file->get_length()); // Expected EOF.

	file->close();

#undef REJECT_CACHE_IF
#undef REJECT_CACHE_IF_PAST_EOF

	if (!paths_to_invalidate.is_empty()) {
		print_verbose(vformat(
				R"(GDScript: Invalidating %d scripts (and their inverse dependencies) from compiler cache...)",
				paths_to_invalidate.size()));
		_invalidate_scripts(paths_to_invalidate);
	}

	print_verbose("GDScript: Compiler cache loaded.");
}

void Registry::set_tab_size(int p_tab_size) {
	ERR_FAIL_COND(p_tab_size <= 0);

	if (p_tab_size == tab_size) {
		return;
	}

	tab_size = p_tab_size;
	_invalidate_all_scripts();
}

void Registry::clear() {
	scripts.clear();
	fqtn_to_path.clear();

	dependencies.clear();
	inverse_dependencies.clear();
}

void Registry::scan_project_dir() {
	_load_cache();

	print_verbose("GDScript: Scanning project...");

	const String project_data_dir = ProjectSettings::get_singleton()->get_project_data_path();
	_scan_dir<true>("res://", project_data_dir);

	print_verbose("GDScript: Project scanned.");
}

void Registry::add_script(const String &p_path) {
	ERR_FAIL_COND(p_path.get_extension() != GD_FILE_EXTENSION);
	ERR_FAIL_COND(scripts.has(p_path));

	_parse_script<false>(p_path);
}

void Registry::add_script_dir(const String &p_dir) {
	ERR_FAIL_COND(p_dir.is_empty());

	const String project_data_dir = ProjectSettings::get_singleton()->get_project_data_path();
	_scan_dir<false>(p_dir, project_data_dir);
}

void Registry::remove_script(const String &p_path) {
	ERR_FAIL_COND(!scripts.has(p_path));

	_invalidate_scripts({ p_path });

	scripts.erase(p_path);
}

void Registry::remove_script_dir(const String &p_dir) {
	ERR_FAIL_COND(p_dir.is_empty());

	const String dir_prefix = p_dir.ends_with("/") ? p_dir : (p_dir + "/");

	LocalVector<String> paths_to_remove;
	for (const KeyValue<String, ScriptInfo> &kv : scripts) {
		if (kv.key.begins_with(dir_prefix)) {
			paths_to_remove.push_back(kv.key);
		}
	}

	if (!paths_to_remove.is_empty()) {
		_invalidate_scripts(paths_to_remove);

		for (const String &path : paths_to_remove) {
			scripts.erase(path);
		}
	}
}

AST Registry::get_ast_by_path(const String &p_path, AST::DesiredStatus p_status) {
	ERR_FAIL_COND_V(!scripts.has(p_path), AST());

	const AST &ast = scripts[p_path].ast;

	if (ast.is_null()) {
		_parse_script<true>(p_path);
		if (ast.is_null()) {
			return AST();
		}
	}

	AST::ClassNode *main_class = ast.get_root()->main_class;

	if (main_class->is_resolving()) {
		return ast;
	}

	switch (p_status) {
		using DesiredStatus = AST::DesiredStatus;
		using NodeStatus = AST::Node::Status;

		case DesiredStatus::UNRESOLVED:
			break; // Nothing to do.
		case DesiredStatus::RESOLVED_INHERITANCE:
			if (main_class->status < NodeStatus::RESOLVED_INHERITANCE) {
				Analyzer analyzer(ast, false);
				analyzer.resolve_inheritance();
			}
			break;
		case DesiredStatus::RESOLVED_INTERFACE:
			if (main_class->status < NodeStatus::RESOLVED_INTERFACE) {
				Analyzer analyzer(ast, false);
				analyzer.resolve_interface();
			}
			break;
		case DesiredStatus::RESOLVED_BODY:
			if (main_class->status < NodeStatus::RESOLVED_BODY) {
				Analyzer analyzer(ast, false);
				analyzer.resolve_body();
			}
			break;
	}

	return ast;
}

AST Registry::get_ast_by_fqtn(const StringName &p_fqtn, AST::DesiredStatus p_status) {
	ERR_FAIL_COND_V(!fqtn_to_path.has(p_fqtn), AST());

	return get_ast_by_path(fqtn_to_path[p_fqtn], p_status);
}

void Registry::add_dependency(const String &p_from_path, const String &p_to_path) {
	ERR_FAIL_COND(!scripts.has(p_from_path));
	ERR_FAIL_COND(!scripts.has(p_to_path));
	ERR_FAIL_COND(p_from_path == p_to_path);

	_add_dependency(p_from_path, p_to_path);
}

AST Registry::get_ast_for_editor(const String &p_path, const String &p_source, AST::DesiredStatus p_status) const {
	AST ast;
	{
		Parser parser(p_path, p_source, tab_size);
		parser.parse();

		ast = parser.get_ast();
	}

	AST::ClassNode *main_class = ast.get_root()->main_class;

	switch (p_status) {
		using DesiredStatus = AST::DesiredStatus;
		using NodeStatus = AST::Node::Status;

		case DesiredStatus::UNRESOLVED:
			break; // Nothing to do.
		case DesiredStatus::RESOLVED_INHERITANCE:
			if (main_class->status < NodeStatus::RESOLVED_INHERITANCE) {
				Analyzer analyzer(ast, true);
				analyzer.resolve_inheritance();
			}
			break;
		case DesiredStatus::RESOLVED_INTERFACE:
			if (main_class->status < NodeStatus::RESOLVED_INTERFACE) {
				Analyzer analyzer(ast, true);
				analyzer.resolve_interface();
			}
			break;
		case DesiredStatus::RESOLVED_BODY:
			if (main_class->status < NodeStatus::RESOLVED_BODY) {
				Analyzer analyzer(ast, true);
				analyzer.resolve_body();
			}
			break;
	}

	return ast;
}
