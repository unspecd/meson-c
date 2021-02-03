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
#include <inttypes.h>

#include "test.h"

#define SEQ_PASS(...) TEST_CHECK(seq_should_pass(__VA_ARGS__))
#define SEQ_FAIL(...) TEST_CHECK(seq_should_fail(__VA_ARGS__))

struct buffer {
	char *buffer;
	size_t length;
};

static void append(struct buffer *buf, const char *format, ...)
{
	int ret = 0;
	va_list args;

	va_start(args, format);
	ret = vsnprintf(buf->buffer, buf->length, format, args);
	va_end(args);

	if (ret > 0) {
		buf->buffer += ret;
		buf->length -= ret;
	}
}

static void lispify(struct ast *ast, struct buffer *target)
{
	void *ptr = ast;

	assert(ast != NULL);
	append(target, "(");

	switch (ast_type(ast)) {
	case AST_EMPTY: {
		append(target, "empty");
		break;
	}
	case AST_SEQUENCE: {
		struct ast_seq *seq = ptr;
		struct list_node *it = NULL;
		struct ast *exp = NULL;
		append(target, "seq");
		while ((exp = list_enum(&seq->exps, &it)) != NULL) {
			append(target, " ");
			lispify(exp, target);
		}
		break;
	}
	case AST_ASSIGNMENT: {
		struct ast_binary *a = ptr;
		append(target, "assign");
		switch (ast_subtype(a)) {
			case AST_ASSIGN: break;
			case AST_ADD_ASSIGN: append(target, "+"); break;
			case AST_SUB_ASSIGN: append(target, "-"); break;
			case AST_MUL_ASSIGN: append(target, "*"); break;
			case AST_DIV_ASSIGN: append(target, "/"); break;
			case AST_MOD_ASSIGN: append(target, "%%"); break;
			default: append(target, "?"); break;
		}
		append(target, " ");
		lispify(a->lhs, target);
		append(target, " ");
		lispify(a->rhs, target);
		break;
	}
	case AST_IF: {
		struct ast_if *if_ = ptr;
		struct ast_if_clause *c = NULL;
		struct list_node *it = NULL;
		append(target, "cond");
		while ((c = list_enum(&if_->clauses, &it)) != NULL) {
			append(target, " (");
			lispify(c->pred, target);
			append(target, " ");
			lispify(c->conseq, target);
			append(target, ")");
		}
		if (if_->alt != NULL) {
			append(target, " (else ");
			lispify(if_->alt, target);
		}
		break;
	}
	case AST_FOREACH: {
		struct ast_foreach *foreach = ptr;
		struct ast *id = NULL;
		struct list_node *it = NULL;
		bool first = true;
		append(target, "foreach");
		append(target, " ids:(");
		while ((id = list_enum(&foreach->ids, &it)) != NULL) {
			if (first) {
				first = false;
			} else {
				append(target, " ");
			}
			lispify(id, target);
		}
		append(target, ") ");
		lispify(foreach->exp, target);
		append(target, " ");
		lispify(foreach->body, target);
		break;
	}
	case AST_JUMP: {
		if (ast_subtype(ast) == AST_BREAK) {
			append(target, "break");
		} else if (ast_subtype(ast) == AST_CONTINUE) {
			append(target, "continue");
		} else {
			append(target, "jump");
		}
		break;
	}
	case AST_UNARY: {
		struct ast_unary *u = ptr;
		append(target, "unary ");
		switch (ast_subtype(u)) {
			case AST_NOT: append(target, "not"); break;
			case AST_PLUS: append(target, "plus"); break;
			case AST_MINUS: append(target, "minus"); break;
			default: append(target, "?"); break;
		}
		append(target, " ");
		lispify(u->exp, target);
		break;
	}
	case AST_LOGICAL: {
		if (ast_subtype(ast) == AST_TERNARY) {
			struct ast_ternary *t = ptr;
			append(target, "ternary ");
			lispify(t->pred, target);
			append(target, " ");
			lispify(t->conseq, target);
			append(target, " ");
			lispify(t->alt, target);
		} else {
			struct ast_binary *a = ptr;
			switch (ast_subtype(a)) {
				case AST_AND: append(target, "and"); break;
				case AST_OR:  append(target, "or"); break;
				default: append(target, "?"); break;
			}
			append(target, " ");
			lispify(a->lhs, target);
			append(target, " ");
			lispify(a->rhs, target);
		}
		break;
	}
	case AST_ARITHMETIC: {
		struct ast_binary *a = ptr;
		switch (ast_subtype(a)) {
			case AST_ADD: append(target, "+"); break;
			case AST_SUB: append(target, "-"); break;
			case AST_MUL: append(target, "*"); break;
			case AST_DIV: append(target, "/"); break;
			case AST_MOD: append(target, "%%"); break;
			default: append(target, "?"); break;
		}
		append(target, " ");
		lispify(a->lhs, target);
		append(target, " ");
		lispify(a->rhs, target);
		break;
	}
	case AST_RELATIONAL: {
		struct ast_binary *a = ptr;
		switch (ast_subtype(a)) {
			case AST_LT: append(target, "<"); break;
			case AST_GT: append(target, ">"); break;
			case AST_EQ: append(target, "=="); break;
			case AST_NE: append(target, "!="); break;
			case AST_LE: append(target, "<="); break;
			case AST_GE: append(target, ">="); break;
			case AST_IN: append(target, "in"); break;
			case AST_NOT_IN: append(target, "notin"); break;
			default: append(target, "?"); break;
		}
		append(target, " ");
		lispify(a->lhs, target);
		append(target, " ");
		lispify(a->rhs, target);
		break;
	}
	case AST_INDEX: {
		struct ast_index *s = ptr;
		append(target, "index ");
		lispify(s->ref, target);
		append(target, " ");
		lispify(s->index, target);
		break;
	}
	case AST_MEMBER: {
		struct ast_member *m = ptr;
		append(target, "member ");
		lispify(m->obj, target);
		append(target, " ");
		lispify(m->field, target);
		break;
	}
	case AST_APPLICATION: {
		struct ast_app *app = ptr;
		struct ast *arg = NULL;
		struct ast_kw_arg *kw = NULL;
		struct list_node *it = NULL;
		append(target, "app ");
		lispify(app->ref, target);
		if (!list_empty(&app->args)) {
			bool first = true;
			append(target, " args:(");
			while ((arg = list_enum(&app->args, &it)) != NULL) {
				if (first) {
					first = false;
				} else {
					append(target, " ");
				}
				lispify(arg, target);
			}
			append(target, ")");
		}
		if (!list_empty(&app->kw_args)) {
			bool first = true;
			append(target, " kw-args:(");
			while ((kw = list_enum(&app->kw_args, &it)) != NULL) {
				if (first) {
					first = false;
				} else {
					append(target, " ");
				}

				append(target, "(");
				lispify(as_ast(kw->id), target);
				append(target, " ");
				lispify(kw->exp, target);
				append(target, ")");
			}
			append(target, ")");
		}
		break;
	}
	case AST_ID: {
		struct ast_id *id = ptr;
		append(target, "id %s", string_text(id->name));
		break;
	}
	case AST_ARRAY: {
		struct ast_array *array = ptr;
		struct list_node *it = NULL;
		struct ast *exp = NULL;
		append(target, "array");
		while ((exp = list_enum(&array->elts, &it)) != NULL) {
			append(target, " ");
			lispify(exp, target);
		}
		break;
	}
	case AST_DICTIONARY: {
		struct ast_dict *dict = ptr;
		struct list_node *it = NULL;
		struct ast_kv *kv = NULL;
		append(target, "dict");
		while ((kv = list_enum(&dict->map, &it)) != NULL) {
			append(target, " ");
			append(target, "(");
			lispify(kv->key, target);
			append(target, " ");
			lispify(kv->value, target);
			append(target, ")");
		}
		break;
	}
	case AST_NUMBER: {
		struct ast_number *num = ptr;
		append(target, "num %" PRIi64, num->value);
		break;
	}
	case AST_STRING: {
		struct ast_string *str = ptr;
		append(target, "str `%s`", string_text(str->value));
		break;
	}
	case AST_BOOLEAN: {
		append(target, "bool %s",
		       ast_subtype(ast) == AST_TRUE ? "true" : "false");
		break;
	}
	default:
		append(target, "unknown:%s", ast_type_name(ast_type(ast)));
		break;
	}

	append(target, ")");
}

