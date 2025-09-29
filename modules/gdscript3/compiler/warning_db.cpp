/**************************************************************************/
/*  warning_db.cpp                                                        */
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

#include "warning_db.h"

#include "core/object/property_info.h"

using namespace gdscript;

WarningDB::Code WarningDB::get_code_from_name(const String &p_name) {
	for (int i = 0; i < (int)Code::MAX; i++) {
		if (get_name_from_code((Code)i) == p_name) {
			return (Code)i;
		}
	}

	return Code::MAX;
}

String WarningDB::get_name_from_code(Code p_code) {
	constexpr const char *warning_names[] = {
		PNAME("UNASSIGNED_VARIABLE"),
		PNAME("UNASSIGNED_VARIABLE_OP_ASSIGN"),
		PNAME("UNUSED_VARIABLE"),
		PNAME("UNUSED_LOCAL_CONSTANT"),
		PNAME("UNUSED_PRIVATE_CLASS_VARIABLE"),
		PNAME("UNUSED_PARAMETER"),
		PNAME("UNUSED_SIGNAL"),
		PNAME("SHADOWED_VARIABLE"),
		PNAME("SHADOWED_VARIABLE_BASE_CLASS"),
		PNAME("SHADOWED_GLOBAL_IDENTIFIER"),
		PNAME("UNREACHABLE_CODE"),
		PNAME("UNREACHABLE_PATTERN"),
		PNAME("STANDALONE_EXPRESSION"),
		PNAME("STANDALONE_TERNARY"),
		PNAME("INCOMPATIBLE_TERNARY"),
		PNAME("UNTYPED_DECLARATION"),
		PNAME("INFERRED_DECLARATION"),
		PNAME("UNSAFE_PROPERTY_ACCESS"),
		PNAME("UNSAFE_METHOD_ACCESS"),
		PNAME("UNSAFE_CALL_ARGUMENT"),
		PNAME("UNSAFE_VOID_RETURN"),
		PNAME("RETURN_VALUE_DISCARDED"),
		PNAME("STATIC_CALLED_ON_INSTANCE"),
		PNAME("MISSING_TOOL"),
		PNAME("REDUNDANT_AWAIT"),
		PNAME("MISSING_AWAIT"),
		PNAME("ASSERT_ALWAYS_TRUE"),
		PNAME("ASSERT_ALWAYS_FALSE"),
		PNAME("INTEGER_DIVISION"),
		PNAME("NARROWING_CONVERSION"),
		PNAME("INT_AS_ENUM_WITHOUT_CAST"),
		PNAME("INT_AS_ENUM_WITHOUT_MATCH"),
		PNAME("ENUM_VARIABLE_WITHOUT_DEFAULT"),
		PNAME("EMPTY_FILE"),
		PNAME("DEPRECATED_KEYWORD"),
		PNAME("CONFUSABLE_IDENTIFIER"),
		PNAME("CONFUSABLE_LOCAL_DECLARATION"),
		PNAME("CONFUSABLE_LOCAL_USAGE"),
		PNAME("CONFUSABLE_CAPTURE_REASSIGNMENT"),
		PNAME("INFERENCE_ON_VARIANT"),
		PNAME("NATIVE_METHOD_OVERRIDE"),
		PNAME("GET_NODE_DEFAULT_WITHOUT_ONREADY"),
		PNAME("ONREADY_WITH_EXPORT"),
	};

	static_assert(
			std_size(warning_names) == (int)Code::MAX,
			"Amount of warning names don't match the amount of warning codes.");
	ERR_FAIL_INDEX_V((int)p_code, (int)Code::MAX, String());
	return warning_names[(int)p_code];
}

String WarningDB::get_setting_path_from_code(Code p_code) {
	return "debug/gdscript3/warnings/" + get_name_from_code(p_code).to_lower();
}

