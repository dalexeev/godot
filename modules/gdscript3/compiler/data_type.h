/**************************************************************************/
/*  data_type.h                                                           */
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

#include "core/variant/variant.h"

namespace gdscript {

enum class Trilean : uint8_t;

class DataType {
	enum class Kind : uint8_t {
		UNKNOWN, // An unknown type.
		VARIANT, // The `Variant` type.
		BUILTIN, // A built-in type (except `Object` and its descendants).
		ENUM, // An enum type (the underlying type at runtime).
		BITFIELD, // A bitfield type (`int` at runtime).
		CLASS, // A class type (`Object` and its descendants).
		UNION, // A union type (only for static analysis).
		META, // A metatype (type of type).
	};

	// The mark is irrelevant to the type system.
	// The analyzer is responsible for handling it correctly.
	enum class Mark : uint8_t {
		NONE, // An explicit return value is required.
		VOID, // An explicit return value is prohibited.
		UNTYPED, // An explicit return value is optional.
	};

	static LocalVector<DataType> cache_builtin_types;
	static HashMap<StringName, DataType> cache_native_classes;

	Kind kind = Kind::UNKNOWN;
	Mark mark = Mark::NONE;
	//bool _enum_is_flags = false; // TODO
	Variant::Type builtin_type = Variant::NIL;
	Vector<DataType> parents;
	Vector<DataType> params;
	StringName fqtn = SNAME("<unknown type>"); // Canonical fully qualified type name.

	static Vector<DataType> _canonicalize_type_list_for_union(const Vector<DataType> &p_types);

public:
	static void initialize();
	static void deinitialize();

	_FORCE_INLINE_ static DataType make_unknown() { return DataType(); }
	static DataType make_variant();
	static DataType make_builtin(Variant::Type p_builtin_type);
	static DataType make_array(const DataType &p_element_type);
	static DataType make_dictionary(const DataType &p_key_type, const DataType &p_value_type);
	static DataType make_enum(const StringName &p_fqtn, const DataType &p_underlying_type);
	static DataType make_bitfield(const DataType &p_enum_type);
	static DataType make_native_class(const StringName &p_fqtn);
	static DataType make_custom_class(const StringName &p_fqtn, const DataType &p_base_type);
	static DataType make_union(const Vector<DataType> &p_types);
	static DataType make_meta(const DataType &p_type);

	static DataType make_marked_void(); // Returns the `Nil` type marked as `void`.
	static DataType make_marked_untyped(); // Returns the `Variant` type marked as untyped.

	static DataType from_class_name(const String &p_fqtn);
	static DataType from_container_type(const ContainerType &p_container_type);
	static DataType from_constant_value(const Variant &p_value);
	static DataType from_property_info_hint_string(const String &p_type_name);
	static DataType from_property_info(const PropertyInfo &p_info, bool p_is_return = false);

	// Return the narrowest common concrete or `Variant` type that is common to `p_types`.
	static DataType find_narrowest_common_type(const Vector<DataType> &p_types);

	_FORCE_INLINE_ static Trilean get_unary_operator_type(
			DataType &r_result_type,
			Variant::Operator p_operator,
			const DataType &p_operand_type) {
		return get_binary_operator_type(r_result_type, p_operator, p_operand_type, make_builtin(Variant::NIL));
	}
	static Trilean get_binary_operator_type(
			DataType &r_result_type,
			Variant::Operator p_operator,
			const DataType &p_left_operand_type,
			const DataType &p_right_operand_type);

	_FORCE_INLINE_ bool is_unknown() const { return kind == Kind::UNKNOWN; }
	_FORCE_INLINE_ bool is_variant() const { return kind == Kind::VARIANT; }

	_FORCE_INLINE_ bool is_builtin() const { return kind == Kind::BUILTIN; }
	_FORCE_INLINE_ bool is_builtin(Variant::Type p_builtin_type) const {
		return kind == Kind::BUILTIN && builtin_type == p_builtin_type;
	}
	bool is_packed_array() const;
	DataType get_packed_array_element_type() const;

