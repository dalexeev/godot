/**************************************************************************/
/*  parser.cpp                                                            */
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

#include "parser.h"

#include "servers/text/text_server.h"

using namespace gdscript;

const Parser::ParseRule *Parser::get_rule(Token::Type p_token_type) {
#define FN(m_name) &Parser::_parse_##m_name
#define PREC Precedence

	// clang-format off
	static const ParseRule rules[] = {
		// PREFIX                   INFIX                     PRECEDENCE (for infix)
		{ nullptr,                  nullptr,                  PREC::NONE           }, // EMPTY
		// --- Basic ---
		{ nullptr,                  nullptr,                  PREC::NONE           }, // ANNOTATION
		{ FN(identifier),           nullptr,                  PREC::NONE           }, // IDENTIFIER
		{ FN(literal),              nullptr,                  PREC::NONE           }, // LITERAL
		// --- Comparison ---
		{ nullptr,                  FN(binary_operator),      PREC::COMPARISON     }, // LESS
		{ nullptr,                  FN(binary_operator),      PREC::COMPARISON     }, // LESS_EQUAL
		{ nullptr,                  FN(binary_operator),      PREC::COMPARISON     }, // GREATER
		{ nullptr,                  FN(binary_operator),      PREC::COMPARISON     }, // GREATER_EQUAL
		{ nullptr,                  FN(binary_operator),      PREC::COMPARISON     }, // EQUAL_EQUAL
		{ nullptr,                  FN(binary_operator),      PREC::COMPARISON     }, // BANG_EQUAL
		// --- Logical ---
		{ nullptr,                  FN(binary_operator),      PREC::LOGIC_AND      }, // AND
		{ nullptr,                  FN(binary_operator),      PREC::LOGIC_OR       }, // OR
		{ FN(unary_operator),       FN(not_in_operator),      PREC::CONTENT_TEST   }, // NOT
		{ nullptr,                  FN(binary_operator),      PREC::LOGIC_AND      }, // AMPERSAND_AMPERSAND
		{ nullptr,                  FN(binary_operator),      PREC::LOGIC_OR       }, // PIPE_PIPE
		{ FN(unary_operator),       nullptr,                  PREC::NONE           }, // BANG
		// --- Bitwise ---
		{ nullptr,                  FN(binary_operator),      PREC::BIT_AND        }, // AMPERSAND
		{ nullptr,                  FN(binary_operator),      PREC::BIT_OR         }, // PIPE
		{ FN(unary_operator),       nullptr,                  PREC::NONE           }, // TILDE
		{ nullptr,                  FN(binary_operator),      PREC::BIT_XOR        }, // CARET
		{ nullptr,                  FN(binary_operator),      PREC::BIT_SHIFT      }, // LESS_LESS
		{ nullptr,                  FN(binary_operator),      PREC::BIT_SHIFT      }, // GREATER_GREATER
		// --- Math ---
		{ FN(unary_operator),       FN(binary_operator),      PREC::ADDITION       }, // PLUS
		{ FN(unary_operator),       FN(binary_operator),      PREC::ADDITION       }, // MINUS
		{ nullptr,                  FN(binary_operator),      PREC::FACTOR         }, // STAR
		{ nullptr,                  FN(binary_operator),      PREC::POWER          }, // STAR_STAR
		{ nullptr,                  FN(binary_operator),      PREC::FACTOR         }, // SLASH
		{ FN(get_node),             FN(binary_operator),      PREC::FACTOR         }, // PERCENT
		// --- Assignment ---
		{ nullptr,                  FN(assignment),           PREC::ASSIGNMENT     }, // EQUAL
		{ nullptr,                  FN(assignment),           PREC::ASSIGNMENT     }, // PLUS_EQUAL
		{ nullptr,                  FN(assignment),           PREC::ASSIGNMENT     }, // MINUS_EQUAL
		{ nullptr,                  FN(assignment),           PREC::ASSIGNMENT     }, // STAR_EQUAL
		{ nullptr,                  FN(assignment),           PREC::ASSIGNMENT     }, // STAR_STAR_EQUAL
		{ nullptr,                  FN(assignment),           PREC::ASSIGNMENT     }, // SLASH_EQUAL
		{ nullptr,                  FN(assignment),           PREC::ASSIGNMENT     }, // PERCENT_EQUAL
		{ nullptr,                  FN(assignment),           PREC::ASSIGNMENT     }, // LESS_LESS_EQUAL
		{ nullptr,                  FN(assignment),           PREC::ASSIGNMENT     }, // GREATER_GREATER_EQUAL
		{ nullptr,                  FN(assignment),           PREC::ASSIGNMENT     }, // AMPERSAND_EQUAL
		{ nullptr,                  FN(assignment),           PREC::ASSIGNMENT     }, // PIPE_EQUAL
		{ nullptr,                  FN(assignment),           PREC::ASSIGNMENT     }, // CARET_EQUAL
		// --- Control flow ---
		{ nullptr,                  FN(ternary_operator),     PREC::TERNARY        }, // IF
		{ nullptr,                  nullptr,                  PREC::NONE           }, // ELIF
		{ nullptr,                  nullptr,                  PREC::NONE           }, // ELSE
		{ nullptr,                  nullptr,                  PREC::NONE           }, // FOR
		{ nullptr,                  nullptr,                  PREC::NONE           }, // WHILE
		{ nullptr,                  nullptr,                  PREC::NONE           }, // BREAK
		{ nullptr,                  nullptr,                  PREC::NONE           }, // CONTINUE
		{ nullptr,                  nullptr,                  PREC::NONE           }, // PASS
		{ nullptr,                  nullptr,                  PREC::NONE           }, // RETURN
		{ nullptr,                  nullptr,                  PREC::NONE           }, // MATCH
		{ nullptr,                  nullptr,                  PREC::NONE           }, // WHEN
		// --- Keywords ---
		{ nullptr,                  FN(cast),                 PREC::CAST           }, // AS
		{ nullptr,                  nullptr,                  PREC::NONE           }, // ASSERT
		{ FN(await),                nullptr,                  PREC::NONE           }, // AWAIT
		{ nullptr,                  nullptr,                  PREC::NONE           }, // BREAKPOINT
		{ nullptr,                  nullptr,                  PREC::NONE           }, // CLASS
		{ nullptr,                  nullptr,                  PREC::NONE           }, // CLASS_NAME
		{ nullptr,                  nullptr,                  PREC::NONE           }, // TK_CONST
		{ nullptr,                  nullptr,                  PREC::NONE           }, // ENUM
		{ nullptr,                  nullptr,                  PREC::NONE           }, // EXTENDS
		{ FN(lambda),               nullptr,                  PREC::NONE           }, // FUNC
		{ nullptr,                  FN(binary_operator),      PREC::CONTENT_TEST   }, // TK_IN
		{ nullptr,                  FN(type_test),            PREC::TYPE_TEST      }, // IS
		{ nullptr,                  nullptr,                  PREC::NONE           }, // NAMESPACE
		{ FN(self),                 nullptr,                  PREC::NONE           }, // SELF
		{ nullptr,                  nullptr,                  PREC::NONE           }, // SIGNAL
		{ FN(super),                nullptr,                  PREC::NONE           }, // SUPER
		{ nullptr,                  nullptr,                  PREC::NONE           }, // TRAIT
		{ nullptr,                  nullptr,                  PREC::NONE           }, // USING
		{ nullptr,                  nullptr,                  PREC::NONE           }, // VAR
		{ FN(yield),                nullptr,                  PREC::NONE           }, // YIELD
		// --- Punctuation ---
		{ FN(array),                FN(subscript),            PREC::SUBSCRIPT      }, // BRACKET_OPEN
		{ nullptr,                  nullptr,                  PREC::NONE           }, // BRACKET_CLOSE
		{ FN(dictionary),           nullptr,                  PREC::NONE           }, // BRACE_OPEN
		{ nullptr,                  nullptr,                  PREC::NONE           }, // BRACE_CLOSE
		{ FN(grouping),             FN(call),                 PREC::CALL           }, // PARENTHESIS_OPEN
		{ nullptr,                  nullptr,                  PREC::NONE           }, // PARENTHESIS_CLOSE
		{ nullptr,                  nullptr,                  PREC::NONE           }, // COMMA
		{ nullptr,                  nullptr,                  PREC::NONE           }, // SEMICOLON
		{ nullptr,                  FN(attribute),            PREC::ATTRIBUTE      }, // PERIOD
		{ nullptr,                  nullptr,                  PREC::NONE           }, // PERIOD_PERIOD
		{ nullptr,                  nullptr,                  PREC::NONE           }, // PERIOD_PERIOD_PERIOD
		{ nullptr,                  nullptr,                  PREC::NONE           }, // COLON
		{ FN(get_node),             nullptr,                  PREC::NONE           }, // DOLLAR
		{ nullptr,                  nullptr,                  PREC::NONE           }, // FORWARD_ARROW
		{ nullptr,                  nullptr,                  PREC::NONE           }, // UNDERSCORE
		// --- Whitespace ---
		{ nullptr,                  nullptr,                  PREC::NONE           }, // COMMENT
		{ nullptr,                  nullptr,                  PREC::NONE           }, // NEWLINE
		{ nullptr,                  nullptr,                  PREC::NONE           }, // INDENTATION
		{ nullptr,                  nullptr,                  PREC::NONE           }, // INDENT
		{ nullptr,                  nullptr,                  PREC::NONE           }, // DEDENT
		// --- Error message improvement ---
		{ nullptr,                  nullptr,                  PREC::NONE           }, // VCS_CONFLICT_MARKER
		{ nullptr,                  nullptr,                  PREC::NONE           }, // BACKTICK
		{ nullptr,                  FN(question_mark),        PREC::CAST           }, // QUESTION_MARK
		// --- Special ---
		{ nullptr,                  nullptr,                  PREC::NONE           }, // ERROR
		{ nullptr,                  nullptr,                  PREC::NONE           }, // TK_EOF
	};
	// clang-format on

#undef FN
#undef PREC

	static_assert(
			std_size(rules) == (int)Token::Type::MAX,
			"Amount of parse rules don't match the amount of token types.");

	// Let's assume this is never invalid, since nothing generates `Token::Type::MAX`.
	return &rules[(int)p_token_type];
}

#define HANDLE_TOKEN(m_token, m_operation) \
	case Token::Type::m_token: \
		return Operation::m_operation

AST::AssignmentNode::Operation Parser::get_assignment_operation(Token::Type p_token_type) {
	// clang-format off
	switch (p_token_type) {
		using Operation = AST::AssignmentNode::Operation;

		HANDLE_TOKEN( EQUAL,                 NONE            );
		HANDLE_TOKEN( PLUS_EQUAL,            ADDITION        );
		HANDLE_TOKEN( MINUS_EQUAL,           SUBTRACTION     );
		HANDLE_TOKEN( STAR_EQUAL,            MULTIPLICATION  );
		HANDLE_TOKEN( SLASH_EQUAL,           DIVISION        );
		HANDLE_TOKEN( PERCENT_EQUAL,         MODULO          );
		HANDLE_TOKEN( STAR_STAR_EQUAL,       POWER           );
		HANDLE_TOKEN( LESS_LESS_EQUAL,       BIT_SHIFT_LEFT  );
		HANDLE_TOKEN( GREATER_GREATER_EQUAL, BIT_SHIFT_RIGHT );
		HANDLE_TOKEN( AMPERSAND_EQUAL,       BIT_AND         );
		HANDLE_TOKEN( PIPE_EQUAL,            BIT_OR          );
		HANDLE_TOKEN( CARET_EQUAL,           BIT_XOR         );

		default:
			ERR_FAIL_V_MSG(Operation::NONE, "GDScript bug: Invalid assignment token.");
	}
	// clang-format on
}

AST::BinaryOperatorNode::Operation Parser::get_binary_operation(Token::Type p_token_type) {
	// clang-format off
	switch (p_token_type) {
		using Operation = AST::BinaryOperatorNode::Operation;

		HANDLE_TOKEN( PLUS,                ADDITION           );
		HANDLE_TOKEN( MINUS,               SUBTRACTION        );
		HANDLE_TOKEN( STAR,                MULTIPLICATION     );
		HANDLE_TOKEN( SLASH,               DIVISION           );
		HANDLE_TOKEN( PERCENT,             MODULO             );
		HANDLE_TOKEN( STAR_STAR,           POWER              );
		HANDLE_TOKEN( LESS_LESS,           BIT_LEFT_SHIFT     );
		HANDLE_TOKEN( GREATER_GREATER,     BIT_RIGHT_SHIFT    );
		HANDLE_TOKEN( AMPERSAND,           BIT_AND            );
		HANDLE_TOKEN( PIPE,                BIT_OR             );
		HANDLE_TOKEN( CARET,               BIT_XOR            );
		HANDLE_TOKEN( AND,                 LOGIC_AND          );
		HANDLE_TOKEN( AMPERSAND_AMPERSAND, LOGIC_AND          );
		HANDLE_TOKEN( OR,                  LOGIC_OR           );
		HANDLE_TOKEN( PIPE_PIPE,           LOGIC_OR           );
		HANDLE_TOKEN( TK_IN,               CONTENT_TEST       );
		HANDLE_TOKEN( EQUAL_EQUAL,         COMP_EQUAL         );
		HANDLE_TOKEN( BANG_EQUAL,          COMP_NOT_EQUAL     );
		HANDLE_TOKEN( LESS,                COMP_LESS          );
		HANDLE_TOKEN( LESS_EQUAL,          COMP_LESS_EQUAL    );
		HANDLE_TOKEN( GREATER,             COMP_GREATER       );
		HANDLE_TOKEN( GREATER_EQUAL,       COMP_GREATER_EQUAL );

		default:
			ERR_FAIL_V_MSG(Operation::ADDITION, "GDScript bug: Invalid binary operator token.");
	}
	// clang-format on
}

AST::UnaryOperatorNode::Operation Parser::get_unary_operation(Token::Type p_token_type) {
	// clang-format off
	switch (p_token_type) {
		using Operation = AST::UnaryOperatorNode::Operation;

		HANDLE_TOKEN( PLUS,  POSITIVE   );
		HANDLE_TOKEN( MINUS, NEGATIVE   );
		HANDLE_TOKEN( TILDE, COMPLEMENT );
		HANDLE_TOKEN( NOT,   LOGIC_NOT  );
		HANDLE_TOKEN( BANG,  LOGIC_NOT  );

		default:
			ERR_FAIL_V_MSG(Operation::POSITIVE, "GDScript bug: Invalid unary operator token.");
	}
	// clang-format on
}

