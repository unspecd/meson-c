# The MIT License

# Copyright 2021 Evgeny Ermakov. <e.v.ermakov@gmail.com>

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

O ?= build
S := $(abspath .)
B := $(abspath $O)

ifeq ($(WINDOWS),1)
  X := .exe
else
  X :=
endif

CFLAGS += -Isrc

CWARNFLAGS += all extra pedantic error

ifeq ($(COVERAGE),1)
  CFLAGS  += -fprofile-arcs -ftest-coverage
  LDFLAGS += -fprofile-arcs -ftest-coverage
  DEBUG := 1
endif

ifeq ($(DEBUG),1)
  CFLAGS += -O0 -g
endif

CFLAGS += $(addprefix -W,$(CWARNFLAGS))

.SUFFIXES:
.SUFFIXES: .c .o

.PRECIOUS: $O/%.o

.PHONY: all
all:

.PHONY: clean
clean:
	$(RM) -f $(CLEANFILES) $O/*.d $O/*.gcda $O/*.gcno
	$(RM) -rf $O/coverage*

.PHONY: coverage-report
ifeq ($(COVERAGE),1)
  COV_OUT := $O/coverage.info
  COV_ARGS := --rc lcov_branch_coverage=1

  coverage-report: coverage
	rm -rf $O/coverage-report
	genhtml --title "Coverage report" -q -s --legend \
		--branch-coverage -o $O/coverage-report $(COV_OUT)
	lcov -l $(COV_OUT)

  .PHONY: coverage
  coverage: check
	lcov $(COV_ARGS) -o $(COV_OUT).run -d $O --capture
	lcov $(COV_ARGS) -o $(COV_OUT) --extract $(COV_OUT).run "$S/src/*"

  .PHONY: coverage-init
  coverage-init:
	@lcov -d $O --zerocounters > /dev/null

  check: coverage-init
else
  coverage-report:
	@echo "Rebuild with COVERAGE=1"
	@echo "  > make clean"
	@echo "  > make COVERAGE=1 $@"
endif

LIB := $O/libmeson-c.a
LIB_OBJS += $O/ast.o
LIB_OBJS += $O/common.o
LIB_OBJS += $O/lexer.o
LIB_OBJS += $O/parser.o
$(LIB): $(LIB_OBJS)
CLEANFILES += $(LIB) $(LIB_OBJS)

# Testing

test-string:
test-lexer: test-string
test-parser: test-lexer

TESTS := $(basename $(notdir $(wildcard tests/test-*.c)))

.PHONY: check
check: $(TESTS)

TEST_OBJS := $(addprefix $O/,$(addsuffix .o,$(TESTS)))
TEST_PROGRAMS := $(patsubst %.o,%$X,$(TEST_OBJS))

.PHONY: all-tests
all-tests: $(TEST_PROGRAMS)

$O/test-%$X: $O/test-%.o $(LIB)
	$(CC) $(LDFLAGS) -o $@ $^
CLEANFILES += $(TEST_OBJS)
CLEANFILES += $(TEST_PROGRAMS)

WRAP :=

ifeq ($(VALGRIND),1)
  WRAP := valgrind -q \
	--error-exitcode=8 \
	--leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--num-callers=32

  ifneq ("$(SUPPRESSIONS)","")
    WRAP += --suppressions=tests/valgrind-$(SUPPRESSIONS).supp
  endif
endif

test-%: $O/test-%$X
	@echo
	@echo "Running $(notdir $<)..."
	@$(WRAP) $< --no-exec $(ARGS)

# Implicit rules

$O/.:
	mkdir -p $@

$O%/.:
	mkdir -p $@

.SECONDEXPANSION:

$O/%.o: src/%.c | $$(@D)/.
	$(CC) $(CFLAGS) -c -MMD -MT $@ -MF $@.d -o $@ $<

$O/%.o: tests/%.c | $$(@D)/.
	$(CC) $(CFLAGS) -c -MMD -MT $@ -MF $@.d -o $@ $<

$O/%.a: | $$(@D)/.
	$(AR) rcsT $@ $?

-include $O/*.d