static bool check(bool should_pass, const char *source,
		  const char *result, const char *error)
{
	char buffer[1024];
	struct parse_result res;
	bool pass = false;

	res = parse(string_from_buf(source));

	if (res.success == should_pass) {
		if (res.success) {
			lispify(res.ast, &(struct buffer) { buffer, sizeof(buffer) });
			pass = strcmp(result, buffer) == 0;
		} else {
			/* fprintf(stderr, "ERR: %s\n", string_text(res.error)); */
			if (!string_is_null(res.error)) {
				pass = strcmp(error, string_text(res.error)) == 0;
			}
		}
	}
	parse_result_free(&res);

	return pass;
}

static bool should_pass(const char *exp, const char *ast)
{
	char seq[1024];

	snprintf(seq, sizeof(seq), "(seq %s)", ast);
	return check(true, exp, seq, NULL);
}

static bool should_fail(const char *exp, const char *error)
{
	return check(false, exp, NULL, error);
}

static bool seq_should_pass(const char *exp, const char *ast)
{
	return check(true, exp, ast, NULL);
}

static bool seq_should_fail(const char *exp, const char *error)
{
	return check(false, exp, NULL, error);
}

static void test_identifier(void)
{
	PASS("sample", "(id sample)");
	PASS("sample123", "(id sample123)");
}

