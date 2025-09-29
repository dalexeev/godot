/**************************************************************************/
/*  diagnostic_list.cpp                                                   */
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

#include "diagnostic_list.h"

#include "core/config/project_settings.h"

using namespace gdscript;

bool DiagnosticList::is_project_ignoring_warnings = false;
WarningDB::Level DiagnosticList::warning_levels[(int)WarningDB::Code::MAX];
LocalVector<DiagnosticList::WarningDirectoryRule> DiagnosticList::warning_directory_rules;

void DiagnosticList::define_project_settings() {
	GLOBAL_DEF("debug/gdscript3/warnings/enable", true);

	const PropertyInfo directory_rules_property_info(
			Variant::DICTIONARY,
			"debug/gdscript3/warnings/directory_rules",
			PROPERTY_HINT_TYPE_STRING,
			vformat("%d/%d:;%d/%d:Exclude,Include", Variant::STRING, PROPERTY_HINT_DIR, Variant::INT, PROPERTY_HINT_ENUM));
	const Dictionary directory_rules_default_value = {
		{ "res://addons", WarningDirectoryRule::Decision::EXCLUDE },
	};
	GLOBAL_DEF(directory_rules_property_info, directory_rules_default_value);

	for (int i = 0; i < (int)WarningDB::Code::MAX; i++) {
		const WarningDB::Code code = (WarningDB::Code)i;
		const Variant default_value = WarningDB::get_default_value(code);
		GLOBAL_DEF(WarningDB::get_property_info(code), default_value);
	}
}

