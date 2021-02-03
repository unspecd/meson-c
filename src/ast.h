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

#ifndef AST_H
#define AST_H

#include "list.h"
#include <stdarg.h>

#define AST_TYPE_MAP(X)		\
	X(EMPTY)		\
	X(SEQUENCE)		\
	X(ASSIGNMENT)		\
	X(IF)			\
	X(IF_CLAUSE)		\
	X(FOREACH)		\
	X(JUMP)			\
	X(UNARY)		\
	X(LOGICAL)		\
	X(ARITHMETIC)		\
	X(RELATIONAL)		\
	X(MEMBER)		\
	X(INDEX)		\
	X(APPLICATION)		\
	X(KEYWORD_ARG)		\
	X(ID)			\
	X(BOOLEAN)		\
	X(NUMBER)		\
	X(STRING)		\
	X(ARRAY)		\
	X(DICTIONARY)		\
	X(KV)

enum ast_type {
#define GEN(N) AST_##N,
	AST_TYPE_MAP(GEN)
#undef GEN
};

enum ast_subtype {
	AST_NONE,
	/* Assignment */
	AST_ASSIGN,
	AST_ADD_ASSIGN, AST_SUB_ASSIGN,
	AST_MUL_ASSIGN, AST_DIV_ASSIGN,
	AST_MOD_ASSIGN,
	/* Escape and return */
	AST_BREAK, AST_CONTINUE,
	/* Unary operations */
	AST_NOT, AST_PLUS, AST_MINUS,
	/* Logical operations */
	AST_OR, AST_AND, AST_TERNARY,
	/* Arithmetic operations */
	AST_ADD, AST_SUB, AST_MUL, AST_DIV, AST_MOD,
	/* Equality and relational operators */
	AST_EQ, AST_NE, AST_LT, AST_LE,
	AST_GT, AST_GE, AST_IN, AST_NOT_IN,
	/* Boolean values */
	AST_TRUE, AST_FALSE
};

#define AST_TYPE(info) \
	((enum ast_type) ((info) & 0xff))

#define AST_SUBTYPE(info) \
	((enum ast_subtype) ((info) >> 8))

#define AST_MKINFO(type, subtype) \
	((type) | (subtype << 8))

struct ast {
	int              info;
	struct list_node node;
	size_t           size;
};

#define AST_LIST_INIT(list) \
	LIST_INIT(list, struct ast, node)

struct ast_id {
	struct ast base;
	struct string name;
};

struct ast_seq {
	struct ast base;
	/* List of `ast` */
	struct list exps;
};

struct ast_if {
	struct ast base;
	/* List of `ast_if_clause` */
	struct list clauses;
	struct ast *alt;
};

struct ast_if_clause {
	struct ast base;
	struct ast *pred;
	struct ast *conseq;
};

struct ast_foreach {
	struct ast base;
	/* List of `ast_id' */
	struct list ids;
	struct ast *exp;
	struct ast *body;
};

struct ast_unary {
	struct ast base;
	struct ast *exp;
};

struct ast_binary {
	struct ast base;
	struct ast *lhs;
	struct ast *rhs;
};

struct ast_ternary {
	struct ast base;
	struct ast *pred;
	struct ast *conseq;
	struct ast *alt;
};

struct ast_member {
	struct ast base;
	struct ast *obj;
	struct ast *field;
};

struct ast_index {
	struct ast base;
	struct ast *ref;
	struct ast *index;
};

struct ast_app {
	struct ast base;
	struct ast *ref;
	/* List of `ast' */
	struct list args;
	/* List of `ast_kw_arg' */
	struct list kw_args;
};

struct ast_kw_arg {
	struct ast base;
	struct ast_id *id;
	struct ast *exp;
};

struct ast_array {
	struct ast base;
	size_t     count;
	/* List of `ast' */
	struct list elts;
};

struct ast_dict {
	struct ast base;
	/* List of `ast_kv' */
	struct list map;
};

struct ast_kv {
	struct ast base;
	struct ast *key;
	struct ast *value;
};

struct ast_string {
	struct ast base;
	struct string value;
};

struct ast_number {
	struct ast base;
	int64_t	   value;
};

struct ast_boolean {
	struct ast base;
};

/* Constructors */

void *ast_new(enum ast_type type, enum ast_subtype subtype);
struct ast *ast_empty(void);
struct ast_seq *ast_seq(void);
struct ast_binary *ast_binary(enum ast_type type,
	enum ast_subtype subtype, struct ast *lhs, struct ast *rhs);
struct ast_ternary *ast_ternary(struct ast *pred,
				struct ast *conseq, struct ast *alt);
struct ast_if *ast_if(void);
struct ast_if_clause *ast_if_clause(struct ast_if *if_,
				    struct ast *pred, struct ast *conseq);
struct ast_foreach *ast_foreach(void);
struct ast_member *ast_member(struct ast *obj, struct ast *field);
struct ast_app *ast_app(void);
struct ast_id *ast_id(struct string s);
struct ast_boolean *ast_boolean(bool value);
struct ast_number *ast_number(int64_t value);
struct ast_string *ast_string(struct string s);
struct ast_array *ast_array(void);
struct ast_dict *ast_dict(void);

void ast_free(void *ast);

static inline struct ast *as_ast(void *ptr)
{
	return ptr;
}

static inline enum ast_type ast_type(void *ptr)
{
	return AST_TYPE(as_ast(ptr)->info);
}

static inline enum ast_subtype ast_subtype(void *ptr)
{
	return AST_SUBTYPE(as_ast(ptr)->info);
}

static inline void *ast_as(void *ptr, enum ast_type type)
{
	if (ast_type(ptr) != type) {
		return NULL;
	}
	return ptr;
}

const char *ast_type_name(enum ast_type type);

#endif /* AST_H */
