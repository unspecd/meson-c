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

#include "parser.h"
#include "ast.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>

enum parse_status { SUCCESS, FAILURE, NO_MEMORY };

struct result {
	enum parse_status status;
	union {
		struct ast *ast;
		struct string error;
	};
};

static enum token_type peek(struct parser *p)
{
	if (!p->ready) {
		p->token = lex(&p->lexer);
		p->ready = true;
	}
	return p->token;
}

/* varargs: (enum token_type, int) x count */
static int accept_any(struct parser *p, size_t count, ...)
{
	int ret = -1;
	va_list args;
	va_start(args, count);

	peek(p);
	for (size_t i = 0; i < count; i++) {
		int token = va_arg(args, int);
		int result = va_arg(args, int);
		if ((enum token_type) token == p->token) {
			p->ready = false;
			ret = result;
			break;
		}
	}

	va_end(args);
	return ret;
}

static bool accept(struct parser *p, enum token_type token)
{
	return accept_any(p, 1, token, 1) == 1;
}

static struct result with_ast(void *ast)
{
	return (struct result) {
		.status = ast == NULL ? NO_MEMORY : SUCCESS, .ast = ast
	};
}

static struct result with_error(struct parser *p, const char *format, ...)
{
	va_list args;
	char buffer[1024];

	UNUSED(p);

	va_start(args, format);
	buffer[0] = '\0';
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	return (struct result) {
		.status = FAILURE,
		.error = string_dup(buffer)
	};
}

static struct result with_status(enum parse_status status)
{
	return (struct result) { .status = status };
}

static struct result with_expected(struct parser *p,
				   const char *where, const char *what)
{
	return with_error(p, "%s: expected %s", where, what);
}

static struct result expect(struct parser *p, enum token_type token,
			    const char *where, const char *what)
{
	if (accept(p, token)) {
		return with_status(SUCCESS);
	}
	return with_expected(p, where, what);
}

static struct result maybe_identifier(struct parser *p)
{
	struct ast *id = NULL;
	struct string s = NULL_STRING;

	if (accept(p, TOKEN_IDENTIFIER)) {
		if (!(s = string_dup_n(p->lexer.lexeme,
				       p->lexer.lexeme_len)).valid) {
			return with_status(NO_MEMORY);
		}
		id = (struct ast *) ast_id(s);
		string_free(&s);
		return with_ast(id);
	} else {
		return with_ast(ast_empty());
	}
}

static struct result expression(struct parser *p);

static struct result dictionary(struct parser *p)
{
	struct ast_dict *dict = NULL;
	struct ast *key = NULL;
	struct ast *val = NULL;
	struct ast_kv *kv = NULL;
	struct result res = { .status = FAILURE };

	if ((res = expect(p, TOKEN_L_BRACE,
			  "dictionary", "opening brace")).status) {
		return res;
	}

	if ((dict = ast_dict()) == NULL) {
		return with_status(NO_MEMORY);
	}

	if (accept(p, TOKEN_R_BRACE)) {
		return with_ast(dict);
	}

	do {
		if ((res = expression(p)).status) {
			goto cleanup;
		}
		if (ast_type(key = res.ast) == AST_EMPTY) {
			res = with_expected(p, "dictionary", "key");
			goto cleanup;
		}

		if (!accept(p, TOKEN_COLON)) {
			res = with_expected(p, "dictionary", "colon");
			goto cleanup;
		}

		if ((res = expression(p)).status) {
			goto cleanup;
		}
		if (ast_type(val = res.ast) == AST_EMPTY) {
			res = with_expected(p, "dictionary", "value");
			goto cleanup;
		}

		if ((kv = ast_new(AST_KV, AST_NONE)) == NULL) {
			res = with_status(NO_MEMORY);
			goto cleanup;
		}

		kv->key = key;
		kv->value = val;
		list_append(&dict->map, kv);

		key = NULL;
		val = NULL;
	} while (accept(p, TOKEN_COMMA) && peek(p) != TOKEN_R_BRACE);

	if (!accept(p, TOKEN_R_BRACE)) {
		ast_free(dict);
		return with_expected(p, "dictionary", "closing brace");
	}

