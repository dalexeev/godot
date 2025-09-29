/**************************************************************************/
/*  data_type.cpp                                                         */
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

#include "data_type.h"

#include "common_defs.h"

#include "core/object/class_db.h"
#include "core/variant/container_type_validate.h"

using namespace gdscript;

LocalVector<DataType> DataType::cache_builtin_types;
HashMap<StringName, DataType> DataType::cache_native_classes;

void DataType::initialize() {
	cache_builtin_types.reserve(Variant::VARIANT_MAX);
	for (int i = Variant::NIL; i < Variant::VARIANT_MAX; i++) {
		if (i == Variant::OBJECT) {
			cache_builtin_types.push_back(make_unknown());
		} else {
			const Variant::Type builtin_type = (Variant::Type)i;

			DataType type;
			type.kind = Kind::BUILTIN;
			type.builtin_type = builtin_type;
			if (builtin_type == Variant::ARRAY) {
				type.params.push_back(make_variant());
			} else if (builtin_type == Variant::DICTIONARY) {
				type.params.push_back(make_variant());
				type.params.push_back(make_variant());
			}
			type.fqtn = Variant::get_type_name(builtin_type);

			cache_builtin_types.push_back(type);
		}
	}
}

void DataType::deinitialize() {
	cache_builtin_types.clear();
	cache_native_classes.clear();
}

Vector<DataType> DataType::_canonicalize_type_list_for_union(const Vector<DataType> &p_types) {
	Vector<DataType> type_list;

	bool has_unknown = false;
	bool has_variant = false;
	for (const DataType &type : p_types) {
		switch (type.kind) {
			case Kind::UNKNOWN:
				has_unknown = true;
				break;
			case Kind::VARIANT:
				has_variant = true;
				break;
			case Kind::BUILTIN:
			case Kind::ENUM:
			case Kind::BITFIELD:
			case Kind::CLASS:
				type_list.push_back(type);
				break;
			case Kind::UNION:
				type_list.append_array(type.params);
				break;
			case Kind::META:
				ERR_FAIL_V_MSG({}, "GDScript bug: Unhandled metatype.");
		}
	}

	if (has_variant) {
		return { make_variant() };
	}
	if (has_unknown) {
		return { make_unknown() };
	}

	{
		int64_t i = 0;
		while (i < type_list.size() - 1) {
			const DataType &first = type_list[i];
			int64_t j = i + 1;
			while (j < type_list.size()) {
				const DataType &second = type_list[j];
				if (first.fqtn == second.fqtn || first.is_supertype_of(second) == Trilean::TRUE) {
					type_list.remove_at(j);
					continue;
				}
				if (second.is_supertype_of(first) == Trilean::TRUE) {
					type_list.remove_at(i);
					i--;
					break;
				}
				j++;
			}
			i++;
		}
	}

	struct DataTypeSort {
		_FORCE_INLINE_ bool operator()(const DataType &p_a, const DataType &p_b) const {
			return p_a.get_fqtn().operator String() < p_b.get_fqtn().operator String();
		}
	};

	type_list.sort_custom<DataTypeSort>();

	return type_list;
}

DataType DataType::make_variant() {
	DataType type;
	type.kind = Kind::VARIANT;
	type.fqtn = SNAME("Variant");

	return type;
}

DataType DataType::make_builtin(Variant::Type p_builtin_type) {
	ERR_FAIL_INDEX_V(p_builtin_type, Variant::VARIANT_MAX, make_unknown());
	ERR_FAIL_COND_V(p_builtin_type == Variant::OBJECT, make_native_class("Object"));

	return cache_builtin_types[p_builtin_type];
}

DataType DataType::make_array(const DataType &p_element_type) {
	ERR_FAIL_COND_V(!p_element_type.is_variant() && !p_element_type.is_concrete(), make_builtin(Variant::ARRAY));

	DataType type;
	type.kind = Kind::BUILTIN;
	type.builtin_type = Variant::ARRAY;
	type.params.push_back(p_element_type);
	type.fqtn = vformat("Array[%s]", p_element_type.fqtn);

	return type;
}

DataType DataType::make_dictionary(const DataType &p_key_type, const DataType &p_value_type) {
	ERR_FAIL_COND_V(!p_key_type.is_variant() && !p_key_type.is_concrete(), make_builtin(Variant::DICTIONARY));
	ERR_FAIL_COND_V(!p_value_type.is_variant() && !p_value_type.is_concrete(), make_builtin(Variant::DICTIONARY));

	DataType type;
	type.kind = Kind::BUILTIN;
	type.builtin_type = Variant::DICTIONARY;
	type.params.push_back(p_key_type);
	type.params.push_back(p_value_type);
	type.fqtn = vformat("Dictionary[%s, %s]", p_key_type.fqtn, p_value_type.fqtn);

	return type;
}

