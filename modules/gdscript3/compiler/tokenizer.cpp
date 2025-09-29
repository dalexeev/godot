/**************************************************************************/
/*  tokenizer.cpp                                                         */
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

#include "tokenizer.h"

using namespace gdscript;

// ===== Tokenizer::Token =====

String Tokenizer::Token::get_name_static(Type p_type) {
	constexpr const char *token_names[] = {
		"Empty", // EMPTY
		// --- Basic ---
		"Annotation", // ANNOTATION
		"Identifier", // IDENTIFIER
		"Literal", // LITERAL
		// --- Comparison ---
		"<", // LESS
		"<=", // LESS_EQUAL
		">", // GREATER
		">=", // GREATER_EQUAL
		"==", // EQUAL_EQUAL
		"!=", // BANG_EQUAL
		// --- Logical ---
		"and", // AND
		"or", // OR
		"not", // NOT
		"&&", // AMPERSAND_AMPERSAND
		"||", // PIPE_PIPE
		"!", // BANG
		// --- Bitwise ---
		"&", // AMPERSAND
		"|", // PIPE
		"~", // TILDE
		"^", // CARET
		"<<", // LESS_LESS
		">>", // GREATER_GREATER
		// --- Math ---
		"+", // PLUS
		"-", // MINUS
		"*", // STAR
		"**", // STAR_STAR
		"/", // SLASH
		"%", // PERCENT
		// --- Assignment ---
		"=", // EQUAL
		"+=", // PLUS_EQUAL
		"-=", // MINUS_EQUAL
		"*=", // STAR_EQUAL
		"**=", // STAR_STAR_EQUAL
		"/=", // SLASH_EQUAL
		"%=", // PERCENT_EQUAL
		"<<=", // LESS_LESS_EQUAL
		">>=", // GREATER_GREATER_EQUAL
		"&=", // AMPERSAND_EQUAL
		"|=", // PIPE_EQUAL
		"^=", // CARET_EQUAL
		// --- Control flow ---
		"if", // IF
		"elif", // ELIF
		"else", // ELSE
		"for", // FOR
		"while", // WHILE
		"break", // BREAK
		"continue", // CONTINUE
		"pass", // PASS
		"return", // RETURN
		"match", // MATCH
		"when", // WHEN
		// --- Keywords ---
		"as", // AS
		"assert", // ASSERT
		"await", // AWAIT
		"breakpoint", // BREAKPOINT
		"class", // CLASS
		"class_name", // CLASS_NAME
		"const", // CONST_
		"enum", // ENUM
		"extends", // EXTENDS
		"func", // FUNC
		"in", // TIN_
		"is", // IS
		"namespace", // NAMESPACE
		"self", // SELF
		"signal", // SIGNAL
		"super", // SUPER
		"trait", // TRAIT
		"using", // USING
		"var", // VAR
		"yield", // YIELD
		// --- Punctuation ---
		"[", // BRACKET_OPEN
		"]", // BRACKET_CLOSE
		"{", // BRACE_OPEN
		"}", // BRACE_CLOSE
		"(", // PARENTHESIS_OPEN
		")", // PARENTHESIS_CLOSE
		",", // COMMA
		";", // SEMICOLON
		".", // PERIOD
		"..", // PERIOD_PERIOD
		"...", // PERIOD_PERIOD_PERIOD
		":", // COLON
		"$", // DOLLAR
		"->", // FORWARD_ARROW
		"_", // UNDERSCORE
		// --- Whitespace ---
		"Newline", // NEWLINE
		"Indent", // INDENT
		"Dedent", // DEDENT
		// --- Error message improvement ---
		"VCS conflict marker", // VCS_CONFLICT_MARKER
		"`", // BACKTICK
		"?", // QUESTION_MARK
		// --- Special ---
		"Error", // ERROR
		"End of file", // EOF_
	};

	static_assert(
			std_size(token_names) == (int)Token::Type::MAX,
			"Amount of token names don't match the amount of token types.");
	ERR_FAIL_INDEX_V((int)p_type, (int)Type::MAX, "<error>");
	return token_names[(int)p_type];
}

bool Tokenizer::Token::can_precede_infix_static(Type p_type) {
	switch (p_type) {
		case Type::IDENTIFIER:
		case Type::LITERAL:
		case Type::SELF:
		case Type::BRACKET_CLOSE:
		case Type::BRACE_CLOSE:
		case Type::PARENTHESIS_CLOSE:
			return true;
		default:
			return false;
	}
}

