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

#include "ast.h"
#include "common.h"

#include <stdlib.h>

static void free_ast_list(struct list *head)
{
	struct ast *ast = NULL;
	struct list_node *it = NULL;

	while ((ast = list_enum(head, &it)) != NULL) {
		ast_free(ast);
	}
}

static void ast_free_app(struct ast_app *app)
{
	struct ast_kw_arg *kw = NULL;
	struct list_node *it = NULL;

	if (app->ref != NULL) {
		ast_free(app->ref);
	}
	free_ast_list(&app->args);

	while ((kw = list_enum(&app->kw_args, &it)) != NULL) {
		ast_free(kw);
	}
}

static void ast_free_foreach(struct ast_foreach *foreach)
{
	free_ast_list(&foreach->ids);

	if (foreach->exp != NULL) {
		ast_free(foreach->exp);
	}
	if (foreach->body != NULL) {
		ast_free(foreach->body);
	}
}

static void ast_free_if(struct ast_if *if_)
{
	struct ast_if_clause *c = NULL;
	struct list_node *it = NULL;

	while ((c = list_enum(&if_->clauses, &it)) != NULL) {
		ast_free(c);
	}

	if (if_->alt != NULL) {
		ast_free(if_->alt);
	}
}

void ast_free(void *ptr)
{
	struct ast *ast = ptr;

	assert(ast != NULL);

	switch (ast_type(ast)) {
	case AST_EMPTY:
		break;

	case AST_SEQUENCE:
		free_ast_list(&((struct ast_seq *) ptr)->exps);
		break;

	case AST_ASSIGNMENT:
	case AST_ARITHMETIC:
	case AST_RELATIONAL:
	case AST_LOGICAL:
		if (ast_subtype(ast) == AST_TERNARY) {
			struct ast_ternary *t = ptr;
			ast_free(t->pred);
			ast_free(t->conseq);
			ast_free(t->alt);
		} else {
			struct ast_binary *b = ptr;
			ast_free(b->lhs);
			ast_free(b->rhs);
		}
		break;

	case AST_IF:
		ast_free_if((struct ast_if *) ptr);
		break;

	case AST_IF_CLAUSE: {
		struct ast_if_clause *c = ptr;
		assert(c->pred != NULL);
		assert(c->conseq != NULL);
		ast_free(c->pred);
		ast_free(c->conseq);
		break;
	}

	case AST_FOREACH:
		ast_free_foreach((struct ast_foreach *) ptr);
		break;

	case AST_JUMP:
		break;

	case AST_UNARY:
		ast_free(((struct ast_unary *) ptr)->exp);
		break;

	case AST_INDEX: {
		struct ast_index *s = ptr;
		ast_free(s->ref);
		ast_free(s->index);
		break;
	}

	case AST_MEMBER: {
		struct ast_member *ref = ptr;
		ast_free(ref->obj);
		ast_free(ref->field);
		break;
	}

	case AST_APPLICATION:
		ast_free_app((struct ast_app *) ptr);
		break;

	case AST_KEYWORD_ARG: {
		struct ast_kw_arg *kw = ptr;
		ast_free(kw->id);
		ast_free(kw->exp);
		break;
	}

	case AST_ID: {
		struct ast_id *id = ptr;
		string_free(&id->name);
		break;
	}

	case AST_ARRAY:
		free_ast_list(&((struct ast_array *) ptr)->elts);
		break;

	case AST_DICTIONARY: {
		struct ast_dict *dict = ptr;
		struct ast_kv *kv = NULL;
		struct list_node *it = NULL;

		while ((kv = list_enum(&dict->map, &it)) != NULL) {
			ast_free(kv);
		}
		break;
	}

	case AST_KV: {
		struct ast_kv *kv = ptr;
		ast_free(kv->key);
		ast_free(kv->value);
		break;
	}

	case AST_NUMBER:
		break;

	case AST_STRING: {
		struct ast_string *s = ptr;
		string_free(&s->value);
		break;
	}

	case AST_BOOLEAN:
		break;

	default:
		UNREACHABLE();
	}

	mem_free(ast, ast->size);
}

void *ast_new(enum ast_type type, enum ast_subtype subtype)
{
	struct ast *ast = NULL;
	size_t size = 0;

	switch (type) {
	case AST_EMPTY:
		size = sizeof(struct ast);
		break;

	case AST_SEQUENCE:
		size = sizeof(struct ast_seq);
		break;

	case AST_ASSIGNMENT:
		size = sizeof(struct ast_binary);
		break;

	case AST_IF:
		size = sizeof(struct ast_if);
		break;

	case AST_IF_CLAUSE:
		size = sizeof(struct ast_if_clause);
		break;

	case AST_FOREACH:
		size = sizeof(struct ast_foreach);
		break;

	case AST_JUMP:
		size = sizeof(struct ast);
		break;

	case AST_UNARY:
		size = sizeof(struct ast_unary);
		break;

	case AST_LOGICAL:
		if (subtype == AST_TERNARY) {
			size = sizeof(struct ast_ternary);
		} else {
			size = sizeof(struct ast_binary);
		}
		break;

	case AST_ARITHMETIC:
	case AST_RELATIONAL:
		size = sizeof(struct ast_binary);
		break;

	case AST_INDEX:
		size = sizeof(struct ast_index);
		break;

	case AST_MEMBER:
		size = sizeof(struct ast_member);
		break;

	case AST_APPLICATION:
		size = sizeof(struct ast_app);
		break;

	case AST_KEYWORD_ARG:
		size = sizeof(struct ast_kw_arg);
		break;

	case AST_ID:
		size = sizeof(struct ast_id);
		break;

	case AST_ARRAY:
		size = sizeof(struct ast_array);
		break;

	case AST_DICTIONARY:
		size = sizeof(struct ast_dict);
		break;

	case AST_KV:
		size = sizeof(struct ast_kv);
		break;

	case AST_NUMBER:
		size = sizeof(struct ast_number);
		break;

	case AST_STRING:
		size = sizeof(struct ast_string);
		break;

	case AST_BOOLEAN:
		size = sizeof(struct ast_boolean);
		break;

	default:
		UNREACHABLE();
		return NULL;
	}

	if ((ast = mem_alloc(size)) == NULL) {
		return NULL;
	}
	ast->info = AST_MKINFO(type, subtype);
	ast->size = size;

	return ast;
}

