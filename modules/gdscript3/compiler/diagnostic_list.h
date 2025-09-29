/**************************************************************************/
/*  diagnostic_list.h                                                     */
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

#include "common_defs.h"
#include "warning_db.h"

#include "core/templates/hash_set.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "core/variant/array.h"

struct PropertyInfo;

namespace gdscript {

class DiagnosticList {
public:
	struct Error {
		String message;
		SourceRegion source_region;

		Error() {}
	};

	struct Warning {
		WarningDB::Code code = WarningDB::Code::MAX;
		Array symbols;
		SourceRegion source_region;

		Warning() {}
	};

	struct WarningDirectoryRule {
		enum class Decision {
			EXCLUDE,
			INCLUDE,
			MAX,
		};

		String directory_path; // With a trailing slash.
		Decision decision = Decision::EXCLUDE;

		WarningDirectoryRule() {}
	};

private:
	struct WarningIgnoredRegion {
		WarningDB::Code code = WarningDB::Code::MAX;
		int start_position = 0;
		int end_position = 0;

		WarningIgnoredRegion() {}
	};

	static bool is_project_ignoring_warnings;
	static WarningDB::Level warning_levels[(int)WarningDB::Code::MAX];
	static LocalVector<WarningDirectoryRule> warning_directory_rules;

	List<Error> errors;

	List<Warning> warnings;
	List<Warning> pending_warnings;
	LocalVector<WarningIgnoredRegion> warning_ignored_regions;
	bool is_script_ignoring_warnings = false;

	HashSet<int> unsafe_lines;

	void evaluate_warning_directory_rules(const String &p_script_path);

public:
	static void define_project_settings();
	static void update_project_settings();
	static void clear_static_data();

	void push_error(const SourceRegion &p_source_region, const String &p_message);

	void push_warning(const SourceRegion &p_source_region, WarningDB::Code p_code, const Array &p_symbols);
	void mark_warning_ignored_region(const SourceRegion &p_source_region, WarningDB::Code p_code);
	void apply_pending_warnings();

	void mark_unsafe_lines(const SourceRegion &p_source_region);

	void sort_errors();
	void sort_warnings();

	_FORCE_INLINE_ const List<Error> &get_errors() const { return errors; }
	_FORCE_INLINE_ const List<Warning> &get_warnings() const { return warnings; }
	_FORCE_INLINE_ const HashSet<int> &get_unsafe_lines() const { return unsafe_lines; }

	DiagnosticList() {}
	DiagnosticList(const String &p_script_path);
};

} // namespace gdscript