bool Tokenizer::Token::is_keyword_static(Type p_type) {
	switch (p_type) {
		case Type::AS:
		case Type::AND:
		case Type::ASSERT:
		case Type::AWAIT:
		case Type::BREAK:
		case Type::BREAKPOINT:
		case Type::CLASS:
		case Type::CLASS_NAME:
		case Type::CONST_:
		case Type::CONTINUE:
		case Type::ELIF:
		case Type::ELSE:
		case Type::ENUM:
		case Type::EXTENDS:
		case Type::FOR:
		case Type::FUNC:
		case Type::IF:
		case Type::IN_:
		case Type::IS:
		case Type::MATCH:
		case Type::NAMESPACE:
		case Type::NOT:
		case Type::OR:
		case Type::PASS:
		case Type::RETURN:
		case Type::SELF:
		case Type::SIGNAL:
		case Type::SUPER:
		case Type::TRAIT:
		case Type::USING:
		case Type::VAR:
		case Type::WHILE:
		case Type::WHEN:
		case Type::YIELD:
			return true;
		default:
			return false;
	}
}

String Tokenizer::Token::get_symbol_name(Type p_type) {
	switch (p_type) {
		case Type::EMPTY:
			return "empty token";
		case Type::ANNOTATION:
			return "annotation";
		case Type::IDENTIFIER:
			return "identifier";
		case Type::LITERAL:
			return "literal";
		case Type::NEWLINE:
			return "newline";
		case Type::INDENT:
			return "indent";
		case Type::DEDENT:
			return "unindent";
		case Type::VCS_CONFLICT_MARKER:
			return "VCS conflict marker";
		case Type::ERROR:
			return "error token";
		case Type::EOF_:
			return "end of file";
		default:
			return get_name_static(p_type).quote();
	}
}

String Tokenizer::Token::get_debug_name() const {
	switch (type) {
		case Type::EMPTY:
			return "empty token";
		case Type::ANNOTATION:
			return vformat(R"(annotation "%s")", data);
		case Type::IDENTIFIER:
			return vformat(R"(identifier "%s")", data);
		case Type::LITERAL:
			return vformat("%s literal", Variant::get_type_name(data.get_type()));
		case Type::NEWLINE:
			return "newline";
		case Type::INDENT:
			return "indent";
		case Type::DEDENT:
			return "unindent";
		case Type::VCS_CONFLICT_MARKER:
			return "VCS conflict marker";
		case Type::ERROR:
			return "error token";
		case Type::EOF_:
			return "end of file";
		default:
			return get_name().quote();
	}
}

bool Tokenizer::Token::is_word(const String &p_full_source) const {
	return type == Type::IDENTIFIER ||
			type == Type::UNDERSCORE ||
			is_keyword() ||
			literals.has(get_source(p_full_source));
}

// ===== Tokenizer =====

HashMap<StringName, Tokenizer::Token::Type> Tokenizer::keywords;
PackedStringArray Tokenizer::keyword_list;

HashMap<StringName, Variant> Tokenizer::literals;
PackedStringArray Tokenizer::literal_list;

void Tokenizer::initialize() {
	for (int i = 0; i < (int)Token::Type::MAX; i++) {
		const Token::Type type = (Token::Type)i;
		if (Token::is_keyword_static(type)) {
			keywords[Token::get_name_static(type)] = type;
			keyword_list.push_back(Token::get_name_static(type));
		}
	}

	literals = HashMap<StringName, Variant>({
			// clang-format off
			{ "null",  Variant() },
			{ "false", false     },
			{ "true",  true      },
			/* // TODO: Resolve math constants in the analyzer (for consistency and to allow shadowing).
			{ "PI",    Math::PI  },
			{ "TAU",   Math::TAU },
			{ "INF",   Math::INF },
			{ "NAN",   Math::NaN },
			*/
			// clang-format on
	});

	for (const KeyValue<StringName, Variant> &kv : literals) {
		literal_list.push_back(kv.key);
	}
}

void Tokenizer::deinitialize() {
	keywords.clear();
	keyword_list.clear();

	literals.clear();
	literal_list.clear();
}

void Tokenizer::advance() {
	if (is_at_end()) {
		return;
	}
	switch (*state.current_ptr) { // Safe since `is_at_end() == false`.
		case U'\uFEFF':
		case '\r':
			break;
		case '\n':
			state.current.line++;
			state.current.column = 1;
			state.current.raw_column = 0;
			break;
		case '\t':
			state.current.column += tab_size - (state.current.column - 1) % tab_size;
			state.current.raw_column++;
			break;
		default:
			state.current.column++;
			state.current.raw_column++;
			break;
	}
	state.current.position++;
	state.current_ptr++;
}