#undef HANDLE_TOKEN

// ----- Utilities -----

AST::IdentifierNode *Parser::make_recovery_identifier() {
	AST::IdentifierNode *identifier = ast.alloc_node<AST::IdentifierNode>();
	identifier->is_recovery = true;
	identifier->name = "<recovery identifier>";
	identifier->source_region = get_previous_token_end_region();

	return identifier;
}

AST::ExpressionNode *Parser::make_recovery_expression() {
	AST::IdentifierNode *identifier = ast.alloc_node<AST::IdentifierNode>();
	identifier->is_recovery = true;
	identifier->name = "<recovery expression>";
	identifier->source_region = get_previous_token_end_region();

	return identifier;
}

AST::TypeNode *Parser::make_recovery_type() {
	AST::TypeNode *type = ast.alloc_node<AST::TypeNode>();
	type->is_recovery = true;
	type->source_region = get_previous_token_end_region();

	type->identifier = ast.alloc_node<AST::IdentifierNode>();
	type->identifier->is_recovery = true;
	type->identifier->name = "<recovery type>";
	type->identifier->source_region = get_previous_token_end_region();

	return type;
}

void Parser::push_error(const SourceRegion &p_origin, const String &p_message, bool p_panic) {
	if (p_panic) {
		if (panic_mode) {
			return; // Ignore cascading errors.
		}
		panic_mode = true;
	}

	ast.get_diagnostic_list().push_error(p_origin, p_message);
}

void Parser::push_error(AST::Node *p_origin, const String &p_message, bool p_panic) {
	if (unlikely(p_origin == nullptr)) {
		ERR_PRINT("GDScript bug: Trying to push error on nullptr.");
		push_error(p_message, p_panic);
		return;
	}

#ifdef DEV_ENABLED
	if (unlikely(node_stack.has(p_origin))) {
		ERR_PRINT("GDScript bug: Trying to push error on a node with incomplete extents.");
		push_error(p_message, p_panic);
		return;
	}
#endif // DEV_ENABLED

	push_error(p_origin->source_region, p_message, p_panic);
}

void Parser::push_warning(const SourceRegion &p_origin, WarningDB::Code p_code, const Array &p_symbols) {
	ast.get_diagnostic_list().push_warning(p_origin, p_code, p_symbols);
}

Tokenizer::Token Parser::_scan() {
	Token token;

	bool found = false;
	while (!found) {
		token = tokenizer.scan();

		switch (token.type) {
			case Token::Type::COMMENT:
				// Nothing to do.
				break;
			case Token::Type::NEWLINE:
				if (multiline_mode) {
					// TODO
				}
				break;
			case Token::Type::INDENTATION:
				if (multiline_mode) {
					// TODO
				}
				break;
			case Token::Type::ERROR:
				push_error(token.source_region, token.data);
				break;
			default:
				found = true;
				break;
		}

		for (const Token::InnerError &inner_error : token.inner_errors) {
			push_error(inner_error.source_region, inner_error.message, false);
		}
	}

	return token;
}

// This method allows you to look ahead one token to decide what the **current** token is. In most cases this method
// is not required; you usually decide what the current token is as soon as you encounter it, depending on the context.
// The GDScript grammar and parser are intended to be simple, so looking ahead more than one token is not provided.
// NOTE: Be careful when using this method in combination with multiline mode switching, since whitespace tokens
// may be ignored depending on the multiline mode.
bool Parser::check_next(Token::Type p_token_type) {
	if (next_token.type == Token::Type::EMPTY) {
		next_token = _scan();
	}

	return next_token.type == p_token_type;
}

void Parser::advance() {
	ERR_FAIL_COND_MSG(is_at_end(), "GDScript bug: Trying to advance past the end of stream.");

	lambda_ended = false; // Empty marker since we're past the end in any case.

	if (current_token.type != Token::Type::DEDENT) {
		previous_token_end = current_token.source_region.end;
	}

	if (next_token.type == Token::Type::EMPTY) {
		current_token = _scan();
	} else {
		current_token = next_token;
		next_token = Token();
	}
}

bool Parser::match(Token::Type p_token_type) {
	if (check(p_token_type)) {
		advance(); // Consume current token.
		return true;
	}
	return false;
}

void Parser::synchronize_statement() {
	while (panic_mode) {
		if (is_at_end()) {
			panic_mode = false;
			return;
		}

		if (check(Token::Type::NEWLINE) || check(Token::Type::SEMICOLON)) {
			advance(); // Consume newline or semicolon.
			panic_mode = false;
			return;
		}

		// TODO
		/*if (current_token.is_inline) {
			advance(); // Consume current token.
			continue;
		}*/

		switch (current_token.type) {
			case Token::Type::USING:
			case Token::Type::CLASS:
			case Token::Type::ENUM:
			case Token::Type::TK_CONST:
			case Token::Type::VAR:
			case Token::Type::FUNC:
			case Token::Type::SIGNAL:
			case Token::Type::IF:
			case Token::Type::FOR:
			case Token::Type::WHILE:
			case Token::Type::MATCH:
			case Token::Type::BREAK:
			case Token::Type::CONTINUE:
			case Token::Type::RETURN:
			case Token::Type::PASS:
			case Token::Type::ANNOTATION:
				panic_mode = false;
				return;
			default:
				advance(); // Consume current token.
				break;
		}
	}
}

void Parser::synchronize(const LocalVector<Token::Type> &p_token_types) {
	while (panic_mode) {
		if (is_at_end()) {
			panic_mode = false;
			return;
		}

		for (Token::Type token_type : p_token_types) {
			if (check(token_type)) {
				panic_mode = false;
				return;
			}
		}

		switch (current_token.type) {
			case Token::Type::IDENTIFIER:
			case Token::Type::LITERAL:
			case Token::Type::LESS:
			case Token::Type::LESS_EQUAL:
			case Token::Type::GREATER:
			case Token::Type::GREATER_EQUAL:
			case Token::Type::EQUAL_EQUAL:
			case Token::Type::BANG_EQUAL:
			case Token::Type::AND:
			case Token::Type::OR:
			case Token::Type::NOT:
			case Token::Type::AMPERSAND_AMPERSAND:
			case Token::Type::PIPE_PIPE:
			case Token::Type::BANG:
			case Token::Type::AMPERSAND:
			case Token::Type::PIPE:
			case Token::Type::TILDE:
			case Token::Type::CARET:
			case Token::Type::LESS_LESS:
			case Token::Type::GREATER_GREATER:
			case Token::Type::PLUS:
			case Token::Type::MINUS:
			case Token::Type::STAR:
			case Token::Type::STAR_STAR:
			case Token::Type::SLASH:
			case Token::Type::PERCENT:
			case Token::Type::EQUAL:
			case Token::Type::PLUS_EQUAL:
			case Token::Type::MINUS_EQUAL:
			case Token::Type::STAR_EQUAL:
			case Token::Type::STAR_STAR_EQUAL:
			case Token::Type::SLASH_EQUAL:
			case Token::Type::PERCENT_EQUAL:
			case Token::Type::LESS_LESS_EQUAL:
			case Token::Type::GREATER_GREATER_EQUAL:
			case Token::Type::AMPERSAND_EQUAL:
			case Token::Type::PIPE_EQUAL:
			case Token::Type::CARET_EQUAL:
			case Token::Type::WHEN:
			case Token::Type::AS:
			case Token::Type::AWAIT:
			case Token::Type::TK_IN:
			case Token::Type::IS:
			case Token::Type::SELF:
			case Token::Type::SUPER:
			case Token::Type::PERIOD:
			case Token::Type::PERIOD_PERIOD:
			case Token::Type::PERIOD_PERIOD_PERIOD:
			case Token::Type::DOLLAR:
			case Token::Type::UNDERSCORE:
			case Token::Type::BACKTICK:
			case Token::Type::QUESTION_MARK:
			case Token::Type::ERROR:
				advance(); // Safe to skip.
				break;
			case Token::Type::ANNOTATION:
			case Token::Type::IF:
			case Token::Type::ELIF:
			case Token::Type::ELSE:
			case Token::Type::FOR:
			case Token::Type::WHILE:
			case Token::Type::BREAK:
			case Token::Type::CONTINUE:
			case Token::Type::PASS:
			case Token::Type::RETURN:
			case Token::Type::MATCH:
			case Token::Type::ASSERT:
			case Token::Type::BREAKPOINT:
			case Token::Type::CLASS:
			case Token::Type::CLASS_NAME:
			case Token::Type::TK_CONST:
			case Token::Type::ENUM:
			case Token::Type::EXTENDS:
			case Token::Type::FUNC:
			case Token::Type::NAMESPACE:
			case Token::Type::SIGNAL:
			case Token::Type::TRAIT:
			case Token::Type::USING:
			case Token::Type::VAR:
			case Token::Type::YIELD:
			case Token::Type::BRACKET_OPEN:
			case Token::Type::BRACKET_CLOSE:
			case Token::Type::BRACE_OPEN:
			case Token::Type::BRACE_CLOSE:
			case Token::Type::PARENTHESIS_OPEN:
			case Token::Type::PARENTHESIS_CLOSE:
			case Token::Type::COMMA:
			case Token::Type::SEMICOLON:
			case Token::Type::COLON:
			case Token::Type::FORWARD_ARROW:
			case Token::Type::NEWLINE:
			case Token::Type::INDENT:
			case Token::Type::DEDENT:
			case Token::Type::VCS_CONFLICT_MARKER:
				return; // Failed to recover.
			case Token::Type::EMPTY:
			case Token::Type::COMMENT:
			case Token::Type::INDENTATION:
			case Token::Type::TK_EOF:
			case Token::Type::MAX:
				ERR_FAIL_MSG("GDScript bug: Unexpected token type.");
		}
	}
}

void Parser::expect(
		const LocalVector<Token::Type> &p_token_types,
		const String &p_context,
		bool p_allow_end,
		bool p_suggest_end) {
	bool found = false;
	for (Token::Type token_type : p_token_types) {
		if (check(token_type)) {
			found = true;
			break;
		}
	}
	if (!found && p_allow_end) {
		found = check(Token::Type::NEWLINE) || check(Token::Type::SEMICOLON) || is_at_end();
	}

	if (!found) {
		Vector<String> expected_symbols;
		expected_symbols.resize(p_token_types.size() + (p_allow_end ? 1 : 0));

		int index = 0;
		for (Token::Type token_type : p_token_types) {
			expected_symbols.write[index] = Token::get_name_static(token_type);
			index++;
		}

		if (p_allow_end && p_suggest_end) {
			expected_symbols.write[index] = "end of statement";
		}

		push_error(vformat(
				"Expected %s after %s, found %s instead.",
				get_alternative_list(expected_symbols),
				p_context,
				current_token.get_name()));
	}

	synchronize(p_token_types);
}

// TODO: Require newline after semicolon if `!is_multiline_container()`.
// TODO: `vformat(R"(Expected newline after ";" at the end of %s.)", p_context)`.
void Parser::end_statement(const String &p_context) {
	bool found = false;
	while (check(Token::Type::NEWLINE) || check(Token::Type::SEMICOLON) || lambda_ended) {
		found = true;
		panic_mode = false;
		if (lambda_ended) {
			lambda_ended = false; // Consume this "token".
		} else {
			advance(); // Consume newline or semicolon.
		}
	}
	// TODO
	/*if (!found && get_expression_indented_block_depth() > 0) {
		// Mark the lambda as done since we found something else to end the statement.
		lambda_ended = true;
		found = true;
	}*/
	if (!found && !is_at_end()) {
		push_error(vformat(
				"Expected end of statement after %s, found %s instead.",
				p_context,
				current_token.get_name()));
	}
}

void Parser::push_context(const AST::Context &p_context) {
	context_stack.push_back(p_context);
	current_context = p_context;
}

void Parser::pop_context() {
	ERR_FAIL_COND_MSG(context_stack.is_empty(), "GDScript bug: Trying to pop from empty context stack.");

	context_stack.resize(context_stack.size() - 1);
	current_context = context_stack.is_empty() ? AST::Context() : context_stack[context_stack.size() - 1];
}

void Parser::push_node(AST::Node *p_node, const SourceRegion &p_initial_region) {
	ERR_FAIL_NULL(p_node);

	p_node->source_region = p_initial_region;
	node_stack.push_back(p_node);
}

void Parser::pop_node(AST::Node *p_node) {
	ERR_FAIL_COND_MSG(node_stack.is_empty(), "GDScript bug: Trying to pop from empty extents tracking stack.");

	AST::Node *last_node = node_stack[node_stack.size() - 1];
	node_stack.resize(node_stack.size() - 1);

	ERR_FAIL_COND_MSG(p_node != last_node, "GDScript bug: Mismatch in extents tracking stack.");

	if (p_node->source_region.start.position <= previous_token_end.position) {
		p_node->source_region.end = previous_token_end;
	} else {
		ERR_PRINT(vformat("GDScript bug: Invalid source region for %s.", p_node->get_node_raw_type_name()));
		p_node->source_region = get_previous_token_end_region();
	}
}

void Parser::push_container(AST::Node *p_node, bool p_is_multiline) {
	ERR_FAIL_NULL(p_node);

	container_stack.push_back({ p_node, p_is_multiline });
}

void Parser::pop_container(AST::Node *p_node) {
	ERR_FAIL_COND_MSG(container_stack.is_empty(), "GDScript bug: Trying to pop from empty container node stack.");

	ERR_FAIL_NULL(p_node);

	AST::Node *last_node = container_stack[container_stack.size() - 1].first;
	container_stack.resize(container_stack.size() - 1);

	ERR_FAIL_COND_MSG(p_node != last_node, "GDScript bug: Mismatch in container node stack.");
}

void Parser::push_multiline_mode(bool p_mode) {
	multiline_mode_stack.push_back(p_mode);
	multiline_mode = p_mode;
}

