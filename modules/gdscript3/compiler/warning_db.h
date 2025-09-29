/**************************************************************************/
/*  warning_db.h                                                          */
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

class Array;
class String;
struct PropertyInfo;

namespace gdscript {

class WarningDB {
public:
	enum class Level {
		IGNORE,
		WARN,
		ERROR,
	};

	enum class Code {
		UNASSIGNED_VARIABLE, // Variable used but never assigned.
		UNASSIGNED_VARIABLE_OP_ASSIGN, // Variable never assigned but used in an assignment operation (`+=`, `*=`, etc.).
		UNUSED_VARIABLE, // Local variable is declared but never used.
		UNUSED_LOCAL_CONSTANT, // Local constant is declared but never used.
		UNUSED_PRIVATE_CLASS_VARIABLE, // Class variable is declared private (`_` prefix) but never used in the class.
		UNUSED_PARAMETER, // Function parameter is never used.
		UNUSED_SIGNAL, // Signal is defined but never explicitly used in the class.
		SHADOWED_VARIABLE, // A local variable/constant shadows a current class member.
		SHADOWED_VARIABLE_BASE_CLASS, // A local variable/constant shadows a base class member.
		SHADOWED_GLOBAL_IDENTIFIER, // A global class or function has the same name as variable.
		UNREACHABLE_CODE, // Code after a return statement.
		UNREACHABLE_PATTERN, // Pattern in a match statement after a catch all pattern (wildcard or bind).
		STANDALONE_EXPRESSION, // Expression not assigned to a variable.
		STANDALONE_TERNARY, // Return value of ternary expression is discarded.
		INCOMPATIBLE_TERNARY, // Possible values of a ternary if are not mutually compatible.
		UNTYPED_DECLARATION, // Variable/parameter/function has no static type, explicitly specified or implicitly inferred.
		INFERRED_DECLARATION, // Variable/constant/parameter has an implicitly inferred static type.
		UNSAFE_PROPERTY_ACCESS, // Property not found in the detected type (but can be in subtypes).
		UNSAFE_METHOD_ACCESS, // Function not found in the detected type (but can be in subtypes).
		UNSAFE_CALL_ARGUMENT, // Function call argument is of a supertype of the required type.
		UNSAFE_VOID_RETURN, // Function returns void but returned a call to a function that can't be type checked.
		RETURN_VALUE_DISCARDED, // Function call returns something but the value isn't used.
		STATIC_CALLED_ON_INSTANCE, // A static method was called on an instance of a class instead of on the class itself.
		MISSING_TOOL, // The base class script has the `@tool` annotation, but this script does not have it.
		REDUNDANT_AWAIT, // `await` is used but expression is synchronous (not a signal nor a coroutine).
		MISSING_AWAIT, // `await` is not used but expression is a coroutine.
		ASSERT_ALWAYS_TRUE, // Expression for `assert` argument is always true.
		ASSERT_ALWAYS_FALSE, // Expression for `assert` argument is always false.
		INTEGER_DIVISION, // Integer divide by integer, decimal part is discarded.
		NARROWING_CONVERSION, // Float value into an integer slot, precision is lost.
		INT_AS_ENUM_WITHOUT_CAST, // An integer value was used as an enum value without casting.
		INT_AS_ENUM_WITHOUT_MATCH, // An integer value was used as an enum value without matching enum member.
		ENUM_VARIABLE_WITHOUT_DEFAULT, // A variable with an enum type does not have a default value. The default will be set to `0` instead of the first enum value.
		EMPTY_FILE, // A script file is empty.
		DEPRECATED_KEYWORD, // The keyword is deprecated and should be replaced.
		CONFUSABLE_IDENTIFIER, // The identifier contains misleading characters that can be confused. E.g. `usеr` (has Cyrillic `е` instead of Latin `e`).
		CONFUSABLE_LOCAL_DECLARATION, // The parent block declares an identifier with the same name below.
		CONFUSABLE_LOCAL_USAGE, // The identifier will be shadowed below in the block.
		CONFUSABLE_CAPTURE_REASSIGNMENT, // Reassigning lambda capture does not modify the outer local variable.
		INFERENCE_ON_VARIANT, // The declaration uses type inference but the value is typed as `Variant`.
		NATIVE_METHOD_OVERRIDE, // The script method overrides a native one, this may not work as intended.
		GET_NODE_DEFAULT_WITHOUT_ONREADY, // A class variable uses `get_node()` (or the `$` notation) as its default value, but does not use the `@onready` annotation.
		ONREADY_WITH_EXPORT, // The `@onready` annotation will set the value after `@export` which is likely not intended.
		MAX,
	};

	static Code get_code_from_name(const String &p_name);
	static String get_name_from_code(Code p_code);
	static String get_setting_path_from_code(Code p_code);
	static int get_default_value(Code p_code);
	static PropertyInfo get_property_info(Code p_code);
	static String get_message(Code p_code, const Array &p_symbols);

	WarningDB() = delete;
};

} // namespace gdscript