Tokenizer::Token &Tokenizer::make_token(Token::Type p_type, const Variant &p_data) {
	state.current_token.type = p_type;
	state.current_token.data = p_data;
	state.current_token.source_region.end = state.current;

	return state.current_token;
}

void Tokenizer::add_inner_error(const String &p_message, const SourceRegion &p_source_region) {
	Token::InnerError inner_error;
	inner_error.message = p_message;
	inner_error.source_region = p_source_region;

	state.current_token.inner_errors.push_back(inner_error);
}

String Tokenizer::get_indent_error_message(bool p_has_mixed_indent) const {
	if (p_has_mixed_indent) {
		return "Mixed use of tabs and spaces for indentation.";
	} else {
		if (state.indent_char == ' ') {
			return "Used tabs for indentation instead of spaces as used before in the file.";
		} else {
			return "Used spaces for indentation instead of tabs as used before in the file.";
		}
	}
}

void Tokenizer::reset_string() {
	string_validated_data = String();
	string_lead_surrogate_char = '\0';
	string_lead_surrogate_region = SourceRegion();
}

void Tokenizer::append_string(char32_t p_char, const SourceRegion &p_char_region) {
	if (string_lead_surrogate_char == '\0') {
		if ((p_char & 0xFFFFFC00) == 0xD800) {
			string_lead_surrogate_char = p_char;
			string_lead_surrogate_region = p_char_region;
		} else if ((p_char & 0xFFFFFC00) == 0xDC00) {
			add_inner_error("Invalid UTF-16 sequence in string, unpaired trail surrogate.", p_char_region);
			string_validated_data += REPLACEMENT_CHAR;
		} else {
			string_validated_data += p_char;
		}
	} else {
		if ((p_char & 0xFFFFFC00) == 0xD800) {
			add_inner_error("Invalid UTF-16 sequence in string, unpaired lead surrogate.", string_lead_surrogate_region);
			string_validated_data += REPLACEMENT_CHAR;
			// Replace with a new lead surrogate character.
			string_lead_surrogate_char = p_char;
			string_lead_surrogate_region = p_char_region;
		} else if ((p_char & 0xFFFFFC00) == 0xDC00) {
			string_validated_data += (string_lead_surrogate_char << 10UL) + p_char - ((0xD800 << 10UL) + 0xDC00 - 0x10000);
			string_lead_surrogate_char = '\0';
			string_lead_surrogate_region = SourceRegion();
		} else {
			add_inner_error("Invalid UTF-16 sequence in string, unpaired lead surrogate.", string_lead_surrogate_region);
			string_validated_data += REPLACEMENT_CHAR;
			string_lead_surrogate_char = '\0';
			string_lead_surrogate_region = SourceRegion();
			// Add a normal character.
			string_validated_data += p_char;
		}
	}
}

void Tokenizer::complete_string() {
	if (string_lead_surrogate_char != '\0') {
		add_inner_error("Invalid UTF-16 sequence in string, unpaired lead surrogate.", string_lead_surrogate_region);
		string_validated_data += REPLACEMENT_CHAR;
		string_lead_surrogate_char = '\0';
		string_lead_surrogate_region = SourceRegion();
	}
}

Tokenizer::Token &Tokenizer::scan_annotation() {
	advance(); // Consume `@` character.

	bool is_invalid = false;
	if (is_unicode_identifier_start(peek())) {
		advance(); // Consume start character.
	} else {
		is_invalid = true;
	}

	while (is_unicode_identifier_continue(peek())) {
		advance(); // Consume all identifier characters.
	}

	if (is_invalid) {
		add_inner_error(R"(Expected annotation identifier after "@".)");
	}

	return make_token(Token::Type::ANNOTATION, StringName(get_token_source()));
}

Tokenizer::Token &Tokenizer::scan_word() {
	bool only_ascii = peek() < 128;

	advance(); // Consume start character.

	while (is_unicode_identifier_continue(peek())) {
		only_ascii = only_ascii && peek() < 128;
		advance(); // Consume all identifier characters.
	}

	const StringName word = get_token_source();

	if (word == SNAME("_")) {
		return make_token(Token::Type::UNDERSCORE);
	}

	if (!only_ascii) {
		// Cannot be a keyword, as keywords are ASCII only.
		return make_token(Token::Type::IDENTIFIER, word);
	}

	{
		const Token::Type *token_type = keywords.getptr(word);
		if (token_type != nullptr) {
			return make_token(*token_type);
		}
	}

	{
		const Variant *value = literals.getptr(word);
		if (value != nullptr) {
			return make_token(Token::Type::LITERAL, *value);
		}
	}

	return make_token(Token::Type::IDENTIFIER, word);
}

