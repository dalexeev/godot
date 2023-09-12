/**************************************************************************/
/*  gdscript_parser_data_type.cpp                                         */
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

#include "gdscript_parser.h"

// TODO: Move functions from analyzer as static methods.
// TODO: Add/check ERR_FAILs for native/script/class.
// TODO: Check script_path, why it is needed.

// This method should only be used for debugging and error/warning messages.
String GDScriptParser::DataType::to_string() const {
	switch (kind) {
		case UNRESOLVED:
		case RESOLVING:
			return "<unresolved type>";
		case UNKNOWN:
			// This should not affect users of untyped GDScript, because in the case of an unknown type
			// there should be no errors, only `UNSAFE_*` warnings.
			return "<unknown type>";
		case VARIANT:
			return "Variant";
		case BUILTIN:
			if (builtin_type == Variant::NIL) {
				return "null";
			} else if (builtin_type == Variant::ARRAY && is_typed_container_type()) {
				return vformat("Array[%s]", container_element_type->to_string());
			} else {
				return Variant::get_type_name(builtin_type);
			}
		case NATIVE:
			return native_type;
		case SCRIPT:
			if (script_type.is_valid() && script_type->get_global_name() != StringName()) {
				return script_type->get_global_name();
			} else if (!script_path.is_empty()) {
				return script_path.get_file();
			} else {
				return native_type;
			}
		case CLASS:
			if (class_type == nullptr) {
				return native_type;
			} else if (class_type->identifier != nullptr) {
				return class_type->identifier->name;
			} else {
				return String(class_type->fqcn).get_file();
			}
		case ENUM:
			// `native_type` contains either the native class defining the enum
			// or the fully qualified class name of the script defining the enum.
			return String(native_type).get_file(); // TODO: `+ "." + enum_type`?
	}
	ERR_FAIL_V_MSG("<unknown type>", "Kind set outside the enum range.");
}

PropertyInfo GDScriptParser::DataType::to_property_info(const String &p_name) const {
	PropertyInfo result;
	result.name = p_name;
	result.usage = PROPERTY_USAGE_NONE;

	switch (kind) {
		case UNRESOLVED:
		case RESOLVING:
			ERR_PRINT("GDScript bug (please report): Trying to get PropertyInfo from unresolved type.");
			result.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
			break;
		case UNKNOWN:
		case VARIANT:
			result.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
			break;
		case BUILTIN:
			result.type = builtin_type;
			if (builtin_type == Variant::ARRAY && has_container_element_type()) {
				const DataType *elem_type = container_element_type;
				switch (elem_type->kind) {
					case UNRESOLVED:
					case RESOLVING:
						ERR_PRINT("GDScript bug (please report): Trying to get PropertyInfo from unresolved type.");
						break;
					case UNKNOWN:
					case VARIANT:
						break;
					case BUILTIN:
						result.hint = PROPERTY_HINT_ARRAY_TYPE;
						result.hint_string = Variant::get_type_name(elem_type->builtin_type);
						break;
					case NATIVE:
						result.hint = PROPERTY_HINT_ARRAY_TYPE;
						result.hint_string = elem_type->native_type;
						break;
					case SCRIPT:
						result.hint = PROPERTY_HINT_ARRAY_TYPE;
						if (elem_type->script_type.is_valid() && elem_type->script_type->get_global_name() != StringName()) {
							result.hint_string = elem_type->script_type->get_global_name();
						} else {
							result.hint_string = elem_type->native_type;
						}
						break;
					case CLASS:
						result.hint = PROPERTY_HINT_ARRAY_TYPE;
						if (elem_type->class_type != nullptr && elem_type->class_type->get_global_name() != StringName()) {
							result.hint_string = elem_type->class_type->get_global_name();
						} else {
							result.hint_string = elem_type->native_type;
						}
						break;
					case ENUM:
						result.hint = PROPERTY_HINT_ARRAY_TYPE;
						result.hint_string = String(elem_type->native_type).replace("::", ".");
						break;
					default:
						ERR_PRINT("Kind set outside the enum range.");
						result.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
						break;
				}
			}
			break;
		case NATIVE:
			result.type = Variant::OBJECT;
			result.class_name = native_type;
			break;
		case SCRIPT:
			result.type = Variant::OBJECT;
			if (script_type.is_valid() && script_type->get_global_name() != StringName()) {
				result.class_name = script_type->get_global_name();
			} else {
				result.class_name = native_type;
			}
			break;
		case CLASS:
			result.type = Variant::OBJECT;
			if (class_type != nullptr && class_type->get_global_name() != StringName()) {
				result.class_name = class_type->get_global_name();
			} else {
				result.class_name = native_type;
			}
			break;
		case ENUM:
			result.type = Variant::INT;
			result.usage |= PROPERTY_USAGE_CLASS_IS_ENUM;
			result.class_name = String(native_type).replace("::", ".");
			break;
		default:
			ERR_PRINT("Kind set outside the enum range.");
			result.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
			break;
	}

	return result;
}

