/**************************************************************************/
/*  gdscript_bridge.cpp                                                   */
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

#include "gdscript_bridge.h"

#include <dlfcn.h>

#include "core/variant/variant.h"
#include "modules/gdscript/gdscript_utility_functions.h"

static const char *gdscript_bridge_c_header = R"^^^(
#ifndef GDSCRIPT_BRIDGE_H
#define GDSCRIPT_BRIDGE_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GODOT_VARIANT_SIZE <sizeof Variant>
#define GODOT_STRING_NAME_SIZE <sizeof StringName>
#define GODOT_CALL_ERROR_SIZE <sizeof Callable::CallError>

typedef struct {
	int type;
	alignas(8) uint8_t _internal_data[GODOT_VARIANT_SIZE - 8];
} Variant;

typedef struct {
	uint8_t _internal_data[GODOT_STRING_NAME_SIZE];
} StringName;

typedef struct {
	int error;
	int argument;
	int expected;
} CallError;

typedef void (*VariantAssignFunc)(Variant *, const Variant *);
extern void gdsc_set_variant_assign(VariantAssignFunc p_func);
void variant_assign(Variant *p_target, const Variant *p_source);

typedef bool (*VariantBooleanizeFunc)(const Variant *);
extern void gdsc_set_variant_booleanize(VariantBooleanizeFunc p_func);
bool variant_booleanize(const Variant *p_variant);

typedef bool (*VariantIsSharedFunc)(const Variant *);
extern void gdsc_set_variant_is_shared(VariantIsSharedFunc p_func);
bool variant_is_shared(const Variant *p_variant);

typedef void (*VariantConstructFunc)(int, Variant *, const Variant **, int, CallError *);
extern void gdsc_set_variant_construct(VariantConstructFunc p_func);
void variant_construct(int p_type, Variant *r_base, const Variant **p_args, int p_argcount, CallError *r_error);

typedef void (*VariantCallpFunc)(Variant *, const StringName *, const Variant **, int, Variant *, CallError *);
extern void gdsc_set_variant_callp(VariantCallpFunc p_func);
void variant_callp(Variant *p_base, const StringName *p_method, const Variant **p_args, int p_argcount, Variant *r_ret, CallError *r_error);

typedef void (*GDScriptUtilityFunc)(int, Variant *, const Variant **, int, CallError *);
extern void gdsc_set_gdscript_utility(GDScriptUtilityFunc p_func);
void gdscript_utility(int p_func_index, Variant *r_ret, const Variant **p_args, int p_argcount, CallError *r_error);

#endif // GDSCRIPT_BRIDGE_H
)^^^";

static const char *gdscript_bridge_c_source = R"^^^(
#include "gdscript_bridge.h"

#include <assert.h>

static_assert(sizeof(Variant) == GODOT_VARIANT_SIZE, "Invalid \"Variant\" size.");
static_assert(sizeof(StringName) == GODOT_STRING_NAME_SIZE, "Invalid \"StringName\" size.");
static_assert(sizeof(CallError) == GODOT_CALL_ERROR_SIZE, "Invalid \"CallError\" size.");

static VariantAssignFunc _variant_assign = NULL;
void gdsc_set_variant_assign(VariantAssignFunc p_func) {
	_variant_assign = p_func;
}
void variant_assign(Variant *p_target, const Variant *p_source) {
	_variant_assign(p_target, p_source);
}

static VariantBooleanizeFunc _variant_booleanize = NULL;
void gdsc_set_variant_booleanize(VariantBooleanizeFunc p_func) {
	_variant_booleanize = p_func;
}
bool variant_booleanize(const Variant *p_variant) {
	return _variant_booleanize(p_variant);
}

static VariantIsSharedFunc _variant_is_shared = NULL;
void gdsc_set_variant_is_shared(VariantIsSharedFunc p_func) {
	_variant_is_shared = p_func;
}
bool variant_is_shared(const Variant *p_variant) {
	return _variant_is_shared(p_variant);
}

static VariantConstructFunc _variant_construct = NULL;
void gdsc_set_variant_construct(VariantConstructFunc p_func) {
	_variant_construct = p_func;
}
void variant_construct(int p_type, Variant *r_base, const Variant **p_args, int p_argcount, CallError *r_error) {
	_variant_construct(p_type, r_base, p_args, p_argcount, r_error);
}