Tokenizer::Token &Tokenizer::scan_number() {
	enum class NumberState {
		INTEGER,
		DECIMAL,
		EXPONENT,
		DONE,
	};

	NumberState number_state = NumberState::INTEGER;
	int base = 10;
	bool (*digit_check_func)(char32_t) = is_digit;
	bool is_float = false;

	if (peek() == '+' || peek() == '-') {
		advance();
	}

	bool has_invalid_base = false;
	char32_t base_letter = '\0';
	if (peek() == '.') {
		advance();
		number_state = NumberState::DECIMAL;
		is_float = true;
	} else if (peek() == '0') {
		switch (peek(1)) {
			case 'x':
			case 'X':
				base_letter = peek(1);
				advance();
				advance();
				if (!is_hex_digit(peek())) {
					has_invalid_base = true;
				}
				base = 16;
				digit_check_func = is_hex_digit;
				break;
			case 'b':
			case 'B':
				base_letter = peek(1);
				advance();
				advance();
				if (!is_binary_digit(peek())) {
					has_invalid_base = true;
				}
				base = 2;
				digit_check_func = is_binary_digit;
				break;
		}
	}

	bool has_digit_group = false;
	bool has_invalid_underscores = false;
	bool has_missing_exponent = false;
	char32_t exponent_letter = '\0';
	while (number_state != NumberState::DONE) {
		bool previous_was_underscore = false;
		while (digit_check_func(peek()) || is_underscore(peek())) {
			if (is_underscore(peek())) {
				if (previous_was_underscore) {
					has_invalid_underscores = true;
				}
				previous_was_underscore = true;
			} else {
				has_digit_group = true;
				previous_was_underscore = false;
			}
			advance();
		}

		NumberState next_number_state = NumberState::DONE;
		if (base == 10) {
			switch (number_state) {
				case NumberState::INTEGER:
					if (peek() == '.') {
						advance();
						is_float = true;
						next_number_state = NumberState::DECIMAL;
					}
					[[fallthrough]];
				case NumberState::DECIMAL:
					if (peek() == 'e' || peek() == 'E') {
						exponent_letter = peek();
						advance();
						if (peek() == '+' || peek() == '-') {
							advance();
						}
						if (is_digit(peek())) {
							next_number_state = NumberState::EXPONENT;
						} else {
							has_missing_exponent = true;
						}
					}
					break;
				case NumberState::EXPONENT:
				case NumberState::DONE:
					break;
			}
		}
		number_state = next_number_state;
	}

	Variant value;
	if (has_invalid_base) {
		value = 0;
	} else {
		const String data = get_token_source().remove_char('_');
		if (is_float) {
			value = data.to_float();
		} else if (base == 16) {
			value = data.hex_to_int();
		} else if (base == 2) {
			value = data.bin_to_int();
		} else {
			value = data.to_int();
		}
	}

	bool has_extra_characters = false;
	if (is_unicode_identifier_continue(peek())) {
		has_extra_characters = true;
		while (is_unicode_identifier_continue(peek())) {
			advance();
		}
	}

	if (has_invalid_base) {
		const String base_name = (base == 16) ? "hexadecimal" : "binary";
		add_inner_error(vformat(R"(Expected a %s digit after "0%c".)", base_name, base_letter));
	}
	if (has_invalid_underscores) {
		add_inner_error("Multiple underscores cannot be adjacent in a numeric literal.");
	}
	if (has_missing_exponent) {
		add_inner_error(vformat(R"(Expected exponent value after "%c".)", exponent_letter));
	}
	if (!has_digit_group || has_extra_characters) {
		add_inner_error("Invalid numeric notation.");
	}

	return make_token(Token::Type::LITERAL, value);
}