DataType DataType::make_enum(const StringName &p_fqtn, const DataType &p_underlying_type) {
	ERR_FAIL_COND_V(p_fqtn.is_empty(), make_unknown());
	ERR_FAIL_COND_V(!p_underlying_type.is_variant() && !p_underlying_type.is_concrete(), make_unknown());
	ERR_FAIL_COND_V(p_underlying_type.is_class(), make_unknown());

	DataType type;
	type.kind = Kind::ENUM;
	type.builtin_type = p_underlying_type.builtin_type;
	type.parents.push_back(p_underlying_type);
	type.fqtn = p_fqtn;

	return type;
}

DataType DataType::make_bitfield(const DataType &p_enum_type) {
	ERR_FAIL_COND_V(!p_enum_type.is_enum(), make_builtin(Variant::INT));
	ERR_FAIL_COND_V(p_enum_type.builtin_type != Variant::INT, make_builtin(Variant::INT));

	DataType type;
	type.kind = Kind::BITFIELD;
	type.builtin_type = Variant::INT;
	type.params.push_back(p_enum_type);
	type.fqtn = vformat("BitField[%s]", p_enum_type.fqtn);

	return type;
}

// TODO: Check is native?
DataType DataType::make_native_class(const StringName &p_fqtn) {
	if (cache_native_classes.has(p_fqtn)) {
		return cache_native_classes[p_fqtn];
	}

	ERR_FAIL_COND_V(p_fqtn.is_empty(), make_native_class("Object"));

	DataType base_type;
	if (p_fqtn == SNAME("Object")) {
		base_type = make_variant();
	} else {
		base_type = make_native_class(ClassDB::get_parent_class(p_fqtn));
	}

	DataType type;
	type.kind = Kind::CLASS;
	type.builtin_type = Variant::OBJECT;
	type.parents.push_back(base_type);
	type.fqtn = p_fqtn;

	cache_native_classes[p_fqtn] = type;

	return type;
}

// TODO: Check is not native? (Or check is custom?)
DataType DataType::make_custom_class(const StringName &p_fqtn, const DataType &p_base_type) {
	ERR_FAIL_COND_V(p_fqtn.is_empty(), make_native_class("Object"));
	ERR_FAIL_COND_V(!p_base_type.is_class(), make_native_class("Object"));

	// TODO: Check for recursion?

	DataType type;
	type.kind = Kind::CLASS;
	type.builtin_type = Variant::OBJECT;
	type.parents.push_back(p_base_type);
	type.fqtn = p_fqtn;

	return type;
}

DataType DataType::make_union(const Vector<DataType> &p_types) {
	ERR_FAIL_COND_V(p_types.is_empty(), make_unknown());

	if (p_types.size() == 1) {
		return p_types[0]; // Safe. Even if it's a union, it's already canonicalized.
	}

	const Vector<DataType> params = _canonicalize_type_list_for_union(p_types);

	if (params.is_empty()) {
		return make_unknown(); // The error has already been printed.
	}

	if (params.size() == 1) {
		return params[0];
	}

	StringName fqtn;
	{
		Vector<String> type_names;
		for (const DataType &param : params) {
			type_names.push_back(param.fqtn);
		}
		// Already sorted.
		fqtn = String("|").join(type_names);
	}

	static StringName variant_as_union_fqtn;
	if (unlikely(variant_as_union_fqtn.is_empty())) {
		Vector<String> type_names;
		for (int i = Variant::NIL; i < Variant::VARIANT_MAX; i++) {
			type_names.push_back(Variant::get_type_name((Variant::Type)i));
		}
		type_names.sort();
		variant_as_union_fqtn = StringName(String("|").join(type_names), true);
	}

	if (unlikely(fqtn == variant_as_union_fqtn)) {
		return make_variant();
	}

	DataType type;
	type.kind = Kind::UNION;
	type.params = params;
	type.fqtn = fqtn;

	return type;
}

DataType DataType::make_meta(const DataType &p_type) {
	DataType type;
	type.kind = Kind::META;
	type.params.push_back(p_type);
	type.fqtn = vformat("<type of %s>", p_type.fqtn);

	return type;
}

DataType DataType::make_marked_void() {
	DataType type = make_builtin(Variant::NIL);
	type.mark = Mark::VOID;

	return type;
}

DataType DataType::make_marked_untyped() {
	DataType type = make_variant();
	type.mark = Mark::UNTYPED;

	return type;
}

DataType DataType::from_class_name(const String &p_fqtn) {
	return make_native_class("Object"); // TODO: Native and custom classes.
}