	return with_ast(dict);

cleanup:
	if (key != NULL) {
		ast_free(key);
	}
	if (val != NULL) {
		ast_free(val);
	}
	ast_free(dict);
	return res;
}

static struct result array(struct parser *p)
{
	struct ast *exp = NULL;
	struct ast_array *array = NULL;
	struct result res = { .status = FAILURE };
	size_t count = 0;

	if ((res = expect(p, TOKEN_L_BRACKET,
			  "array", "opening bracket")).status) {
		return res;
	}

	if ((array = ast_array()) == NULL) {
		return with_status(NO_MEMORY);
	}

	if (accept(p, TOKEN_R_BRACKET)) {
		return with_ast(array);
	}

	do {
		if ((res = expression(p)).status) {
			ast_free(array);
			return res;
		}
		if (ast_type(exp = res.ast) == AST_EMPTY) {
			ast_free(array);
			ast_free(exp);
			return with_expected(p, "array", "expression");
		}
		list_append(&array->elts, exp);
		count++;

	} while (accept(p, TOKEN_COMMA) && peek(p) != TOKEN_R_BRACKET);

	if (!accept(p, TOKEN_R_BRACKET)) {
		ast_free(array);
		return with_expected(p, "array", "closing bracket");
	}
	array->count = count;

	return with_ast(array);
}

static struct result string(struct parser *p)
{
	struct string s = NULL_STRING;
	struct ast_string *str = NULL;
	struct result res = { .status = FAILURE };

	if ((res = expect(p, p->token,
			  "literal", "string literal")).status) {
		return res;
	}
	if (!(s = string_dup_n(p->lexer.lexeme, p->lexer.lexeme_len)).valid) {
		return with_status(NO_MEMORY);
	}
	str = ast_string(s);
	string_free(&s);

	return with_ast(str);
}

static struct result number(struct parser *p)
{
	int base = 10;
	struct result res = { .status = FAILURE };
	const char *lexeme = p->lexer.lexeme;

	if ((res = expect(p, p->token,
			  "literal", "integer constant")).status) {
		return res;
	}

	switch (p->token) {
	case TOKEN_BIN_NUMBER: base =  2; break;
	case TOKEN_OCT_NUMBER: base =  8; break;
	case TOKEN_DEC_NUMBER: base = 10; break;
	case TOKEN_HEX_NUMBER: base = 16; break;
	default:
		return with_error(p, "invalid number: `%s'", lexeme);
	}

	return with_ast(ast_number(strtoll(lexeme, NULL, base)));
}

static struct result boolean(struct parser *p)
{
	struct result res = { .status = FAILURE };

	if ((res = expect(p, p->token,
			  "literal", "boolean value")).status) {
		return res;
	}
	return with_ast(ast_boolean(p->token == TOKEN_TRUE ? true : false));
}

static struct result literal(struct parser *p)
{
	switch (peek(p)) {
	case TOKEN_TRUE:
	case TOKEN_FALSE:
		return boolean(p);
	case TOKEN_DEC_NUMBER:
	case TOKEN_BIN_NUMBER:
	case TOKEN_OCT_NUMBER:
	case TOKEN_HEX_NUMBER:
		return number(p);
	case TOKEN_STRING:
	case TOKEN_MULTILINE_STRING:
		return string(p);
	case TOKEN_L_BRACKET:
		return array(p);
	case TOKEN_L_BRACE:
		return dictionary(p);
	default:
		return with_ast(ast_empty());
	}
}

static struct result primary(struct parser *p)
{
	struct ast *exp = NULL;
	struct result res = { .status = FAILURE };

	switch (peek(p)) {
	case TOKEN_L_PAREN:
		accept(p, TOKEN_L_PAREN);
		if ((res = expression(p)).status) {
			return res;
		}
		if (ast_type(exp = res.ast) == AST_EMPTY) {
			ast_free(exp);
			return with_error(p, "invalid expression");
		}
		if (!accept(p, TOKEN_R_PAREN)) {
			ast_free(exp);
			return with_error(p, "expected closing paren");
		}
		return with_ast(exp);
	case TOKEN_IDENTIFIER:
		return maybe_identifier(p);
	default:
		return literal(p);
	}
}