struct ast *ast_empty(void)
{
	return ast_new(AST_EMPTY, AST_NONE);
}

struct ast_seq *ast_seq(void)
{
	struct ast_seq *seq = NULL;

	if ((seq = ast_new(AST_SEQUENCE, AST_NONE)) != NULL) {
		AST_LIST_INIT(&seq->exps);
	}

	return seq;
}

struct ast_binary *ast_binary(enum ast_type type,
			      enum ast_subtype subtype,
			      struct ast *lhs, struct ast *rhs)
{
	struct ast_binary *binary = NULL;

	assert(lhs != NULL);
	assert(rhs != NULL);

	if ((binary = ast_new(type, subtype)) != NULL) {
		binary->lhs = lhs;
		binary->rhs = rhs;
	}

	return binary;
}

struct ast_ternary *ast_ternary(struct ast *pred, struct ast *conseq, struct ast *alt)
{
	struct ast_ternary *ternary = NULL;

	assert(pred != NULL);
	assert(conseq != NULL);
	assert(alt != NULL);

	if ((ternary = ast_new(AST_LOGICAL, AST_TERNARY)) != NULL) {
		ternary->pred = pred;
		ternary->conseq = conseq;
		ternary->alt = alt;
	}

	return ternary;
}

struct ast_if *ast_if(void)
{
	struct ast_if *if_ = NULL;

	if ((if_ = ast_new(AST_IF, AST_NONE)) != NULL) {
		AST_LIST_INIT(&if_->clauses);
		if_->alt = NULL;
	}

	return if_;
}

struct ast_if_clause *ast_if_clause(struct ast_if *if_,
				    struct ast *pred, struct ast *conseq)
{
	struct ast_if_clause *c = NULL;

	if ((c = ast_new(AST_IF_CLAUSE, AST_NONE)) != NULL) {
		c->pred = pred;
		c->conseq = conseq;
		list_append(&if_->clauses, c);
	}

	return c;
}

struct ast_foreach *ast_foreach(void)
{
	struct ast_foreach *foreach = NULL;

	if ((foreach = ast_new(AST_FOREACH, AST_NONE)) != NULL) {
		AST_LIST_INIT(&foreach->ids);
		foreach->exp = NULL;
		foreach->body = NULL;
	}

	return foreach;
}

struct ast_member *ast_member(struct ast *obj, struct ast *field)
{
	struct ast_member *m = NULL;

	assert(obj != NULL);
	assert(field != NULL);

	if ((m = ast_new(AST_MEMBER, AST_NONE)) != NULL) {
		m->obj = obj;
		m->field = field;
	}

	return m;
}

struct ast_app *ast_app(void)
{
	struct ast_app *app = NULL;

	if ((app = ast_new(AST_APPLICATION, AST_NONE)) != NULL) {
		AST_LIST_INIT(&app->args);
		AST_LIST_INIT(&app->kw_args);
	}

	return app;
}

struct ast_id *ast_id(struct string s)
{
	struct ast_id *id = NULL;

	if ((id = ast_new(AST_ID, AST_NONE)) != NULL) {
		id->name = string_dup(string_text(s));
	}

	return id;
}

struct ast_boolean *ast_boolean(bool value)
{
	return ast_new(AST_BOOLEAN, value ? AST_TRUE : AST_FALSE);
}

struct ast_number *ast_number(int64_t value)
{
	struct ast_number *node;

	if ((node = ast_new(AST_NUMBER, AST_NONE)) != NULL) {
		node->value = value;
	}

	return node;
}

struct ast_string *ast_string(struct string s)
{
	struct ast_string *str = NULL;

	if ((str = ast_new(AST_STRING, AST_NONE)) != NULL) {
		str->value = string_dup(string_text(s));
	}

	return str;
}

struct ast_array *ast_array(void)
{
	struct ast_array *array = NULL;

	if ((array = ast_new(AST_ARRAY, AST_NONE)) != NULL) {
		AST_LIST_INIT(&array->elts);
	}

	return array;
}

struct ast_dict *ast_dict(void)
{
	struct ast_dict *dict = NULL;

	if ((dict = ast_new(AST_DICTIONARY, AST_NONE)) != NULL) {
		AST_LIST_INIT(&dict->map);
	}

	return dict;
}

const char *ast_type_name(enum ast_type type)
{
	switch (type) {
#define GEN(N) \
	case AST_##N: return #N;
		AST_TYPE_MAP(GEN)
#undef GEN
	default: return "unknown";
	}
}