DataType DataType::from_container_type(const ContainerType &p_container_type) {
	DataType result;

	if (p_container_type.builtin_type != Variant::NIL) {
		if (p_container_type.script.is_valid()) {
			ERR_PRINT("Scripts are not supported in GDScript 3.");
			result = make_native_class("Object");
		} else if (!p_container_type.class_name.is_empty()) {
			result = from_class_name(p_container_type.class_name);
		} else {
			result = make_builtin(p_container_type.builtin_type);
		}
	} else {
		result = make_variant();
	}

	return result;
}

DataType DataType::from_constant_value(const Variant &p_value) {
	switch (p_value.get_type()) {
		case Variant::ARRAY: {
			const Array array = p_value;
			return make_array(from_container_type(array.get_element_type()));
		} break;
		case Variant::DICTIONARY: {
			const Dictionary dictionary = p_value;
			return make_dictionary(
					from_container_type(dictionary.get_key_type()),
					from_container_type(dictionary.get_value_type()));
		} break;
		default: {
			return make_builtin(p_value.get_type());
		} break;
	}
}

DataType DataType::from_property_info_hint_string(const String &p_type_name) {
	const Variant::Type builtin_type = Variant::get_type_by_name(p_type_name);
	if (builtin_type < Variant::VARIANT_MAX) {
		return make_builtin(builtin_type);
	}

	const StringName type_name = p_type_name;
	if (type_name == SNAME("Variant")) {
		return make_variant();
	}
	// TODO: Classes, enums, bitfields.

	ERR_FAIL_V_MSG(make_unknown(), vformat(R"(Could not find type "%s".)", type_name));
}

DataType DataType::from_property_info(const PropertyInfo &p_info, bool p_is_return) {
	switch (p_info.type) {
		case Variant::NIL:
			if (p_info.usage & PROPERTY_USAGE_NIL_IS_VARIANT) {
				return make_variant();
			}
			if (p_is_return) {
				return make_marked_void();
			}
			break;
		case Variant::INT:
			if (p_info.usage & PROPERTY_USAGE_CLASS_IS_ENUM) {
				return make_enum(p_info.class_name, make_builtin(Variant::INT));
			}
			if (p_info.usage & PROPERTY_USAGE_CLASS_IS_BITFIELD) {
				return make_bitfield(make_enum(p_info.class_name, make_builtin(Variant::INT)));
			}
			break;
		case Variant::ARRAY:
			if (p_info.hint == PROPERTY_HINT_ARRAY_TYPE) {
				return make_array(from_property_info_hint_string(p_info.hint_string));
			}
			break;
		case Variant::DICTIONARY:
			if (p_info.hint == PROPERTY_HINT_DICTIONARY_TYPE) {
				return make_dictionary(
						from_property_info_hint_string(p_info.hint_string.get_slicec(';', 0)),
						from_property_info_hint_string(p_info.hint_string.get_slicec(';', 1)));
			}
			break;
		case Variant::OBJECT:
			return from_class_name(p_info.class_name);
		default:
			break;
	}

	return make_builtin(p_info.type);
}

DataType DataType::find_narrowest_common_type(const Vector<DataType> &p_types) {
	ERR_FAIL_COND_V_MSG(p_types.is_empty(), make_unknown(), "GDScript bug: Empty type list.");

	const DataType variant_type = make_variant();

	LocalVector<const DataType *> common_hierarchy;
	List<DataType> union_narrowest_common_types;
	bool is_first_type = true;

	for (const DataType &type : p_types) {
		LocalVector<const DataType *> current_hierarchy;
		{
			const DataType *current = &type;
			bool variant_reached = false;
			while (!variant_reached) {
				current_hierarchy.push_back(current);
				switch (current->kind) {
					case Kind::UNKNOWN:
						current_hierarchy[current_hierarchy.size() - 1] = &variant_type;
						[[fallthrough]];
					case Kind::VARIANT:
						variant_reached = true;
						break;
					case Kind::BUILTIN:
						if (current->is_typed_array()) {
							current = &cache_builtin_types[Variant::ARRAY];
						} else if (current->is_typed_dictionary()) {
							current = &cache_builtin_types[Variant::DICTIONARY];
						} else {
							current = &variant_type;
						}
						break;
					case Kind::ENUM:
						current = &current->get_enum_underlying_type();
						break;
					case Kind::BITFIELD:
						current = &cache_builtin_types[Variant::INT];
						break;
					case Kind::CLASS:
						current = &current->get_class_base_type();
						break;
					case Kind::UNION:
						union_narrowest_common_types.push_back(current->erase_union());
						current = &union_narrowest_common_types.back()->get();
						break;
					case Kind::META:
						ERR_FAIL_V_MSG(make_unknown(), "GDScript bug: Unhandled metatype.");
				}
			}
			current_hierarchy.reverse();
		}

		if (is_first_type) {
			common_hierarchy = current_hierarchy;
		} else {
			const DataType *common_type = common_hierarchy[common_hierarchy.size() - 1];

			if (type.is_class() && common_type->is_builtin(Variant::NIL)) {
				common_hierarchy = current_hierarchy;
				continue;
			}
			if (common_type->is_class() && type.is_builtin(Variant::NIL)) {
				continue;
			}

			// TODO: Bitfields?

			uint32_t max_common_index = 0;
			for (uint32_t i = 1; i < MIN(common_hierarchy.size(), current_hierarchy.size()); i++) {
				if (common_hierarchy[i]->fqtn == current_hierarchy[i]->fqtn) {
					max_common_index = i;
				} else {
					break;
				}
			}

			common_hierarchy.resize(max_common_index + 1);
		}
	}

	return *(common_hierarchy[common_hierarchy.size() - 1]);
}

