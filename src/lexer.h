/*
 * The MIT License
 *
 * Copyright 2021 Evgeny Ermakov <e.v.ermakov@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef LEXER_H
#define LEXER_H

#include "common.h"

/**
 * \brief Token type
 */
enum token_type {
	TOKEN_INVALID,

	TOKEN_AND,
	TOKEN_BREAK,
	TOKEN_CONTINUE,
	TOKEN_ELIF,
	TOKEN_ELSE,
	TOKEN_ENDFOREACH,
	TOKEN_ENDIF,
	TOKEN_FALSE,
	TOKEN_FOREACH,
	TOKEN_IF,
	TOKEN_IN,
	TOKEN_NOT,
	TOKEN_OR,
	TOKEN_TRUE,

	TOKEN_IDENTIFIER,
	TOKEN_BIN_NUMBER,
	TOKEN_DEC_NUMBER,
	TOKEN_OCT_NUMBER,
	TOKEN_HEX_NUMBER,
	TOKEN_STRING,
	TOKEN_MULTILINE_STRING,

	TOKEN_L_PAREN,
	TOKEN_L_BRACE,
	TOKEN_L_BRACKET,
	TOKEN_R_PAREN,
	TOKEN_R_BRACE,
	TOKEN_R_BRACKET,

	TOKEN_ASSIGN,
	TOKEN_DOT,
	TOKEN_COLON,
	TOKEN_COMMA,
	TOKEN_TERNARY,

	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_STAR,
	TOKEN_SLASH,
	TOKEN_PERCENT,

	TOKEN_ADD_ASSIGN,
	TOKEN_SUB_ASSIGN,
	TOKEN_MOD_ASSIGN,
	TOKEN_MUL_ASSIGN,
	TOKEN_DIV_ASSIGN,

	TOKEN_LT,
	TOKEN_LE,
	TOKEN_GT,
	TOKEN_GE,
	TOKEN_EQ,
	TOKEN_NE,

	TOKEN_INC,
	TOKEN_DEC,

	TOKEN_NEWLINE,
	TOKEN_END,
	TOKEN_ERROR
};

struct lexer {
	bool error;
	const char *input;
	size_t input_pos;
	size_t input_len;
	size_t lexeme_len;
	size_t lexeme_max;
	char *lexeme;
	int lexeme_pos;
};

void lexer_init(struct lexer *l, struct string input);
void lexer_free(struct lexer *l);

enum token_type lex(struct lexer *l);

#endif /* LEXER_H */