static struct result subscript(struct parser *p)
{
	struct ast *index = NULL;
	struct ast_index *ast = NULL;
	struct result res = { .status = FAILURE };

	if ((res = expect(p, TOKEN_L_BRACKET,
			  "subscript", "opening bracket")).status) {
		return res;
	}

	if ((res = expression(p)).status) {
		return res;
	}
	if (ast_type(index = res.ast) == AST_EMPTY) {
		ast_free(index);
		return with_expected(p, "subscript", "expression");
	}

	if (!accept(p, TOKEN_R_BRACKET)) {
		ast_free(index);
		return with_expected(p, "subscript", "closing bracket");
	}
	if ((ast = ast_new(AST_INDEX, AST_NONE)) == NULL) {
		ast_free(index);
		return with_status(NO_MEMORY);
	}
	ast->index = index;
	return with_ast(ast);
}

static struct result application(struct parser *p)
{
	struct ast_app *app = NULL;
	struct result res = { .status = FAILURE };
	bool keywords = false;

	if ((res = expect(p, TOKEN_L_PAREN,
			  "application", "opening paren")).status) {
		return res;
	}

	if ((app = ast_app()) == NULL) {
		return with_status(NO_MEMORY);
	}

	if (accept(p, TOKEN_R_PAREN)) {
		return with_ast(app);
	}

	/* Parse arguments. */
	do {
		bool colon = false;
		struct ast *arg = NULL;

		if ((res = expression(p)).status) {
			ast_free(app);
			return res;
		}
		if (ast_type(arg = res.ast) == AST_EMPTY) {
			ast_free(app);
			ast_free(arg);
			return with_expected(p, "application", "argument");
		}

		colon = accept(p, TOKEN_COLON);
		keywords |= colon;

		if (keywords && !colon) {
			ast_free(app);
			ast_free(arg);
			return with_expected(p, "application", "keyword");
		}
		if (!keywords) {
			list_append(&app->args, arg);
		} else {
			struct ast_kw_arg *kw = NULL;
			struct ast *value = NULL;

			if (ast_type(arg) != AST_ID) {
				ast_free(app);
				ast_free(arg);
				return with_expected(p, "application", "kwarg name");
			}
			if ((res = expression(p)).status) {
				ast_free(app);
				ast_free(arg);
				return res;
			}
			if (ast_type((value = res.ast)) == AST_EMPTY) {
				ast_free(app);
				ast_free(arg);
				ast_free(value);
				return with_expected(p, "application", "kwarg value");
			}
			if ((kw = ast_new(AST_KEYWORD_ARG, AST_NONE)) == NULL) {
				ast_free(app);
				ast_free(arg);
				ast_free(value);
				return with_status(NO_MEMORY);
			}
			kw->id = (struct ast_id *) arg;
			kw->exp = value;
			list_append(&app->kw_args, kw);
		}
	} while (accept(p, TOKEN_COMMA) && peek(p) != TOKEN_R_PAREN);

	if (!accept(p, TOKEN_R_PAREN)) {
		ast_free(app);
		return with_expected(p, "application", "closing paren");
	}

	return with_ast(app);
}

static struct result postfix(struct parser *p)
{
	struct ast *ast = NULL;
	struct result res = { .status = FAILURE };

	if ((res = primary(p)).status) {
		return res;
	}
	if (ast_type(ast = res.ast) != AST_ID) {
		return res;
	}