// TODO: Add comment.
GDScriptParser::DataType GDScriptParser::DataType::get_metatype() const {
	GDScriptParser::DataType result;
	switch (kind) {
		case UNRESOLVED:
		case RESOLVING:
			ERR_PRINT("GDScript bug (please report): Trying to get metatype of unresolved type.");
			break;
		case UNKNOWN:
			ERR_PRINT("GDScript bug (please report): Trying to get metatype of unknown type.");
			break;
		case VARIANT:
		case BUILTIN:
			ERR_PRINT(vformat(R"("%s" is not a first class type.)", to_string()));
			break;
		case NATIVE:
			result.kind = NATIVE;
			result.builtin_type = Variant::OBJECT;
			result.native_type = GDScriptNativeClass::get_class_static();
			break;
		case SCRIPT:
			result.kind = NATIVE;
			result.builtin_type = Variant::OBJECT;
			result.native_type = script_type.is_valid() ? script_type->get_class() : Script::get_class_static();
			break;
		case CLASS:
			result.kind = NATIVE;
			result.builtin_type = Variant::OBJECT;
			result.native_type = GDScript::get_class_static();
			break;
		case ENUM:
			// TODO: Check GlobalScope and native enums (they are not "first class types.").
			result.kind = BUILTIN;
			result.builtin_type = Variant::DICTIONARY;
			break;
		default:
			ERR_PRINT("Kind set outside the enum range.");
			break;
	}
	return result;
}

// TODO: Add comment.
Trilean GDScriptParser::DataType::is_equal_to(const DataType &p_type) const {
	ERR_FAIL_COND_V_MSG(!p_type.is_resolved(), Trilean::UNKNOWN,
			"GDScript bug (please report): Trying to check type equality for unresolved type.");

	if (p_type.kind == UNKNOWN) {
		return Trilean::UNKNOWN;
	}

	// From here `p_type` is resolved and known.

	switch (kind) {
		case UNRESOLVED:
		case RESOLVING:
			ERR_PRINT("GDScript bug (please report): Trying to check type equality for unresolved type.");
			return Trilean::UNKNOWN;
		case UNKNOWN:
			return Trilean::UNKNOWN;
		case VARIANT:
			return Trilean::TRUE; // All variants are the same.
		case BUILTIN:
			if (builtin_type == Variant::INT && p_type.kind == ENUM) {
				return Trilean::TRUE; // All enums are considered `int`.
			}
			if (builtin_type == Variant::ARRAY && p_type.kind == BUILTIN && p_type.builtin_type == Variant::ARRAY) {
				if (is_typed_container_type() && p_type.is_typed_container_type()) {
					return container_element_type->is_equal_to(p_type->container_element_type);
				} else {
					return BOOLEAN_TO_TRILEAN(!is_typed_container_type() && !p_type.is_typed_container_type());
				}
			}
			return BOOLEAN_TO_TRILEAN(p_type.kind == BUILTIN && p_type.builtin_type == builtin_type);
		case NATIVE:
			if (p_type.kind == NATIVE) {
				ERR_FAIL_COND_V(p_type.native_type == StringName() || native_type == StringName(), Trilean::UNKNOWN);
				return BOOLEAN_TO_TRILEAN(p_type.native_type == native_type);
			}
			return Trilean::FALSE;
		case SCRIPT:
			if (p_type.kind == SCRIPT) {
				ERR_FAIL_COND_V(p_type.script_type.is_null() || script_type.is_null(), Trilean::UNKNOWN);
				return BOOLEAN_TO_TRILEAN(p_type.script_type == script_type);
			}
			return Trilean::FALSE;
		case CLASS:
			if (p_type.kind == CLASS) {
				ERR_FAIL_COND_V(p_type.class_type == nullptr || class_type == nullptr, Trilean::UNKNOWN);
				return BOOLEAN_TO_TRILEAN(p_type.class_type == class_type || p_type.class_type->fqcn == class_type->fqcn);
			}
			return Trilean::FALSE;
		case ENUM:
			// All enums are considered `int`, and therefore equal to it and each other.
			// We have warnings, but they do not provide strict guarantees.
			return BOOLEAN_TO_TRILEAN(p_type.kind == ENUM || (p_type.kind == BUILTIN && p_type.builtin_type == Variant::INT));
	}

	ERR_FAIL_V_MSG(Trilean::UNKNOWN, "Kind set outside the enum range.");
}