static void test_boolean(void)
{
	PASS("true", "(bool true)");
	PASS("false", "(bool false)");
}

static void test_number(void)
{
	PASS("0", "(num 0)");
	PASS("1", "(num 1)");
	PASS("0x10", "(num 16)");
	PASS("0o10", "(num 8)");
	PASS("0b11", "(num 3)");
}

static void test_string(void)
{
	PASS("''", "(str ``)");
	PASS("' '", "(str ` `)");
	PASS("''' '''", "(str ` `)");
	PASS("'sample'", "(str `sample`)");
}

static void test_array(void)
{
	PASS("[]", "(array)");
	PASS("[1]", "(array (num 1))");
	PASS("[1,]", "(array (num 1))");
	PASS("[1,2,3]", "(array (num 1) (num 2) (num 3))");

	FAIL("[", "array: expected expression");
	FAIL("[,]", "array: expected expression");
	FAIL("[1", "array: expected closing bracket");
	FAIL("[()", "invalid expression");
}

static void test_dictionary(void)
{
	PASS("{}", "(dict)");
	PASS("{'a':1}", "(dict ((str `a`) (num 1)))");
	PASS("{'a':1,}", "(dict ((str `a`) (num 1)))");

	FAIL("{", "dictionary: expected key");
	FAIL("{()", "invalid expression");
	FAIL("{,}", "dictionary: expected key");
	FAIL("{'a'", "dictionary: expected colon");
	FAIL("{'a':", "dictionary: expected value");
	FAIL("{'a':1", "dictionary: expected closing brace");
	FAIL("{'a':()", "invalid expression");
}

static void test_primary(void)
{
	PASS("(0)", "(num 0)");
	FAIL("(1", "expected closing paren");
	FAIL("()", "invalid expression");
}

static void test_member(void)
{
	PASS("o.f", "(member (id o) (id f))");

	FAIL("o.", "expected field name");
	FAIL("o.true", "field name must be plain id");
	FAIL("o.false", "field name must be plain id");
	FAIL("o.123", "field name must be plain id");
	FAIL("o.[]", "field name must be plain id");
	FAIL("o.{}", "field name must be plain id");
}

static void test_subscript(void)
{
	PASS("a[0]", "(index (id a) (num 0))");
	PASS("a[i]", "(index (id a) (id i))");
	PASS("o.a[0]", "(index (member (id o) (id a)) (num 0))");

	FAIL("a[()", "invalid expression");
	FAIL("a[", "subscript: expected expression");
	FAIL("a[i", "subscript: expected closing bracket");
}