void Parser::pop_multiline_mode(Token::Type p_token_type, const String &p_context) {
	ERR_FAIL_COND_MSG(multiline_mode_stack.is_empty(), "GDScript bug: Trying to pop from empty multiline mode stack.");

	multiline_mode_stack.resize(multiline_mode_stack.size() - 1);
	multiline_mode = multiline_mode_stack.is_empty() ? false : multiline_mode_stack[multiline_mode_stack.size() - 1];

	if (p_token_type != Token::Type::MAX && !match(p_token_type)) {
		const String msg = vformat(R"*(Expected "%s" after %s.)*", Token::get_name_static(p_token_type), p_context);
		push_error(get_previous_token_end_region(), msg);

		// TODO
		/*if (!multiline_mode) {
			readvance();
		}*/
	}
}

// ----- Main  -----

void Parser::parse_script(bool p_parse_body) {
	ERR_FAIL_COND(ast.get_root() != nullptr);

	AST::RootNode *root = ast.alloc_node<AST::RootNode>();
	ast.set_root(root);

	push_node(root);

	AST::ClassNode *main_class = ast.alloc_node<AST::ClassNode>();
	root->main_class = main_class;
	main_class->parent_node = root;

	push_node(main_class);
	push_context(current_context.with_class(main_class));

	bool can_have_class_name_or_extends = true;

	while (!is_at_end()) {
		if (check(Token::Type::ANNOTATION)) {
			AST::AnnotationNode *annotation = parse_annotation();
			if (annotation->applies_to(AnnotationTarget::CLASS)) {
				// We do not know in advance what the annotation will be applied to:
				// the main class or the subsequent inner class. If we encounter `class_name`, `extends`,
				// or pure `SCRIPT` annotation, then it's the main class, otherwise it's an inner class.
				pending_annotations.push_back(annotation);
			} else if (annotation->applies_to(AnnotationTarget::SCRIPT)) {
				push_applicable_annotations(main_class, pending_annotations);
				if (annotation->name == SNAME("@tool")) {
					root->annotations.push_back(annotation);
					annotation->parent_node = root;
				} else {
					push_error(annotation, "Unexpected script annotation.", false);
				}
			} else if (annotation->applies_to(AnnotationTarget::STANDALONE)) {
				if (AST::is_export_grouping_annotation(annotation->name)) {
					main_class->members.push_back(annotation);
					annotation->parent_node = main_class;
					// This annotation must appear after `SCRIPT` annotations and `class_name`/`extends`.
					can_have_class_name_or_extends = false;
					break;
				} else {
					handle_annotation_default(annotation);
				}
			} else if (annotation->applies_to(AnnotationTarget::CLASS_LEVEL)) {
				pending_annotations.push_back(annotation);
				// This annotation must appear after `SCRIPT` annotations and `class_name`/`extends`.
				can_have_class_name_or_extends = false;
				break;
			} else {
				handle_annotation_default(annotation);
			}
		} else if (check(Token::Type::INDENT)) {
			push_error("Unexpected indent in class body.", false);
			advance(); // Consume extra indent.
		} else if (check(Token::Type::DEDENT)) {
			advance(); // Consume extra dedent.
		} else if (check(Token::Type::LITERAL) && current_token.is_string_literal()) {
			// Allow strings in class body as multiline comments.
			const Tokenizer::Token string_literal = current_token;
			advance(); // Consume string literal.
			if (!match(Token::Type::NEWLINE)) {
				push_error(string_literal, "Expected newline after comment string.");
			}
		} else {
			break;
		}

		synchronize_statement();
	}

	// TODO: Check `can_have_class_name_or_extends`?
	if (check(Token::Type::CLASS_NAME) || check(Token::Type::EXTENDS)) {
		// Change the start position only if `class_name`/`extends` is present.
		main_class->source_region.start = current_token.source_region.start;
	}

	while (can_have_class_name_or_extends) {
		// Order here doesn't matter, but there should be only one of each at most.
		switch (current_token.type) {
			case Token::Type::CLASS_NAME:
				push_applicable_annotations(main_class, pending_annotations);
				parse_class_name();
				break;
			case Token::Type::EXTENDS:
				push_applicable_annotations(main_class, pending_annotations);
				parse_extends();
				end_statement("superclass");
				break;
			case Token::Type::TK_EOF:
				push_applicable_annotations(main_class, pending_annotations);
				can_have_class_name_or_extends = false;
				break;
			case Token::Type::INDENT:
				push_error("Unexpected indent in class body.", false);
				advance(); // Consume extra indent.
				break;
			case Token::Type::DEDENT:
				advance(); // Consume extra dedent.
				break;
			case Token::Type::LITERAL:
				if (current_token.is_string_literal()) {
					// Allow strings in class body as multiline comments.
					const Tokenizer::Token string_literal = current_token;
					advance(); // Consume string literal.
					if (!match(Token::Type::NEWLINE)) {
						push_error(string_literal, "Expected newline after comment string.");
					}
					break;
				}
				[[fallthrough]];
			default:
				// No tokens are allowed between `SCRIPT` annotations and `class_name`/`extends`.
				can_have_class_name_or_extends = false;
				break;
		}

		synchronize_statement();
	}

	if (!p_parse_body) {
		//push_applicable_annotations(main_class, pending_annotations); // TODO
		return;
	}

	push_container(main_class, true);
	parse_class_body();
	pop_container(main_class);

	while (check(Token::Type::DEDENT)) {
		advance(); // Consume extra dedents.
	}

	pop_node(main_class);
	pop_context();

	pop_node(root);

	push_applicable_annotations(main_class, pending_annotations);
	ast.close_warning_regions(current_token.source_region.end);
	//root->comments = tokenizer.get_comments(); // TODO

	if (!is_at_end()) {
		ERR_PRINT("GDScript bug: Expected end of file.");
	}
	if (!node_stack.is_empty()) {
		ERR_PRINT("GDScript bug: Imbalanced extents tracking stack.");
	}
	if (!container_stack.is_empty()) {
		ERR_PRINT("GDScript bug: Imbalanced container node stack.");
	}
	if (!multiline_mode_stack.is_empty()) {
		ERR_PRINT("GDScript bug: Imbalanced multiline mode stack.");
	}
}

void Parser::parse_class_body() {
	const Token first_body_token = current_token;
	bool has_valid_body = false;
	bool body_ended = false;
	int extra_indent_count = 0;

	while (!body_ended) {
		List<AST::AnnotationNode *> member_annotations;

		// Use annotations from `parse_script()` for the first member.
		if (unlikely(!pending_annotations.is_empty())) {
			for (AST::AnnotationNode *&annotation : pending_annotations) {
				member_annotations.push_back(annotation);
			}
			pending_annotations.clear();
		}

		while (check(Token::Type::ANNOTATION)) {
			AST::AnnotationNode *annotation = parse_annotation();
			if (annotation->applies_to(AnnotationTarget::CLASS_LEVEL)) {
				member_annotations.push_back(annotation);
			} else if (annotation->applies_to(AnnotationTarget::STANDALONE)) {
				if (AST::is_export_grouping_annotation(annotation->name)) {
					current_context.get_class()->members.push_back(annotation);
					annotation->parent_node = current_context.get_class();
				} else {
					handle_annotation_default(annotation);
				}
			} else {
				handle_annotation_default(annotation);
			}
		}

		AST::Node *member = nullptr;

		switch (current_token.type) {
			case Token::Type::USING:
				member = Parser::parse_type_alias();
				break;
			case Token::Type::CLASS:
				member = Parser::parse_class();
				break;
			case Token::Type::ENUM:
				member = Parser::parse_enum();
				break;
			case Token::Type::TK_CONST:
				member = Parser::parse_constant();
				break;
			case Token::Type::VAR:
				member = Parser::parse_variable(true);
				break;
			case Token::Type::FUNC:
				member = Parser::parse_function(FunctionType::METHOD);
				break;
			case Token::Type::SIGNAL:
				member = Parser::parse_signal();
				break;
			case Token::Type::PASS:
				has_valid_body = true;
				advance(); // Consume `pass`.
				end_statement(R"("pass")");
				break;
			case Token::Type::INDENT:
				extra_indent_count++;
				push_error("Unexpected indent in class body.", false);
				advance(); // Consume extra indent.
				break;
			case Token::Type::DEDENT:
				if (extra_indent_count <= 0) {
					body_ended = true;
					// No need to consume, will be done by the caller.
				} else {
					extra_indent_count--;
					advance(); // Consume extra dedent.
				}
				break;
			case Token::Type::TK_EOF:
				body_ended = true;
				break;
			case Token::Type::LITERAL:
				// TODO: Check `p_is_multiline`.
				if (current_token.is_string_literal()) {
					// Allow strings in class body as multiline comments.
					const Token string_literal = current_token;
					advance(); // Consume string literal.
					if (!match(Token::Type::NEWLINE)) {
						push_error(string_literal, "Expected newline after comment string.");
					}
					break;
				}
				[[fallthrough]];
			default:
				if (check(Token::Type::IDENTIFIER)) {
					const SourceRegion source_region = current_token.source_region;
					const String id = current_token.data;

					advance(); // Consume unexpected identifier.

					String removed_in = "Godot 4";
					String use_instead;
					bool panic = true;
					if (id == "static") {
						removed_in = "GDScript 3";
						use_instead = R"(the "@static" annotation)";
						panic = !check(Token::Type::VAR) && !check(Token::Type::FUNC);
					} else if (id == "export") {
						use_instead = R"*(an export annotation ("@export", "@export_range", etc.))*";
						panic = !check(Token::Type::VAR);
					} else if (id == "tool") {
						use_instead = R"(the "@tool" annotation)";
					} else if (id == "onready") {
						use_instead = R"(the "@onready" annotation)";
						panic = !check(Token::Type::VAR);
					} else if (id == "remote") {
						use_instead = R"(the "@rpc" annotation with "any_peer")";
						panic = !check(Token::Type::FUNC);
					} else if (id == "remotesync") {
						use_instead = R"(the "@rpc" annotation with "any_peer" and "call_local")";
						panic = !check(Token::Type::FUNC);
					} else if (id == "puppet") {
						use_instead = R"(the "@rpc" annotation with "authority")";
						panic = !check(Token::Type::FUNC);
					} else if (id == "puppetsync") {
						use_instead = R"(the "@rpc" annotation with "authority" and "call_local")";
						panic = !check(Token::Type::FUNC);
					} else if (id == "master") {
						use_instead = R"(the "@rpc" annotation with "any_peer")";
						use_instead += " ";
						use_instead += "and perform a check inside the function";
						panic = !check(Token::Type::FUNC);
					} else if (id == "mastersync") {
						use_instead = R"(the "@rpc" annotation with "any_peer" and "call_local")";
						use_instead += ", ";
						use_instead += "and perform a check inside the function";
						panic = !check(Token::Type::FUNC);
					}

					if (use_instead.is_empty()) {
						push_error(source_region, vformat(R"(Unexpected identifier "%s" in class body.)", id));
					} else {
						const String msg = vformat(
								R"(The "%s" keyword was removed in %s. Use %s instead.)",
								id,
								removed_in,
								use_instead);
						push_error(source_region, msg, panic);
					}
				} else {
					push_error(vformat("Unexpected %s in class body.", current_token.get_name()));
					advance(); // Consume unexpected token.
				}
				break;
		}

		if (member == nullptr) {
			clear_unused_annotations(member_annotations);
		} else {
			current_context.get_class()->members.push_back(member);
			member->parent_node = current_context.get_class();

			push_applicable_annotations(member, member_annotations);
		}

		synchronize_statement();

		if (!is_multiline_container()) {
			body_ended = true;
		}
	}

	AST::ClassNode *current_class = current_context.get_class();
	if (!current_class->members.is_empty() ||
			current_class == ast.get_root()->main_class ||
			current_class->extends_type != nullptr) {
		has_valid_body = true;
	}

	if (!has_valid_body) {
		push_error(first_body_token, "Expected class body.");
	}
}

// TODO: Pass multiline flag to nested declarations/statements to forbid confusable nesting.
// TODO: Fix source region for empty/invalid block.
AST::BlockNode *Parser::parse_block(const String &p_context) {
	expect({ Token::Type::COLON }, vformat("%s header", p_context));
	const bool has_colon = match(Token::Type::COLON);

	const bool is_multiline = match(Token::Type::NEWLINE);

	AST::BlockNode *block = ast.alloc_node<AST::BlockNode>();

	if (!is_multiline_container() && is_multiline) {
		push_error(vformat("Expected single-line block because parent %s is also single-line.", p_context));
	}
	if (is_multiline) {
		if (!match(Token::Type::INDENT)) {
			push_error(vformat("Expected indented block after %s header.", p_context));
			block->source_region = get_previous_token_end_region();
			return block;
		}
	} else {
		if (!has_colon) {
			block->source_region = get_previous_token_end_region();
			return block;
		}
	}

	panic_mode = false; // Force recover after possible missing colon/newline/indent errors.

	push_node(block);
	push_context(current_context.with_block(block));

	const Token first_body_token = current_token;
	bool has_valid_body = false;
	bool body_ended = false;
	int extra_indent_count = 0;

	push_container(block, is_multiline);
	while (!body_ended) {
		List<AST::AnnotationNode *> statement_annotations;

		while (check(Token::Type::ANNOTATION)) {
			AST::AnnotationNode *annotation = parse_annotation();
			if (annotation->applies_to(AnnotationTarget::STATEMENT)) {
				statement_annotations.push_back(annotation);
			} else {
				handle_annotation_default(annotation);
			}
		}

		AST::Node *statement = nullptr;

		switch (current_token.type) {
			case Token::Type::USING:
				statement = parse_type_alias();
				break;
			case Token::Type::TK_CONST:
				statement = parse_constant();
				break;
			case Token::Type::VAR:
				statement = parse_variable(false);
				break;
			case Token::Type::IF:
				statement = parse_if();
				break;
			case Token::Type::MATCH:
				statement = parse_match();
				break;
			case Token::Type::FOR:
				statement = parse_for();
				break;
			case Token::Type::WHILE:
				statement = parse_while();
				break;
			case Token::Type::BREAK:
				statement = parse_break();
				break;
			case Token::Type::CONTINUE:
				statement = parse_continue();
				break;
			case Token::Type::RETURN:
				statement = parse_return();
				break;
			case Token::Type::ASSERT:
				statement = parse_assert();
				break;
			case Token::Type::BREAKPOINT:
				statement = parse_breakpoint();
				break;
			case Token::Type::PASS:
				has_valid_body = true;
				advance(); // Consume `pass`.
				end_statement(R"("pass")");
				break;
			case Token::Type::INDENT:
				extra_indent_count++;
				push_error(vformat("Unexpected indent in %s block.", p_context), false);
				advance(); // Consume extra indent.
				break;
			case Token::Type::DEDENT:
				if (extra_indent_count <= 0) {
					body_ended = true;
					// No need to consume, will be done below.
				} else {
					extra_indent_count--;
					advance(); // Consume extra dedent.
				}
				break;
			case Token::Type::TK_EOF:
				body_ended = true;
				break;
			default:
				AST::ExpressionNode *expression = parse_expression_or_null();
				if (expression == nullptr) {
					// TODO
					/*if (get_expression_indented_block_depth() > 0) {
						// If it's not a valid expression beginning, it might be the continuation
						// of the outer expression where this lambda is.
						body_ended = true;
						break;
					}*/
					push_error(vformat("Expected statement, found %s instead.", current_token.get_name()));
					advance(); // Consume unexpected token.
				} else {
					if (expression->is_recovery) {
						push_error(expression, "Expected statement, found incomplete expression instead.");
					}
					statement = expression;
					end_statement("expression");
				}
				break;
		}

		if (statement == nullptr) {
			clear_unused_annotations(statement_annotations);
		} else {
			block->statements.push_back(statement);
			statement->parent_node = block;

			push_applicable_annotations(statement, statement_annotations);
		}

		synchronize_statement();

		if (!is_multiline) {
			body_ended = true;
		}
	}
	pop_container(block);

	pop_node(block);
	pop_context();

	if (!block->statements.is_empty()) {
		has_valid_body = true;
	}

	if (!has_valid_body) {
		push_error(first_body_token, vformat("Expected %s block body.", p_context));
	}

	if (is_multiline) {
		// TODO
		/*if (!match(Token::Type::DEDENT) && !is_at_end() && get_expression_indented_block_depth() <= 0) {
			push_error(vformat("Missing unindent at the end of %s block.", p_context));
		}*/
	} // TODO: else if previous SEMICOLON require NEWLINE...

	return block;
}

