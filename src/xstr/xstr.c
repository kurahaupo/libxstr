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
xstr_panic(CALLEE char const *fmt, ...) {
    va_list v;
  #if HIDE_CALLER
    fprintf(stderr, "PANIC XSTR: ");
  #else
    fprintf(stderr, "PANIC %s#%u XSTR: ", file, lineno);
  #endif
    va_start(v, fmt);
    vfprintf(stderr, fmt, v);
    va_end(v);
    exit(88);
}

void *
xstr__memdup(CALLEE const void *p, size_t l ) {
    void *r = malloc(l);
    if (!r) {
        xstr_panic(__FILE__, __LINE__, "xstr_memdup: malloc(%zu) failed; %m", l);
        return NULL;
    }
    memcpy(r, p, l);
    return r;
}

char const *
xstr_state_name(CALLEE xstr_state_t st) {
    static const char *sig_names[8] = {
        [xstr_null]  = "null",
        [xstr_valid] = "valid",
    };
    if (st < 0 || st >= 8)
        return "invalid";
    return sig_names[st] ?: "invalid";
}

#ifdef XSTR_DEBUG
void
xstr_print_state(CALLEE char const *fn, S *s) {
    fprintf(stderr, "XSTR: %s(s=%p, ", fn, s);
    fprintf(stderr, "base=%p", s->base);
    if (s->base) {
        size_t l = xstr_len(OUTER_CALL s);
        static int SnapLen = 128;
        if (l>SnapLen)
            fprintf(stderr, "[\"%.*s…\"]", (int) SnapLen, CS(*s));
        else
            fprintf(stderr, "[\"%.*s\"]", (int) l, CS(*s));
    }
    fprintf(stderr, ", size=%zu", (size_t) s->size);
    fprintf(stderr, ", state=%s", xstr_state_name(OUTER_CALL s->signature));
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