Trilean DataType::get_binary_operator_type(
		DataType &r_result_type,
		Variant::Operator p_operator,
		const DataType &p_left_operand_type,
		const DataType &p_right_operand_type) {
	ERR_FAIL_COND_V_MSG(
			p_left_operand_type.is_meta() || p_right_operand_type.is_meta(),
			Trilean::FALSE, // Operations on types are not allowed, not even comparisons.
			"GDScript bug: Unhandled metatype.");

	switch (p_operator) {
		case Variant::OP_EQUAL:
		case Variant::OP_NOT_EQUAL:
			if (p_left_operand_type.is_builtin(Variant::NIL) || p_right_operand_type.is_builtin(Variant::NIL)) {
				// `==` and `!=` operators **always** return a boolean when comparing to `null`.
				r_result_type = make_builtin(Variant::BOOL);
				return Trilean::TRUE;
			}
			break;
		case Variant::OP_MODULE:
			if (p_left_operand_type.is_builtin(Variant::STRING)) {
				// The `%` operator on string acts as formatting and will **always** return a string.
				r_result_type = make_builtin(Variant::STRING);
				return Trilean::TRUE;
			}
			break;
		default:
			break; // Nothing to do.
	}

	if (p_left_operand_type.is_unknown() || p_right_operand_type.is_unknown()) {
		return Trilean::UNKNOWN;
	}
	if (p_left_operand_type.is_variant() || p_right_operand_type.is_variant()) {
		return Trilean::UNKNOWN;
	}

	if (p_left_operand_type.is_union() || p_right_operand_type.is_union()) {
		const Vector<DataType> left_types = p_left_operand_type.unpack_union();
		const Vector<DataType> right_types = p_right_operand_type.unpack_union();

		Vector<DataType> subresult_types;
		Vector<Trilean> subresult_valid;
		for (const DataType &left_type : left_types) {
			for (const DataType &right_type : right_types) {
				DataType result_type;
				const Trilean valid = get_binary_operator_type(result_type, p_operator, left_type, right_type);
				subresult_types.push_back(result_type);
				subresult_valid.push_back(valid);
			}
		}

		r_result_type = make_union(subresult_types);
		return trilean_consensus(subresult_valid);
	}

	const DataType &left_underlying_type = p_left_operand_type.erase_enum_bitfield();
	const DataType &right_underlying_type = p_right_operand_type.erase_enum_bitfield();

	const Variant::Type left_type = left_underlying_type.builtin_type;
	const Variant::Type right_type = right_underlying_type.builtin_type;

	switch (p_operator) {
		case Variant::OP_ADD:
			if (left_type == Variant::ARRAY && right_type == Variant::ARRAY) {
				if (left_underlying_type.is_equal_to(right_underlying_type) == Trilean::TRUE) {
					// Concatenation of the same typed arrays preserves the type.
					r_result_type = left_underlying_type;
					return Trilean::TRUE;
				}
			}
			break;
		case Variant::OP_BIT_AND:
			if (left_type == Variant::INT && right_type == Variant::INT) {
				const DataType &left = p_left_operand_type.is_bitfield()
						? p_left_operand_type.get_bitfield_enum_type()
						: p_left_operand_type;
				const DataType &right = p_right_operand_type.is_bitfield()
						? p_right_operand_type.get_bitfield_enum_type()
						: p_right_operand_type;

				DataType result;
				if (left.is_enum() && right.is_enum()) {
					if (left.is_supertype_of(right) == Trilean::TRUE) {
						result = right;
					} else if (right.is_supertype_of(left) == Trilean::TRUE) {
						result = left;
					} // else: We don't have any intersection types yet.
				} else if (left.is_builtin(Variant::INT) && right.is_enum()) {
					result = right;
				} else if (left.is_enum() && right.is_builtin(Variant::INT)) {
					result = left;
				}

				if (result.is_enum() && result.builtin_type == Variant::INT) {
					r_result_type = make_bitfield(result);
					return Trilean::TRUE;
				}
			}
			break;
		case Variant::OP_BIT_OR:
		case Variant::OP_BIT_XOR:
			if (left_type == Variant::INT && right_type == Variant::INT) {
				const DataType &left = p_left_operand_type.is_bitfield()
						? p_left_operand_type.get_bitfield_enum_type()
						: p_left_operand_type;
				const DataType &right = p_right_operand_type.is_bitfield()
						? p_right_operand_type.get_bitfield_enum_type()
						: p_right_operand_type;

				if (left.is_enum() && right.is_enum()) {
					const DataType result = find_narrowest_common_type({ left, right });
					if (result.is_enum() && result.builtin_type == Variant::INT) {
						r_result_type = make_bitfield(result);
						return Trilean::TRUE;
					}
				}
			}
			break;
		default:
			break; // Nothing to do.
	}

	using Evaluator = Variant::ValidatedOperatorEvaluator;
	const Evaluator evaluator = Variant::get_validated_operator_evaluator(p_operator, left_type, right_type);
	if (evaluator == nullptr) {
		return Trilean::FALSE;
	}

	r_result_type = make_builtin(Variant::get_operator_return_type(p_operator, left_type, right_type));
	return Trilean::TRUE;
}

