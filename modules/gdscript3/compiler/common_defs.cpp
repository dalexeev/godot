/**************************************************************************/
/*  common_defs.cpp                                                        */
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

#include "common_defs.h"

namespace gdscript {

// This operation has no neutral element. No value can act as the initial value for an accumulator variable.
// Therefore, this function accepts a list of **all** operands and is not implemented as a binary operation.
Trilean trilean_consensus(const Vector<Trilean> &p_values) {
	ERR_FAIL_COND_V(p_values.is_empty(), Trilean::UNKNOWN);

	const Trilean first_value = p_values[0];

	if (first_value == Trilean::UNKNOWN) {
		return Trilean::UNKNOWN;
	}

	for (int64_t i = 1; i < p_values.size(); i++) {
		if (p_values[i] != first_value) {
			return Trilean::UNKNOWN;
		}
	}

	return first_value;
}

String get_alternative_list(const Vector<String> &p_items) {
	ERR_FAIL_COND_V(p_items.is_empty(), String());

	if (p_items.size() == 1) {
		return p_items[0];
	} else if (p_items.size() == 2) {
		return p_items[0] + " or " + p_items[1];
	} else {
		String result = p_items[0];
		for (int64_t i = 1; i < p_items.size() - 1; i++) {
			result += ", " + p_items[i];
		}
		result += ", or " + p_items[p_items.size() - 1];
		return result;
	}
}

} // namespace gdscript