static VariantCallpFunc _variant_callp = NULL;
void gdsc_set_variant_callp(VariantCallpFunc p_func) {
	_variant_callp = p_func;
}
void variant_callp(Variant *p_base, const StringName *p_method, const Variant **p_args, int p_argcount, Variant *r_ret, CallError *r_error) {
	_variant_callp(p_base, p_method, p_args, p_argcount, r_ret, r_error);
}

static GDScriptUtilityFunc _gdscript_utility = NULL;
void gdsc_set_gdscript_utility(GDScriptUtilityFunc p_func) {
	_gdscript_utility = p_func;
}
void gdscript_utility(int p_func_index, Variant *r_ret, const Variant **p_args, int p_argcount, CallError *r_error) {
	_gdscript_utility(p_func_index, r_ret, p_args, p_argcount, r_error);
}
)^^^";

using VariantAssignFunc = void (*)(Variant *, const Variant *);
extern "C" void gdsc_variant_assign(Variant *p_target, const Variant *p_source) {
	*p_target = *p_source;
}

using VariantBooleanizeFunc = bool (*)(const Variant *);
extern "C" bool gdsc_variant_booleanize(const Variant *p_variant) {
	return p_variant->booleanize();
}

using VariantIsSharedFunc = bool (*)(const Variant *);
extern "C" bool gdsc_variant_is_shared(const Variant *p_variant) {
	return p_variant->is_shared();
}

using VariantConstructFunc = void (*)(int, Variant *, const Variant **, int, Callable::CallError *);
extern "C" void gdsc_variant_construct(int p_type, Variant *r_base, const Variant **p_args, int p_argcount, Callable::CallError *r_error) {
	Variant::construct((Variant::Type)p_type, *r_base, p_args, p_argcount, *r_error);
}

using VariantCallpFunc = void (*)(Variant *, const StringName *, const Variant **, int, Variant *, Callable::CallError *);
extern "C" void gdsc_variant_callp(Variant *p_base, const StringName *p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError *r_error) {
	p_base->callp(*p_method, p_args, p_argcount, *r_ret, *r_error);
}

using GDScriptUtilityFunc = void (*)(int, Variant *, const Variant **, int, Callable::CallError *);
extern "C" void gdsc_gdscript_utility(int p_func_index, Variant *r_ret, const Variant **p_args, int p_argcount, Callable::CallError *r_error) {
	// TODO
}

// === GDScriptBridge ===

void *GDScriptBridge::handler = nullptr;

bool GDScriptBridge::_init_library() {
#define SET_FUNC(m_type, m_name)                                                          \
	{                                                                                     \
		void (*set_func)(m_type) = (void (*)(m_type))dlsym(handler, "gdsc_set_" #m_name); \
		ERR_FAIL_NULL_V_MSG(set_func, false, "Function \"" #m_name "\" not found.");      \
		set_func(gdsc_##m_name);                                                          \
	}

	SET_FUNC(VariantAssignFunc, variant_assign);
	SET_FUNC(VariantBooleanizeFunc, variant_booleanize);
	SET_FUNC(VariantIsSharedFunc, variant_is_shared);
	SET_FUNC(VariantConstructFunc, variant_construct);
	SET_FUNC(VariantCallpFunc, variant_callp);
	SET_FUNC(GDScriptUtilityFunc, gdscript_utility);

#undef SET_FUNC

	return true;
}

String GDScriptBridge::get_c_header() {
#define REPLACE(m_type) replace("<sizeof " #m_type ">", itos(sizeof(m_type)))
	return String(gdscript_bridge_c_header).lstrip("\n").REPLACE(Variant).REPLACE(StringName).REPLACE(Callable::CallError);
#undef REPLACE
}

String GDScriptBridge::get_c_source() {
	return String(gdscript_bridge_c_source).lstrip("\n");
}

void GDScriptBridge::load_library(const String &p_path) {
	ERR_FAIL_COND_MSG(handler != nullptr, "A GDScript library already loaded.");
	const CharString path = p_path.ascii();
	handler = dlopen(path.ptr(), RTLD_LAZY);
	ERR_FAIL_NULL_MSG(handler, "Failed to load GDScript library.");
	if (!_init_library()) {
		unload_library();
	}
}

void GDScriptBridge::unload_library() {
	ERR_FAIL_NULL_MSG(handler, "No GDScript library loaded.");
	const int err = dlclose(handler);
	ERR_FAIL_COND_MSG(err, "Failed to unload GDScript library.");
	handler = nullptr;
}