bool DataType::is_packed_array() const {
	if (!is_builtin()) {
		return false;
	}

	switch (builtin_type) {
		case Variant::PACKED_BYTE_ARRAY:
		case Variant::PACKED_INT32_ARRAY:
		case Variant::PACKED_INT64_ARRAY:
		case Variant::PACKED_FLOAT32_ARRAY:
		case Variant::PACKED_FLOAT64_ARRAY:
		case Variant::PACKED_STRING_ARRAY:
		case Variant::PACKED_VECTOR2_ARRAY:
		case Variant::PACKED_VECTOR3_ARRAY:
		case Variant::PACKED_COLOR_ARRAY:
		case Variant::PACKED_VECTOR4_ARRAY:
			return true;
		default:
			return false;
	}
}

DataType DataType::get_packed_array_element_type() const {
	ERR_FAIL_COND_V(!is_builtin(), make_unknown());

	switch (builtin_type) {
		case Variant::PACKED_BYTE_ARRAY:
		case Variant::PACKED_INT32_ARRAY:
		case Variant::PACKED_INT64_ARRAY:
			return make_builtin(Variant::INT);
		case Variant::PACKED_FLOAT32_ARRAY:
		case Variant::PACKED_FLOAT64_ARRAY:
			return make_builtin(Variant::FLOAT);
		case Variant::PACKED_STRING_ARRAY:
			return make_builtin(Variant::STRING);
		case Variant::PACKED_VECTOR2_ARRAY:
			return make_builtin(Variant::VECTOR2);
		case Variant::PACKED_VECTOR3_ARRAY:
			return make_builtin(Variant::VECTOR3);
		case Variant::PACKED_COLOR_ARRAY:
			return make_builtin(Variant::COLOR);
		case Variant::PACKED_VECTOR4_ARRAY:
			return make_builtin(Variant::VECTOR4);
		default:
			ERR_FAIL_V(make_unknown());
	}
}

const DataType &DataType::erase_enum_bitfield() const {
	if (is_enum()) {
		return get_enum_underlying_type().erase_enum_bitfield();
	}
	if (is_bitfield()) {
		return get_bitfield_enum_type().erase_enum_bitfield();
	}
	return *this;
}

String DataType::to_string(bool p_is_return) const {
	if (p_is_return && is_marked_void()) {
		return "void";
	}
	return fqtn;
}

String DataType::to_debug_string() const {
	switch (mark) {
		case Mark::NONE:
			break; // Nothing to do.
		case Mark::VOID:
			return "void";
		case Mark::UNTYPED:
			return "<untyped>";
	}

	return fqtn;
}

String DataType::to_property_info_hint_string() const {
	switch (kind) {
		case Kind::UNKNOWN:
		case Kind::VARIANT:
			break;
		case Kind::BUILTIN:
			return Variant::get_type_name(builtin_type);
		case Kind::ENUM: // TODO: Not supported? TODO: underlying type?
		case Kind::BITFIELD: // TODO: Not supported?
		case Kind::CLASS:
			return fqtn;
		case Kind::UNION:
			return erase_union().to_property_info_hint_string();
		case Kind::META:
			ERR_PRINT("GDScript bug: Unhandled metatype.");
			break;
	}

	return "Variant";
}

