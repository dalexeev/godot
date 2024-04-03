/**************************************************************************/
/*  gdscript_bridge.c                                                     */
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

typedef void *VariantPtr;
typedef void *StringNamePtr;

typedef struct {
	int error;
	int argument;
	int expected;
} CallError;

// ========== gds_variant_construct ==========

typedef void (*VariantConstruct)(int, VariantPtr, const VariantPtr *, int, CallError *);

static VariantConstruct variant_construct;

extern void gds_variant_construct(int p_type, VariantPtr r_base, const VariantPtr *p_args, int p_argcount, CallError *r_error) {
	variant_construct(p_type, r_base, p_args, p_argcount, r_error);
}

extern void gds_set_variant_construct(VariantConstruct p_func) {
	variant_construct = p_func;
}

// ========== gds_variant_callp ==========

typedef void (*VariantCallp)(VariantPtr, const StringNamePtr, const VariantPtr *, int, VariantPtr, CallError *);

static VariantCallp variant_callp;

extern void gds_variant_callp(VariantPtr p_base, const StringNamePtr p_method, const VariantPtr *p_args, int p_argcount, VariantPtr r_ret, CallError *r_error) {
	variant_callp(p_base, p_method, p_args, p_argcount, r_ret, r_error);
}

extern void gds_set_variant_callp(VariantCallp p_func) {
	variant_callp = p_func;
}

// ========== gds_gdscript_utility ==========

typedef void (*GDScriptUtility)(VariantPtr, const VariantPtr *, int, CallError *);

static GDScriptUtility *gdscript_utilities;

extern void gds_gdscript_utility(int p_func_index, VariantPtr r_ret, const VariantPtr *p_args, int p_argcount, CallError *r_error) {
	gdscript_utilities[p_func_index](r_ret, *p_args, p_argcount, r_error);
}

extern void gds_set_gdscript_utilities(GDScriptUtility *p_funcs) {
	gdscript_utilities = p_funcs;
}

// TODO: variant destroy (?), static methods, const call (?), variant utility.
// TODO: Add version/hash.