AST::AnnotationNode *Parser::parse_annotation() {
	DEV_ASSERT(check(Token::Type::ANNOTATION));

	AST::AnnotationNode *annotation = ast.alloc_node<AST::AnnotationNode>();
	annotation->name = current_token.data;
	if (AST::annotation_exists(annotation->name)) {
		annotation->info = AST::get_annotation_info(annotation->name);
	}

	push_node(annotation);

	advance(); // Consume annotation token.

	if (check(Token::Type::PARENTHESIS_OPEN)) {
		push_multiline_mode(true);
		advance(); // Consume `(`.

		do { // Allow trailing comma.
			if (check(Token::Type::PARENTHESIS_CLOSE) || is_at_end()) {
				break;
			}

			AST::ExpressionNode *argument = parse_expression();
			if (argument->is_recovery) {
				push_error(argument, "Expected expression as the annotation argument.");
			}

			annotation->arguments.push_back(argument);
			argument->parent_node = annotation;

			expect({ Token::Type::COMMA, Token::Type::PARENTHESIS_CLOSE }, "annotation argument");
		} while (match(Token::Type::COMMA));

		pop_multiline_mode(Token::Type::PARENTHESIS_CLOSE, "annotation arguments");
	}

	pop_node(annotation);

	if (annotation->applies_to(AnnotationTarget::INDEPENDENT)) {
		if (!match(Token::Type::NEWLINE)) {
			push_error(annotation, vformat(R"(Expected newline after the "%s" annotation.)", annotation->name));
		}
	} else {
		match(Token::Type::NEWLINE); // Newline after annotation is optional.
	}

	return annotation;
}

void Parser::handle_annotation_default(AST::AnnotationNode *p_annotation) {
	if (p_annotation->info == nullptr) {
		String msg = vformat(R"(Annotation "%s" does not exist.)", p_annotation->name);
		if (p_annotation->name == SNAME("@deprecated")) {
			msg += R"( Use "## @deprecated: Reason here." instead.)";
		} else if (p_annotation->name == SNAME("@experimental")) {
			msg += R"( Use "## @experimental: Reason here." instead.)";
		} else if (p_annotation->name == SNAME("@tutorial")) {
			msg += R"( Use "## @tutorial(Title): https://example.com" instead.)";
		}
		push_error(p_annotation, msg, false);
	} else if (p_annotation->applies_to(AnnotationTarget::STANDALONE)) {
		if (AST::is_warning_region_annotation(p_annotation->name)) {
			ast.get_root()->annotations.push_back(p_annotation);
			p_annotation->parent_node = ast.get_root();
		} else {
			push_error(p_annotation, "Unexpected standalone annotation.", false);
		}
	} else if (p_annotation->applies_to(AnnotationTarget::SCRIPT)) {
		const String msg = vformat(
				R"(Annotation "%s" must be at the top of the script, before "extends" and "class_name".)",
				p_annotation->name);
		push_error(p_annotation, msg, false);
	} else {
		const String msg = vformat(R"(Annotation "%s" is not allowed in this level.)", p_annotation->name);
		push_error(p_annotation, msg, false);
	}
}

// TODO: Use direct order, just skip non-applicable annotations?
// But take into account export grouping annotation.
void Parser::push_applicable_annotations(AST::Node *p_target, List<AST::AnnotationNode *> &p_annotations) {
	ERR_FAIL_NULL(p_target);

	List<AST::AnnotationNode *> applicable_annotations;
	while (!p_annotations.is_empty()) {
		AST::AnnotationNode *annotation = p_annotations.back()->get();
		p_annotations.pop_back();
		if (annotation->applies_to(p_target->get_annotation_target())) {
			applicable_annotations.push_front(annotation);
		} else {
			const String msg = vformat(
					R"(Annotation "%s" cannot be applied to a %s.)",
					annotation->name,
					p_target->get_node_type_name());
			push_error(annotation, msg, false);
			clear_unused_annotations(p_annotations);
			break;
		}
	}

	for (AST::AnnotationNode *&annotation : applicable_annotations) {
		p_target->annotations.push_back(annotation);
		annotation->parent_node = p_target;
	}
}

void Parser::clear_unused_annotations(List<AST::AnnotationNode *> &p_annotations) {
	for (AST::AnnotationNode *annotation : p_annotations) {
		const String msg = vformat(
				R"(Annotation "%s" does not precede a valid target, so it will have no effect.)",
				annotation->name);
		push_error(annotation, msg, false);
	}

	p_annotations.clear();
}

AST::TypeNode *Parser::parse_type(bool p_allow_file_path) {
	AST::TypeNode *type = ast.alloc_node<AST::TypeNode>();

	push_node(type);

	if (p_allow_file_path && check(Token::Type::LITERAL) && current_token.is_string_literal()) {
		type->is_file_path = true;
		type->file_path = parse_literal();
	} else if (check(Token::Type::IDENTIFIER)) {
		type->identifier = parse_identifier();
		type->identifier->parent_node = type;
	} else {
		pop_node(type);
		return make_recovery_type();
	}

	// TODO: multiline mode inside `[...]`, trailing comma?
	if (match(Token::Type::BRACKET_OPEN)) {
		bool first_pass = true;
		do {
			AST::TypeNode *param_type = parse_type();
			if (param_type->is_recovery) {
				push_error(param_type, vformat(R"(Expected type parameter after "%s".)", first_pass ? "[" : ","));
			}
			type->parameters.push_back(param_type);
			param_type->parent_node = type;
			first_pass = false;
		} while (match(Token::Type::COMMA));

		if (!match(Token::Type::BRACKET_CLOSE)) {
			push_error(R"(Expected "]" after type parameters.)");
			pop_node(type);
			return type;
		}
	}

	pop_node(type);

	if (match(Token::Type::PERIOD)) {
		AST::TypeNode *inner_type = parse_type();
		if (inner_type->is_recovery) {
			push_error(inner_type, R"(Expected inner type name after ".".)");
		}

		type->parent_node = inner_type;

		inner_type->outer_type = type;
		inner_type->source_region.start = type->source_region.start;

		return inner_type;
	}

	return type;
}

// ----- Declarations -----

void Parser::parse_class_name() {
	DEV_ASSERT(check(Token::Type::CLASS_NAME));

	const Token token = current_token;

	advance(); // Consume `class_name`.

	AST::IdentifierNode *identifier = parse_identifier();
	if (identifier->is_recovery) {
		push_error(identifier, R"(Expected identifier after "class_name".)");
	}

	AST::ClassNode *current_class = current_context.get_class();
	if (current_class->identifier == nullptr) {
		current_class->identifier = identifier;
		current_class->identifier->parent_node = current_class;
	} else {
		push_error(token, R"("class_name" can only be used once.)", false);
	}

	expect({ Token::Type::EXTENDS }, "class name", true);

	// Allow `extends` on the same line.
	if (check(Token::Type::EXTENDS)) {
		parse_extends();
	}

	end_statement(R"("class_name")");
}

void Parser::parse_extends() {
	DEV_ASSERT(check(Token::Type::EXTENDS));

	const Token token = current_token;

	advance(); // Consume `extends`.

	AST::TypeNode *type = parse_type(true);
	if (type->is_recovery) {
		push_error(type, R"(Expected type specifier after "extends".)");
	}

	AST::ClassNode *current_class = current_context.get_class();
	if (current_class->extends_type == nullptr) {
		current_class->extends_type = type;
		current_class->extends_type->parent_node = current_class;
	} else {
		push_error(token, R"("extends" can only be used once per class.)", false);
	}
}

// TODO: `using Name = Other.Name` or `use Other.Name as Name` (`as Name` is optional)?
AST::TypeAliasNode *Parser::parse_type_alias() {
	DEV_ASSERT(check(Token::Type::USING));

	AST::TypeAliasNode *type_alias = ast.alloc_node<AST::TypeAliasNode>();

	push_node(type_alias);

	advance(); // Consume `using`.

	type_alias->identifier = parse_identifier();
	type_alias->identifier->parent_node = type_alias;
	if (type_alias->identifier->is_recovery) {
		push_error(type_alias->identifier, R"(Expected identifier after "using".)");
	}

	expect({ Token::Type::EQUAL }, "type alias name");

	if (match(Token::Type::EQUAL)) {
		type_alias->source_type = parse_type(true);
		type_alias->source_type->parent_node = type_alias;
		if (type_alias->source_type->is_recovery) {
			push_error(type_alias->source_type, R"(Expected type specifier after "=".)");
		}
	}

	pop_node(type_alias);
	end_statement("type alias declaration");

	return type_alias;
}

AST::ClassNode *Parser::parse_class() {
	DEV_ASSERT(check(Token::Type::CLASS));

	AST::ClassNode *class_node = ast.alloc_node<AST::ClassNode>();

	push_node(class_node);
	push_context(current_context.with_class(class_node));

	advance(); // Consume `class`.

	class_node->identifier = parse_identifier();
	class_node->identifier->parent_node = class_node;
	if (class_node->identifier->is_recovery) {
		push_error(class_node->identifier, R"(Expected identifier after "class".)");
	}

	expect({ Token::Type::EXTENDS, Token::Type::COLON }, "class name");

	if (check(Token::Type::EXTENDS)) {
		parse_extends();
	}

	expect({ Token::Type::COLON }, "class declaration");

	if (!match(Token::Type::COLON)) {
		synchronize({ Token::Type::NEWLINE }); // TODO
		if (!check(Token::Type::NEWLINE)) {
			pop_node(class_node);
			pop_context();

			return class_node;
		}
	}

	const bool is_multiline = match(Token::Type::NEWLINE);

	if (is_multiline && !match(Token::Type::INDENT)) {
		push_error("Expected indented block after class declaration.");

		pop_node(class_node);
		pop_context();

		return class_node;
	}

	// It can be inside the class, but only at the beginning.
	if (check(Token::Type::EXTENDS)) {
		parse_extends();
		end_statement("superclass");
	}

	push_container(class_node, is_multiline);
	parse_class_body();
	pop_container(class_node);

	pop_node(class_node);
	pop_context();

	if (is_multiline && !match(Token::Type::DEDENT) && !is_at_end()) {
		push_error("Missing unindent at the end of the class body.");
	}

	return class_node;
}

AST::EnumNode *Parser::parse_enum() {
	DEV_ASSERT(check(Token::Type::ENUM));

	AST::EnumNode *enum_node = ast.alloc_node<AST::EnumNode>();

	push_node(enum_node);

	advance(); // Consume `enum`.

	bool named = false;
	if (check(Token::Type::IDENTIFIER)) {
		enum_node->identifier = parse_identifier();
		enum_node->identifier->parent_node = enum_node;
		named = true;
	}

	if (named) {
		expect({ Token::Type::COLON, Token::Type::BRACE_OPEN }, named ? "enum name" : R"("enum")");

		if (match(Token::Type::COLON)) {
			enum_node->underlying_type = parse_type();
			enum_node->underlying_type->parent_node = enum_node;
			if (enum_node->underlying_type->is_recovery) {
				push_error(enum_node->underlying_type, R"(Expected type specifier after ":".)");
			}

			expect({ Token::Type::BRACE_OPEN }, "enum underlying type specifier");
		}
	} else {
		expect({ Token::Type::BRACE_OPEN }, named ? "enum name" : R"("enum")");
	}

	if (check(Token::Type::BRACE_OPEN)) {
		push_multiline_mode(true);
		advance(); // Consume `{`.

		do { // Allow trailing comma.
			if (check(Token::Type::BRACE_CLOSE) || is_at_end()) {
				break;
			}

			AST::EnumNode::Item item;

			item.identifier = parse_identifier();
			item.identifier->parent_node = enum_node;
			if (item.identifier->is_recovery) {
				push_error(item.identifier, "Expected identifier for enum key.");
			}

			expect({ Token::Type::EQUAL, Token::Type::COMMA, Token::Type::BRACE_CLOSE }, "enum key");

			if (match(Token::Type::EQUAL)) {
				item.custom_value = parse_expression();
				item.custom_value->parent_node = enum_node;
				if (item.custom_value->is_recovery) {
					push_error(item.custom_value, R"(Expected expression after "=".)");
				}
			}

			enum_node->items.push_back(item);

			expect({ Token::Type::COMMA, Token::Type::BRACE_CLOSE }, "enum element");
		} while (match(Token::Type::COMMA));

		pop_multiline_mode(Token::Type::BRACE_CLOSE, "enum body");
	}

	pop_node(enum_node);
	end_statement("enum declaration");

	return enum_node;
}

