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

#include "common.h"
#include <stdlib.h>
#include <string.h>

void *mem_alloc(size_t size)
{
	void *ptr = NULL;

	if ((ptr = malloc(size)) != NULL) {
		memset(ptr, 0, size);
	}

	return ptr;
}

void mem_free(void *ptr, size_t size)
{
	memset(ptr, 0, size);
	free(ptr);
}

struct string string_alloc(size_t length)
{
	struct string_data *d = NULL;

	if ((d = mem_alloc(sizeof(*d) + length + 1)) == NULL) {
		return NULL_STRING;
	}
	d->buffer = (void *) (d + 1);
	d->length = length;

	return (struct string) { .data = d, .valid = 1, .length = length };
}

struct string string_dup(const char *s)
{
	return string_dup_n(s, strlen(s));
}

struct string string_dup_n(const char *s, size_t length)
{
	struct string str;

	if (!(str = string_alloc(length)).valid) {
		return NULL_STRING;
	}
	memcpy(str.data->buffer, s, length);

	return str;
}

void string_free(struct string *s)
{
	if (!s->raw) {
		if (s->data != NULL) {
			mem_free(s->data, 0);
		}
	}
	*s = NULL_STRING;

}

char *string_buffer(struct string s)
{
	if (string_is_null(s)) {
		return NULL;
	}
	if (s.raw) {
		return s.buffer;
	}
	return s.data->buffer;
}

bool string_equal(struct string a, struct string b)
{
	size_t length = string_length(a);

	return length == string_length(b) &&
		memcmp(string_buffer(a), string_buffer(b), length) == 0;
}