static void test_application()
{
	PASS("f()", "(app (id f))");
	PASS("f(a)",  "(app (id f) args:((id a)))");
	PASS("f(a,)", "(app (id f) args:((id a)))");
	PASS("f(a,b)", "(app (id f) args:((id a) (id b)))");
	PASS("f(a:1)", "(app (id f) kw-args:(((id a) (num 1))))");
	PASS("f(a,k:v)", "(app (id f) args:((id a)) kw-args:(((id k) (id v))))");
	PASS("o.f(x)", "(app (member (id o) (id f)) args:((id x)))");

	FAIL("f(", "application: expected argument");
	FAIL("f(,", "application: expected argument");
	FAIL("f(a", "application: expected closing paren");

	FAIL("f(1:", "application: expected kwarg name");
	FAIL("f(k:", "application: expected kwarg value");
	FAIL("f(k:()", "invalid expression");
	FAIL("f(k:v", "application: expected closing paren");
	FAIL("f(k:v, l", "application: expected keyword");

	FAIL("f(()", "invalid expression");
}

static void test_unary(void)
{
	PASS("+1", "(unary plus (num 1))");
	PASS("-1", "(unary minus (num 1))");
	PASS("not 1", "(unary not (num 1))");

	FAIL("-", "unary: expected expression");
	FAIL("+", "unary: expected expression");
	FAIL("not", "unary: expected expression");
}

static void test_multiplicative(void)
{
	PASS("a * b", "(* (id a) (id b))");
	PASS("a / b", "(/ (id a) (id b))");
	PASS("a % b", "(% (id a) (id b))");

	FAIL("a *", "multiplicative: expected expression");
	FAIL("a /", "multiplicative: expected expression");
	FAIL("a %", "multiplicative: expected expression");

	FAIL("a * ()", "invalid expression");
}

static void test_additive(void)
{
	PASS("a + b", "(+ (id a) (id b))");
	PASS("a - b", "(- (id a) (id b))");

	FAIL("a +", "additive: expected expression");
	FAIL("a -", "additive: expected expression");

	FAIL("a + ()", "invalid expression");
}

static void test_relational(void)
{
	PASS("a < b", "(< (id a) (id b))");
	PASS("a > b", "(> (id a) (id b))");
	PASS("a <= b", "(<= (id a) (id b))");
	PASS("a >= b", "(>= (id a) (id b))");
	PASS("a in b", "(in (id a) (id b))");
	PASS("a not in b", "(notin (id a) (id b))");

	FAIL("a <", "relational: expected expression");
	FAIL("a >", "relational: expected expression");
	FAIL("a <=", "relational: expected expression");
	FAIL("a >=", "relational: expected expression");
	FAIL("a in", "relational: expected expression");
	FAIL("a not in", "relational: expected expression");
	FAIL("a not", "expected `in' after `not'");

	FAIL("a < ()", "invalid expression");
}

static void test_equality(void)
{
	PASS("a == b", "(== (id a) (id b))");
	PASS("a != b", "(!= (id a) (id b))");

	FAIL("a ==", "equality: expected expression");
	FAIL("a !=", "equality: expected expression");

	FAIL("a == ()", "invalid expression");
}

static void test_logical_and(void)
{
	PASS("a and b", "(and (id a) (id b))");

	FAIL("a and", "logical and: expected expression");
	FAIL("a and ()", "invalid expression");
}

static void test_logical_or(void)
{
	PASS("a or b", "(or (id a) (id b))");

	FAIL("a or", "logical or: expected expression");
	FAIL("a or ()", "invalid expression");
}

static void test_ternary(void)
{
	PASS("a ? b : c", "(ternary (id a) (id b) (id c))");

	FAIL("a ?", "ternary: expected true clause");
	FAIL("a ? b", "ternary: expected colon");
	FAIL("a ? b :", "ternary: expected false clause");

	FAIL("a ? ()", "invalid expression");
	FAIL("a ? b : ()", "invalid expression");
}

