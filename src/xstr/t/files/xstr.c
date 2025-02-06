#undef XSTR_DEBUG
#define XSTR_DEBUG

#include <malloc.h> /* malloc, free */
#include <memory.h> /* memcpy, strlen */
#include <stdint.h> /* SIZE_MAX */

#if defined XSTR_DEBUG
#include <assert.h> /* assert */
#include <stdlib.h> /* exit */
#include <stdarg.h>
#endif

#ifdef XSTR_DEBUG
#include <stdio.h>  /* for fprintf, fputs; because we have DEBUG enabled */
#endif

#include "xstr.h"

/******************************************************************************/

#ifndef XSTR_DEBUG
void
xstr_print_state(char const *fn, S *s) {
    fprintf(stderr, "XSTR: %s(s=%p, ", fn, s);
    fprintf(stderr, "base=%p", s->base);
    if (s->base) {
        size_t l = s->size;
        if (s->is_buffer)
            l = xstr_to_xbuf(s)->len;
        static int SnapLen = 128;
        if (l>SnapLen)
            fprintf(stderr, "[\"%.*s…\"]", (int) SnapLen, CS(*s));
        else
            fprintf(stderr, "[\"%.*s\"]", (int) l, CS(*s));
    }
    fprintf(stderr, ", size=%zu", (size_t) s->size);
    fprintf(stderr, ", state=%s", xstr_state_name(s->signature));
    fprintf(stderr, "%s%s%s",
                        s->writable   ? ", writable"    : ", readonly",
                        s->loan       ? ", +loan"       : "",
                        s->data_alloc ? ", data=alloc"  : "");
  #ifdef XSTR_USE_ALLOC
    fprintf(stderr, "%s",
                        s->self_alloc ? ", self=alloc"  : "");
  #endif
    fprintf(stderr, ")\n");
}
#endif

/******************************************************************************/

void test_func1( XS bs ) {
    xstr_print_state("func1(param)", &bs.offer);
    S s = Take(bs);
    xstr_print_state("func1(taken)", &s);
    Finish(s);
    xstr_print_state("func1(finished)", &s);
}

void test_func2( XS bs ) {
    xstr_print_state("func2(param)", &bs.offer);
    S s = Borrow(bs);
    xstr_print_state("func2(borrowed)", &s);
    test_func1(Loan(s));
    xstr_print_state("func2(loaned)", &s);
    test_func1(Give(s));
    xstr_print_state("func2(given)", &s);
    Finish(s);
    xstr_print_state("func2(finished)", &s);
}

void test_func3( XS bs ) {
    xstr_print_state("func3(param)", &bs.offer);
    S s = Borrow(bs);
    xstr_print_state("func3(borrowed)", &s);
    Finish(s);
    xstr_print_state("func3(finished)", &s);
}

void test_func4() {
    S s = LS("Hello world");
    xstr_print_state("func4(param)", &s);
    test_func3(Loan(s));
    xstr_print_state("func4(loaned)", &s);
    test_func3(Give(s));
    xstr_print_state("func4(given)", &s);
    Finish(s);
    xstr_print_state("func4(finished)", &s);
}

void test_func5( XS bs ) {
    xstr_print_state("func5(param)", &bs.offer);
    S s = Take(bs);
    xstr_print_state("func5(taken)", &s);
    test_func3(Give(s));
    xstr_print_state("func5(given)", &s);
    Finish(s);
    xstr_print_state("func5(finished)", &s);
}

void test_func6() {
    S s = LS("Hello world");
    xstr_print_state("func6(literal)", &s);
    test_func1(Loan(s));
    xstr_print_state("func6(loaned)", &s);
    test_func1(Give(s));
    xstr_print_state("func6(given)", &s);
    test_func1(Loan(s)); // DIE HERE
    xstr_print_state("func6(re-loaned)", &s);
    Finish(s);
    xstr_print_state("func6(finished)", &s);
}

void test_func7() {
    S s = LS("Hello world");
    xstr_print_state("func6(literal)", &s);
    test_func1(Loan(s));
    xstr_print_state("func6(loaned)", &s);
    test_func2(Loan(s));
    xstr_print_state("func6(loaned)", &s);
    test_func3(Loan(s));
    xstr_print_state("func6(loaned)", &s);
    test_func4(Loan(s));
    xstr_print_state("func6(loaned)", &s);
    test_func5(Loan(s));
    xstr_print_state("func6(loaned)", &s);
    test_func5(Give(s));
    xstr_print_state("func6(given)", &s);
    Finish(s);
    xstr_print_state("func6(finished)", &s);
}

int main() {
#ifdef TEST_FAIL
    test_func6();
#else
    test_func4();
    test_func7();
#endif
    return 0;
}