AST::ConstantNode *Parser::parse_constant() {
	DEV_ASSERT(check(Token::Type::TK_CONST));

	AST::ConstantNode *constant = ast.alloc_node<AST::ConstantNode>();

	push_node(constant);

	advance(); // Consume `const`.

	constant->identifier = parse_identifier();
	constant->identifier->parent_node = constant;
	if (constant->identifier->is_recovery) {
		push_error(constant->identifier, R"(Expected identifier after "const".)");
	}

	// Allow `end of statement` (to move the error to the analysis stage), but do not suggest it as an alternative.
	expect({ Token::Type::COLON, Token::Type::EQUAL }, "constant name", true, false);

	if (match(Token::Type::COLON)) {
		if (check((Token::Type::EQUAL))) {
			constant->infer_datatype = true;
		} else {
			constant->type_specifier = parse_type();
			constant->type_specifier->parent_node = constant;
			if (constant->type_specifier->is_recovery) {
				push_error(constant->type_specifier, R"(Expected type specifier after ":".)");
			}
		}
	}

	// Allow `end of statement` (to move the error to the analysis stage), but do not suggest it as an alternative.
	expect({ Token::Type::EQUAL }, "constant type specifier", true, false);

	if (match(Token::Type::EQUAL)) {
		constant->initializer = parse_expression();
		constant->initializer->parent_node = constant;
		if (constant->initializer->is_recovery) {
			push_error(constant->initializer, R"(Expected expression after "=".)");
		}
	}

	pop_node(constant);
	end_statement("constant declaration");

	return constant;
}

AST::VariableNode *Parser::parse_variable(bool p_allow_property) {
	DEV_ASSERT(check(Token::Type::VAR));

	AST::VariableNode *variable = ast.alloc_node<AST::VariableNode>();

	push_node(variable);

	advance(); // Consume `var`.

	variable->identifier = parse_identifier();
	variable->identifier->parent_node = variable;
	if (variable->identifier->is_recovery) {
		push_error(variable->identifier, R"(Expected variable name after "var".)");
	}

	expect({ Token::Type::COLON, Token::Type::EQUAL }, "variable name", true);

	if (match(Token::Type::COLON)) {
		if (check(Token::Type::NEWLINE)) {
			if (p_allow_property) {
				advance(); // Consume newline.
				return parse_property(variable, true);
			} else {
				push_error(R"(Expected type specifier after ":".)");
				pop_node(variable);
				return variable;
			}
		} else if (check(Token::Type::EQUAL)) {
			variable->infer_datatype = true;
		} else {
			if (p_allow_property) {
				if (check(Token::Type::IDENTIFIER)) {
					const StringName id = current_token.data;
					if (id == SNAME("get") || id == SNAME("set")) {
						return parse_property(variable, false);
					}
				}
			}

			variable->type_specifier = parse_type();
			variable->type_specifier->parent_node = variable;
			if (variable->type_specifier->is_recovery) {
				push_error(variable->type_specifier, R"(Expected type specifier after ":".)");
			}
		}
	}

	if (p_allow_property) {
		expect({ Token::Type::EQUAL, Token::Type::COLON }, "variable declaration", true);
	} else {
		expect({ Token::Type::EQUAL }, "variable declaration", true);
	}

	if (match(Token::Type::EQUAL)) {
		variable->initializer = parse_expression();
		variable->initializer->parent_node = variable;
		if (variable->initializer->is_recovery) {
			push_error(variable->initializer, R"(Expected expression after "=".)");
		}
	}

	if (p_allow_property) {
		if (match(Token::Type::COLON)) {
			if (match(Token::Type::NEWLINE)) {
				return parse_property(variable, true);
			} else {
				return parse_property(variable, false);
			}
		} else if (check(Token::Type::NEWLINE) && check_next(Token::Type::INDENT)) {
			// Recover after missing colon.
			push_error(R"(Expected ":" or end of statement after variable declaration.)", false);
			advance(); // Consume newline.
			return parse_property(variable, true);
		}
	}

	pop_node(variable);
	end_statement("variable declaration");

	return variable;
}

AST::VariableNode *Parser::parse_property(AST::VariableNode *p_variable, bool p_need_indent) {
	using PropertyStyle = AST::VariableNode::PropertyStyle;

	if (p_need_indent) {
		if (!match(Token::Type::INDENT)) {
			push_error(R"(Expected indented block for property after ":".)");
			pop_node(p_variable);
			return p_variable;
		}
	}

	if (!check(Token::Type::IDENTIFIER)) {
		push_error(R"(Expected "get" or "set" for property declaration.)");
		pop_node(p_variable);
		return p_variable;
	}

	if (check_next(Token::Type::EQUAL)) {
		p_variable->property_style = PropertyStyle::POINTER;
	} else {
		p_variable->property_style = PropertyStyle::INLINE;
		if (!p_need_indent) {
			push_error("Property with inline code must go to an indented block.");
		}
	}

	push_container(p_variable, p_need_indent);
	while (true) {
		if (is_at_end()) {
			break;
		}
		if (p_need_indent) {
			if (check(Token::Type::DEDENT)) {
				break;
			}
		} else {
			if (check(Token::Type::NEWLINE)) {
				break;
			}
		}

		if (!check(Token::Type::IDENTIFIER)) {
			push_error(R"(Expected "get" or "set" for property declaration.)");
			break;
		}

		const StringName id = current_token.data;
		if (id != SNAME("get") && id != SNAME("set")) {
			push_error(current_token, R"(Expected "get" or "set" for property declaration.)", false);
		}

		if (p_variable->property_style == PropertyStyle::INLINE) {
			AST::VariableNode::Property property;

			property.inline_function = parse_function(FunctionType::PROPERTY);
			property.inline_function->parent_node = p_variable;

			p_variable->properties.push_back(property);

			if (!p_need_indent) {
				break;
			}
		} else {
			String type_name;
			if (id == SNAME("get")) {
				type_name = "getter pointer";
			} else if (id == SNAME("set")) {
				type_name = "setter pointer";
			} else {
				type_name = "property pointer";
			}

			AST::VariableNode::Property property;

			property.pointer_key = parse_identifier();
			property.pointer_key->parent_node = p_variable;

			expect({ Token::Type::EQUAL }, property.pointer_key->name.operator String().quote());

			if (match(Token::Type::EQUAL)) {
				property.pointer_value = parse_identifier();
				property.pointer_value->parent_node = p_variable;
				if (property.pointer_value->is_recovery) {
					push_error(property.pointer_value, R"(Expected function name after "=".)");
				}
			} else {
				push_error(vformat(R"(Expected "=" after "%s".)", property.pointer_key->name));
			}

			p_variable->properties.push_back(property);

			if (p_need_indent) {
				expect({ Token::Type::COMMA, Token::Type::DEDENT }, type_name, true);
			} else {
				expect({ Token::Type::COMMA }, type_name, true);
			}

			if (match(Token::Type::COMMA)) {
				// Consume potential newline.
				if (match(Token::Type::NEWLINE)) {
					if (!p_need_indent) {
						push_error(R"(Single-line property cannot span across multiple lines (use "\" if needed).)");
					}
				}
			} else {
				break;
			}
		}
	}
	pop_container(p_variable);

	pop_node(p_variable);

	if (p_variable->property_style == PropertyStyle::POINTER) {
		end_statement("property declaration");
	}

	if (p_need_indent) {
		if (!match(Token::Type::DEDENT) && !is_at_end()) {
			push_error("Expected end of indented block for property.");
		}
	}

	return p_variable;
}

AST::ParameterNode *Parser::parse_parameter_or_null() {
	if (!check(Token::Type::IDENTIFIER) && !check(Token::Type::PERIOD_PERIOD_PERIOD)) {
		return nullptr;
	}

	AST::ParameterNode *parameter = ast.alloc_node<AST::ParameterNode>();

	push_node(parameter);

	parameter->is_rest = match(Token::Type::PERIOD_PERIOD_PERIOD);

	parameter->identifier = parse_identifier();
	parameter->identifier->parent_node = parameter;
	if (parameter->identifier->is_recovery) {
		push_error(parameter->identifier, "Expected parameter name.");
	}

	expect({ Token::Type::COLON, Token::Type::EQUAL, Token::Type::COMMA, Token::Type::PARENTHESIS_CLOSE }, "parameter name");

	if (match(Token::Type::COLON)) {
		if (check((Token::Type::EQUAL))) {
			parameter->infer_datatype = true;
		} else {
			parameter->type_specifier = parse_type();
			parameter->type_specifier->parent_node = parameter;
			if (parameter->type_specifier->is_recovery) {
				push_error(parameter->type_specifier, R"(Expected type specifier after ":".)");
			}
		}
	}

	expect({ Token::Type::EQUAL, Token::Type::COMMA, Token::Type::PARENTHESIS_CLOSE }, "parameter type specifier");

	if (match(Token::Type::EQUAL)) {
		parameter->initializer = parse_expression();
		parameter->initializer->parent_node = parameter;
		if (parameter->initializer->is_recovery) {
			push_error(parameter->initializer, R"(Expected expression after "=".)");
		}
	}

	pop_node(parameter);

	return parameter;
}

AST::FunctionNode *Parser::parse_function(FunctionType p_function_type) {
	String type_name;
	switch (p_function_type) {
		case FunctionType::METHOD: {
			DEV_ASSERT(check(Token::Type::FUNC));
			type_name = "function";
		} break;
		case FunctionType::LAMBDA: {
			DEV_ASSERT(check(Token::Type::FUNC));
			type_name = "lambda";
		} break;
		case FunctionType::PROPERTY: {
			DEV_ASSERT(check(Token::Type::IDENTIFIER));
			const StringName id = current_token.data;
			if (id == SNAME("get")) {
				type_name = "inline getter";
			} else if (id == SNAME("set")) {
				type_name = "inline setter";
			} else {
				type_name = "inline property";
			}
		} break;
	}

	AST::FunctionNode *function = ast.alloc_node<AST::FunctionNode>();

	push_node(function);
	push_context(current_context.with_function(function));

	if (p_function_type != FunctionType::PROPERTY) {
		advance(); // Consume `func`.
	}

	const bool has_name = check(Token::Type::IDENTIFIER);
	if (has_name) {
		function->identifier = parse_identifier();
		function->identifier->parent_node = function;
	} else if (p_function_type == FunctionType::METHOD) {
		push_error(R"(Expected function name after "func".)");
		function->identifier = make_recovery_identifier();
		function->identifier->parent_node = function;
	}

	if (p_function_type != FunctionType::PROPERTY) {
		expect({ Token::Type::PARENTHESIS_OPEN }, has_name ? type_name + " name" : R"("func")");
	}

	if (check(Token::Type::PARENTHESIS_OPEN)) {
		push_multiline_mode(true);
		advance(); // Consume `(`.

		bool first_pass = true;
		bool has_optional_parameters = false;
		do { // Allow trailing comma.
			if (check(Token::Type::PARENTHESIS_CLOSE) || is_at_end()) {
				break;
			}

			AST::ParameterNode *parameter = parse_parameter_or_null();
			if (parameter == nullptr) {
				push_error(vformat(
						R"*(Expected %s parameter or ")" after "%s", found %s instead.)*",
						type_name,
						first_pass ? "(" : ",",
						current_token.get_name()));
			} else {
				if (function->rest_used) {
					push_error(parameter, "Cannot have parameters after the rest parameter.", false);
				}

				if (parameter->is_rest) {
					function->rest_used = true;
					if (p_function_type == FunctionType::PROPERTY) {
						const String msg = vformat("An %s parameter cannot be a rest parameter.", type_name);
						push_error(parameter, msg, false);
					}
				}

				if (parameter->type_specifier != nullptr) {
					if (p_function_type == FunctionType::PROPERTY) {
						const String msg = vformat("An %s parameter cannot have a type specifier.", type_name);
						push_error(parameter->type_specifier, msg, false);
					}
				}

				if (parameter->initializer == nullptr) {
					if (has_optional_parameters && !parameter->is_rest) {
						push_error(parameter, "Cannot have mandatory parameters after optional parameters.", false);
					}
				} else {
					if (p_function_type == FunctionType::PROPERTY) {
						const String msg = vformat("An %s parameter cannot have a default value.", type_name);
						push_error(parameter->initializer, msg, false);
					}
					if (parameter->is_rest) {
						push_error(parameter->initializer, "A rest parameter cannot have a default value.", false);
					}
					has_optional_parameters = true;
				}

				function->parameters.push_back(parameter);
				parameter->parent_node = function;
			}

			first_pass = false;
			expect({ Token::Type::COMMA, Token::Type::PARENTHESIS_CLOSE }, vformat("%s parameter", type_name));
		} while (match(Token::Type::COMMA));

		pop_multiline_mode(Token::Type::PARENTHESIS_CLOSE, vformat("%s parameters", type_name));
	}

	if (match(Token::Type::FORWARD_ARROW)) {
		function->return_type = parse_type();
		function->return_type->parent_node = function;
		if (function->return_type->is_recovery) {
			push_error(function->return_type, R"(Expected return type after "->".)");
		}
		if (p_function_type == FunctionType::PROPERTY) {
			push_error(function->return_type, vformat("An %s cannot have a return type.", type_name), false);
		}
	}

	function->header_end = previous_token_end;

	const bool has_body = p_function_type != FunctionType::METHOD || check(Token::Type::COLON) ||
			// Try to recover after missing colon. The error will be reported by `parse_block()`.
			(check(Token::Type::NEWLINE) && check_next(Token::Type::INDENT));
	if (has_body) {
		function->body = parse_block(type_name);
		function->body->parent_node = function;
	}

	pop_node(function);
	pop_context();

	if (!has_body) {
		end_statement("bodyless function declaration");
	}

	return function;
}

