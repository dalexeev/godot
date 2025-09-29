/**************************************************************************/
/*  tokenizer.h                                                           */
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

#include "common_defs.h"

#include "core/variant/variant.h"

namespace gdscript {

class Tokenizer {
public:
	struct Token {
		enum class Type {
			EMPTY,
			// --- Basic ---
			ANNOTATION,
			IDENTIFIER,
			LITERAL,
			// --- Comparison ---
			LESS,
			LESS_EQUAL,
			GREATER,
			GREATER_EQUAL,
			EQUAL_EQUAL,
			BANG_EQUAL,
			// --- Logical ---
			AND,
			OR,
			NOT,
			AMPERSAND_AMPERSAND,
			PIPE_PIPE,
			BANG,
			// --- Bitwise ---
			AMPERSAND,
			PIPE,
			TILDE,
			CARET,
			LESS_LESS,
			GREATER_GREATER,
			// --- Math ---
			PLUS,
			MINUS,
			STAR,
			STAR_STAR,
			SLASH,
			PERCENT,
			// --- Assignment ---
			EQUAL,
			PLUS_EQUAL,
			MINUS_EQUAL,
			STAR_EQUAL,
			STAR_STAR_EQUAL,
			SLASH_EQUAL,
			PERCENT_EQUAL,
			LESS_LESS_EQUAL,
			GREATER_GREATER_EQUAL,
			AMPERSAND_EQUAL,
			PIPE_EQUAL,
			CARET_EQUAL,
			// --- Control flow ---
			IF,
			ELIF,
			ELSE,
			FOR,
			WHILE,
			BREAK,
			CONTINUE,
			PASS,
			RETURN,
			MATCH,
			WHEN,
			// --- Keywords ---
			AS,
			ASSERT,
			AWAIT,
			BREAKPOINT,
			CLASS,
			CLASS_NAME,
			CONST_, // Conflict with WinAPI.
			ENUM,
			EXTENDS,
			FUNC,
			IN_, // Conflict with WinAPI.
			IS,
			NAMESPACE,
			SELF,
			SIGNAL,
			SUPER,
			TRAIT,
			USING,
			VAR,
			YIELD,
			// --- Punctuation ---
			BRACKET_OPEN,
			BRACKET_CLOSE,
			BRACE_OPEN,
			BRACE_CLOSE,
			PARENTHESIS_OPEN,
			PARENTHESIS_CLOSE,
			COMMA,
			SEMICOLON,
			PERIOD,
			PERIOD_PERIOD,
			PERIOD_PERIOD_PERIOD,
			COLON,
			DOLLAR,
			FORWARD_ARROW,
			UNDERSCORE,
			// --- Whitespace ---
			NEWLINE,
			INDENT,
			DEDENT,
			// --- Error message improvement ---
			VCS_CONFLICT_MARKER,
			BACKTICK,
			QUESTION_MARK,
			// --- Special ---
			ERROR,
			EOF_, // Conflict with standard library.
			MAX,
		};

		struct InnerError {
			String message;
			SourceRegion source_region;

			InnerError() {}
		};

		Type type = Type::EMPTY;
		bool is_inline = false; // For recovering in the parser.
		Variant data; // Annotation/identifier name, literal value or error message.
		SourceRegion source_region;
		Vector<InnerError> inner_errors;

		static String get_name_static(Type p_type);
		static bool can_precede_infix_static(Type p_type);
		static bool is_keyword_static(Type p_type);

		static String get_symbol_name(Type p_type);

		_FORCE_INLINE_ String get_name() const { return get_name_static(type); }
		_FORCE_INLINE_ bool can_precede_infix() const { return can_precede_infix_static(type); }
		_FORCE_INLINE_ bool is_keyword() const { return is_keyword_static(type); }

		String get_debug_name() const;

		_FORCE_INLINE_ String get_source(const String &p_full_source) const {
			return source_region.get_text(p_full_source);
		}
		bool is_word(const String &p_full_source) const;
		_FORCE_INLINE_ bool is_string_literal() const {
			return type == Type::LITERAL && data.get_type() == Variant::STRING;
		}

		Token() {}
	};

private:
	struct Indent {
		int char_width = 0;
		int column_width = 0;

		Indent() {}
	};

public:
	struct State {
		friend class Tokenizer;

	private:
		const char32_t *current_ptr = nullptr;
		Token current_token;
		SourcePosition current;
		int pending_dedents = 0;