static void test_assignment(void)
{
	PASS("a = b", "(assign (id a) (id b))");
	PASS("a += b", "(assign+ (id a) (id b))");
	PASS("a -= b", "(assign- (id a) (id b))");
	PASS("a *= b", "(assign* (id a) (id b))");
	PASS("a /= b", "(assign/ (id a) (id b))");
	PASS("a %= b", "(assign% (id a) (id b))");

	FAIL("a =", "assignment: expected expression");
	FAIL("a +=", "assignment: expected expression");
	FAIL("a -=", "assignment: expected expression");
	FAIL("a *=", "assignment: expected expression");
	FAIL("a /=", "assignment: expected expression");
	FAIL("a %=", "assignment: expected expression");

	FAIL("a = ()", "invalid expression");
	FAIL("1 = a", "assignment target must be an id");
}

static void test_sequence(void)
{
	SEQ_PASS("", "(empty)");
	SEQ_PASS("a", "(seq (id a))");
	SEQ_PASS("a b c", "(seq (id a) (id b) (id c))");

	SEQ_FAIL("a b = ()", "invalid expression");
}

static void test_iteration(void)
{
	PASS("foreach x : xs endforeach",
	     "(foreach ids:((id x)) (id xs) (empty))");

	PASS("foreach x, y, z : xs endforeach",
	     "(foreach ids:((id x) (id y) (id z)) (id xs) (empty))");

	PASS("foreach x : xs a b c endforeach",
	     "(foreach ids:((id x)) (id xs)"
	     " (seq (id a) (id b) (id c)))");

	PASS("foreach x : xs break endforeach",
	     "(foreach ids:((id x)) (id xs)"
	     " (seq (break)))");

	PASS("foreach x : xs continue endforeach",
	     "(foreach ids:((id x)) (id xs)"
	     " (seq (continue)))");

	PASS("break", "(break)");
	PASS("continue", "(continue)");

	FAIL("foreach", "foreach: expected identifier");
	FAIL("foreach x", "foreach: expected colon");
	FAIL("foreach x,", "foreach: expected identifier");
	FAIL("foreach x :", "foreach: expected expression");
	FAIL("foreach x : xs", "foreach: expected endforeach");

	FAIL("foreach x : ()", "invalid expression");
}

static void test_selection(void)
{
	PASS("if a endif",
	     "(cond"
	     " ((id a) (empty)))");

	PASS("if a 1 endif",
	     "(cond"
	     " ((id a) (seq (num 1))))");

	PASS("if a 1 else 2 endif",
	     "(cond"
	     " ((id a) (seq (num 1)))"
	     " (else (seq (num 2)))");

	PASS("if a 1 else 2 endif",
	     "(cond"
	     " ((id a) (seq (num 1)))"
	     " (else (seq (num 2)))");

	PASS("if a 1 elif b 2 else 3 endif",
	     "(cond"
	     " ((id a) (seq (num 1)))"
	     " ((id b) (seq (num 2)))"
	     " (else (seq (num 3)))");

	FAIL("if", "if: expected predicate");
	FAIL("if true", "if: expected endif");

	FAIL("if ()", "invalid expression");
	FAIL("if true () endif", "invalid expression");
}

TEST_LIST = {
	{ "identifiers", test_identifier },
	{ "boolean literals", test_boolean },
	{ "numeric literals", test_number },
	{ "string literals", test_string },
	{ "array literals", test_array },
	{ "dictionary literals", test_dictionary },
	{ "primary expressions", test_primary },
	{ "member access expressions", test_member },
	{ "subscript expressions", test_subscript },
	{ "application expressions", test_application },
	{ "unary expressions", test_unary },
	{ "multiplicative expressions", test_multiplicative },
	{ "additive expressions", test_additive },
	{ "relational expressions", test_relational },
	{ "equality expressions", test_equality },
	{ "logical AND expressions", test_logical_and },
	{ "logical OR expressions", test_logical_or },
	{ "ternary expressions", test_ternary },
	{ "assignment expressions", test_assignment },
	{ "sequence of statements", test_sequence },
	{ "iteration statements", test_iteration },
	{ "selection statements", test_selection },
	{ NULL, NULL }
};