	while (res.status == 0) {
		switch (peek(p)) {
		case TOKEN_DOT: {
			struct ast *right = NULL;
			struct ast_member *ref = NULL;
			accept(p, TOKEN_DOT);
			if ((res = primary(p)).status) {
				break;
			}
			if (ast_type(right = res.ast) == AST_EMPTY) {
				ast_free(right);
				res = with_error(p, "expected field name");
				break;
			}
			if (ast_type(right) != AST_ID) {
				ast_free(right);
				res = with_error(p,
					 "field name must be plain id");
				break;
			}
			if ((ref = ast_member(ast, right)) == NULL) {
				res = with_status(NO_MEMORY);
				break;
			}
			ast = as_ast(ref);
			break;
		}
		case TOKEN_L_PAREN: {
			struct ast_app *app = NULL;
			if ((res = application(p)).status == 0) {
				app = (struct ast_app *) res.ast;
				app->ref = ast;
				ast = as_ast(app);
			}
			break;
		}
		case TOKEN_L_BRACKET: {
			struct ast_index *index = NULL;
			if ((res = subscript(p)).status == 0) {
				index = (struct ast_index *) res.ast;
				index->ref = ast;
				ast = as_ast(index);
			}
			break;
		}
		default:
			return with_ast(ast);
		}
	}

	ast_free(ast);
	return res;
}

static struct result unary(struct parser *p)
{
	struct ast *ast = NULL;
	struct ast_unary *unary = NULL;
	struct result res = { .status = FAILURE };
	int ret = -1;

	ret = accept_any(p, 3,
		 TOKEN_NOT, AST_NOT,
		 TOKEN_PLUS, AST_PLUS,
		 TOKEN_MINUS, AST_MINUS);

	if ((res = postfix(p)).status) {
		return res;
	}
	if (ast_type(ast = res.ast) == AST_EMPTY) {
		if (ret == -1) {
			return with_ast(ast);
		}
		ast_free(ast);
		return with_expected(p, "unary", "expression");
	}
	if (ret != -1) {
		if ((unary = ast_new(AST_UNARY, ret)) == NULL) {
			ast_free(ast);
			return with_status(NO_MEMORY);
		}
		unary->exp = ast;
		ast = as_ast(unary);
	}

	return with_ast(ast);
}

static struct result multiplicative(struct parser *p)
{
	struct ast *ast = NULL;
	struct ast *rhs = NULL;
	struct ast_binary *b = NULL;
	struct result res = { .status = FAILURE };
	int ret = -1;

	if ((res = unary(p)).status) {
		return res;
	}
	ast = res.ast;

	while ((ret = accept_any(p, 3,
			 TOKEN_STAR, AST_MUL,
			 TOKEN_PERCENT, AST_MOD,
			 TOKEN_SLASH, AST_DIV)) != -1) {
		if ((res = unary(p)).status) {
			ast_free(ast);
			return res;
		}
		if (ast_type(rhs = res.ast) == AST_EMPTY) {
			ast_free(ast);
			ast_free(rhs);
			return with_expected(p, "multiplicative", "expression");
		}
		if ((b = ast_binary(AST_ARITHMETIC, ret, ast, rhs)) == NULL) {
			ast_free(ast);
			ast_free(rhs);
			return with_status(NO_MEMORY);
		}

		ast = as_ast(b);
	}

	return with_ast(ast);
}

static struct result additive(struct parser *p)
{
	struct ast *ast = NULL;
	struct ast *rhs = NULL;
	struct ast_binary *b = NULL;
	struct result res = { .status = FAILURE };
	int ret = -1;

	if ((res = multiplicative(p)).status) {
		return res;
	}
	ast = res.ast;

	while ((ret = accept_any(p, 2,
			 TOKEN_PLUS, AST_ADD,
			 TOKEN_MINUS, AST_SUB)) != -1) {
		if ((res = multiplicative(p)).status) {
			ast_free(ast);
			return res;
		}
		if (ast_type(rhs = res.ast) == AST_EMPTY) {
			ast_free(ast);
			ast_free(rhs);
			return with_expected(p, "additive", "expression");
		}
		if ((b = ast_binary(AST_ARITHMETIC, ret, ast, rhs)) == NULL) {
			ast_free(ast);
			ast_free(rhs);
			return with_status(NO_MEMORY);
		}

		ast = as_ast(b);
	}

	return with_ast(ast);
}

static struct result relational(struct parser *p)
{
	struct ast *lhs = NULL;
	struct ast *rhs = NULL;
	struct ast_binary *b = NULL;
	struct result res = { .status = FAILURE };
	int ret = -1;

	if ((res = additive(p)).status) {
		return res;
	}
	lhs = res.ast;

