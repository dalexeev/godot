/**************************************************************************/
/*  print_utils.cpp                                                       */
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

#include "print_utils.h"

#include "../../compiler/common_defs.h"

#include "core/string/string_builder.h"
#include "core/variant/variant.h"

namespace gdscript {

// Newlines are not taken into account; all characters except tabs are considered to have a width of 1.
String replace_tabs(const String &p_string) {
	StringBuilder result;

	int buffer_start = 0;
	int column_index = 0;
	for (int i = 0; i < p_string.size(); i++) {
		if (p_string[i] == '\t') {
			const int tab_width = DEFAULT_TAB_SIZE - column_index % DEFAULT_TAB_SIZE;

			result += p_string.substr(buffer_start, i - buffer_start);
			result += String::chr(' ').repeat(tab_width);

			buffer_start = i + 1;
			column_index += tab_width;
		} else {
			column_index++;
		}
	}

	result += p_string.substr(buffer_start);

	return result.as_string();
}

// `p_offset` and `p_width` must be non-negative.
void print_source_pointer(char32_t p_symbol, int p_offset, int p_width) {
	if (p_width == 0) {
		p_symbol = '*';
		p_width = 1;
	}
	print_line(String::chr(' ').repeat(p_offset) + String::chr(p_symbol).repeat(p_width));
}

void print_source_region(
		const SourceRegion &p_source_region,
		const Vector<String> &p_lines,
		const HashSet<int> &p_unsafe_lines) {
	constexpr int START_OFFSET = 6;

	if (p_source_region.start.line != p_source_region.end.line) {
		int line_width = 0;
		const int start_line_index = p_source_region.start.line - 1;
		if (start_line_index >= 0 && start_line_index < p_lines.size()) {
			ERR_FAIL_INDEX(start_line_index, p_lines.size());
			line_width = replace_tabs(p_lines[start_line_index]).length();
		}

		const int offset = START_OFFSET + MAX(0, p_source_region.start.column - 1);
		const int width = MAX(0, line_width - p_source_region.start.column + 1);
		print_source_pointer('v', offset, width);
	}

	ERR_FAIL_COND(p_source_region.start.line > p_source_region.end.line);
	for (int line = p_source_region.start.line; line <= p_source_region.end.line; line++) {
		ERR_FAIL_INDEX(line - 1, p_lines.size());
		const char32_t line_marker = p_unsafe_lines.has(line) ? '!' : ' ';
		print_line(vformat("%c%04d %s", line_marker, line, replace_tabs(p_lines[line - 1])));
	}

	if (p_source_region.start.line == p_source_region.end.line) {
		const int offset = START_OFFSET + MAX(0, p_source_region.start.column - 1);
		const int width = MAX(0, p_source_region.end.column - p_source_region.start.column);
		print_source_pointer('^', offset, width);
	} else {
		const int offset = START_OFFSET;
		const int width = MAX(0, p_source_region.end.column - 1);
		print_source_pointer('^', offset, width);
	}
}

} // namespace gdscript
