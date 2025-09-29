/**************************************************************************/
/*  common_defs.h                                                         */
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

#include "core/string/ustring.h"

namespace gdscript {

inline constexpr const char *GD_FILE_EXTENSION = "gd3";

inline constexpr int DEFAULT_TAB_SIZE = 4;

struct SourcePosition {
	int position = 0; // Offset relative to the beginning of the file.
	int line = 1; // Visual line number (1-based).
	int column = 1; // Visual column number (1-based). Tab-aware.
	int raw_column = 0; // Offset relative to the beginning of the line.

	SourcePosition() {}
};

struct SourceRegion {
	SourcePosition start;
	SourcePosition end;

	_FORCE_INLINE_ String get_text(const String &p_full_source) const {
		return p_full_source.substr(start.position, end.position - start.position);
	}

	SourceRegion() {}
	SourceRegion(const SourcePosition &p_start, const SourcePosition &p_end) :
			start(p_start),
			end(p_end) {}
};

struct Comment {
	bool is_inline = false;
	SourceRegion source_region;

	Comment() {}
};

enum class Trilean : uint8_t {
	FALSE,
	UNKNOWN,
	TRUE,
};

_FORCE_INLINE_ constexpr Trilean boolean_to_trilean(bool p_value) {
	return p_value ? Trilean::TRUE : Trilean::FALSE;
}

_FORCE_INLINE_ constexpr Trilean trilean_not(Trilean p_a) {
	return (Trilean)((uint8_t)Trilean::TRUE - (uint8_t)p_a);
}

_FORCE_INLINE_ constexpr Trilean trilean_and(Trilean p_a, Trilean p_b) {
	return MIN(p_a, p_b);
}

_FORCE_INLINE_ constexpr Trilean trilean_or(Trilean p_a, Trilean p_b) {
	return MAX(p_a, p_b);
}

Trilean trilean_consensus(const Vector<Trilean> &p_values);

String get_alternative_list(const Vector<String> &p_items);

} // namespace gdscript