	if ((ret = accept_any(p, 6,
		      TOKEN_LT, AST_LT,
		      TOKEN_LE, AST_LE,
		      TOKEN_GT, AST_GT,
		      TOKEN_GE, AST_GE,
		      TOKEN_IN, AST_IN,
		      TOKEN_NOT, AST_NOT_IN)) != -1) {
		if (ret == AST_NOT_IN && !accept(p, TOKEN_IN)) {
			ast_free(lhs);
			return with_error(p, "expected `in' after `not'");
		}

		if ((res = additive(p)).status) {
			ast_free(lhs);
			return res;
		}
		if (ast_type(rhs = res.ast) == AST_EMPTY) {
			ast_free(lhs);
			ast_free(rhs);
			return with_expected(p, "relational", "expression");
		}

		if ((b = ast_binary(AST_RELATIONAL, ret, lhs, rhs)) == NULL) {
			ast_free(lhs);
			ast_free(rhs);
			return with_status(NO_MEMORY);
		}
		return with_ast(b);
	}

	return with_ast(lhs);
}

static struct result equality(struct parser *p)
{
	struct ast *lhs = NULL;
	struct ast *rhs = NULL;
	struct ast_binary *b = NULL;
	struct result res = { .status = FAILURE };
	int ret = -1;

	if ((res = relational(p)).status) {
		return res;
	}
	lhs = res.ast;

	if ((ret = accept_any(p, 2,
		      TOKEN_EQ, AST_EQ,
		      TOKEN_NE, AST_NE)) != -1) {
		if ((res = relational(p)).status) {
			ast_free(lhs);
			return res;
		}
		if (ast_type(rhs = res.ast) == AST_EMPTY) {
			ast_free(lhs);
			ast_free(rhs);
			return with_expected(p, "equality", "expression");
		}

		if ((b = ast_binary(AST_RELATIONAL, ret, lhs, rhs)) == NULL) {
			ast_free(lhs);
			ast_free(rhs);
			return with_status(NO_MEMORY);
		}
		return with_ast(b);
	}

	return with_ast(lhs);
}

static struct result logical_and(struct parser *p)
{
	struct ast *ast = NULL;
	struct ast *rhs = NULL;
	struct ast_binary *b = NULL;
	struct result res = { .status = FAILURE };

	if ((res = equality(p)).status) {
		return res;
	}
	ast = res.ast;

	while (accept(p, TOKEN_AND)) {
		if ((res = equality(p)).status) {
			ast_free(ast);
			return res;
		}
		if (ast_type(rhs = res.ast) == AST_EMPTY) {
			ast_free(ast);
			ast_free(rhs);
			return with_expected(p, "logical and", "expression");
		}
		if ((b = ast_binary(AST_LOGICAL, AST_AND, ast, rhs)) == NULL) {
			ast_free(ast);
			ast_free(rhs);
			return with_status(NO_MEMORY);
		}

		ast = as_ast(b);
	}

	return with_ast(ast);
}

static struct result logical_or(struct parser *p)
{
	struct ast *ast = NULL;
	struct ast *rhs = NULL;
	struct ast_binary *b = NULL;
	struct result res = { .status = FAILURE };

	if ((res = logical_and(p)).status) {
		return res;
	}
	ast = res.ast;

	while (accept(p, TOKEN_OR)) {
		if ((res = logical_and(p)).status) {
			ast_free(ast);
			return res;
		}
		if (ast_type(rhs = res.ast) == AST_EMPTY) {
			ast_free(ast);
			ast_free(rhs);
			return with_expected(p, "logical or", "expression");
		}

		if ((b = ast_binary(AST_LOGICAL, AST_OR, ast, rhs)) == NULL) {
			ast_free(ast);
			ast_free(rhs);
			return with_status(NO_MEMORY);
		}

		ast = as_ast(b);
	}

	return with_ast(ast);
}

static struct result conditional(struct parser *p)
{
	struct ast *exp = NULL;
	struct ast *conseq = NULL;
	struct ast *alt = NULL;
	struct ast_ternary *t = NULL;
	struct result res = { .status = FAILURE };