Tokenizer::Token &Tokenizer::scan_string() {
	enum class StringType {
		REGULAR,
		RAW,
		STRING_NAME,
		NODE_PATH,
	};

	StringType type = StringType::REGULAR;
	if (peek() == 'r') {
		advance();
		type = StringType::RAW;
	} else if (peek() == '&') {
		advance();
		type = StringType::STRING_NAME;
	} else if (peek() == '^') {
		advance();
		type = StringType::NODE_PATH;
	}

	const char32_t quote_char = peek();
	advance();

	bool is_multiline = false;
	if (peek() == quote_char && peek(1) == quote_char) {
		is_multiline = true;
		advance();
		advance();
	}

	reset_string();

	while (true) {
		if (is_at_end()) {
			add_inner_error("Unterminated string.");
			break;
		}

		const char32_t c = peek();
		const SourcePosition char_start = state.current;

		if (c == 0x200E || c == 0x200F || (c >= 0x202A && c <= 0x202E) || (c >= 0x2066 && c <= 0x2069)) {
			advance();

			String message = "Invisible text direction control character present in the string, ";
			if (type == StringType::RAW) {
				message += "use regular string literal instead of raw string literal.";
			} else {
				message += vformat(R"(escape it ("\u%04X") to avoid confusion.)", static_cast<int32_t>(c));
			}

			add_inner_error(message, char_start);
			append_string(REPLACEMENT_CHAR, char_start);
			continue;
		}

		if (c == '\\') {
			advance();
			if (is_at_end()) {
				add_inner_error("Unterminated string.");
				break;
			}

			if (type == StringType::RAW) {
				// Raw string literals cannot escape surrogate characters, so we can append
				// `string_validated_data` directly instead of using `append_string()`.
				if (peek() == quote_char) {
					advance();
					if (is_at_end()) {
						add_inner_error("Unterminated string.");
						break;
					}
					string_validated_data += '\\';
					string_validated_data += quote_char;
				} else if (peek() == '\\') { // For `\\\"`.
					advance();
					if (is_at_end()) {
						add_inner_error("Unterminated string.");
						break;
					}
					string_validated_data += '\\';
					string_validated_data += '\\';
				} else {
					string_validated_data += '\\';
				}
			} else {
				const char32_t code = peek();
				advance();
				if (is_at_end()) {
					add_inner_error("Unterminated string.");
					break;
				}

				switch (code) {
					case 'a':
						append_string('\a', char_start);
						break;
					case 'b':
						append_string('\b', char_start);
						break;
					case 'f':
						append_string('\f', char_start);
						break;
					case 'n':
						append_string('\n', char_start);
						break;
					case 'r':
						append_string('\r', char_start);
						break;
					case 't':
						append_string('\t', char_start);
						break;
					case 'v':
						append_string('\v', char_start);
						break;
					case '\'':
						append_string('\'', char_start);
						break;
					case '\"':
						append_string('\"', char_start);
						break;
					case '\\':
						append_string('\\', char_start);
						break;
					case 'U':
					case 'u': {
						const int hex_len = (code == 'U') ? 6 : 4;
						char32_t escaped = '\0';
						bool is_valid = true;

						for (int i = 0; i < hex_len; i++) {
							if (is_at_end()) {
								is_valid = false;
								add_inner_error("Unterminated string.");
								break;
							}

							const char32_t digit = peek();
							char32_t value = 0;
							if (is_digit(digit)) {
								value = digit - '0';
							} else if (digit >= 'a' && digit <= 'f') {
								value = digit - 'a';
								value += 10;
							} else if (digit >= 'A' && digit <= 'F') {
								value = digit - 'A';
								value += 10;
							} else {
								is_valid = false;
								const String msg = vformat(
										R"(Invalid unicode escape sequence. Expected %d hexadecimal digits after "\%c".)",
										hex_len,
										code);
								add_inner_error(msg, char_start);
								append_string(REPLACEMENT_CHAR, char_start);
								break;
							}

							escaped <<= 4;
							escaped |= value;

							advance();
						}

						if (is_valid) {
							append_string(escaped, char_start);
						}
					} break;
					case '\r':
						if (peek() != '\n') {
							// Carriage return without newline in string.
							// Just add it to the string and keep going.
							append_string('\r', char_start);
							break;
						}
						advance();
						[[fallthrough]];
					case '\n':
						// Escaping newline. Don't add it to the string.
						break;
					default:
						add_inner_error("Invalid escape sequence.", char_start);
						append_string(REPLACEMENT_CHAR, char_start);
						break;
				}
			}
		} else if (c == quote_char) {
			advance();

			if (is_multiline) {
				if (peek() == quote_char && peek(1) == quote_char) {
					// Ended the multiline string. Consume all quotes.
					advance();
					advance();
					break;
				} else {
					// Not a multiline string termination, add consumed quote.
					append_string(quote_char, char_start);
				}
			} else {
				// Ended single-line string.
				break;
			}
		} else {
			advance();
			append_string(c, char_start);
		}
	}

	complete_string();

	Variant value;
	switch (type) {
		case StringType::REGULAR:
		case StringType::RAW:
			value = string_validated_data;
			break;
		case StringType::STRING_NAME:
			value = StringName(string_validated_data);
			break;
		case StringType::NODE_PATH:
			value = NodePath(string_validated_data);
			break;
	}

	return make_token(Token::Type::LITERAL, value);
}

