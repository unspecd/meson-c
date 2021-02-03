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

#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>

struct result {
	bool success;
	enum token_type token;
};

static char peek(struct lexer *l)
{
	return l->input[l->input_pos];
}

static void advance(struct lexer *l)
{
	l->input_pos++;
}

static bool grow(struct lexer *l)
{
	char *ptr = NULL;

	if (l->error) {
		return false;
	}

	if (l->lexeme_len >= l->lexeme_max) {
		l->lexeme_max *= 2;
		if ((ptr = realloc(l->lexeme, l->lexeme_max)) == NULL) {
			l->error = true;
			return false;
		}
		l->lexeme = ptr;
	}
	return true;
}

static void append(struct lexer *l)
{
	if (grow(l)) {
		l->lexeme[l->lexeme_len++] = peek(l);
	}
	advance(l);
}

static struct result fail(void)
{
	return (struct result) { .success = false };
}

static struct result finish(struct lexer *l, enum token_type token)
{
	if (grow(l)) {
		l->lexeme[l->lexeme_len] = '\0';
		return (struct result) { .success = true, .token = token };
	}
	return fail();
}

static bool is_end(struct lexer *l)
{
	return l->input_pos == l->input_len;
}

static bool is_punct(struct lexer *l)
{
	char c = peek(l);

	return c == '(' || c == ')' ||
	       c == '{' || c == '}' ||
	       c == '[' || c == ']' ||
	       c == '.' || c == ',' ||
	       c == ':' || c == '?' ||
	       c == '+' || c == '-' ||
	       c == '*' || c == '/' ||
	       c == '%' || c == '=' ||
	       c == '<' || c == '>' ||
	       c == '!';
}

static bool is_space(struct lexer *l)
{
	return isspace(peek(l));
}

static bool is_symbol(struct lexer *l)
{
	return isalpha(peek(l)) || peek(l) == '_';
}

static bool is_bdigit(struct lexer *l)
{
	char c = peek(l);
	return c == '0' || c == '1';
}

static bool is_odigit(struct lexer *l)
{
	char c = peek(l);
	return c >= '0' && c <= '7';
}

static bool is_xdigit(struct lexer *l)
{
	return isxdigit(peek(l));
}

static bool is_digit(struct lexer *l)
{
	return isdigit(peek(l));
}

static bool is_quote(struct lexer *l)
{
	return peek(l) == '\'';
}

static bool match(struct lexer *l, char c)
{
	if (peek(l) == c) {
		advance(l);
		return true;
	}
	return false;
}

static bool accept(struct lexer *l, char c)
{
	if (peek(l) == c) {
		append(l);
		return true;
	}
	return false;
}

static struct result symbol(struct lexer *l)
{
	static const struct {
		bool prefix;
		const char *name;
		enum token_type token;
	} keywords[] = {
		{ false, "and", TOKEN_AND },
		{ false, "break", TOKEN_BREAK },
		{ false, "continue", TOKEN_CONTINUE },
		{  true, "elif", TOKEN_ELIF },
		{  true, "else", TOKEN_ELSE },
		{  true, "endforeach", TOKEN_ENDFOREACH },
		{ false, "endif", TOKEN_ENDIF },
		{  true, "false", TOKEN_FALSE },
		{ false, "foreach", TOKEN_FOREACH },
		{  true, "if", TOKEN_IF },
		{ false, "in", TOKEN_IN },
		{ false, "not", TOKEN_NOT },
		{ false, "or", TOKEN_OR },
		{ false, "true", TOKEN_TRUE },
		{ false, NULL, TOKEN_INVALID }
	};

	size_t i = 0;
	size_t j = 0;

	while (keywords[i].name != NULL) {
		if (keywords[i].name[j] == '\0') {
			if (is_end(l) || is_space(l) || is_punct(l)) {
				return finish(l, keywords[i].token);
			}
			break;
		}
		if (accept(l, keywords[i].name[j])) {
			j++;
		} else {
			if (!keywords[i].prefix) {
				j = 0;
			}
			i++;
		}
	}

	while (is_symbol(l) || is_digit(l)) {
		append(l);
	}

	return finish(l, TOKEN_IDENTIFIER);
}

static struct result multiline_string(struct lexer *l)
{
	while (!is_end(l)) {
		int i = 0;

		while (!is_end(l) && !is_quote(l)) {
			append(l);
		}

		while (i < 3 && is_quote(l)) {
			append(l);
			i++;
		}
		if (i == 3)  {
			l->lexeme_len -= 3;
			return finish(l, TOKEN_MULTILINE_STRING);
		}

		append(l);
	}

	return fail();
}

static struct result string(struct lexer *l)
{
	match(l, '\'');