PropertyInfo DataType::to_property_info(const String &p_name) const {
	PropertyInfo result;
	result.name = p_name;
	result.usage = PROPERTY_USAGE_NONE;

	switch (kind) {
		case Kind::UNKNOWN:
		case Kind::VARIANT:
			result.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
		case Kind::BUILTIN:
			result.type = builtin_type;
			if (builtin_type == Variant::ARRAY) {
				const DataType &elem_type = get_array_element_type();
				if (!elem_type.is_variant()) {
					result.hint = PROPERTY_HINT_ARRAY_TYPE;
					result.hint_string = elem_type.to_property_info_hint_string();
				}
			} else if (builtin_type == Variant::DICTIONARY) {
				const DataType &key_type = get_dictionary_key_type();
				const DataType &value_type = get_dictionary_value_type();
				if (!key_type.is_variant() || !value_type.is_variant()) {
					result.hint = PROPERTY_HINT_DICTIONARY_TYPE;
					result.hint_string = key_type.to_property_info_hint_string() + ";" +
							value_type.to_property_info_hint_string();
				}
			}
			break;
		case Kind::ENUM:
			result.type = builtin_type;
			if (builtin_type == Variant::INT) {
				result.usage |= PROPERTY_USAGE_CLASS_IS_ENUM;
				result.class_name = fqtn;
			}
			break;
		case Kind::BITFIELD:
			result.type = Variant::INT;
			result.usage |= PROPERTY_USAGE_CLASS_IS_BITFIELD;
			result.class_name = fqtn;
			break;
		case Kind::CLASS:
			result.type = Variant::OBJECT;
			result.class_name = fqtn;
			break;
		case Kind::UNION:
			return erase_union().to_property_info(p_name);
		case Kind::META:
			ERR_PRINT("GDScript bug: Unhandled metatype.");
			result.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
			break;
	}

	return result;
}

void DataType::adjust_constant_value(Variant &r_value) const {
	if (r_value.get_type() == Variant::ARRAY) {
		Array array = r_value;
		array.make_read_only();
	} else if (r_value.get_type() == Variant::DICTIONARY) {
		Dictionary dictionary = r_value;
		dictionary.make_read_only();
	}

	ERR_FAIL_COND_MSG(is_unknown(), "GDScript bug: Trying to adjust constant value to unknown type.");
	ERR_FAIL_COND_MSG(is_union(), "GDScript bug: Trying to adjust constant value to a union type.");
	ERR_FAIL_COND_MSG(is_meta(), "GDScript bug: Unhandled metatype.");

	const DataType &target_type = erase_enum_bitfield();

	if (target_type.is_variant()) {
		return; // `Variant` can accept anything.
	}

	const DataType original_type = from_constant_value(r_value);
	if (target_type.is_equal_to(original_type) == Trilean::TRUE) {
		return; // The value is already of the target type.
	}
	if (target_type.can_accept(original_type) != Trilean::TRUE) {
		return; // No need to adjust the value. The error should be handled outside of this method.
	}

	Variant result;
	const Variant *from = &r_value;
	Callable::CallError err;
	Variant::construct(target_type.builtin_type, result, &from, 1, err);

	ERR_FAIL_COND_MSG(err.error != Callable::CallError::CALL_OK, "GDScript bug: Failed to adjust constant value.");

	if (target_type.builtin_type == Variant::ARRAY) {
		const DataType &elem_type = target_type.get_array_element_type();

		Array array;
		array.set_typed(
				elem_type.builtin_type,
				elem_type.is_class() ? elem_type.fqtn : StringName(),
				Variant());
		array.assign(result);
		array.make_read_only();

		result = array;
	} else if (target_type.builtin_type == Variant::DICTIONARY) {
		const DataType &key_type = target_type.get_dictionary_key_type();
		const DataType &value_type = target_type.get_dictionary_value_type();

		Dictionary dictionary;
		dictionary.set_typed(
				key_type.builtin_type,
				key_type.is_class() ? key_type.fqtn : StringName(),
				Variant(),
				value_type.builtin_type,
				value_type.is_class() ? value_type.fqtn : StringName(),
				Variant());
		dictionary.assign(result);
		dictionary.make_read_only();

		result = dictionary;
	}

	r_value = result;
}

Trilean DataType::is_equal_to(const DataType &p_other) const {
	ERR_FAIL_COND_V_MSG(is_meta() || p_other.is_meta(), Trilean::UNKNOWN, "GDScript bug: Unhandled metatype.");

	if (is_unknown() || p_other.is_unknown()) {
		return Trilean::UNKNOWN; // We can't say anything for sure.
	}

	return boolean_to_trilean(fqtn == p_other.fqtn);
}