Tokenizer::Token Tokenizer::scan() {
	if (state.pending_dedents > 0) {
		DEV_ASSERT(state.current_token.type == Token::Type::DEDENT);
		state.pending_dedents--;
		state.current_token.inner_errors.clear(); // Don't repeat inner errors for subsequent dedents.
		return state.current_token;
	}

	const Token previous_token = state.current_token;

	state.current_token = Token();

	const bool is_beginning_of_line = state.current.column == 1;

	if (!is_beginning_of_line) {
		switch (previous_token.type) {
			case Token::Type::INDENT:
			case Token::Type::DEDENT:
				break;
			default:
				state.current_token.is_inline = true;
		}
	}

	Indent indent;
	SourcePosition indent_start = state.current;
	bool has_mismatched_indent = false;
	bool has_mixed_indent = false;

	bool skip_whitespace = true;
	bool is_blank_line = is_beginning_of_line;
	while (skip_whitespace) {
		switch (peek()) {
			case U'\uFEFF': {
				if (state.current_ptr == begin_ptr) {
					advance();
				} else {
					set_token_start();
					advance();
					return make_error("soft:Unexpected BOM character in the middle of the file.");
				}
			} break;
			case ' ':
			case '\t': {
				if (is_beginning_of_line && !multiline_mode) {
					if (unlikely(state.indent_char == '\0')) {
						state.indent_char = peek();
						state.indent_char_width = (state.indent_char == ' ') ? 1 : tab_size;
					}

					while (peek() == state.indent_char) {
						indent.char_width++;
						indent.column_width += state.indent_char_width;
						advance();
					}

					if (peek() == ' ' || peek() == '\t') {
						has_mismatched_indent = true;
						while (peek() == ' ' || peek() == '\t') {
							if (peek() == state.indent_char) {
								has_mixed_indent = true;
							}

							indent.char_width++;
							if (peek() == ' ') {
								indent.column_width++;
							} else {
								indent.column_width += tab_size - (state.current.column - 1) % tab_size;
							}

							advance();
						}
					}
				} else {
					advance();
				}
			} break;
			case '\r': {
				if (peek(1) == '\n') {
					advance();
				} else {
					set_token_start();
					advance();
					return make_error("Stray carriage return character in source code.");
				}
			} break;
			case '\n': {
				if (multiline_mode) {
					advance();
					state.current_token.is_inline = false;
				} else if (is_blank_line) {
					// This a blank line (no code, only whitespace or comment).
					// Let's skip it to avoid generating extra newlines.
					advance();

					indent = Indent();
					indent_start = state.current;
					has_mismatched_indent = false;
					has_mixed_indent = false;
				} else {
					set_token_start();
					const Token &newline_token = make_token(Token::Type::NEWLINE);
					advance();
					return newline_token;
				}
			} break;
			case '\\': {
				if (peek(1) == '\n') {
					advance();
					advance();
					// Skip indentation to prevent its interpretation.
					while (peek() == ' ' || peek() == '\t') {
						advance();
					}
					is_blank_line = true;
				} else {
					set_token_start();
					advance();
					return make_error(R"(Expected newline after "\".)");
				}
			} break;
			case '#': {
				Comment comment;

				comment.is_inline = state.current_token.is_inline;

				comment.source_region.start = state.current;

				while (peek() != '\n' && !is_at_end()) {
					advance();
				}

				comment.source_region.end = state.current;

				comments[comment.source_region.start.line] = comment;
			} break;
			default: {
				skip_whitespace = false;
			} break;
		}
	}

	if (is_beginning_of_line && !multiline_mode) {
		const Indent prev_indent = get_indent();

		if (indent.column_width > prev_indent.column_width) {
			state.indent_stack.push_back(indent);

			Token &indent_token = make_token(Token::Type::INDENT);

			indent_token.source_region.start = indent_start;
			indent_token.source_region.start.position += prev_indent.char_width;
			indent_token.source_region.start.column += prev_indent.column_width;
			indent_token.source_region.start.raw_column += prev_indent.char_width;

			indent_token.source_region.end = indent_start;
			indent_token.source_region.end.position += indent.char_width;
			indent_token.source_region.end.column += indent.column_width;
			indent_token.source_region.end.raw_column += indent.char_width;

			if (has_mismatched_indent || has_mixed_indent) {
				add_inner_error(get_indent_error_message(has_mixed_indent), indent_start);
			}

			return indent_token;
		} else if (indent.column_width < prev_indent.column_width) {
			int dedent_count = 0;
			Indent last_discarded_indent;
			while (indent.column_width < get_indent().column_width) {
				last_discarded_indent = get_indent();
				state.indent_stack.resize(state.indent_stack.size() - 1);
				dedent_count++;
			}

			const bool is_valid = indent.column_width == get_indent().column_width;
			if (!is_valid) {
				state.indent_stack.push_back(last_discarded_indent);
				dedent_count--;
				if (dedent_count <= 0) {
					set_token_start(indent_start);
					return make_error("soft:Unindent doesn't match the previous indentation level.");
				}
			}

			DEV_ASSERT(dedent_count > 0);

			state.pending_dedents = dedent_count - 1;

			Token &dedent_token = make_token(Token::Type::DEDENT);

			dedent_token.source_region.start = indent_start;
			dedent_token.source_region.start.position += indent.char_width;
			dedent_token.source_region.start.column += indent.column_width;
			dedent_token.source_region.start.raw_column += indent.char_width;

			dedent_token.source_region.end = dedent_token.source_region.start;

			if (has_mismatched_indent || has_mixed_indent) {
				add_inner_error(get_indent_error_message(has_mixed_indent), indent_start);
			}
			if (!is_valid) {
				add_inner_error("Unindent doesn't match the previous indentation level.", dedent_token.source_region);
			}

			return dedent_token;
		} else { // indent.column_width == prev_indent.column_width
			if (has_mismatched_indent || has_mixed_indent) {
				set_token_start(indent_start);
				return make_error("soft:" + get_indent_error_message(has_mixed_indent));
			}
		}
	}

	set_token_start();

	if (is_at_end()) {
		return make_token(Token::Type::EOF_);
	}

	const char32_t first_char = peek();

	// Scan `x`.
#define SCAN_CHAR(m_id) \
	advance(); \
	return make_token(Token::Type::m_id)

	// Scan `x` or `x=`.
#define SCAN_CHAR_EQUAL(m_id) \
	advance(); \
	if (peek() == '=') { \
		advance(); \
		return make_token(Token::Type::m_id##_EQUAL); \
	} \
	return make_token(Token::Type::m_id)

	// Scan `x`, `xx`, or `x=`.
#define SCAN_TWO_CHARS_EQUAL_SHORT(m_id) \
	advance(); \
	if (peek() == first_char) { \
		advance(); \
		return make_token(Token::Type::m_id##_##m_id); \
	} \
	if (peek() == '=') { \
		advance(); \
		return make_token(Token::Type::m_id##_EQUAL); \
	} \
	return make_token(Token::Type::m_id)

	// Scan `x`, `xx`, `x=`, or `xx=`.
#define SCAN_TWO_CHARS_EQUAL_FULL(m_id) \
	advance(); \
	if (peek() == first_char) { \
		advance(); \
		if (peek() == '=') { \
			advance(); \
			return make_token(Token::Type::m_id##_##m_id##_EQUAL); \
		} \
		return make_token(Token::Type::m_id##_##m_id); \
	} \
	if (peek() == '=') { \
		advance(); \
		return make_token(Token::Type::m_id##_EQUAL); \
	} \
	return make_token(Token::Type::m_id)

#define CHECK_VCS_CONFLICT_MARKER() \
	{ \
		unsigned chars = 1; \
		while (peek(chars) == first_char) { \
			chars++; \
		} \
		if (chars >= 7) { \
			while (chars > 0) { \
				advance(); \
				chars--; \
			} \
			return make_token(Token::Type::VCS_CONFLICT_MARKER); \
		} \
	}

	switch (first_char) {
		// Single characters (`x`).
		case '.':
			if (is_digit(peek(1))) {
				// Number starting with '.'.
				return scan_number();
			}
			if (peek(1) == '.') {
				advance();
				advance();
				if (peek() == '.') {
					advance();
					return make_token(Token::Type::PERIOD_PERIOD_PERIOD);
				}
				return make_token(Token::Type::PERIOD_PERIOD);
			}
			SCAN_CHAR(PERIOD);
		case ',':
			SCAN_CHAR(COMMA);
		case ':':
			SCAN_CHAR(COLON);
		case ';':
			SCAN_CHAR(SEMICOLON);
		case '(':
			SCAN_CHAR(PARENTHESIS_OPEN);
		case ')':
			SCAN_CHAR(PARENTHESIS_CLOSE);
		case '[':
			SCAN_CHAR(BRACKET_OPEN);
		case ']':
			SCAN_CHAR(BRACKET_CLOSE);
		case '{':
			SCAN_CHAR(BRACE_OPEN);
		case '}':
			SCAN_CHAR(BRACE_CLOSE);
		case '~':
			SCAN_CHAR(TILDE);
		case '$':
			SCAN_CHAR(DOLLAR);
		case '`':
			SCAN_CHAR(BACKTICK);
		case '?':
			SCAN_CHAR(QUESTION_MARK);

		// Double characters (`x` or `x=`).
		case '+':
			if (is_digit(peek(1)) && !previous_token.can_precede_infix()) {
				// Number starting with '+'.
				return scan_number();
			}
			SCAN_CHAR_EQUAL(PLUS);
		case '-':
			if (is_digit(peek(1)) && !previous_token.can_precede_infix()) {
				// Number starting with '-'.
				return scan_number();
			}
			if (peek(1) == '>') {
				advance();
				advance();
				return make_token(Token::Type::FORWARD_ARROW);
			}
			SCAN_CHAR_EQUAL(MINUS);
		case '^':
			if (peek(1) == '"' || peek(1) == '\'') {
				// `NodePath` literal.
				return scan_string();
			}
			SCAN_CHAR_EQUAL(CARET);
		case '=':
			CHECK_VCS_CONFLICT_MARKER();
			SCAN_CHAR_EQUAL(EQUAL);
		case '!':
			SCAN_CHAR_EQUAL(BANG);
		case '/':
			SCAN_CHAR_EQUAL(SLASH);
		case '%':
			SCAN_CHAR_EQUAL(PERCENT);

		// Double characters (`x`, `xx`, or `x=`).
		case '&':
			if (peek(1) == '"' || peek(1) == '\'') {
				// `StringName` literal.
				return scan_string();
			}
			SCAN_TWO_CHARS_EQUAL_SHORT(AMPERSAND);
		case '|':
			SCAN_TWO_CHARS_EQUAL_SHORT(PIPE);

		// Triple characters (`x`, `xx`, `x=`, or `xx=`).
		case '*':
			SCAN_TWO_CHARS_EQUAL_FULL(STAR);
		case '<':
			CHECK_VCS_CONFLICT_MARKER();
			SCAN_TWO_CHARS_EQUAL_FULL(LESS);
		case '>':
			CHECK_VCS_CONFLICT_MARKER();
			SCAN_TWO_CHARS_EQUAL_FULL(GREATER);

		// Annotation.
		case '@':
			return scan_annotation();

		// String literal.
		case '"':
		case '\'':
			return scan_string();

		default:
			if (is_digit(first_char)) {
				return scan_number();
			}
			if (first_char == 'r' && (peek(1) == '"' || peek(1) == '\'')) {
				// Raw string literal.
				return scan_string();
			}
			if (is_unicode_identifier_start(first_char)) {
				return scan_word();
			}

			advance();
			if (is_whitespace(first_char)) {
				return make_error(vformat(
						R"(Invalid whitespace character U+%04X.)",
						static_cast<int32_t>(first_char)));
			} else {
				return make_error(vformat(
						R"(Invalid character "%c" (U+%04X).)",
						first_char,
						static_cast<int32_t>(first_char)));
			}
	}

#undef SCAN_CHAR
#undef SCAN_CHAR_EQUAL
#undef SCAN_TWO_CHARS_EQUAL_SHORT
#undef SCAN_TWO_CHARS_EQUAL_FULL
#undef CHECK_VCS_CONFLICT_MARKER
}

void Tokenizer::push_expression_indented_block() {
	indent_stack_stack.push_back(state.indent_stack);
	//state.indent_stack = Vector<Indent>(); // TODO
}

void Tokenizer::pop_expression_indented_block() {
	ERR_FAIL_COND(indent_stack_stack.is_empty());

	state.indent_stack = indent_stack_stack[indent_stack_stack.size() - 1];
	indent_stack_stack.resize(indent_stack_stack.size() - 1);
}

Tokenizer::Tokenizer(const String &p_source, int p_tab_size) {
	ERR_FAIL_COND(p_tab_size <= 0);

	source = p_source;
	begin_ptr = source.is_empty() ? nullptr : source.ptr();
	end_ptr = begin_ptr + source.length();
	tab_size = p_tab_size;

	state.current_ptr = begin_ptr;
}