AST::SignalNode *Parser::parse_signal() {
	DEV_ASSERT(check(Token::Type::SIGNAL));

	AST::SignalNode *signal = ast.alloc_node<AST::SignalNode>();

	push_node(signal);

	advance(); // Consume `signal`.

	signal->identifier = parse_identifier();
	signal->identifier->parent_node = signal;
	if (signal->identifier->is_recovery) {
		push_error(signal->identifier, R"(Expected signal name after "signal".)");
	}

	expect({ Token::Type::PARENTHESIS_OPEN }, "signal name", true);

	if (check(Token::Type::PARENTHESIS_OPEN)) {
		push_multiline_mode(true);
		advance(); // Consume `(`.

		bool first_pass = true;
		do { // Allow trailing comma.
			if (check(Token::Type::PARENTHESIS_CLOSE) || is_at_end()) {
				break;
			}

			AST::ParameterNode *parameter = parse_parameter_or_null();
			if (parameter == nullptr) {
				push_error(vformat(
						R"*(Expected signal parameter or ")" after "%s", found %s instead.)*",
						first_pass ? "(" : ",",
						current_token.get_name()));
			} else {
				if (parameter->is_rest) {
					push_error(parameter, "A signal parameter cannot be a rest parameter.", false);
				}
				if (parameter->initializer != nullptr) {
					push_error(parameter->initializer, "A signal parameter cannot have a default value.", false);
				}

				signal->parameters.push_back(parameter);
				parameter->parent_node = signal;
			}

			first_pass = false;
			expect({ Token::Type::COMMA, Token::Type::PARENTHESIS_CLOSE }, "signal parameter");
		} while (match(Token::Type::COMMA));

		pop_multiline_mode(Token::Type::PARENTHESIS_CLOSE, "signal parameters");
	}

	pop_node(signal);
	end_statement("signal declaration");

	return signal;
}

// ----- Control Flow -----

AST::IfNode *Parser::parse_if() {
	DEV_ASSERT(check(Token::Type::IF) || check(Token::Type::ELIF));

	AST::IfNode *if_node = ast.alloc_node<AST::IfNode>();

	push_node(if_node);

	const String token_name = current_token.get_raw_name();

	advance(); // Consume `if` or `elif`.

	if_node->condition = parse_expression();
	if_node->condition->parent_node = if_node;
	if (if_node->condition->is_recovery) {
		push_error(if_node->condition, vformat(R"(Expected conditional expression after "%s".)", token_name));
	}

	if_node->true_block = parse_block(token_name.quote());
	if_node->true_block->parent_node = if_node;

	if (check(Token::Type::ELIF)) {
		AST::BlockNode *else_block = ast.alloc_node<AST::BlockNode>();

		push_node(else_block);

		if_node->false_block = else_block;
		if_node->false_block->parent_node = if_node;

		AST::IfNode *elif = parse_if();

		else_block->statements.push_back(elif);
		elif->parent_node = else_block;

		pop_node(else_block);
	} else if (match(Token::Type::ELSE)) {
		if_node->false_block = parse_block(R"("else")");
		if_node->false_block->parent_node = if_node;
	}

	pop_node(if_node);

	return if_node;
}

AST::MatchNode *Parser::parse_match() {
	DEV_ASSERT(check(Token::Type::MATCH));

	AST::MatchNode *match_node = ast.alloc_node<AST::MatchNode>();

	push_node(match_node);

	advance(); // Consume `match`;

	match_node->test = parse_expression();
	match_node->test->parent_node = match_node;
	if (match_node->test->is_recovery) {
		push_error(match_node->test, R"(Expected expression after "match".)");
	}

	expect({ Token::Type::COLON }, R"("match" header)");
	const bool has_colon = match(Token::Type::COLON);

	const bool is_multiline = match(Token::Type::NEWLINE);

	if (!is_multiline_container() && is_multiline) {
		push_error(R"(Expected inline block because parent "match" is also inline.)");
	}
	if (is_multiline) {
		// TODO
		/*if (!match(Token::Type::INDENT)) {
			push_error(R"(Expected indented block after "match" header.)");
			return match_node;
		}*/
	} else {
		if (!has_colon) {
			return match_node;
		}
	}

	panic_mode = false; // Force recover after possible missing colon/newline/indent errors.

	const Token first_body_token = current_token;
	bool has_valid_body = false;
	bool body_ended = false;
	int extra_indent_count = 0;

	push_container(match_node, is_multiline);
	while (!body_ended) {
		List<AST::AnnotationNode *> branch_annotations;

		while (check(Token::Type::ANNOTATION)) {
			AST::AnnotationNode *annotation = parse_annotation();
			if (annotation->applies_to(AnnotationTarget::STATEMENT)) {
				branch_annotations.push_back(annotation);
			} else {
				handle_annotation_default(annotation);
			}
		}

		AST::MatchBranchNode *branch = nullptr;

		switch (current_token.type) {
			case Token::Type::PASS:
				has_valid_body = true;
				advance(); // Consume `pass`.
				end_statement(R"("pass")");
				break;
			case Token::Type::INDENT:
				extra_indent_count++;
				push_error(R"(Unexpected indent in "match" body.)", false);
				advance(); // Consume extra indent.
				break;
			case Token::Type::DEDENT:
				if (extra_indent_count <= 0) {
					body_ended = true;
					// No need to consume, will be done below.
				} else {
					extra_indent_count--;
					advance(); // Consume extra dedent.
				}
				break;
			case Token::Type::TK_EOF:
				body_ended = true;
				break;
			default:
				branch = parse_match_branch();
				// TODO: advance if nullptr?
				break;
		}

		if (branch == nullptr) {
			clear_unused_annotations(branch_annotations);
		} else {
			match_node->branches.push_back(branch);
			branch->parent_node = match_node;

			push_applicable_annotations(branch, branch_annotations);
		}

		synchronize_statement();
	}
	pop_container(match_node);

	pop_node(match_node);

	if (!match_node->branches.is_empty()) {
		has_valid_body = true;
	}

	if (!has_valid_body) {
		push_error(first_body_token, R"(Expected "match" body.)");
	}

	if (is_multiline) {
		// TODO
		/*if (!match(Token::Type::DEDENT) && !is_at_end() && get_expression_indented_block_depth() <= 0) {
			push_error(R"(Missing unindent at the end of "match" block.)");
		}*/
	} // TODO: else if previous SEMICOLON require NEWLINE...

	return match_node;
}

AST::MatchBranchNode *Parser::parse_match_branch() {
	AST::MatchBranchNode *branch = ast.alloc_node<AST::MatchBranchNode>();

	push_node(branch);

	do {
		AST::MatchPatternNode *pattern = parse_match_pattern();

		// TODO: Checks?

		branch->patterns.push_back(pattern);
		pattern->parent_node = branch;

		expect({ Token::Type::COMMA, Token::Type::WHEN, Token::Type::COLON }, "match pattern");
	} while (match(Token::Type::COMMA));

	if (match(Token::Type::WHEN)) {
		branch->pattern_guard = parse_expression();
		branch->pattern_guard->parent_node = branch;
		if (branch->pattern_guard->is_recovery) {
			push_error(branch->pattern_guard, R"(Expected expression for pattern guard after "when".)");
		}

		expect({ Token::Type::COLON }, "pattern guard expression");
	}

	branch->block = parse_block("match branch");
	branch->block->parent_node = branch;

	pop_node(branch);

	return branch;
}

AST::MatchPatternNode *Parser::parse_match_pattern() {
	using PatternType = AST::MatchPatternNode::PatternType;

	AST::MatchPatternNode *pattern = ast.alloc_node<AST::MatchPatternNode>();

	push_node(pattern);

	switch (current_token.type) {
		case Token::Type::VAR:
			pattern->pattern_type = PatternType::BIND;
			advance(); // Consume `var`.
			pattern->bind = parse_identifier();
			pattern->bind->parent_node = pattern;
			if (pattern->bind->is_recovery) {
				push_error(pattern->bind, R"(Expected bind name after "var".)");
			}
			break;
		case Token::Type::BRACKET_OPEN:
			pattern->pattern_type = PatternType::ARRAY;

			push_multiline_mode(true);
			advance(); // Consume `[`.

			do { // Allow trailing comma.
				if (check(Token::Type::BRACKET_CLOSE) || is_at_end()) {
					break;
				}

				if (pattern->rest_used) {
					push_error(R"(The ".." pattern must be the last element in the array pattern.)", false);
				}

				AST::MatchPatternNode::Element element;

				if (match(Token::Type::PERIOD_PERIOD)) {
					element.is_rest = true;
					pattern->rest_used = true;
				} else {
					element.value = parse_match_pattern();
					element.value->parent_node = pattern;

					// TODO: Checks?

					pattern->elements.push_back(element);
				}

				expect({ Token::Type::COMMA, Token::Type::BRACKET_CLOSE }, "array pattern element");
			} while (match(Token::Type::COMMA));

			pop_multiline_mode(Token::Type::BRACKET_CLOSE, "array pattern elements");

			break;
		case Token::Type::BRACE_OPEN:
			pattern->pattern_type = PatternType::DICTIONARY;

			push_multiline_mode(true);
			advance(); // Consume `{`.

			do { // Allow trailing comma.
				if (check(Token::Type::BRACKET_CLOSE) || is_at_end()) {
					break;
				}

				if (pattern->rest_used) {
					push_error(R"(The ".." pattern must be the last element in the dictionary pattern.)", false);
				}

				AST::MatchPatternNode::Element element;

				if (match(Token::Type::PERIOD_PERIOD)) {
					element.is_rest = true;
					pattern->rest_used = true;
				} else if (check(Token::Type::VAR)) {
					element.key = make_recovery_expression();
					element.key->parent_node = pattern;
					push_error("Expected expression as dictionary pattern key. Binding pattern is disallowed.");
					advance(); // Consume `var`.
					match(Token::Type::IDENTIFIER); // Consume potential identifier.
				} else if (check(Token::Type::UNDERSCORE)) {
					element.key = make_recovery_expression();
					element.key->parent_node = pattern;
					push_error("Expected expression as dictionary pattern key. Wildcard pattern is disallowed.");
					advance(); // Consume `_`.
				} else {
					element.key = parse_precedence(get_higher_precedence(Precedence::ASSIGNMENT));
					element.key->parent_node = pattern;
					if (element.key->is_recovery) {
						push_error(element.key, "Expected expression as dictionary pattern key.");
					}
				}

				// TODO: Add Lua style support?
				// NOTE: Value is optional.
				if (match(Token::Type::COLON)) {
					element.value = parse_match_pattern();
					element.value->parent_node = pattern;

					// TODO: Checks?
				}

				pattern->elements.push_back(element);

				expect({ Token::Type::COMMA, Token::Type::BRACE_CLOSE }, "dictionary pattern element");
			} while (match(Token::Type::COMMA));

			pop_multiline_mode(Token::Type::BRACE_CLOSE, "dictionary pattern elements");

			break;
		case Token::Type::UNDERSCORE:
			pattern->pattern_type = PatternType::WILDCARD;
			advance(); // Consume `_`.
			break;
		default:
			pattern->pattern_type = PatternType::EXPRESSION;
			pattern->expression = parse_expression();
			pattern->expression->parent_node = pattern;
			if (pattern->expression->is_recovery) {
				push_error(pattern->expression, R"(Expected "match" pattern.)");
			}
			break;
	}

	pop_node(pattern);

	return pattern;
}

AST::ForNode *Parser::parse_for() {
	DEV_ASSERT(check(Token::Type::FOR));

	AST::ForNode *for_node = ast.alloc_node<AST::ForNode>();

	push_node(for_node);

	advance(); // Consume `for`.

	for_node->iterator = parse_identifier();
	for_node->iterator->parent_node = for_node;
	if (for_node->iterator->is_recovery) {
		push_error(for_node->iterator, R"(Expected iterator variable name after "for".)");
	}

	if (match(Token::Type::COLON)) {
		for_node->iterator_type = parse_type();
		for_node->iterator_type->parent_node = for_node;
		if (for_node->iterator_type->is_recovery) {
			push_error(for_node->iterator_type, R"(Expected type specifier after ":".)");
		}
	}

	if (for_node->iterator_type == nullptr) {
		if (!match(Token::Type::TK_IN)) {
			push_error(R"(Expected "in" or ":" after iterator variable name.)");
		}
	} else {
		if (!match(Token::Type::TK_IN)) {
			push_error(R"(Expected "in" after iterator variable type specifier.)");
		}
	}

	for_node->iterable = parse_expression();
	for_node->iterable->parent_node = for_node;
	if (for_node->iterable->is_recovery) {
		push_error(for_node->iterable, R"(Expected iterable expression after "in".)");
	}

	push_context(current_context.with_loop(for_node));
	for_node->loop = parse_block(R"("for")");
	for_node->loop->parent_node = for_node;
	pop_context();

	pop_node(for_node);

	return for_node;
}

AST::WhileNode *Parser::parse_while() {
	DEV_ASSERT(check(Token::Type::WHILE));

	AST::WhileNode *while_node = ast.alloc_node<AST::WhileNode>();

	push_node(while_node);

	advance(); // Consume `while`.

	while_node->condition = parse_expression();
	while_node->condition->parent_node = while_node;
	if (while_node->condition->is_recovery) {
		push_error(while_node->condition, R"(Expected conditional expression after "while".)");
	}

	push_context(current_context.with_loop(while_node));
	while_node->loop = parse_block(R"("while")");
	while_node->loop->parent_node = while_node;
	pop_context();

	pop_node(while_node);

	return while_node;
}

AST::BreakNode *Parser::parse_break() {
	DEV_ASSERT(check(Token::Type::BREAK));

	AST::BreakNode *break_node = ast.alloc_node<AST::BreakNode>();
	break_node->source_region = current_token.source_region;

	advance(); // Consume `break`.

	end_statement(R"("break")");

	return break_node;
}

AST::ContinueNode *Parser::parse_continue() {
	DEV_ASSERT(check(Token::Type::CONTINUE));

	AST::ContinueNode *continue_node = ast.alloc_node<AST::ContinueNode>();
	continue_node->source_region = current_token.source_region;

	advance(); // Consume `continue`.

	end_statement(R"("continue")");

	return continue_node;
}