void DiagnosticList::update_project_settings() {
	is_project_ignoring_warnings = !GLOBAL_GET("debug/gdscript3/warnings/enable").booleanize();

	for (int i = 0; i < (int)WarningDB::Code::MAX; i++) {
		const String setting_path = WarningDB::get_setting_path_from_code((WarningDB::Code)i);
		warning_levels[i] = (WarningDB::Level)(int)GLOBAL_GET(setting_path);
	}

	warning_directory_rules.clear();

	const Dictionary rules = GLOBAL_GET("debug/gdscript3/warnings/directory_rules");
	for (const KeyValue<Variant, Variant> &kv : rules) {
		String dir = kv.key.operator String().simplify_path();

		ERR_CONTINUE_MSG(
				!dir.begins_with("res://"),
				R"(Paths in the project setting "debug/gdscript3/warnings/directory_rules" keys must start with the "res://" prefix.)");

		if (!dir.ends_with("/")) {
			dir += '/';
		}

		const int decision = kv.value;
		ERR_CONTINUE((int)decision < 0 || (int)decision >= (int)WarningDirectoryRule::Decision::MAX);

		WarningDirectoryRule rule;
		rule.directory_path = dir;
		rule.decision = (WarningDirectoryRule::Decision)decision;

		warning_directory_rules.push_back(rule);
	}

	struct RuleSort {
		bool operator()(const WarningDirectoryRule &p_a, const WarningDirectoryRule &p_b) const {
			return p_a.directory_path.count("/") > p_b.directory_path.count("/");
		}
	};

	warning_directory_rules.sort_custom<RuleSort>();
}

void DiagnosticList::clear_static_data() {
	warning_directory_rules.clear();
}

void DiagnosticList::push_error(const SourceRegion &p_source_region, const String &p_message) {
	Error error;
	error.message = p_message;
	error.source_region = p_source_region;

	errors.push_back(error);
}

void DiagnosticList::push_warning(const SourceRegion &p_source_region, WarningDB::Code p_code, const Array &p_symbols) {
	ERR_FAIL_INDEX((int)p_code, (int)WarningDB::Code::MAX);

	if (is_project_ignoring_warnings || is_script_ignoring_warnings) {
		return;
	}

	const WarningDB::Level warn_level = warning_levels[(int)p_code];
	if (warn_level == WarningDB::Level::IGNORE) {
		return;
	}

	Warning warning;
	warning.code = p_code;
	warning.symbols = p_symbols;
	warning.source_region = p_source_region;

	pending_warnings.push_back(warning);
}

void DiagnosticList::mark_warning_ignored_region(const SourceRegion &p_source_region, WarningDB::Code p_code) {
	ERR_FAIL_INDEX((int)p_code, (int)WarningDB::Code::MAX);

	const int start_pos = p_source_region.start.position;
	const int end_pos = p_source_region.end.position;

	ERR_FAIL_COND(start_pos > end_pos);

	if (unlikely(start_pos == end_pos)) {
		return;
	}

	WarningIgnoredRegion region;
	region.code = p_code;
	region.start_position = start_pos;
	region.end_position = end_pos;

	warning_ignored_regions.push_back(region);
}

void DiagnosticList::apply_pending_warnings() {
	if (pending_warnings.is_empty()) {
		return;
	}

	Vector<Pair<int, int>> regions_by_code[(int)WarningDB::Code::MAX];
	for (const WarningIgnoredRegion &region : warning_ignored_regions) {
		regions_by_code[(int)region.code].push_back({ region.start_position, region.end_position });
	}

	for (int i = 0; i < (int)WarningDB::Code::MAX; i++) {
		Vector<Pair<int, int>> &regions = regions_by_code[i];

		if (regions.size() < 2) {
			continue;
		}

		regions.sort();

		Vector<Pair<int, int>> new_regions;

		Pair<int, int> last = regions[0];
		for (int64_t j = 1; j < regions.size(); j++) {
			const Pair<int, int> &current = regions[j];
			if (last.second < current.first) {
				new_regions.push_back(last);
				last = current;
			} else if (last.second < current.second) {
				last.second = current.second;
			}
		}
		new_regions.push_back(last);

		regions = new_regions;
	}

	for (const Warning &warning : pending_warnings) {
		const Vector<Pair<int, int>> &regions = regions_by_code[(int)warning.code];
		if (!regions.is_empty()) {
			const int start_pos = warning.source_region.start.position;
			const int end_pos = warning.source_region.end.position;

			int64_t low = 0;
			int64_t high = regions.size() - 1;
			int64_t middle = (low + high) / 2;

			bool enclosing_region_found = false;
			while (low <= high) {
				const Pair<int, int> &region = regions[middle];

				if (end_pos < region.first) {
					high = middle - 1;
				} else if (start_pos > region.second) {
					low = middle + 1;
				} else {
					enclosing_region_found = region.first <= start_pos && end_pos <= region.second;
					break;
				}

				middle = (low + high) / 2;
			}

			if (enclosing_region_found) {
				continue;
			}
		}

		const WarningDB::Level warn_level = warning_levels[(int)warning.code];
		if (warn_level == WarningDB::Level::ERROR) {
			const String message = WarningDB::get_message(warning.code, warning.symbols);
			push_error(warning.source_region, message + " (Warning treated as error.)");
		} else {
			warnings.push_back(warning);
		}
	}

	pending_warnings.clear();
}

void DiagnosticList::mark_unsafe_lines(const SourceRegion &p_source_region) {
	ERR_FAIL_COND(p_source_region.start.line > p_source_region.end.line);

	for (int line = p_source_region.start.line; line <= p_source_region.end.line; line++) {
		unsafe_lines.insert(line);
	}
}

void DiagnosticList::sort_errors() {
	// TODO
}

void DiagnosticList::sort_warnings() {
	// TODO
}

void DiagnosticList::evaluate_warning_directory_rules(const String &p_script_path) {
	is_script_ignoring_warnings = false;
	for (const WarningDirectoryRule &rule : warning_directory_rules) {
		if (p_script_path.begins_with(rule.directory_path)) {
			switch (rule.decision) {
				case WarningDirectoryRule::Decision::EXCLUDE:
					is_script_ignoring_warnings = true;
					return; // Stop checking rules.
				case WarningDirectoryRule::Decision::INCLUDE:
					is_script_ignoring_warnings = false;
					return; // Stop checking rules.
				case WarningDirectoryRule::Decision::MAX:
					return; // Unreachable.
			}
		}
	}
}

DiagnosticList::DiagnosticList(const String &p_script_path) {
	ERR_FAIL_COND(p_script_path.is_empty());
	evaluate_warning_directory_rules(p_script_path);
}