// Returns whether this type is a supertype of (or equal to) `p_type`.
//
// A is subtype of B <=> B is supertype of A <=> ∀x: x is A => x is B <=> x ∈ A ⊆ B.
//
// Do not confuse with `is_compatible_with()`. For example, you can pass `int` where `float` is expected,
// but `(1 is int) == true`, `(1 is float) == false`, so `int` is **not** a subtype of `float`.
// Also, `TYPE_NIL` is **not** a subtype of `Object` (`null is Object == false`).
Trilean GDScriptParser::DataType::is_supertype_of(const DataType &p_type) const {
	ERR_FAIL_COND_V_MSG(!p_type.is_resolved(), (kind == VARIANT) ? Trilean::TRUE : Trilean::UNKNOWN,
			"GDScript bug (please report): Trying to check type inclusion for unresolved type.");

	if (p_type.kind == UNKNOWN) {
		return (kind == VARIANT) ? Trilean::TRUE : Trilean::UNKNOWN;
	}

	// TODO: Check if p_type is Variant?
	// From here `p_type` is resolved and known.

	switch (kind) {
		case UNRESOLVED:
		case RESOLVING:
			ERR_PRINT("GDScript bug (please report): Trying to check type inclusion for unresolved type.");
			return Trilean::UNKNOWN;
		case UNKNOWN:
			return Trilean::UNKNOWN;
		case VARIANT:
			return Trilean::TRUE; // `Variant` is a supertype of any type.
		case BUILTIN:
			if (builtin_type == Variant::INT && p_type.kind == ENUM) {
				return Trilean::TRUE; // All enums are considered `int`.
			}
			if (builtin_type == Variant::ARRAY && p_type.kind == BUILTIN && p_type.builtin_type == Variant::ARRAY) {
				if (is_typed_container_type()) {
					if (p_type.is_typed_container_type()) {
						// The element types must be equal. For example, `Array[Object]` is **not** a supertype of `Array[Node]`.
						return container_element_type->is_equal_to(p_type->container_element_type);
					} else {
						return Trilean::FALSE; // A typed array cannot be a supertype of untyped array.
					}
				} else {
					// `Array` (aka `Array[Variant]`) **is** a supertype of any `Array` type (including typed arrays).
					// This is unsafe, but the exception was intentionally added for the convenience of users.
					return Trilean::TRUE;
				}
			}
			return BOOLEAN_TO_TRILEAN(p_type.kind == BUILTIN && p_type.builtin_type == builtin_type);
		case NATIVE:
			// A script/class type cannot be a supertype of a native type, so skip script/class inheritance.
			// native_type ⊇ p_type.native_type [⊃ p_type.script_type / p_type.class_type].
			if (p_type.kind == NATIVE || p_type.kind == SCRIPT || p_type.kind == CLASS) {
				ERR_FAIL_COND_V(native_type == StringName() || p_type.native_type == StringName(), Trilean::UNKNOWN);
				return BOOLEAN_TO_TRILEAN(ClassDB::is_parent_class(p_type.native_type, native_type));
				// TODO: Old code uses get_instance_base_type() for scripts. Check if it is now correct.
			}
			return Trilean::FALSE;
		case SCRIPT:
		case CLASS: {
			ERR_FAIL_COND_V(kind == SCRIPT && script_type.is_null(), Trilean::UNKNOWN);
			ERR_FAIL_COND_V(kind == CLASS && class_type == nullptr, Trilean::UNKNOWN);

			Ref<Script> sub_script;
			const ClassNode *sub_class = nullptr;

			if (p_type.kind == SCRIPT) {
				if (kind == CLASS) {
					return Trilean::FALSE; // A class type cannot be a supertype of a script type.
				}
				sub_script = p_type.script_type;
			} else if (p_type.kind == CLASS) {
				sub_class = p_type.class_type;
			} else {
				return Trilean::FALSE; // A script/class type can only be a supertype of a script/class type.
			}

			// Check if we can come from `p_type` to this type in the inheritance hierarchy.

			while (sub_class != nullptr) {
				if (kind == CLASS && (sub_class == class_type || sub_class->fqcn == class_type->fqcn)) {
					return Trilean::TRUE;
				} else if (sub_class->base_type.kind == SCRIPT) {
					sub_script = sub_class->base_type.script_type;
					break;
				} else if (sub_class->base_type.kind == CLASS) {
					sub_class = sub_class->base_type.class_type;
				} else {
					return Trilean::FALSE;
				}
			}

			while (sub_script.is_valid()) {
				if (sub_script == script_type) {
					return Trilean::TRUE;
				}
				sub_script = sub_script->get_base_script();
			}

			return Trilean::FALSE;
		} break;
		case ENUM:
			// All enums are considered `int`, and therefore equal to it and each other.
			// We have warnings, but they do not provide strict guarantees.
			return BOOLEAN_TO_TRILEAN(p_type.kind == ENUM || (p_type.kind == BUILTIN && p_type.builtin_type == Variant::INT));
	}

	ERR_FAIL_V_MSG(Trilean::UNKNOWN, "Kind set outside the enum range.");
}