Trilean DataType::is_supertype_of(const DataType &p_other) const {
	ERR_FAIL_COND_V_MSG(is_meta() || p_other.is_meta(), Trilean::UNKNOWN, "GDScript bug: Unhandled metatype.");

	if (is_variant()) {
		return Trilean::TRUE; // `Variant` is a supertype of any type.
	}

	if (is_unknown() || p_other.is_unknown()) {
		return Trilean::UNKNOWN; // We can't say anything for sure.
	}

	if (p_other.is_variant()) {
		return Trilean::FALSE; // No type can be a supertype of `Variant`, except `Variant` itself (checked above).
	}

	if (fqtn == p_other.fqtn) {
		return Trilean::TRUE; // Cheap type equality check.
	}

	if (is_union() || p_other.is_union()) {
		const Vector<DataType> left_types = unpack_union();
		const Vector<DataType> right_types = p_other.unpack_union();

		Trilean result = Trilean::TRUE;
		for (const DataType &right_type : right_types) {
			Trilean subresult = Trilean::FALSE;
			for (const DataType &left_type : left_types) {
				subresult = trilean_or(subresult, left_type.is_supertype_of(right_type));
				if (subresult == Trilean::TRUE) {
					break;
				}
			}

			result = trilean_and(result, subresult);
			if (result == Trilean::FALSE) {
				break;
			}
		}

		return result;
	}

	switch (kind) {
		case Kind::BUILTIN: {
			const DataType &other = p_other.erase_enum_bitfield();
			if (builtin_type == Variant::ARRAY && other.is_builtin(Variant::ARRAY)) {
				const DataType &left_elem = get_array_element_type();
				const DataType &right_elem = other.get_array_element_type();
				if (left_elem.is_variant()) {
					return Trilean::TRUE;
				}
				return left_elem.is_equal_to(right_elem);
			}
			if (builtin_type == Variant::DICTIONARY && other.is_builtin(Variant::DICTIONARY)) {
				const DataType &left_key = get_dictionary_key_type();
				const DataType &left_value = get_dictionary_value_type();
				const DataType &right_key = other.get_dictionary_key_type();
				const DataType &right_value = other.get_dictionary_value_type();
				if (left_key.is_variant() && left_value.is_variant()) {
					return Trilean::TRUE;
				}
				return trilean_and(left_key.is_equal_to(right_key), left_value.is_equal_to(right_value));
			}
			return boolean_to_trilean(other.is_builtin(builtin_type));
		} break;
		case Kind::ENUM: {
			if (p_other.is_enum()) {
				const DataType *current = &p_other;
				while (current->is_enum()) {
					if (current->fqtn == fqtn) {
						return Trilean::TRUE;
					}
					current = &current->get_enum_underlying_type();
				}
			}
			return Trilean::FALSE;
		} break;
		case Kind::BITFIELD: {
			if (p_other.is_enum()) {
				if (p_other.builtin_type == Variant::INT) {
					return get_bitfield_enum_type().is_supertype_of(p_other);
				}
			} else if (p_other.is_bitfield()) {
				return get_bitfield_enum_type().is_supertype_of(p_other.get_bitfield_enum_type());
			}
			return Trilean::FALSE;
		} break;
		case Kind::CLASS: {
			if (p_other.erase_enum_bitfield().is_builtin(Variant::NIL)) {
				return Trilean::TRUE;
			}
			if (p_other.is_class()) {
				const DataType *current = &p_other;
				while (current->is_class()) {
					if (current->fqtn == fqtn) {
						return Trilean::TRUE;
					}
					current = &current->get_class_base_type();
				}
			}
			return Trilean::FALSE;
		} break;
		case Kind::UNKNOWN:
		case Kind::VARIANT:
		case Kind::UNION:
		case Kind::META: {
			DEV_ASSERT(false); // Unreachable.
		} break;
	}

	ERR_FAIL_V(Trilean::UNKNOWN);
}