AST::ReturnNode *Parser::parse_return() {
	DEV_ASSERT(check(Token::Type::RETURN));

	AST::ReturnNode *return_node = ast.alloc_node<AST::ReturnNode>();

	push_node(return_node);

	advance(); // Consume `return`.

	// Return value is optional.
	return_node->return_value = parse_expression_or_null();
	if (return_node->return_value != nullptr) {
		return_node->return_value->parent_node = return_node;
		if (return_node->return_value->is_recovery) {
			push_error(R"(Expected expression after "return".)");
		}
	}

	pop_node(return_node);
	end_statement(R"("return")");

	return return_node;
}

AST::AssertNode *Parser::parse_assert() {
	DEV_ASSERT(check(Token::Type::ASSERT));

	AST::AssertNode *assert = ast.alloc_node<AST::AssertNode>();

	push_node(assert);

	advance(); // Consume `assert`.

	expect({ Token::Type::PARENTHESIS_OPEN }, R"("assert")");

	if (!check(Token::Type::PARENTHESIS_OPEN)) {
		assert->condition = make_recovery_expression();
		assert->condition->parent_node = assert;

		pop_node(assert);

		return assert;
	}

	push_multiline_mode(true);
	advance(); // Consume `(`.

	assert->condition = parse_expression();
	assert->condition->parent_node = assert;
	if (assert->condition->is_recovery) {
		push_error("Expected expression to assert.");
	}

	expect({ Token::Type::COMMA, Token::Type::PARENTHESIS_CLOSE }, "assert condition");

	if (match(Token::Type::COMMA) && !check(Token::Type::PARENTHESIS_CLOSE)) {
		assert->message = parse_expression();
		assert->message->parent_node = assert;
		if (assert->message->is_recovery) {
			push_error(assert->message, R"(Expected error message for assert after ",".)");
		}

		expect({ Token::Type::COMMA, Token::Type::PARENTHESIS_CLOSE }, "assert message");
		match(Token::Type::COMMA);
	}

	pop_multiline_mode(Token::Type::PARENTHESIS_CLOSE, "assert arguments");

	pop_node(assert);
	end_statement(R"("assert")");

	return assert;
}

AST::BreakpointNode *Parser::parse_breakpoint() {
	DEV_ASSERT(check(Token::Type::BREAKPOINT));

	AST::BreakpointNode *breakpoint = ast.alloc_node<AST::BreakpointNode>();
	breakpoint->source_region = current_token.source_region;

	advance(); // Consume `breakpoint`.

	end_statement(R"("breakpoint")");

	return breakpoint;
}

// ----- Expressions -----

AST::ExpressionNode *Parser::parse_expression() {
	return parse_precedence(Precedence::ASSIGNMENT);
}

AST::ExpressionNode *Parser::parse_expression_or_null() {
	if (get_rule(current_token.type)->prefix == nullptr && get_rule(current_token.type)->infix == nullptr) {
		return nullptr;
	}
	return parse_precedence(Precedence::ASSIGNMENT);
}

AST::ExpressionNode *Parser::parse_precedence(Precedence p_precedence) {
	AST::ExpressionNode *previous_operand = nullptr;

	const ParseRule::Prefix prefix = get_rule(current_token.type)->prefix;
	if (prefix == nullptr) {
		previous_operand = make_recovery_expression();
	} else {
		previous_operand = (this->*prefix)();
		DEV_ASSERT(previous_operand != nullptr);
	}

	while (p_precedence <= get_rule(current_token.type)->precedence) {
		// An infix function must be specified in the table if the precedence is not `NONE`.
		const ParseRule::Infix infix = get_rule(current_token.type)->infix;
		previous_operand = (this->*infix)(previous_operand);
		DEV_ASSERT(previous_operand != nullptr);
	}

	return previous_operand;
}

AST::IdentifierNode *Parser::parse_identifier() {
	if (check(Token::Type::IDENTIFIER)) {
		return static_cast<AST::IdentifierNode *>(_parse_identifier());
	}

	return make_recovery_identifier();
}

AST::LiteralNode *Parser::parse_literal() {
	DEV_ASSERT(check(Token::Type::LITERAL));

	return static_cast<AST::LiteralNode *>(_parse_literal());
}

AST::ExpressionNode *Parser::_parse_array() {
	DEV_ASSERT(check(Token::Type::BRACKET_OPEN));

	AST::ArrayNode *array = ast.alloc_node<AST::ArrayNode>();

	push_node(array);

	push_multiline_mode(true);
	advance(); // Consume `[`.

	do { // Allow trailing comma.
		if (check(Token::Type::BRACKET_CLOSE) || is_at_end()) {
			break;
		}

		AST::ExpressionNode *element = parse_expression();
		if (element->is_recovery) {
			push_error(element, "Expected expression as array element.");
		}

		array->elements.push_back(element);
		element->parent_node = array;

		expect({ Token::Type::COMMA, Token::Type::BRACKET_CLOSE }, "array element");
	} while (match(Token::Type::COMMA));

	pop_multiline_mode(Token::Type::BRACKET_CLOSE, "array elements");

	pop_node(array);

	return array;
}

AST::ExpressionNode *Parser::_parse_await() {
	DEV_ASSERT(check(Token::Type::AWAIT));

	AST::AwaitNode *await = ast.alloc_node<AST::AwaitNode>();

	push_node(await);

	advance(); // Consume `await`.

	await->operand = parse_precedence(Precedence::AWAIT);
	await->operand->parent_node = await;
	if (await->operand->is_recovery) {
		push_error(await->operand, R"(Expected expression after "await".)");
	}

	pop_node(await);

	return await;
}

AST::ExpressionNode *Parser::_parse_dictionary() {
	DEV_ASSERT(check(Token::Type::BRACE_OPEN));

	using Style = AST::DictionaryNode::Style;

	AST::DictionaryNode *dictionary = ast.alloc_node<AST::DictionaryNode>();

	push_node(dictionary);

	push_multiline_mode(true);
	advance(); // Consume `{`.

	// TODO: For Lua style, keys are `StringName`s, not `String`s.
	// Maybe we should require `StringName` literals? Or forbid string literals?
	bool decided_style = false;
	if (check(Token::Type::IDENTIFIER) || current_token.is_string_literal()) {
		if (check_next(Token::Type::COLON)) {
			decided_style = true;
		} else if (check_next(Token::Type::EQUAL)) {
			decided_style = true;
			dictionary->style = Style::LUA_TABLE;
		}
	} else {
		decided_style = true;
	}

	do { // Allow trailing comma.
		if (check(Token::Type::BRACE_CLOSE) || is_at_end()) {
			break;
		}

		const Token key_token = current_token;

		AST::DictionaryNode::Pair pair;
		pair.key = parse_precedence(get_higher_precedence(Precedence::ASSIGNMENT));
		pair.key->parent_node = dictionary;

		switch (dictionary->style) {
			case Style::PYTHON_DICT:
				if (pair.key->is_recovery) {
					push_error(pair.key, "Expected expression as dictionary key.");
				}

				expect({ Token::Type::COLON, Token::Type::COMMA, Token::Type::BRACE_CLOSE }, "dictionary key");

				if (!match(Token::Type::COLON)) {
					if (decided_style) {
						String msg = R"(Expected ":" after dictionary key.)";
						if (check(Token::Type::EQUAL)) {
							msg += " Mixing dictionary styles is not allowed.";
							push_error(msg, false);
							advance(); // Consume `=`.
						} else {
							push_error(msg);
						}
					} else {
						push_error(R"(Expected ":" or "=" after dictionary key.)");
					}
				}
				break;
			case Style::LUA_TABLE:
				if (pair.key->is_recovery) {
					push_error(pair.key, "Expected expression as dictionary key.");
				} else if (key_token.type != Token::Type::IDENTIFIER && !key_token.is_string_literal()) {
					const String msg = R"(Expected identifier or string as Lua-style dictionary key (e.g "{ key = value }").)";
					push_error(pair.key, msg, false);
				}

				expect({ Token::Type::EQUAL, Token::Type::COMMA, Token::Type::BRACE_CLOSE }, "dictionary key");

				if (!match(Token::Type::EQUAL)) {
					String msg = R"(Expected "=" after dictionary key.)";
					if (check(Token::Type::COLON)) {
						msg += " Mixing dictionary styles is not allowed.";
						push_error(msg, false);
						advance(); // Consume `:`.
					} else {
						push_error(msg);
					}
				}
				break;
		}

		pair.value = parse_expression();
		pair.value->parent_node = dictionary;
		if (pair.value->is_recovery) {
			push_error(pair.value, "Expected expression as dictionary value.");
		}

		dictionary->elements.push_back(pair);

		expect({ Token::Type::COMMA, Token::Type::BRACE_CLOSE }, "dictionary element");
	} while (match(Token::Type::COMMA));

	pop_multiline_mode(Token::Type::BRACE_CLOSE, "dictionary elements");

	pop_node(dictionary);

	return dictionary;
}

AST::ExpressionNode *Parser::_parse_get_node() {
	DEV_ASSERT(check(Token::Type::DOLLAR) || check(Token::Type::PERCENT));

	AST::GetNodeNode *get_node = ast.alloc_node<AST::GetNodeNode>();
	get_node->use_dollar = check(Token::Type::DOLLAR);

	push_node(get_node);

	advance(); // Consume `$` or `%`.

	enum class State {
		START,
		SLASH,
		PERCENT,
		NODE_NAME,
	};

	State state = get_node->use_dollar ? State::START : State::PERCENT;

	// TODO: Simplify the grammar by prohibiting complex paths like `$A/"B"/C/"D"` (you can use `$"A/B/C/D"`)?
	bool path_ended = false;
	while (!path_ended) {
		switch (current_token.type) {
			case Token::Type::SLASH:
				if (state != State::START && state != State::NODE_NAME) {
					push_error(R"("/" is only valid at the beginning of the path or after a node name.)", false);
				}
				get_node->full_path += "/";
				advance(); // Consume `/`.
				state = State::SLASH;
				break;
			case Token::Type::PERCENT:
				if (state != State::START && state != State::SLASH) {
					push_error(R"("%" is only valid at the beginning of a node name.)", false);
				}
				get_node->full_path += "%";
				advance(); // Consume `%`.
				state = State::PERCENT;
				break;
			default:
				if (state == State::NODE_NAME) {
					path_ended = true;
				} else if (current_token.is_word(ast.get_source())) {
					const String name = current_token.get_source(ast.get_source());
					get_node->full_path += name;
					if (TS->has_feature(TextServer::FEATURE_UNICODE_SECURITY) && TS->spoof_check(name)) {
						push_warning(current_token.source_region, WarningDB::Code::CONFUSABLE_IDENTIFIER, { name });
					}
					advance(); // Consume word.
					state = State::NODE_NAME;
				} else if (current_token.is_string_literal()) {
					get_node->full_path += current_token.data.operator String();
					advance(); // Consume string literal.
					state = State::NODE_NAME;
				} else {
					path_ended = true;
				}
				break;
		}
	}

	if (state != State::NODE_NAME) {
		push_error(get_node, "Incomplete node path.", false);
	}

	pop_node(get_node);

	return get_node;
}

AST::ExpressionNode *Parser::_parse_grouping() {
	DEV_ASSERT(check(Token::Type::PARENTHESIS_OPEN));

	push_multiline_mode(true);
	advance(); // Consume `(`.

	AST::ExpressionNode *grouped = parse_expression();
	if (grouped->is_recovery) {
		push_error(grouped, "Expected grouping expression.");
	}

	pop_multiline_mode(Token::Type::PARENTHESIS_CLOSE, "grouping expression");

	return grouped;
}

AST::ExpressionNode *Parser::_parse_identifier() {
	DEV_ASSERT(check(Token::Type::IDENTIFIER));

	const StringName id = current_token.data;

	AST::IdentifierNode *identifier = ast.alloc_node<AST::IdentifierNode>();
	identifier->name = id;
	identifier->source_region = current_token.source_region;

	advance(); // Consume identifier.

	if (TS->has_feature(TextServer::FEATURE_UNICODE_SECURITY)) {
		bool valid = true;

		if (!id.operator String().is_valid_ascii_identifier()) {
			{ // if (valid)
				const PackedStringArray &keyword_list = Tokenizer::get_keyword_list();
				const int64_t confusable_keyword = TS->is_confusable(id, keyword_list);
				if (confusable_keyword >= 0) {
					valid = false;
					const String msg = vformat(
							R"(Identifier "%s" is visually similar to the keyword "%s" and thus not allowed.)",
							id,
							keyword_list[confusable_keyword]);
					push_error(identifier, msg, false); // TODO: Push `CONFUSABLE_IDENTIFIER` warning instead?
				}
			}

			if (valid) {
				const PackedStringArray &literal_list = Tokenizer::get_literal_list();
				const int64_t confusable_literal = TS->is_confusable(id, literal_list);
				if (confusable_literal >= 0) {
					valid = false;
					const String msg = vformat(
							R"(Identifier "%s" is visually similar to the literal "%s" and thus not allowed.)",
							id,
							literal_list[confusable_literal]);
					push_error(identifier, msg, false); // TODO: Push `CONFUSABLE_IDENTIFIER` warning instead?
				}
			}
		}

		if (valid && TS->spoof_check(identifier->name)) {
			//valid = false;
			push_warning(identifier->source_region, WarningDB::Code::CONFUSABLE_IDENTIFIER, { id });
		}
	}

	return identifier;
}

AST::ExpressionNode *Parser::_parse_lambda() {
	DEV_ASSERT(check(Token::Type::FUNC));

	AST::LambdaNode *lambda = ast.alloc_node<AST::LambdaNode>();

	push_node(lambda);

	const bool was_multiline_mode = multiline_mode;

	if (was_multiline_mode) {
		push_multiline_mode(false);
		//push_expression_indented_block(); // TODO
	}

	lambda->function = parse_function(FunctionType::LAMBDA);
	lambda->function->parent_node = lambda;

	if (was_multiline_mode) {
		//pop_expression_indented_block(); // TODO
		pop_multiline_mode();
		//readvance(); // Discard spurious whitespace tokens. // TODO
	}

	pop_node(lambda);

	lambda_ended = true;

	return lambda;
}