		char32_t indent_char = '\0';
		int indent_char_width = 0;
		Vector<Indent> indent_stack;

	public:
		State() {}
	};

private:
	static constexpr char32_t REPLACEMENT_CHAR = 0xFFFD;

	static HashMap<StringName, Token::Type> keywords;
	static PackedStringArray keyword_list;

	static HashMap<StringName, Variant> literals;
	static PackedStringArray literal_list;

	String source;
	const char32_t *begin_ptr = nullptr;
	const char32_t *end_ptr = nullptr;
	int tab_size = DEFAULT_TAB_SIZE;

	State state;
	HashMap<int, Comment> comments; // Safe to store out of `state`.
	Vector<Vector<Indent>> indent_stack_stack; // Need to store out of `state`.
	bool multiline_mode = false;

	String string_validated_data;
	char32_t string_lead_surrogate_char = '\0';
	SourceRegion string_lead_surrogate_region;

	_FORCE_INLINE_ bool is_at_end() const {
		return state.current_ptr >= end_ptr;
	}
	_FORCE_INLINE_ char32_t peek(unsigned p_offset = 0) const {
		return state.current_ptr + p_offset < end_ptr ? state.current_ptr[p_offset] : '\0';
	}
	void advance();

	_FORCE_INLINE_ void set_token_start(const SourcePosition &p_start) {
		state.current_token.source_region.start = p_start;
	}
	_FORCE_INLINE_ void set_token_start() {
		state.current_token.source_region.start = state.current;
	}
	_FORCE_INLINE_ const SourcePosition &get_token_start() const {
		return state.current_token.source_region.start;
	}

	_FORCE_INLINE_ String get_token_source() const {
		return source.substr(get_token_start().position, state.current.position - get_token_start().position);
	}

	Token &make_token(Token::Type p_type, const Variant &p_data = Variant());
	_FORCE_INLINE_ Token &make_error(const String &p_message) {
		return make_token(Token::Type::ERROR, p_message);
	}

	void add_inner_error(const String &p_message, const SourceRegion &p_source_region);
	_FORCE_INLINE_ void add_inner_error(const String &p_message, const SourcePosition &p_from) {
		add_inner_error(p_message, SourceRegion(p_from, state.current));
	}
	_FORCE_INLINE_ void add_inner_error(const String &p_message) {
		add_inner_error(p_message, SourceRegion(get_token_start(), state.current));
	}

	String get_indent_error_message(bool p_has_mixed_indent) const;
	_FORCE_INLINE_ Indent get_indent() const {
		return state.indent_stack.is_empty() ? Indent() : state.indent_stack[state.indent_stack.size() - 1];
	}

	void reset_string();
	void append_string(char32_t p_char, const SourceRegion &p_char_region);
	_FORCE_INLINE_ void append_string(char32_t p_char, const SourcePosition &p_char_start) {
		append_string(p_char, SourceRegion(p_char_start, state.current));
	}
	void complete_string();

	Token &scan_annotation();
	Token &scan_word(); // Identifier, keyword, literal, or underscore.
	Token &scan_number();
	Token &scan_string();

public:
	static void initialize();
	static void deinitialize();

	_FORCE_INLINE_ static const PackedStringArray &get_keyword_list() { return keyword_list; }
	_FORCE_INLINE_ static const PackedStringArray &get_literal_list() { return literal_list; }

	Token scan();

	// These methods allow you to roll back the tokenizer to a previously obtained state
	// (for example, if you want to disable multiline mode and scan whitespace tokens again).
	_FORCE_INLINE_ void set_state(const State &p_state) { state = p_state; }
	_FORCE_INLINE_ const State &get_state() const { return state; }

	// In multiline mode, whitespace tokens are not generated and indentation is not tracked.
	_FORCE_INLINE_ void set_multiline_mode(bool p_state) { multiline_mode = p_state; }
	_FORCE_INLINE_ bool is_multiline_mode() const { return multiline_mode; }

	// For blocks inside expressions (like lambdas).
	void push_expression_indented_block();
	void pop_expression_indented_block();
	_FORCE_INLINE_ int get_expression_indented_block_depth() const { return indent_stack_stack.size(); }

	// Returns all comments found so far.
	_FORCE_INLINE_ const HashMap<int, Comment> &get_comments() const { return comments; }

	Tokenizer(const String &p_source = String(), int p_tab_size = DEFAULT_TAB_SIZE);
};

} // namespace gdscript
