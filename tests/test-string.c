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
#include "common.h"
#include <string.h>

static void test_literal(void)
{
	struct string str;

	str = CSTRING("sample");
	TEST_CHECK(!string_is_null(str));
	TEST_CHECK(!strcmp(string_text(str), "sample"));
	TEST_CHECK(string_equal(str, str));
	TEST_CHECK(string_equal(str, CSTRING("sample")));

	string_free(&str);
}

static void test_alloc(void)
{
	struct string str;

	str = string_dup("sample");
	TEST_CHECK(!string_is_null(str));
	TEST_CHECK(!strcmp(string_text(str), "sample"));
	TEST_CHECK(string_equal(str, str));
	TEST_CHECK(string_equal(str, CSTRING("sample")));

	string_free(&str);
}

TEST_LIST = {
	{ "literal strings", test_literal },
	{ "allocated strings", test_alloc },
	{ NULL, NULL }
};