// TODO: Warnings like it was in the analyzer?
// TODO: Add comment with short example.
Trilean GDScriptParser::DataType::is_compatible_with(const DataType &p_type, bool p_allow_implicit_conversion) const {
	ERR_FAIL_COND_V_MSG(!p_type.is_resolved(), (kind == VARIANT) ? Trilean::TRUE : Trilean::UNKNOWN,
			"GDScript bug (please report): Trying to check type compatibility for unresolved source type.");

	if (p_type.kind == UNKNOWN || p_type.kind == VARIANT) {
		return (kind == VARIANT) ? Trilean::TRUE : Trilean::UNKNOWN;
	}

	// From here `p_type` is resolved, known, and not variant.

	switch (kind) {
		case UNRESOLVED:
		case RESOLVING:
			ERR_PRINT("GDScript bug (please report): Trying to check type compatibility for unresolved target type.");
			return Trilean::UNKNOWN;
		case UNKNOWN:
			return Trilean::UNKNOWN;
		case VARIANT:
			return Trilean::TRUE; // Variant can receive anything.
		case BUILTIN:
			if (builtin_type == Variant::INT && p_type.kind == ENUM) {
				return Trilean::TRUE; // Enum value is `int`.
			}
			if (builtin_type == Variant::ARRAY && p_type.kind == BUILTIN && p_type.builtin_type == Variant::ARRAY) {
				if (is_typed_container_type() && p_type.is_typed_container_type()) {
					return container_element_type->is_equal_to(p_type->container_element_type);
				} else {
					return BOOLEAN_TO_TRILEAN(!is_typed_container_type() && !p_type.is_typed_container_type());
				}
			}
			if (p_allow_implicit_conversion) {
				return BOOLEAN_TO_TRILEAN(Variant::can_convert_strict(p_type.builtin_type, builtin_type));
			}
			return BOOLEAN_TO_TRILEAN(p_type.kind == BUILTIN && p_type.builtin_type == builtin_type);
		case NATIVE:
		case SCRIPT:
		case CLASS: {
			if (p_type.kind == BUILTIN && p_type.builtin_type == Variant::NIL) {
				return Trilean::TRUE; // `null` is acceptable for objects.
			} else if (p_type.kind != NATIVE && p_type.kind != SCRIPT && p_type.kind != CLASS) {
				return Trilean::FALSE;
			}

			switch (is_supertype_of(p_type)) {
				case Trilean::FALSE:
					if (is_subtype_of(p_type) == Trilean::FALSE) {
						return Trilean::FALSE;
					} else {
						return Trilean::UNKNOWN;
					}
				case Trilean::UNKNOWN:
					return Trilean::UNKNOWN;
				case Trilean::TRUE:
					return Trilean::TRUE;
			}

			return Trilean::UNKNOWN;
		} break;
		case ENUM:
			// All enums are considered `int`, and therefore equal to it and each other.
			// We have warnings, but they do not provide strict guarantees.
			if (p_allow_implicit_conversion) {
				return BOOLEAN_TO_TRILEAN(Variant::can_convert_strict(p_type.builtin_type, Variant::INT));
			}
			return BOOLEAN_TO_TRILEAN(p_type.kind == BUILTIN && p_type.builtin_type == Variant::INT);
	}

	ERR_FAIL_V_MSG(Trilean::UNKNOWN, "Kind set outside the enum range.");
}

static Variant::Type _variant_type_to_packed_array_element_type(Variant::Type p_type) {
	switch (p_type) {
		case Variant::PACKED_BYTE_ARRAY:
		case Variant::PACKED_INT32_ARRAY:
		case Variant::PACKED_INT64_ARRAY:
			return Variant::INT;
		case Variant::PACKED_FLOAT32_ARRAY:
		case Variant::PACKED_FLOAT64_ARRAY:
			return Variant::FLOAT;
		case Variant::PACKED_STRING_ARRAY:
			return Variant::STRING;
		case Variant::PACKED_VECTOR2_ARRAY:
			return Variant::VECTOR2;
		case Variant::PACKED_VECTOR3_ARRAY:
			return Variant::VECTOR3;
		case Variant::PACKED_COLOR_ARRAY:
			return Variant::COLOR;
		default:
			return Variant::NIL;
	}
}

bool GDScriptParser::DataType::is_packed_array_type() const {
	return kind == BUILTIN && _variant_type_to_packed_array_element_type(builtin_type) != Variant::NIL;
}

GDScriptParser::DataType GDScriptParser::DataType::get_packed_array_element_type() const {
	GDScriptParser::DataType type;
	type.kind = BUILTIN;
	type.builtin_type = _variant_type_to_packed_array_element_type(builtin_type);
	return type;
}
