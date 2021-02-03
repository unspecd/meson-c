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

#ifndef LIST_H
#define LIST_H

#include "defs.h"
#include <assert.h>

struct list_node;

struct list_node {
	struct list_node *next;
	struct list_node *prev;
};

struct list {
	struct list_node head;
	size_t offset;
};

static inline void list_init(struct list *list, size_t offset)
{
	list->head.next = &list->head;
	list->head.prev = &list->head;
	list->offset = offset;
}

#define LIST_INIT(list, type, field) \
	list_init(list, offsetof(type, field))

#define NODE_FROM_DATA(list, data) \
	((struct list_node *) ((size_t) data + list->offset))

#define DATA_FROM_NODE(list, node) \
	((struct list_node *) ((size_t) node - list->offset))

static inline bool list_empty(struct list *list)
{
	assert(list->head.next != NULL);
	assert(list->head.prev != NULL);

	return list->head.next == &list->head;
}

static inline void list_append(struct list *list, void *data)
{
	struct list_node *head = &list->head;
	struct list_node *node = NULL;
	struct list_node *prev = NULL;

	assert(head->next != NULL);
	assert(head->prev != NULL);
	assert(data != NULL);

	node = NODE_FROM_DATA(list, data);
	assert(node->next == NULL);
	assert(node->prev == NULL);

	prev = head->prev;

	node->next = head;
	node->prev = prev;

	prev->next = node;
	head->prev = node;
}

static inline void *list_enum(struct list *list, struct list_node **iterator)
{
	struct list_node *node = *iterator;

	if (node == NULL) {
		node = list->head.next;
	}
	if (node == &list->head) {
		*iterator = NULL;
		return NULL;
	}
	*iterator = node->next;

	return DATA_FROM_NODE(list, node);
}

#endif /* LIST_H */