int WarningDB::get_default_value(Code p_code) {
	constexpr Level default_warning_levels[] = {
		Level::WARN, // UNASSIGNED_VARIABLE
		Level::WARN, // UNASSIGNED_VARIABLE_OP_ASSIGN
		Level::WARN, // UNUSED_VARIABLE
		Level::WARN, // UNUSED_LOCAL_CONSTANT
		Level::WARN, // UNUSED_PRIVATE_CLASS_VARIABLE
		Level::WARN, // UNUSED_PARAMETER
		Level::WARN, // UNUSED_SIGNAL
		Level::WARN, // SHADOWED_VARIABLE
		Level::WARN, // SHADOWED_VARIABLE_BASE_CLASS
		Level::WARN, // SHADOWED_GLOBAL_IDENTIFIER
		Level::WARN, // UNREACHABLE_CODE
		Level::WARN, // UNREACHABLE_PATTERN
		Level::WARN, // STANDALONE_EXPRESSION
		Level::WARN, // STANDALONE_TERNARY
		Level::WARN, // INCOMPATIBLE_TERNARY
		Level::IGNORE, // UNTYPED_DECLARATION // Static typing is optional, we don't want to spam warnings.
		Level::IGNORE, // INFERRED_DECLARATION // Static typing is optional, we don't want to spam warnings.
		Level::IGNORE, // UNSAFE_PROPERTY_ACCESS // Too common in untyped scenarios.
		Level::IGNORE, // UNSAFE_METHOD_ACCESS // Too common in untyped scenarios.
		Level::IGNORE, // UNSAFE_CALL_ARGUMENT // Too common in untyped scenarios.
		Level::WARN, // UNSAFE_VOID_RETURN
		Level::IGNORE, // RETURN_VALUE_DISCARDED // Too spammy by default on common cases (`connect()`, `Tween`, etc.).
		Level::WARN, // STATIC_CALLED_ON_INSTANCE
		Level::WARN, // MISSING_TOOL
		Level::WARN, // REDUNDANT_AWAIT
		Level::IGNORE, // MISSING_AWAIT // May be a false positive, interferes with a valid feature.
		Level::WARN, // ASSERT_ALWAYS_TRUE
		Level::WARN, // ASSERT_ALWAYS_FALSE
		Level::WARN, // INTEGER_DIVISION
		Level::WARN, // NARROWING_CONVERSION
		Level::WARN, // INT_AS_ENUM_WITHOUT_CAST
		Level::WARN, // INT_AS_ENUM_WITHOUT_MATCH
		Level::WARN, // ENUM_VARIABLE_WITHOUT_DEFAULT
		Level::WARN, // EMPTY_FILE
		Level::WARN, // DEPRECATED_KEYWORD
		Level::WARN, // CONFUSABLE_IDENTIFIER
		Level::WARN, // CONFUSABLE_LOCAL_DECLARATION
		Level::WARN, // CONFUSABLE_LOCAL_USAGE
		Level::WARN, // CONFUSABLE_CAPTURE_REASSIGNMENT
		Level::ERROR, // INFERENCE_ON_VARIANT // Most likely done by accident, usually inference is trying for a particular type.
		Level::ERROR, // NATIVE_METHOD_OVERRIDE // May not work as expected.
		Level::ERROR, // GET_NODE_DEFAULT_WITHOUT_ONREADY // May not work as expected.
		Level::ERROR, // ONREADY_WITH_EXPORT // May not work as expected.
	};

	static_assert(
			std_size(default_warning_levels) == (int)Code::MAX,
			"Amount of default warning levels don't match the amount of warning codes.");
	ERR_FAIL_INDEX_V((int)p_code, (int)Code::MAX, (int)Level::IGNORE);
	return (int)default_warning_levels[(int)p_code];
}

PropertyInfo WarningDB::get_property_info(Code p_code) {
	return PropertyInfo(Variant::INT, get_setting_path_from_code(p_code), PROPERTY_HINT_ENUM, "Ignore,Warn,Error");
}