AST::ExpressionNode *Parser::_parse_literal() {
	DEV_ASSERT(check(Token::Type::LITERAL));

	AST::LiteralNode *literal = ast.alloc_node<AST::LiteralNode>();
	literal->value = current_token.data;
	literal->source_region = current_token.source_region;

	advance(); // Consume literal.

	return literal;
}

AST::ExpressionNode *Parser::_parse_self() {
	DEV_ASSERT(check(Token::Type::SELF));

	AST::SelfNode *self = ast.alloc_node<AST::SelfNode>();
	self->source_region = current_token.source_region;

	advance(); // Consume `self`.

	return self;
}

AST::ExpressionNode *Parser::_parse_super() {
	DEV_ASSERT(check(Token::Type::SUPER));

	AST::SuperNode *super = ast.alloc_node<AST::SuperNode>();

	push_node(super);

	advance(); // Consume `super`.

	if (match(Token::Type::PERIOD)) {
		super->identifier = parse_identifier();
		super->identifier->parent_node = super;
		if (super->identifier->is_recovery) {
			push_error(super->identifier, R"(Expected identifier after ".".)");
		}
	}

	pop_node(super);

	return super;
}

AST::ExpressionNode *Parser::_parse_unary_operator() {
	using Operation = AST::UnaryOperatorNode::Operation;

	const String operator_name = current_token.get_raw_name();
	const Operation operation = get_unary_operation(current_token.type);

	Precedence next_precedence = Precedence::SIGN;
	switch (operation) {
		case Operation::POSITIVE:
		case Operation::NEGATIVE:
			next_precedence = Precedence::SIGN;
			break;
		case Operation::COMPLEMENT:
			next_precedence = Precedence::BIT_NOT;
			break;
		case Operation::LOGIC_NOT:
			next_precedence = Precedence::LOGIC_NOT;
			break;
	}

	AST::UnaryOperatorNode *unary_operator = ast.alloc_node<AST::UnaryOperatorNode>();
	unary_operator->operation = operation;

	push_node(unary_operator);

	advance(); // Consume unary operator token.

	unary_operator->operand = parse_precedence(next_precedence);
	unary_operator->operand->parent_node = unary_operator;
	if (unary_operator->operand->is_recovery) {
		push_error(unary_operator->operand, vformat(R"(Expected expression after "%s" operator.)", operator_name));
	}

	pop_node(unary_operator);

	return unary_operator;
}

AST::ExpressionNode *Parser::_parse_yield() {
	DEV_ASSERT(check(Token::Type::YIELD));

	push_error(R"("yield" was removed in Godot 4. Use "await" instead.)");

	advance(); // Consume `yield`.

	return make_recovery_expression();
}

AST::ExpressionNode *Parser::_parse_assignment(AST::ExpressionNode *p_previous_operand) {
	const String operator_name = current_token.get_raw_name();

	AST::AssignmentNode *assignment = ast.alloc_node<AST::AssignmentNode>();
	assignment->operation = get_assignment_operation(current_token.type);

	push_node(assignment, p_previous_operand->source_region);

	assignment->assignee = p_previous_operand;
	assignment->assignee->parent_node = assignment;
	if (assignment->assignee->is_recovery) {
		push_error(assignment->assignee, vformat(R"(Expected expression before "%s".)", operator_name));
	}

	advance(); // Consume assignment token.

	assignment->assigned_value = parse_expression();
	assignment->assigned_value->parent_node = assignment;
	if (assignment->assigned_value->is_recovery) {
		push_error(assignment->assigned_value, vformat(R"(Expected expression after "%s".)", operator_name));
	}

	pop_node(assignment);

	return assignment;
}

AST::ExpressionNode *Parser::_parse_attribute(AST::ExpressionNode *p_previous_operand) {
	DEV_ASSERT(check(Token::Type::PERIOD));

	AST::SubscriptNode *subscript = ast.alloc_node<AST::SubscriptNode>();
	subscript->is_attribute = true;

	push_node(subscript, p_previous_operand->source_region);

	subscript->base = p_previous_operand;
	subscript->base->parent_node = subscript;
	if (subscript->base->is_recovery) {
		push_error(subscript->base, R"(Expected expression before "." for attribute access.)");
	}

	advance(); // Consume `.`.

	if (check(Token::Type::IDENTIFIER)) {
		subscript->attribute = parse_identifier();
	} else if (current_token.is_word(ast.get_source())) {
		subscript->attribute = ast.alloc_node<AST::IdentifierNode>();
		subscript->attribute->name = current_token.get_source(ast.get_source());
		advance(); // Consume word.
	} else {
		push_error(R"(Expected identifier after "." for attribute access.)");
	}
	subscript->attribute->parent_node = subscript;

	pop_node(subscript);

	return subscript;
}

AST::ExpressionNode *Parser::_parse_binary_operator(AST::ExpressionNode *p_previous_operand) {
	const String operator_name = current_token.get_raw_name();
	const Precedence next_precedence = get_higher_precedence(get_rule(current_token.type)->precedence);

	AST::BinaryOperatorNode *binary_operator = ast.alloc_node<AST::BinaryOperatorNode>();
	binary_operator->operation = get_binary_operation(current_token.type);

	push_node(binary_operator, p_previous_operand->source_region);

	binary_operator->left_operand = p_previous_operand;
	binary_operator->left_operand->parent_node = binary_operator;
	if (binary_operator->left_operand->is_recovery) {
		const String msg = vformat(R"(Expected expression before "%s" operator.)", operator_name);
		push_error(binary_operator->left_operand, msg);
	}

	advance(); // Consume binary operator token.

	binary_operator->right_operand = parse_precedence(next_precedence);
	binary_operator->right_operand->parent_node = binary_operator;
	if (binary_operator->right_operand->is_recovery) {
		const String msg = vformat(R"(Expected expression after "%s" operator.)", operator_name);
		push_error(binary_operator->right_operand, msg);
	}

	pop_node(binary_operator);

	return binary_operator;
}

AST::ExpressionNode *Parser::_parse_call(AST::ExpressionNode *p_previous_operand) {
	DEV_ASSERT(check(Token::Type::PARENTHESIS_OPEN));

	AST::CallNode *call = ast.alloc_node<AST::CallNode>();

	push_node(call, p_previous_operand->source_region);

	call->callee = p_previous_operand;
	call->callee->parent_node = call;
	if (call->callee->is_recovery) {
		push_error(call->callee, R"(Expected expression before "(".)");
	}

	push_multiline_mode(true);
	advance(); // Consume `(`.

	do { // Allow trailing comma.
		if (check(Token::Type::PARENTHESIS_CLOSE) || is_at_end()) {
			break;
		}

		AST::ExpressionNode *argument = parse_expression();
		if (argument->is_recovery) {
			push_error(argument, "Expected expression as the call argument.");
		}

		call->arguments.push_back(argument);
		argument->parent_node = call;

		expect({ Token::Type::COMMA, Token::Type::PARENTHESIS_CLOSE }, "call argument");
	} while (match(Token::Type::COMMA));

	pop_multiline_mode(Token::Type::PARENTHESIS_CLOSE, "call arguments");

	pop_node(call);

	return call;
}

AST::ExpressionNode *Parser::_parse_cast(AST::ExpressionNode *p_previous_operand) {
	DEV_ASSERT(check(Token::Type::AS));

	AST::CastNode *cast = ast.alloc_node<AST::CastNode>();

	push_node(cast, p_previous_operand->source_region);

	cast->operand = p_previous_operand;
	cast->operand->parent_node = cast;
	if (cast->operand->is_recovery) {
		push_error(cast->operand, R"(Expected expression before "as".)");
	}

	advance(); // Consume `as`.

	cast->cast_type = parse_type();
	cast->cast_type->parent_node = cast;
	if (cast->cast_type->is_recovery) {
		push_error(cast->cast_type, R"(Expected type specifier after "as".)");
	}

	pop_node(cast);

	return cast;
}

AST::ExpressionNode *Parser::_parse_not_in_operator(AST::ExpressionNode *p_previous_operand) {
	DEV_ASSERT(check(Token::Type::NOT));

	SourceRegion operator_source_region = current_token.source_region;

	advance(); // Consume `not`.

	if (match(Token::Type::TK_IN)) {
		operator_source_region.end = previous_token_end;
	} else {
		push_error(R"(Expected "in" after "not" in content test operator.)");
		return p_previous_operand;
	}

	AST::UnaryOperatorNode *not_node = ast.alloc_node<AST::UnaryOperatorNode>();
	not_node->operation = AST::UnaryOperatorNode::Operation::LOGIC_NOT;

	AST::BinaryOperatorNode *content_test = ast.alloc_node<AST::BinaryOperatorNode>();
	content_test->operation = AST::BinaryOperatorNode::Operation::CONTENT_TEST;

	push_node(not_node, p_previous_operand->source_region);
	push_node(content_test, p_previous_operand->source_region);

	content_test->left_operand = p_previous_operand;
	content_test->left_operand->parent_node = content_test;
	if (content_test->left_operand->is_recovery) {
		push_error(operator_source_region, R"(Expected expression before "not in" operator.)");
	}

	content_test->right_operand = parse_precedence(get_higher_precedence(Precedence::CONTENT_TEST));
	content_test->right_operand->parent_node = content_test;
	if (content_test->right_operand->is_recovery) {
		push_error(content_test->right_operand, R"(Expected expression after "not in" operator.)");
	}

	not_node->operand = content_test;
	content_test->parent_node = not_node;

	pop_node(content_test);
	pop_node(not_node);

	return not_node;
}

AST::ExpressionNode *Parser::_parse_question_mark(AST::ExpressionNode *p_previous_operand) {
	DEV_ASSERT(check(Token::Type::QUESTION_MARK));

	push_error(R"(Unexpected "?". If you want a ternary operator, use "truthy_value if condition else falsy_value".)");

	advance(); // Consume `?`.

	return p_previous_operand;
}

AST::ExpressionNode *Parser::_parse_subscript(AST::ExpressionNode *p_previous_operand) {
	DEV_ASSERT(check(Token::Type::BRACKET_OPEN));

	AST::SubscriptNode *subscript = ast.alloc_node<AST::SubscriptNode>();

	push_node(subscript, p_previous_operand->source_region);

	subscript->base = p_previous_operand;
	subscript->base->parent_node = subscript;
	if (subscript->base->is_recovery) {
		push_error(subscript->base, R"(Expected expression before "[".)");
	}

	push_multiline_mode(true);
	advance(); // Consume `[`.

	subscript->index = parse_expression();
	subscript->index->parent_node = subscript;
	if (subscript->index->is_recovery) {
		push_error(subscript->index, R"(Expected expression after "[".)");
	}

	pop_multiline_mode(Token::Type::BRACKET_CLOSE, "subscription index");

	pop_node(subscript);

	return subscript;
}

AST::ExpressionNode *Parser::_parse_ternary_operator(AST::ExpressionNode *p_previous_operand) {
	DEV_ASSERT(check(Token::Type::IF));

	advance(); // Consume `if`.

	AST::TernaryOperatorNode *ternary_operator = ast.alloc_node<AST::TernaryOperatorNode>();

	push_node(ternary_operator, p_previous_operand->source_region);

	ternary_operator->true_expr = p_previous_operand;
	ternary_operator->true_expr->parent_node = ternary_operator;
	if (ternary_operator->true_expr->is_recovery) {
		push_error(ternary_operator->true_expr, R"(Expected expression before ternary operator "if".)");
	}

	ternary_operator->condition = parse_precedence(Precedence::TERNARY);
	ternary_operator->condition->parent_node = ternary_operator;
	if (ternary_operator->condition->is_recovery) {
		push_error(ternary_operator->condition, R"(Expected expression as ternary condition after "if".)");
	}

	if (match(Token::Type::ELSE)) {
		ternary_operator->false_expr = parse_precedence(Precedence::TERNARY);
		if (ternary_operator->false_expr->is_recovery) {
			push_error(ternary_operator->false_expr, R"(Expected expression after "else".)");
		}
	} else {
		push_error(R"(Expected "else" after ternary operator condition.)");
		ternary_operator->false_expr = make_recovery_expression();
	}
	ternary_operator->false_expr->parent_node = ternary_operator;

	pop_node(ternary_operator);

	return ternary_operator;
}

AST::ExpressionNode *Parser::_parse_type_test(AST::ExpressionNode *p_previous_operand) {
	DEV_ASSERT(check(Token::Type::IS));

	SourceRegion operator_source_region = current_token.source_region;

	advance(); // Consume `is`.

	String operator_name = "is";
	AST::UnaryOperatorNode *not_node = nullptr;
	if (match(Token::Type::NOT)) {
		operator_source_region.end = previous_token_end;
		operator_name = "is not";

		not_node = ast.alloc_node<AST::UnaryOperatorNode>();
		not_node->operation = AST::UnaryOperatorNode::Operation::LOGIC_NOT;

		push_node(not_node, p_previous_operand->source_region);
	}

	AST::TypeTestNode *type_test = ast.alloc_node<AST::TypeTestNode>();

	push_node(type_test, p_previous_operand->source_region);

	type_test->operand = p_previous_operand;
	type_test->operand->parent_node = type_test;
	if (type_test->operand->is_recovery) {
		push_error(operator_source_region, vformat(R"(Expected expression before "%s".)", operator_name));
	}

	type_test->test_type = parse_type();
	type_test->test_type->parent_node = type_test;
	if (type_test->test_type->is_recovery) {
		push_error(type_test->test_type, vformat(R"(Expected type specifier after "%s".)", operator_name));
	}

	pop_node(type_test);

	if (not_node != nullptr) {
		not_node->operand = type_test;
		type_test->parent_node = not_node;

		pop_node(not_node);

		return not_node;
	}

	return type_test;
}

// ----- Public methods -----

Parser::Parser(const String &p_script_path, const String &p_source, int p_tab_size) {
	tokenizer = Tokenizer(p_source, p_tab_size);
	current_token = _scan();

	ast = AST(p_script_path, p_source);
}

// TODO: Add match, lambda.
// TODO: Add recovering for missing colons.
// TODO: Limit single-line declarations/statements.
// TODO: Check some nodes (like variable setter/getter).
// TODO: Check overall consistency.
