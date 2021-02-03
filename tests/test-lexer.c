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

#include "test.h"
#include "lexer.h"

static bool check(bool should_pass, const char *source,
		  enum token_type token, const char *lexeme)
{
	struct lexer l;
	enum token_type result = TOKEN_INVALID;
	bool success = false;
	bool pass = false;

	lexer_init(&l, string_from_buf(source));
	result = lex(&l);
	success = result != TOKEN_ERROR;

	if (success == should_pass) {
		pass = token == result;
		if (success) {
			pass &= strlen(lexeme) == l.lexeme_len &&
				strcmp(lexeme, l.lexeme) == 0;
		}
	}
	lexer_free(&l);

	return pass;
}

static bool should_pass(const char *source, enum token_type token,
			const char *lexeme)
{
	return check(true, source, token, lexeme);
}

static bool should_fail(const char *source)
{
	return check(false, source, TOKEN_ERROR, NULL);
}

static void test_spaces()
{
	PASS("", TOKEN_END, "");
	PASS(" ", TOKEN_END, "");
	PASS("# comment", TOKEN_END, "");
	PASS("# comment\n", TOKEN_END, "");
}

static void test_numbers()
{
	PASS("0", TOKEN_DEC_NUMBER, "0");
	PASS("0123456789", TOKEN_DEC_NUMBER, "0123456789");
	FAIL("0_");

	PASS("0b01", TOKEN_BIN_NUMBER, "01");
	FAIL("0b");
	FAIL("0b_");
	FAIL("0b2");

	FAIL("0o");
	PASS("0o1234567", TOKEN_OCT_NUMBER, "1234567");
	FAIL("0o8");
	FAIL("0o_");

	FAIL("0x");
	PASS("0x123456789abcdef", TOKEN_HEX_NUMBER, "123456789abcdef");
	PASS("0x123456789ABCDEF", TOKEN_HEX_NUMBER, "123456789ABCDEF");
	FAIL("0x_");
}

static void test_strings()
{
	PASS("''", TOKEN_STRING, "");
	PASS("'sample'", TOKEN_STRING, "sample");
	PASS("''''''", TOKEN_MULTILINE_STRING, "");
	PASS("'''sample'''", TOKEN_MULTILINE_STRING, "sample");
	PASS("'''sam'ple'''", TOKEN_MULTILINE_STRING, "sam'ple");
	PASS("'''sam''ple'''", TOKEN_MULTILINE_STRING, "sam''ple");

	FAIL("'");
	FAIL("'''");
}

static void test_keywords()
{
	PASS("and", TOKEN_AND, "and");
	PASS("break", TOKEN_BREAK, "break");
	PASS("continue", TOKEN_CONTINUE, "continue");
	PASS("elif", TOKEN_ELIF, "elif");
	PASS("else", TOKEN_ELSE, "else");
	PASS("endforeach", TOKEN_ENDFOREACH, "endforeach");
	PASS("endif", TOKEN_ENDIF, "endif");
	PASS("false", TOKEN_FALSE, "false");
	PASS("foreach", TOKEN_FOREACH, "foreach");
	PASS("if", TOKEN_IF, "if");
	PASS("in", TOKEN_IN, "in");
	PASS("not", TOKEN_NOT, "not");
	PASS("or", TOKEN_OR, "or");
	PASS("true", TOKEN_TRUE, "true");
}

static void test_identifiers(void)
{
	PASS("sample", TOKEN_IDENTIFIER, "sample");

	PASS("and_", TOKEN_IDENTIFIER, "and_");
	PASS("break_", TOKEN_IDENTIFIER, "break_");
	PASS("continue_", TOKEN_IDENTIFIER, "continue_");
	PASS("elif_", TOKEN_IDENTIFIER, "elif_");
	PASS("else_", TOKEN_IDENTIFIER, "else_");
	PASS("endforeach_", TOKEN_IDENTIFIER, "endforeach_");
	PASS("endif_", TOKEN_IDENTIFIER, "endif_");
	PASS("false_", TOKEN_IDENTIFIER, "false_");
	PASS("foreach_", TOKEN_IDENTIFIER, "foreach_");
	PASS("if_", TOKEN_IDENTIFIER, "if_");
	PASS("in_", TOKEN_IDENTIFIER, "in_");
	PASS("not_", TOKEN_IDENTIFIER, "not_");
	PASS("or_", TOKEN_IDENTIFIER, "or_");
	PASS("true_", TOKEN_IDENTIFIER, "true_");
}

static void test_punctuators(void)
{
	PASS("(", TOKEN_L_PAREN, "(");
	PASS(")", TOKEN_R_PAREN, ")");
	PASS("{", TOKEN_L_BRACE, "{");
	PASS("}", TOKEN_R_BRACE, "}");
	PASS("[", TOKEN_L_BRACKET, "[");
	PASS("]", TOKEN_R_BRACKET, "]");
	PASS(".", TOKEN_DOT, ".");
	PASS(",", TOKEN_COMMA, ",");
	PASS(":", TOKEN_COLON, ":");
}

static void test_operators(void)
{
	PASS("+", TOKEN_PLUS, "+");
	PASS("-", TOKEN_MINUS, "-");
	PASS("*", TOKEN_STAR, "*");
	PASS("/", TOKEN_SLASH, "/");
	PASS("%", TOKEN_PERCENT, "%");
	PASS("?", TOKEN_TERNARY, "?");

	PASS("=", TOKEN_ASSIGN, "=");
	PASS("+=", TOKEN_ADD_ASSIGN, "+=");
	PASS("*=", TOKEN_MUL_ASSIGN, "*=");
	PASS("/=", TOKEN_DIV_ASSIGN, "/=");
	PASS("%=", TOKEN_MOD_ASSIGN, "%=");

	PASS("==", TOKEN_EQ, "==");
	PASS("!=", TOKEN_NE, "!=");

	PASS(">", TOKEN_GT, ">");
	PASS(">=", TOKEN_GE, ">=");
	PASS("<", TOKEN_LT, "<");
	PASS("<=", TOKEN_LE, "<=");
}

static void test_backslash(void)
{
	PASS("\\sample", TOKEN_IDENTIFIER, "sample");
	PASS("\\ \n sample", TOKEN_IDENTIFIER, "sample");
}

TEST_LIST = {
	{ "spaces", test_spaces },
	{ "numbers", test_numbers },
	{ "strings", test_strings },
	{ "keywords", test_keywords },
	{ "identifiers", test_identifiers },
	{ "punctuation", test_punctuators },
	{ "operators", test_operators },
	{ "backslash", test_backslash },
	{ NULL, NULL }
};