	if (match(l, '\'')) {
		if (match(l, '\'')) {
			return multiline_string(l);
		}
		return finish(l, TOKEN_STRING);
	}
	while (!is_end(l) && !is_quote(l)) {
		append(l);
	}
	if (!match(l, '\'')) {
		return fail();
	}

	return finish(l, TOKEN_STRING);
}

static struct result number(struct lexer *l)
{
	enum token_type token = TOKEN_DEC_NUMBER;
	bool (* cond)(struct lexer *l) = is_digit;

	if (accept(l, '0')) {
		switch (peek(l)) {
		case 'b': cond = is_bdigit; token = TOKEN_BIN_NUMBER; break;
		case 'o': cond = is_odigit; token = TOKEN_OCT_NUMBER; break;
		case 'x': cond = is_xdigit; token = TOKEN_HEX_NUMBER; break;
		}

		if (token != TOKEN_DEC_NUMBER) {
			advance(l);
			if (!cond(l)) {
				return fail();
			}
			l->lexeme_len--;
		}
	}

	while (cond(l)) {
		append(l);
	}

	if (is_end(l) || is_space(l) || is_punct(l)) {
		return finish(l, token);
	}

	return fail();
}

static struct result maybe_eq(struct lexer *l,
			      enum token_type if_eq,
			      enum token_type if_not_eq)
{
	append(l);
	if (accept(l, '=')) {
		return finish(l, if_eq);
	}
	return finish(l, if_not_eq);
}

static struct result append_finish(struct lexer *l, enum token_type token)
{
	append(l);
	return finish(l, token);
}

static struct result do_lex(struct lexer *l)
{
	l->lexeme_len = 0;

	for (;;) {
#if 0
		if (peek(l) == '\n') {
			append(l);
			return TOKEN_NEWLINE;
		}
#endif
		while (!is_end(l) && is_space(l)) {
			advance(l);
		}

		if (is_end(l)) {
			return finish(l, TOKEN_END);
		}

		if (match(l, '\\')) {
			continue;
		}

		if (peek(l) != '#') {
			break;
		}

		/* Skip comment. */
		while (!is_end(l) && peek(l) != '\n') {
			advance(l);
		}
	}

	if (is_symbol(l)) {
		return symbol(l);
	}

	switch (peek(l)) {
	case '(': return append_finish(l, TOKEN_L_PAREN);
	case ')': return append_finish(l, TOKEN_R_PAREN);
	case '{': return append_finish(l, TOKEN_L_BRACE);
	case '}': return append_finish(l, TOKEN_R_BRACE);
	case '[': return append_finish(l, TOKEN_L_BRACKET);
	case ']': return append_finish(l, TOKEN_R_BRACKET);
	case '.': return append_finish(l, TOKEN_DOT);
	case ',': return append_finish(l, TOKEN_COMMA);
	case ':': return append_finish(l, TOKEN_COLON);
	case '?': return append_finish(l, TOKEN_TERNARY);

	case '+': return maybe_eq(l, TOKEN_ADD_ASSIGN, TOKEN_PLUS);
	case '-': return maybe_eq(l, TOKEN_SUB_ASSIGN, TOKEN_MINUS);
	case '*': return maybe_eq(l, TOKEN_MUL_ASSIGN, TOKEN_STAR);
	case '/': return maybe_eq(l, TOKEN_DIV_ASSIGN, TOKEN_SLASH);
	case '%': return maybe_eq(l, TOKEN_MOD_ASSIGN, TOKEN_PERCENT);
	case '<': return maybe_eq(l, TOKEN_LE, TOKEN_LT);
	case '>': return maybe_eq(l, TOKEN_GE, TOKEN_GT);
	case '=': return maybe_eq(l, TOKEN_EQ, TOKEN_ASSIGN);
	case '!': return maybe_eq(l, TOKEN_NE, TOKEN_INVALID);

	case '\'': return string(l);

	case '0':
	case '1': case '2': case '3':
	case '4': case '5': case '6':
	case '7': case '8': case '9':
		return number(l);

	default:
		return fail();
	}
}

void lexer_init(struct lexer *l, struct string input)
{
	memset(l, 0, sizeof(*l));
	l->input = string_text(input);
	l->input_len = string_length(input);
	l->lexeme_max = 128;
	l->lexeme = mem_alloc(l->lexeme_max);
}

void lexer_free(struct lexer *l)
{
	mem_free(l->lexeme, l->lexeme_max);
	memset(l, 0, sizeof(*l));
}

enum token_type lex(struct lexer *l)
{
	struct result res;

	if ((res = do_lex(l)).success) {
		return l->error ? TOKEN_ERROR : res.token;
	}

	return TOKEN_ERROR;
}