	if ((res = logical_or(p)).status) {
		return res;
	}
	exp = res.ast;

	if (!accept(p, TOKEN_TERNARY)) {
		return with_ast(exp);
	}

	/* Parse ternary expression. */

	if ((res = expression(p)).status) {
		ast_free(exp);
		return res;
	}
	if (ast_type(conseq = res.ast) == AST_EMPTY) {
		ast_free(exp);
		ast_free(conseq);
		return with_expected(p, "ternary", "true clause");
	}

	if (!accept(p, TOKEN_COLON)) {
		ast_free(exp);
		ast_free(conseq);
		return with_expected(p, "ternary", "colon");
	}

	if ((res = expression(p)).status) {
		ast_free(exp);
		ast_free(conseq);
		return res;
	}
	if (ast_type(alt = res.ast) == AST_EMPTY) {
		ast_free(exp);
		ast_free(conseq);
		ast_free(alt);
		return with_expected(p, "ternary", "false clause");
	}
	if ((t = ast_ternary(exp, conseq, alt)) == NULL) {
		ast_free(exp);
		ast_free(conseq);
		ast_free(alt);
		return with_status(NO_MEMORY);
	}

	return with_ast(t);
}

static struct result assignment(struct parser *p)
{
	struct ast *lhs = NULL;
	struct ast *rhs = NULL;
	struct ast_binary *b = NULL;
	struct result res = { .status = FAILURE };
	int ret = -1;

	if ((res = conditional(p)).status) {
		return res;
	}
	lhs = res.ast;

	/* Parse assignment. */
	if ((ret = accept_any(p, 6,
		      TOKEN_ASSIGN, AST_ASSIGN,
		      TOKEN_ADD_ASSIGN, AST_ADD_ASSIGN,
		      TOKEN_SUB_ASSIGN, AST_SUB_ASSIGN,
		      TOKEN_MUL_ASSIGN, AST_MUL_ASSIGN,
		      TOKEN_DIV_ASSIGN, AST_DIV_ASSIGN,
		      TOKEN_MOD_ASSIGN, AST_MOD_ASSIGN)) != -1) {
		if (ast_type(lhs) != AST_ID) {
			ast_free(lhs);
			return with_error(p, "assignment target must be an id");
		}
		if ((res = expression(p)).status) {
			ast_free(lhs);
			return res;
		}
		if (ast_type(rhs = res.ast) == AST_EMPTY) {
			ast_free(lhs);
			ast_free(rhs);
			return with_expected(p, "assignment", "expression");
		}

		if ((b = ast_binary(AST_ASSIGNMENT, ret, lhs, rhs)) == NULL) {
			ast_free(lhs);
			ast_free(rhs);
			return with_status(NO_MEMORY);
		}
		return with_ast(b);
	}

	return with_ast(lhs);
}

static struct result expression(struct parser *p)
{
	return assignment(p);
}

static struct result sequence(struct parser *p);

static struct result iteration(struct parser *p)
{
	struct ast *id = NULL;
	struct ast_foreach *foreach = NULL;
	struct ast *exp = NULL;
	struct result res = { .status = FAILURE };

	if ((res = expect(p, TOKEN_FOREACH,
			  "foreach", "`foreach' keyword")).status) {
		return res;
	}

	if ((foreach = ast_foreach()) == NULL) {
		return with_status(NO_MEMORY);
	}

	/* Parse bound variables. */
	do {
		if ((res = maybe_identifier(p)).status) {
			ast_free(foreach);
			return res;
		}
		if (ast_type(id = res.ast) != AST_ID) {
			ast_free(foreach);
			ast_free(id);
			return with_expected(p, "foreach", "identifier");
		}
		list_append(&foreach->ids, id);
	} while (accept(p, TOKEN_COMMA));

	if (!accept(p, TOKEN_COLON)) {
		ast_free(foreach);
		return with_expected(p, "foreach", "colon");
	}

	/* Parse expression. */
	if ((res = expression(p)).status) {
		ast_free(foreach);
		return res;
	}
	if (ast_type(exp = res.ast) == AST_EMPTY) {
		ast_free(foreach);
		ast_free(exp);
		return with_expected(p, "foreach", "expression");
	}
	foreach->exp = exp;