Trilean DataType::can_accept(const DataType &p_other, bool p_strict_collections) const {
	ERR_FAIL_COND_V_MSG(is_meta() || p_other.is_meta(), Trilean::UNKNOWN, "GDScript bug: Unhandled metatype.");

	if (is_variant()) {
		return Trilean::TRUE; // `Variant` can accept anything.
	}

	if (is_unknown() || p_other.is_unknown()) {
		return Trilean::UNKNOWN; // We can't say anything for sure.
	}

	if (p_other.is_variant()) {
		return Trilean::UNKNOWN; // Allowed, but unsafe (requires a runtime check).
	}

	if (fqtn == p_other.fqtn) {
		return Trilean::TRUE; // Cheap type equality check.
	}

	if (is_union() || p_other.is_union()) {
		const Vector<DataType> left_types = unpack_union();
		const Vector<DataType> right_types = p_other.unpack_union();

		Vector<Trilean> subresults;
		for (const DataType &right_type : right_types) {
			Trilean subresult = Trilean::FALSE;
			for (const DataType &left_type : left_types) {
				subresult = trilean_or(subresult, left_type.can_accept(right_type, p_strict_collections));
				if (subresult == Trilean::TRUE) {
					break;
				}
			}

			subresults.push_back(subresult);
		}

		return trilean_consensus(subresults);
	}

	switch (kind) {
		case Kind::BUILTIN:
		case Kind::ENUM: // TODO?
		case Kind::BITFIELD: { // TODO?
			const DataType &left = erase_enum_bitfield();
			const DataType &right = p_other.erase_enum_bitfield();
			if (right.is_builtin()) {
				switch (left.builtin_type) {
					case Variant::NIL: {
						// `Variant::can_convert_strict()` is incorrect or treats `NIL` as `Variant`.
						return boolean_to_trilean(right.is_builtin(Variant::NIL));
					} break;
					case Variant::ARRAY: {
						if (right.is_builtin(Variant::ARRAY)) {
							const DataType &left_elem = left.get_array_element_type();
							const DataType &right_elem = right.get_array_element_type();
							if (!left_elem.is_variant()) {
								if (right_elem.is_variant()) {
									return p_strict_collections ? Trilean::FALSE : Trilean::UNKNOWN;
								}
								return left_elem.is_equal_to(right_elem);
							}
							return Trilean::TRUE;
						}
						if (right.is_packed_array()) {
							const DataType &left_elem = left.get_array_element_type();
							//const DataType right_elem = right.get_packed_array_element_type();
							//return left_elem.can_accept(right_elem, p_strict_collections);
							return boolean_to_trilean(left_elem.is_variant());
						}
					} break;
					case Variant::DICTIONARY: {
						if (right.is_builtin(Variant::DICTIONARY)) {
							const DataType &left_key = left.get_dictionary_key_type();
							const DataType &left_value = left.get_dictionary_value_type();
							const DataType &right_key = right.get_dictionary_key_type();
							const DataType &right_value = right.get_dictionary_value_type();
							if (!left_key.is_variant() || !left_value.is_variant()) {
								if (right_key.is_variant() && right_value.is_variant()) {
									return p_strict_collections ? Trilean::FALSE : Trilean::UNKNOWN;
								}
								return trilean_and(
										left_key.is_equal_to(right_key),
										left_value.is_equal_to(right_value));
							}
							return Trilean::TRUE;
						}
					} break;
					/*case Variant::PACKED_BYTE_ARRAY:
					case Variant::PACKED_INT32_ARRAY:
					case Variant::PACKED_INT64_ARRAY:
					case Variant::PACKED_FLOAT32_ARRAY:
					case Variant::PACKED_FLOAT64_ARRAY:
					case Variant::PACKED_STRING_ARRAY:
					case Variant::PACKED_VECTOR2_ARRAY:
					case Variant::PACKED_VECTOR3_ARRAY:
					case Variant::PACKED_COLOR_ARRAY:
					case Variant::PACKED_VECTOR4_ARRAY: {
						if (right.is_builtin(Variant::ARRAY)) {
							const DataType left_elem = left.get_packed_array_element_type();
							const DataType &right_elem = right.get_array_element_type();
							return left_elem.can_accept(right_elem, p_strict_collections);
						}
					} break;*/
					default: {
						// Nothing to do.
					} break;
				}
				return boolean_to_trilean(Variant::can_convert_strict(right.builtin_type, left.builtin_type));
			} else if (right.is_class()) {
				if (left.builtin_type == Variant::NIL) {
					return Trilean::UNKNOWN;
				}
				return Trilean::FALSE;
			}
			DEV_ASSERT(false); // Unreachable.
		} break;
		case Kind::CLASS: {
			if (p_other.erase_enum_bitfield().is_builtin(Variant::NIL)) {
				return Trilean::TRUE;
			}
			if (p_other.is_class()) {
				const DataType *current = &p_other;
				while (current->is_class()) {
					if (current->fqtn == fqtn) {
						return Trilean::TRUE;
					}
					current = &current->get_class_base_type();
				}

				current = this;
				while (current->is_class()) {
					if (current->fqtn == p_other.fqtn) {
						return Trilean::UNKNOWN;
					}
					current = &current->get_class_base_type();
				}
			}
			return Trilean::FALSE;
		} break;
		case Kind::UNKNOWN:
		case Kind::VARIANT:
		case Kind::UNION:
		case Kind::META: {
			DEV_ASSERT(false); // Unreachable.
		} break;
	}

	ERR_FAIL_V(Trilean::UNKNOWN);
}
