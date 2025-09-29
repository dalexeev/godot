/**************************************************************************/
/*  test_data_type.h                                                      */
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

#include "../../compiler/common_defs.h"
#include "../../compiler/data_type.h"

#include "tests/test_macros.h"

#define LIST(m_first, ...) { m_first, __VA_ARGS__ }

namespace gdscript {

TEST_CASE("[Modules][GDScript3][Compiler][DataType] Type relations") {
	const DataType unknown_type = DataType::make_unknown();
	const DataType variant_type = DataType::make_variant();
	const DataType nil_type = DataType::make_builtin(Variant::NIL);
	const DataType int_type = DataType::make_builtin(Variant::INT);
	const DataType float_type = DataType::make_builtin(Variant::FLOAT);
	const DataType string_type = DataType::make_builtin(Variant::STRING);

	const DataType array_type = DataType::make_builtin(Variant::ARRAY);
	const DataType array_int_type = DataType::make_array(int_type);
	const DataType array_float_type = DataType::make_array(float_type);
	const DataType array_string_type = DataType::make_array(string_type);
	const DataType packed_byte_array_type = DataType::make_builtin(Variant::PACKED_BYTE_ARRAY);
	const DataType packed_int32_array_type = DataType::make_builtin(Variant::PACKED_INT32_ARRAY);
	const DataType packed_float32_array_type = DataType::make_builtin(Variant::PACKED_FLOAT32_ARRAY);
	const DataType packed_string_array_type = DataType::make_builtin(Variant::PACKED_STRING_ARRAY);

	const DataType object_type = DataType::make_native_class("Object");
	const DataType node_type = DataType::make_native_class("Node");
	const DataType resource_type = DataType::make_native_class("Resource");

	const DataType union_int_string_type = DataType::make_union({ int_type, string_type });
	const DataType union_int_string_array_type = DataType::make_union({ int_type, string_type, array_type });
	const DataType union_int_array_type = DataType::make_union({ int_type, array_type });
	const DataType union_float_string_type = DataType::make_union({ float_type, string_type });
	const DataType union_array_nil_type = DataType::make_union({ array_type, nil_type });

#define CHECK_RELATIONS(m_a, m_b, m_equality, m_inclusion, m_compatibility) \
	CHECK(m_a.is_equal_to(m_b) == Trilean::m_equality); \
	CHECK(m_a.is_supertype_of(m_b) == Trilean::m_inclusion); \
	CHECK(m_a.can_accept(m_b) == Trilean::m_compatibility)

	// clang-format off

	// --- General ---

	CHECK_RELATIONS( unknown_type, unknown_type, UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( unknown_type, variant_type, UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( unknown_type, nil_type,     UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( unknown_type, int_type,     UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( unknown_type, float_type,   UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( unknown_type, string_type,  UNKNOWN, UNKNOWN, UNKNOWN );

	CHECK_RELATIONS( variant_type, unknown_type, UNKNOWN, TRUE,    TRUE    );
	CHECK_RELATIONS( variant_type, variant_type, TRUE,    TRUE,    TRUE    );
	CHECK_RELATIONS( variant_type, nil_type,     FALSE,   TRUE,    TRUE    );
	CHECK_RELATIONS( variant_type, int_type,     FALSE,   TRUE,    TRUE    );
	CHECK_RELATIONS( variant_type, float_type,   FALSE,   TRUE,    TRUE    );
	CHECK_RELATIONS( variant_type, string_type,  FALSE,   TRUE,    TRUE    );

	CHECK_RELATIONS( nil_type,     unknown_type, UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( nil_type,     variant_type, FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( nil_type,     nil_type,     TRUE,    TRUE,    TRUE    );
	CHECK_RELATIONS( nil_type,     int_type,     FALSE,   FALSE,   FALSE   );
	CHECK_RELATIONS( nil_type,     float_type,   FALSE,   FALSE,   FALSE   );
	CHECK_RELATIONS( nil_type,     string_type,  FALSE,   FALSE,   FALSE   );

	CHECK_RELATIONS( int_type,     unknown_type, UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( int_type,     variant_type, FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( int_type,     nil_type,     FALSE,   FALSE,   FALSE   );
	CHECK_RELATIONS( int_type,     int_type,     TRUE,    TRUE,    TRUE    );
	CHECK_RELATIONS( int_type,     float_type,   FALSE,   FALSE,   TRUE    );
	CHECK_RELATIONS( int_type,     string_type,  FALSE,   FALSE,   FALSE   );

	CHECK_RELATIONS( float_type,   unknown_type, UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( float_type,   variant_type, FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( float_type,   nil_type,     FALSE,   FALSE,   FALSE   );
	CHECK_RELATIONS( float_type,   int_type,     FALSE,   FALSE,   TRUE    );
	CHECK_RELATIONS( float_type,   float_type,   TRUE,    TRUE,    TRUE    );
	CHECK_RELATIONS( float_type,   string_type,  FALSE,   FALSE,   FALSE   );

	CHECK_RELATIONS( string_type,  unknown_type, UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( string_type,  variant_type, FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( string_type,  nil_type,     FALSE,   FALSE,   FALSE   );
	CHECK_RELATIONS( string_type,  int_type,     FALSE,   FALSE,   FALSE   );
	CHECK_RELATIONS( string_type,  float_type,   FALSE,   FALSE,   FALSE   );
	CHECK_RELATIONS( string_type,  string_type,  TRUE,    TRUE,    TRUE    );

	// --- Typed and packed arrays ---

	CHECK_RELATIONS( array_type,                array_type,                TRUE,  TRUE,    TRUE    );
	CHECK_RELATIONS( array_type,                array_int_type,            FALSE, TRUE,    TRUE    );
	CHECK_RELATIONS( array_type,                array_float_type,          FALSE, TRUE,    TRUE    );
	CHECK_RELATIONS( array_type,                array_string_type,         FALSE, TRUE,    TRUE    );
	CHECK_RELATIONS( array_type,                packed_byte_array_type,    FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( array_type,                packed_int32_array_type,   FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( array_type,                packed_float32_array_type, FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( array_type,                packed_string_array_type,  FALSE, FALSE,   TRUE    );

	CHECK_RELATIONS( array_int_type,            array_type,                FALSE, FALSE,   UNKNOWN );
	CHECK_RELATIONS( array_int_type,            array_int_type,            TRUE,  TRUE,    TRUE    );
	CHECK_RELATIONS( array_int_type,            array_float_type,          FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_int_type,            array_string_type,         FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_int_type,            packed_byte_array_type,    FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_int_type,            packed_int32_array_type,   FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_int_type,            packed_float32_array_type, FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_int_type,            packed_string_array_type,  FALSE, FALSE,   FALSE   );

	CHECK_RELATIONS( array_float_type,          array_type,                FALSE, FALSE,   UNKNOWN );
	CHECK_RELATIONS( array_float_type,          array_int_type,            FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_float_type,          array_float_type,          TRUE,  TRUE,    TRUE    );
	CHECK_RELATIONS( array_float_type,          array_string_type,         FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_float_type,          packed_byte_array_type,    FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_float_type,          packed_int32_array_type,   FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_float_type,          packed_float32_array_type, FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_float_type,          packed_string_array_type,  FALSE, FALSE,   FALSE   );

	CHECK_RELATIONS( array_string_type,         array_type,                FALSE, FALSE,   UNKNOWN );
	CHECK_RELATIONS( array_string_type,         array_int_type,            FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_string_type,         array_float_type,          FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_string_type,         array_string_type,         TRUE,  TRUE,    TRUE    );
	CHECK_RELATIONS( array_string_type,         packed_byte_array_type,    FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_string_type,         packed_int32_array_type,   FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_string_type,         packed_float32_array_type, FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( array_string_type,         packed_string_array_type,  FALSE, FALSE,   FALSE   );

	CHECK_RELATIONS( packed_byte_array_type,    array_type,                FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_byte_array_type,    array_int_type,            FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_byte_array_type,    array_float_type,          FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_byte_array_type,    array_string_type,         FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_byte_array_type,    packed_byte_array_type,    TRUE,  TRUE,    TRUE    );
	CHECK_RELATIONS( packed_byte_array_type,    packed_int32_array_type,   FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( packed_byte_array_type,    packed_float32_array_type, FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( packed_byte_array_type,    packed_string_array_type,  FALSE, FALSE,   FALSE   );

	CHECK_RELATIONS( packed_int32_array_type,   array_type,                FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_int32_array_type,   array_int_type,            FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_int32_array_type,   array_float_type,          FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_int32_array_type,   array_string_type,         FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_int32_array_type,   packed_byte_array_type,    FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( packed_int32_array_type,   packed_int32_array_type,   TRUE,  TRUE,    TRUE    );
	CHECK_RELATIONS( packed_int32_array_type,   packed_float32_array_type, FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( packed_int32_array_type,   packed_string_array_type,  FALSE, FALSE,   FALSE   );

	CHECK_RELATIONS( packed_float32_array_type, array_type,                FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_float32_array_type, array_int_type,            FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_float32_array_type, array_float_type,          FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_float32_array_type, array_string_type,         FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_float32_array_type, packed_byte_array_type,    FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( packed_float32_array_type, packed_int32_array_type,   FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( packed_float32_array_type, packed_float32_array_type, TRUE,  TRUE,    TRUE    );
	CHECK_RELATIONS( packed_float32_array_type, packed_string_array_type,  FALSE, FALSE,   FALSE   );

	CHECK_RELATIONS( packed_string_array_type,  array_type,                FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_string_array_type,  array_int_type,            FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_string_array_type,  array_float_type,          FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_string_array_type,  array_string_type,         FALSE, FALSE,   TRUE    );
	CHECK_RELATIONS( packed_string_array_type,  packed_byte_array_type,    FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( packed_string_array_type,  packed_int32_array_type,   FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( packed_string_array_type,  packed_float32_array_type, FALSE, FALSE,   FALSE   );
	CHECK_RELATIONS( packed_string_array_type,  packed_string_array_type,  TRUE,  TRUE,    TRUE    );

	// --- Classes ---

	CHECK_RELATIONS( unknown_type,   object_type,   UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( unknown_type,   node_type,     UNKNOWN, UNKNOWN, UNKNOWN );

	CHECK_RELATIONS( variant_type,   object_type,   FALSE,   TRUE,    TRUE    );
	CHECK_RELATIONS( variant_type,   node_type,     FALSE,   TRUE,    TRUE    );

	CHECK_RELATIONS( object_type,    unknown_type,  UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( object_type,    variant_type,  FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( object_type,    object_type,   TRUE,    TRUE,    TRUE    );
	CHECK_RELATIONS( object_type,    node_type,     FALSE,   TRUE,    TRUE    );
	CHECK_RELATIONS( object_type,    nil_type,      FALSE,   TRUE,    TRUE    );

	CHECK_RELATIONS( node_type,      unknown_type,  UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( node_type,      variant_type,  FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( node_type,      object_type,   FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( node_type,      node_type,     TRUE,    TRUE,    TRUE    );
	CHECK_RELATIONS( node_type,      nil_type,      FALSE,   TRUE,    TRUE    );

	CHECK_RELATIONS( nil_type,       object_type,   FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( nil_type,       node_type,     FALSE,   FALSE,   UNKNOWN );

	CHECK_RELATIONS( node_type,      resource_type, FALSE,   FALSE,   FALSE   );
	CHECK_RELATIONS( resource_type,  node_type,     FALSE,   FALSE,   FALSE   );

	// --- Unions ---

	CHECK_RELATIONS( union_int_string_type,       union_int_string_type,       TRUE,    TRUE,    TRUE    );

	CHECK_RELATIONS( union_int_string_type,       unknown_type,                UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( union_int_string_type,       variant_type,                FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( union_int_string_type,       int_type,                    FALSE,   TRUE,    TRUE    );
	CHECK_RELATIONS( union_int_string_type,       array_type,                  FALSE,   FALSE,   FALSE   );
	CHECK_RELATIONS( union_int_string_type,       union_int_string_array_type, FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( union_int_string_type,       union_int_array_type,        FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( union_int_string_type,       union_array_nil_type,        FALSE,   FALSE,   FALSE   );

	CHECK_RELATIONS( unknown_type,                union_int_string_type,       UNKNOWN, UNKNOWN, UNKNOWN );
	CHECK_RELATIONS( variant_type,                union_int_string_type,       FALSE,   TRUE,    TRUE    );
	CHECK_RELATIONS( int_type,                    union_int_string_type,       FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( array_type,                  union_int_string_type,       FALSE,   FALSE,   FALSE   );
	CHECK_RELATIONS( union_int_string_array_type, union_int_string_type,       FALSE,   TRUE,    TRUE    );
	CHECK_RELATIONS( union_int_array_type,        union_int_string_type,       FALSE,   FALSE,   UNKNOWN );
	CHECK_RELATIONS( union_array_nil_type,        union_int_string_type,       FALSE,   FALSE,   FALSE   );

	// clang-format on

#undef CHECK_RELATIONS
}

TEST_CASE("[Modules][GDScript3][Compiler][DataType] Union type construction") {
	const DataType unknown_type = DataType::make_unknown();
	const DataType variant_type = DataType::make_variant();
	const DataType nil_type = DataType::make_builtin(Variant::NIL);
	const DataType int_type = DataType::make_builtin(Variant::INT);
	const DataType float_type = DataType::make_builtin(Variant::FLOAT);
	const DataType string_type = DataType::make_builtin(Variant::STRING);

	const DataType array_type = DataType::make_builtin(Variant::ARRAY);
	const DataType array_int_type = DataType::make_array(int_type);
	const DataType array_float_type = DataType::make_array(float_type);

	const DataType object_type = DataType::make_native_class("Object");
	const DataType node_type = DataType::make_native_class("Node");
	const DataType sprite2d_type = DataType::make_native_class("Sprite2D");
	const DataType control_type = DataType::make_native_class("Control");
	const DataType resource_type = DataType::make_native_class("Resource");

	const DataType union_int_string_type = DataType::make_union({ int_type, string_type });
	const DataType union_int_string_array_type = DataType::make_union({ int_type, string_type, array_type });
	const DataType union_int_array_type = DataType::make_union({ int_type, array_type });

#define CHECK_UNION(m_components, m_fqtn) \
	CHECK(DataType::make_union(m_components).get_fqtn() == m_fqtn)

	// clang-format off

	// --- General ---

	CHECK_UNION( LIST( unknown_type, unknown_type ), "<unknown type>" );
	CHECK_UNION( LIST( unknown_type, variant_type ), "Variant"        );
	CHECK_UNION( LIST( unknown_type, int_type     ), "<unknown type>" );
	CHECK_UNION( LIST( unknown_type, float_type   ), "<unknown type>" );
	CHECK_UNION( LIST( unknown_type, nil_type     ), "<unknown type>" );

	CHECK_UNION( LIST( variant_type, unknown_type ), "Variant"        );
	CHECK_UNION( LIST( variant_type, variant_type ), "Variant"        );
	CHECK_UNION( LIST( variant_type, int_type     ), "Variant"        );
	CHECK_UNION( LIST( variant_type, float_type   ), "Variant"        );
	CHECK_UNION( LIST( variant_type, nil_type     ), "Variant"        );

	CHECK_UNION( LIST( int_type,     unknown_type ), "<unknown type>" );
	CHECK_UNION( LIST( int_type,     variant_type ), "Variant"        );
	CHECK_UNION( LIST( int_type,     int_type     ), "int"            );
	CHECK_UNION( LIST( int_type,     float_type   ), "float|int"      );
	CHECK_UNION( LIST( int_type,     nil_type     ), "Nil|int"        );

	CHECK_UNION( LIST( float_type,   unknown_type ), "<unknown type>" );
	CHECK_UNION( LIST( float_type,   variant_type ), "Variant"        );
	CHECK_UNION( LIST( float_type,   int_type     ), "float|int"      );
	CHECK_UNION( LIST( float_type,   float_type   ), "float"          );
	CHECK_UNION( LIST( float_type,   nil_type     ), "Nil|float"      );

	CHECK_UNION( LIST( nil_type,     unknown_type ), "<unknown type>" );
	CHECK_UNION( LIST( nil_type,     variant_type ), "Variant"        );
	CHECK_UNION( LIST( nil_type,     int_type     ), "Nil|int"        );
	CHECK_UNION( LIST( nil_type,     float_type   ), "Nil|float"      );
	CHECK_UNION( LIST( nil_type,     nil_type     ), "Nil"            );

	// --- Typed arrays ---

	CHECK_UNION( LIST( array_type,       array_type       ), "Array"                   );
	CHECK_UNION( LIST( array_type,       array_int_type   ), "Array"                   );
	CHECK_UNION( LIST( array_type,       array_float_type ), "Array"                   );

	CHECK_UNION( LIST( array_int_type,   array_type       ), "Array"                   );
	CHECK_UNION( LIST( array_int_type,   array_int_type   ), "Array[int]"              );
	CHECK_UNION( LIST( array_int_type,   array_float_type ), "Array[float]|Array[int]" );

	CHECK_UNION( LIST( array_float_type, array_type       ), "Array"                   );
	CHECK_UNION( LIST( array_float_type, array_int_type   ), "Array[float]|Array[int]" );
	CHECK_UNION( LIST( array_float_type, array_float_type ), "Array[float]"            );

	// --- Classes ---

	// TODO

	// --- Unions ---

	// TODO

	// clang-format on

#undef CHECK_UNION
}

TEST_CASE("[Modules][GDScript3][Compiler][DataType] Narrowest common type") {
	// TODO
}

TEST_CASE("[Modules][GDScript3][Compiler][DataType] Operator type") {
	// TODO
}

} // namespace gdscript

#undef LIST
