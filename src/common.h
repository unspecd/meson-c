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

#ifndef COMMON_H
#define COMMON_H

#include "defs.h"

struct string_data {
	char *buffer;
	size_t length;
};

struct string string_alloc(size_t length);
struct string string_dup(const char *s);
struct string string_dup_n(const char *s, size_t length);
void          string_free(struct string *s);
char *        string_buffer(struct string s);
bool          string_equal(struct string a, struct string b);

static inline struct string string_from_buf_n(const char *buffer, size_t length)
{
	return (struct string) {
		.buffer = (char *) buffer,
		.valid = 1,
		.length = length,
		.raw = 1
	};
}

#define CSTRING(literal) string_from_buf_n(literal, sizeof(literal) - 1)

static inline struct string string_from_buf(const char *s)
{
	return string_from_buf_n(s, strlen(s));
}

static inline bool string_is_null(struct string s)
{
	return !s.valid;
}

static inline size_t string_length(struct string s)
{
	return s.length;
}

static inline const char *string_text(struct string s)
{
	return string_buffer(s);
}

void *mem_alloc(size_t size);
void  mem_free(void *ptr, size_t size);

#define ALLOC_SIZEOF(expr) mem_alloc(sizeof(expr))

#endif /* COMMON_H */
