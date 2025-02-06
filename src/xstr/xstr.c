#undef XSTR_DEBUG
#define XSTR_DEBUG

#include <malloc.h> /* malloc, free */
#include <memory.h> /* memcpy, strlen */
#include <stdint.h> /* SIZE_MAX */

#if 1 || defined XSTR_DEBUG
#include <assert.h> /* assert */
#include <stdlib.h> /* exit */
#include <stdarg.h>
#endif

#ifdef XSTR_DEBUG
#include <stdio.h>  /* for fprintf, fputs; because we have DEBUG enabled */
#endif

#include "xstr.h"

_Noreturn void
xstr_panic(char const *fmt, ...) {
    va_list v;
    va_start(v, fmt);
    fputs("XSTR PANIC: ", stderr);
    vfprintf(stderr, fmt, v);
    va_end(v);
    exit(88);
}

void *
xstr__memdup( const void *p, size_t l ) {
    void *r = malloc(l);
    if (!r) {
        xstr_panic("xstr_memdup: malloc(%zu) failed; %m", l);
        return NULL;
    }
    memcpy(r, p, l);
    return r;
}

char const *
xstr_state_name(xstr_state_t st) {
    static const char *sig_names[8] = {
        [xstr_null]  = "null",
        [xstr_valid] = "valid",
        [xstr_stale] = "stale",
    };
    if (st < 0 || st > xstr_max)
        return "invalid";
    return sig_names[st] ?: "invalid";
}

#ifdef XSTR_DEBUG
void
xstr_print_state(char const *fn, S *s) {
    fprintf(stderr, "XSTR: %s(s=%p, ", fn, s);
    fprintf(stderr, "base=%p", s->base);
    if (s->base) {
        size_t l = xstr_len(s);
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