String WarningDB::get_message(Code p_code, const Array &p_symbols) {
	ERR_FAIL_INDEX_V((int)p_code, (int)Code::MAX, String());

#define MSG(m_msg) return String(m_msg).format(p_symbols)

	switch (p_code) {
		case Code::UNASSIGNED_VARIABLE:
			MSG(R"*(The variable "{0}" is used before being assigned a value.)*");
		case Code::UNASSIGNED_VARIABLE_OP_ASSIGN:
			MSG(R"*(The variable "{0}" is modified with the compound-assignment operator "{1}=" but was not previously initialized.)*");
		case Code::UNUSED_VARIABLE:
			MSG(R"*(The local variable "{0}" is declared but never used in the block. If this is intended, prefix it with an underscore: "_{0}".)*");
		case Code::UNUSED_LOCAL_CONSTANT:
			MSG(R"*(The local constant "{0}" is declared but never used in the block. If this is intended, prefix it with an underscore: "_{0}".)*");
		case Code::UNUSED_PRIVATE_CLASS_VARIABLE:
			MSG(R"*(The class variable "{0}" is declared but never used in the class.)*");
		case Code::UNUSED_PARAMETER:
			MSG(R"*(The parameter "{1}" is never used in the function "{0}()". If this is intended, prefix it with an underscore: "_{1}".)*");
		case Code::UNUSED_SIGNAL:
			MSG(R"*(The signal "{0}" is declared but never explicitly used in the class.)*");
		case Code::SHADOWED_VARIABLE:
			MSG(R"*(The local {0} "{1}" is shadowing an already-declared {2} at line {3} in the current class.)*");
		case Code::SHADOWED_VARIABLE_BASE_CLASS:
			if (p_symbols.size() > 4) {
				MSG(R"*(The local {0} "{1}" is shadowing an already-declared {2} at line {3} in the base class "{4}".)*");
			}
			MSG(R"*(The local {0} "{1}" is shadowing an already-declared {2} in the base class "{3}".)*");
		case Code::SHADOWED_GLOBAL_IDENTIFIER:
			MSG(R"*(The {0} "{1}" has the same name as a {2}.)*");
		case Code::UNREACHABLE_CODE:
			MSG(R"*(Unreachable code (statement after return) in function "{0}()".)*");
		case Code::UNREACHABLE_PATTERN:
			MSG(R"*(Unreachable pattern (pattern after wildcard or bind).)*");
		case Code::STANDALONE_EXPRESSION:
			MSG(R"*(Standalone expression (the line may have no effect).)*");
		case Code::STANDALONE_TERNARY:
			MSG(R"*(Standalone ternary operator (the return value is being discarded).)*");
		case Code::INCOMPATIBLE_TERNARY:
			MSG(R"*(Values of the ternary operator are not mutually compatible.)*");
		case Code::UNTYPED_DECLARATION:
			if (!p_symbols.is_empty() && p_symbols[0] == "Function") {
				MSG(R"*({0} "{1}()" has no static return type.)*");
			}
			MSG(R"*({0} "{1}" has no static type.)*");
		case Code::INFERRED_DECLARATION:
			MSG(R"*({0} "{1}" has an implicitly inferred static type.)*");
		case Code::UNSAFE_PROPERTY_ACCESS:
			MSG(R"*(The property "{0}" is not present on the inferred type "{1}" (but may be present on a subtype).)*");
		case Code::UNSAFE_METHOD_ACCESS:
			MSG(R"*(The method "{0}()" is not present on the inferred type "{1}" (but may be present on a subtype).)*");
		case Code::UNSAFE_CALL_ARGUMENT:
			MSG(R"*(The argument {0} of the {1} "{2}()" requires the subtype "{3}" but the supertype "{4}" was provided.)*");
		case Code::UNSAFE_VOID_RETURN:
			MSG(R"*(The method "{0}()" returns "void" but it's trying to return a call to "{1}()" that can't be ensured to also be "void".)*");
		case Code::RETURN_VALUE_DISCARDED:
			MSG(R"*(The function "{0}()" returns a value that will be discarded if not used.)*");
		case Code::STATIC_CALLED_ON_INSTANCE:
			MSG(R"*(The function "{0}()" is a static function but was called from an instance. Instead, it should be directly called from the type: "{1}.{0}()".)*");
		case Code::MISSING_TOOL:
			MSG(R"*(The base class script has the "@tool" annotation, but this script does not have it.)*");
		case Code::REDUNDANT_AWAIT:
			MSG(R"*("await" keyword is unnecessary because the expression isn't a coroutine nor a signal.)*");
		case Code::MISSING_AWAIT:
			MSG(R"*("await" keyword might be desired because the expression is a coroutine.)*");
		case Code::ASSERT_ALWAYS_TRUE:
			MSG(R"*(Assert statement is redundant because the expression is always true.)*");
		case Code::ASSERT_ALWAYS_FALSE:
			MSG(R"*(Assert statement will raise an error because the expression is always false.)*");
		case Code::INTEGER_DIVISION:
			MSG(R"*(Integer division. Decimal part will be discarded.)*");
		case Code::NARROWING_CONVERSION:
			MSG(R"*(Narrowing conversion (float is converted to int and loses precision).)*");
		case Code::INT_AS_ENUM_WITHOUT_CAST:
			MSG(R"*(Integer used when an enum value is expected. If this is intended, cast the integer to the enum type using the \"as\" keyword.)*");
		case Code::INT_AS_ENUM_WITHOUT_MATCH:
			MSG(R"*(Cannot {0} {1} as enum "{2}": no enum member has matching value.)*");
		case Code::ENUM_VARIABLE_WITHOUT_DEFAULT:
			MSG(R"*(The variable "{0}" has an enum type and does not set an explicit default value. The default will be set to "0".)*");
		case Code::EMPTY_FILE:
			MSG(R"*(Empty script file.)*");
		case Code::DEPRECATED_KEYWORD:
			MSG(R"*(The "{0}" keyword is deprecated and will be removed in a future release. Please replace it with "{1}".)*");
		case Code::CONFUSABLE_IDENTIFIER:
			MSG(R"*(The identifier "{0}" has misleading characters and might be confused with something else.)*");
		case Code::CONFUSABLE_LOCAL_DECLARATION:
			MSG(R"*(The {0} "{1}" is declared below in the parent block.)*");
		case Code::CONFUSABLE_LOCAL_USAGE:
			MSG(R"*(The identifier "{0}" will be shadowed below in the block.)*");
		case Code::CONFUSABLE_CAPTURE_REASSIGNMENT:
			MSG(R"*(Reassigning lambda capture does not modify the outer local variable "{0}".)*");
		case Code::INFERENCE_ON_VARIANT:
			MSG(R"*(The {0} type is being inferred from a Variant value, so it will be typed as Variant.)*");
		case Code::NATIVE_METHOD_OVERRIDE:
			MSG(R"*(The method "{0}()" overrides a method from native class "{1}". This won't be called by the engine and may not work as expected.)*");
		case Code::GET_NODE_DEFAULT_WITHOUT_ONREADY:
			MSG(R"*(The default value uses "{0}" which won't return nodes in the scene tree before "_ready()" is called. Use the "@onready" annotation to solve this.)*");
		case Code::ONREADY_WITH_EXPORT:
			MSG(R"*("@onready" will set the default value after "@export" takes effect and will override it.)*");
		case Code::MAX:
			break; // Unreachable.
	}

#undef MSG

	ERR_FAIL_V(get_name_from_code(p_code));
}