	/* Parse body. */
	if ((res = sequence(p)).status) {
		ast_free(foreach);
		return res;
	}
	foreach->body = res.ast;

	if (!accept(p, TOKEN_ENDFOREACH)) {
		ast_free(foreach);
		return with_expected(p, "foreach", "endforeach");
	}

	return with_ast(foreach);
}

static struct result selection(struct parser *p)
{
	struct ast *pred = NULL;
	struct ast_if *cond = NULL;
	struct result res = { .status = FAILURE };

	if ((res = expect(p, TOKEN_IF, "if", "`if' keyword")).status) {
		return res;
	}

	if ((cond = ast_if()) == NULL) {
		return with_status(NO_MEMORY);
	}

	do {
		/* Parse predicate. */
		if ((res = expression(p)).status) {
			ast_free(cond);
			return res;
		}
		if (ast_type(pred = res.ast) == AST_EMPTY) {
			ast_free(cond);
			ast_free(pred);
			return with_expected(p, "if", "predicate");
		}

		/* Parse body. */
		if ((res = sequence(p)).status) {
			ast_free(cond);
			ast_free(pred);
			return res;
		}
		if (ast_if_clause(cond, pred, res.ast) == NULL) {
			ast_free(cond);
			ast_free(pred);
			ast_free(res.ast);
			return with_status(NO_MEMORY);
		}
	} while (accept(p, TOKEN_ELIF));

	if (accept(p, TOKEN_ELSE)) {
		if ((res = sequence(p)).status) {
			ast_free(cond);
			return res;
		}
		cond->alt = res.ast;
	}

	if (!accept(p, TOKEN_ENDIF)) {
		ast_free(cond);
		return with_expected(p, "if", "endif");
	}

	return with_ast(cond);
}

static struct result statement(struct parser *p)
{
	switch (peek(p)) {
	case TOKEN_END:
		return with_ast(ast_empty());
	case TOKEN_IF:
		return selection(p);
	case TOKEN_FOREACH:
		return iteration(p);
	case TOKEN_BREAK:
		accept(p, TOKEN_BREAK);
		return with_ast(ast_new(AST_JUMP, AST_BREAK));
	case TOKEN_CONTINUE:
		accept(p, TOKEN_CONTINUE);
		return with_ast(ast_new(AST_JUMP, AST_CONTINUE));
	default:
		return expression(p);
	}
}

static struct result sequence(struct parser *p)
{
	struct ast_seq *seq = NULL;
	struct result res = { .status = FAILURE };

	if ((res = statement(p)).status) {
		return res;
	}
	if (ast_type(res.ast) == AST_EMPTY) {
		return with_ast(res.ast);
	}
	if ((seq = ast_seq()) == NULL) {
		ast_free(res.ast);
		return with_status(NO_MEMORY);
	}

	do {
		list_append(&seq->exps, res.ast);
		res = statement(p);
	} while (res.status == 0 && ast_type(res.ast) != AST_EMPTY);

	if (res.status) {
		ast_free(seq);
		return res;
	}
	ast_free(res.ast);

	return with_ast(seq);
}

struct parse_result parse(struct string source)
{
	struct parser p;
	struct result res = { .status = FAILURE };

	memset(&p, 0, sizeof(p));
	lexer_init(&p.lexer, source);
	res = sequence(&p);
	lexer_free(&p.lexer);

	switch (res.status) {
	case SUCCESS:
		assert(res.ast != NULL);
		return (struct parse_result) {
			.success = true, .ast = res.ast
		};
	case FAILURE:
		return (struct parse_result) {
			.error = res.error
		};
	case NO_MEMORY:
		return (struct parse_result) {
			.error = string_from_buf("not enough memory")
		};
	default:
		return (struct parse_result) {
			.error = NULL_STRING
		};
	}
}

void parse_result_free(struct parse_result *result)
{
	if (result->success) {
		ast_free(result->ast);
	} else {
		string_free(&result->error);
	}
	result->success = false;
	result->ast = NULL;
}