	_FORCE_INLINE_ bool is_enum() const { return kind == Kind::ENUM; }
	_FORCE_INLINE_ bool is_bitfield() const { return kind == Kind::BITFIELD; }
	_FORCE_INLINE_ bool is_class() const { return kind == Kind::CLASS; }
	_FORCE_INLINE_ bool is_union() const { return kind == Kind::UNION; }
	_FORCE_INLINE_ bool is_meta() const { return kind == Kind::META; }

	_FORCE_INLINE_ bool is_concrete() const { return Kind::BUILTIN <= kind && kind <= Kind::CLASS; }

	_FORCE_INLINE_ bool is_marked_void() const { return mark == Mark::VOID; }
	_FORCE_INLINE_ bool is_marked_untyped() const { return mark == Mark::UNTYPED; }

	_FORCE_INLINE_ const DataType &get_enum_underlying_type() const {
		DEV_ASSERT(is_enum());
		return parents[0];
	}
	// For `Object`, the base type is `Variant`.
	_FORCE_INLINE_ const DataType &get_class_base_type() const {
		DEV_ASSERT(is_class());
		return parents[0];
	}

	_FORCE_INLINE_ const DataType &get_array_element_type() const {
		DEV_ASSERT(is_builtin(Variant::ARRAY));
		return params[0];
	}
	_FORCE_INLINE_ const DataType &get_dictionary_key_type() const {
		DEV_ASSERT(is_builtin(Variant::DICTIONARY));
		return params[0];
	}
	_FORCE_INLINE_ const DataType &get_dictionary_value_type() const {
		DEV_ASSERT(is_builtin(Variant::DICTIONARY));
		return params[1];
	}
	_FORCE_INLINE_ const DataType &get_bitfield_enum_type() const {
		DEV_ASSERT(is_bitfield());
		return params[0];
	}
	_FORCE_INLINE_ const Vector<DataType> &get_union_component_types() const {
		DEV_ASSERT(is_union());
		return params;
	}
	_FORCE_INLINE_ const DataType &get_meta_wrapped_type() const {
		DEV_ASSERT(is_meta());
		return params[0];
	}

	_FORCE_INLINE_ bool is_untyped_array() const {
		return is_builtin(Variant::ARRAY) && params[0].is_variant();
	}
	_FORCE_INLINE_ bool is_typed_array() const {
		return is_builtin(Variant::ARRAY) && !params[0].is_variant();
	}
	_FORCE_INLINE_ bool is_untyped_dictionary() const {
		return is_builtin(Variant::DICTIONARY) && params[0].is_variant() && params[1].is_variant();
	}
	_FORCE_INLINE_ bool is_typed_dictionary() const {
		return is_builtin(Variant::DICTIONARY) && (!params[0].is_variant() || !params[1].is_variant());
	}

	_FORCE_INLINE_ const StringName &get_fqtn() const { return fqtn; }

	const DataType &erase_enum_bitfield() const;

	_FORCE_INLINE_ DataType erase_union() const {
		return is_union() ? find_narrowest_common_type(params) : *this;
	}
	_FORCE_INLINE_ Vector<DataType> unpack_union() const {
		return is_union() ? params : Vector<DataType>{ *this };
	}

	String to_string(bool p_is_return = false) const;
	String to_debug_string() const;
	String to_property_info_hint_string() const;
	PropertyInfo to_property_info(const String &p_name) const;

	void adjust_constant_value(Variant &r_value) const;

	Trilean is_equal_to(const DataType &p_other) const;
	Trilean is_supertype_of(const DataType &p_other) const;
	Trilean can_accept(const DataType &p_other, bool p_strict_collections = false) const;

	DataType() {}
};

} // namespace gdscript
