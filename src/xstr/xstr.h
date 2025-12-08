/*
 * xstr.h
 *
 * NOTE: include these first:
 *
 *  <malloc.h> - for malloc, free
 *  <memory.h> - for memcpy, strlen
 *  <stdint.h> - for SIZE_MAX
 *
 * In addition, if XSTR_DEBUG is defined, also include:
 *
 *  <stdio.h>
 */

typedef enum xstr_state_e {
    xstr_null    = 0, /* Either implicitly 0-fill initialized, or
                       * already given away */
    xstr_valid   = 5, /* Correct value; a random 3-bit number other than 0 */
} xstr_state_t;

typedef struct xstring {
    void *base;

    /*
     * To allow a "struct xstring" to be passed in just two machine words, crib
     * a few bits from the top of the length field, provided it's at least 56
     * bits.
     */
  # if  (1ULL << 56) > 1 && SIZE_MAX >= (1ULL << 56)
    /* TODO: figure out how to cope if bitfields are not allowed for sizes wider than unsigned int */
    uint64_t    size :56;
  # else
    size_t      size;
  # endif
    xstr_state_t signature :3; /* Value has been given away */
  #ifdef XSTR_USE_ALLOC
    _Bool       self_alloc :1; /* free self upon Finish */
  #else
    _Bool       :1; /* filler */
  #endif
    _Bool       data_alloc :1; /* free base upon Finish */
    _Bool       is_buffer  :1; /* [B & XB only] is a buffer rather than a string, and so has a separate "length" field; otherwise assume length==size */
    _Bool       writable   :1; /* not a string literal */
    _Bool       loan       :1; /* [XS only] Receiver does not own the object; "Take" will clone it */
} S;

typedef struct offered_string {
    S offer;
} XS;

typedef struct xbuffer {
    S s;    /* MUST be first so that (B*)&(b->s) == b */
    size_t len;
} B;

typedef struct offered_buffer {
    B offer;
} XB;

/************************************************************************/

extern void * xstr__memdup( const void *p, size_t l );
extern _Noreturn void xstr_panic(char const *fmt, ...);
extern char const * xstr_state_name(xstr_state_t st);
#ifdef XSTR_DEBUG
extern void xstr_print_state(char const *fn, S *s);
#endif

/************************************************************************/

#define xstr_assert_valid(s) \
    if ((s)->signature == xstr_valid) {} else \
        xstr_panic("%s: XS %p not valid\n", __FUNCTION__, (s))

#define xstr_assert_validx(xs) \
    xstr_assert_valid(&(xs)->offer)

static inline B *
xstr_to_xbuf(S *s)
{
    if (!s->is_buffer)
        xstr_panic("not a buffer");
    return (B *)s;
}

static inline void
xstr_finish_or_ignore(S *s, _Bool ignoring) {
  # ifdef XSTR_DEBUG
    xstr_print_state(ignoring ? "Ignore" : "Finish", s);
  # endif
    if ( s->signature == xstr_null ) {
      # ifdef XSTR_DEBUG
        fprintf(stderr, "XSTR: ... %s; already null or state\n",
                ignoring ? "ignored" : "finished");
      # endif
    }
    else {
        xstr_assert_valid(s);
      # ifdef XSTR_DEBUG
        fprintf(stderr, "XSTR: ... setting base=NULL, size=0, sig=stale\n");
      # endif
        if ( s->data_alloc ) {
          # ifdef XSTR_DEBUG
            fprintf(stderr, "XSTR: alloc_data, free(base=%p)\n", s->base);
          # endif
            free((void*) s->base);
            s->data_alloc = 0;
        }
        s->base = NULL;
        s->size = 0U;
        if ( s->is_buffer )
            xstr_to_xbuf(s)->len = 0;
        s->signature = xstr_null;
    }
  #ifdef XSTR_USE_ALLOC
    if ( s->self_alloc ) {
      # ifdef XSTR_DEBUG
        fprintf(stderr, "XSTR: ... alloc_self, free(self=%p)\n", s);
      # endif
        free(s);
    }
  #endif
}

/* literal xstring */
static inline S
xstr_new(char const *s, size_t l, _Bool writable) {
    return (S){
                .base = (void *) s,
                .size = l,
                .writable = writable,
                .signature = xstr_valid,
            };
}

static inline char const *
xstr_cstr(S *s) {
    xstr_assert_valid(s);
    return s->base;
}

static inline size_t
xstr_size(S *s) {
    xstr_assert_valid(s);
    return s->size;
}

static inline size_t
xstr_len(S *s) {
    xstr_assert_valid(s);
    if (s->is_buffer)
        return xstr_to_xbuf(s)->len;
    return s->size;
}

static inline size_t
xstr_lenz(S *s) {
    xstr_assert_valid(s);
    if (s->is_buffer)
        return xstr_to_xbuf(s)->len + 1; /* including null terminator */
    return s->size;
}

static inline XS
xstr_loan(S *s) {
    xstr_assert_valid(s);
    XS ret = {
        .offer = {
            .signature = xstr_valid,
            .base = s->base,
            .size = s->size,
  #ifdef XSTR_USE_ALLOC
            .self_alloc = 0,
  #endif
            .data_alloc = 0,
            .is_buffer = 0,
            .writable = 0,
            .loan = 1,
        }
    };
  # ifdef XSTR_DEBUG
    xstr_print_state("Loan", s);
    xstr_print_state(" ...", &ret.offer);
  # endif
    return ret;
}

static inline S
xstr_borrow(XS *bs) {
    xstr_assert_validx(bs);
  # ifdef XSTR_DEBUG
    xstr_print_state("Borrow", &bs->offer);
    fprintf(stderr, "XSTR: ... no change\n");
  # endif
    return bs->offer;
}

/* Giving and Returning are the same logic */
static inline XS
xstr_give_or_return(S *s, _Bool returning) {
    xstr_assert_valid(s);
  # ifdef XSTR_DEBUG
    xstr_print_state(returning ? "Returning" : "Giving", s);
  # endif
    XS ret = {
        .offer = {
            .signature = xstr_valid,
            .base = s->base,
            .size = s->size,
          #ifdef XSTR_USE_ALLOC
            .self_alloc = 0,
          #endif
            .data_alloc = s->data_alloc,
            .is_buffer = 0,
            .writable = s->writable,
            .loan = s->loan,
        },
    };
    if (!returning) {
        /* cleanup just enough that Finish/Ignore doesn't dealloc */
        s->signature = xstr_null;
        s->data_alloc = 0;
        s->size = 0;
        s->base = NULL;
    }
  # ifdef XSTR_DEBUG
    xstr_print_state(" ...", &ret.offer);
        fprintf(stderr, "XSTR: %s(offer=%p, base=%p, size=%zu)\n",
                returning ? "returned" : "given",
                s, s->base, (size_t) s->size);
  # endif
    xstr_finish_or_ignore(s);
    return ret;
}

static inline S
xstr_take(XS *ts) {
    xstr_assert_validx(ts);
    S *s = &ts->offer;
    if ( s->loan ) {
      # ifdef XSTR_DEBUG
        fprintf(stderr, "XSTR: taking clone from loan...\n");
        xstr_print_state(" ...", s);
      # endif
        size_t len = xstr_lenz(s);  /* if it's a buffer, only copy the occupied part, plus room for a NUL terminator */
        S ret = {
            .signature = xstr_valid,
            .base = xstr__memdup(s->base, len),
            .size = len,
            .data_alloc = 1,
            .writable = 1,
            .loan = 0,
          #ifdef XSTR_USE_ALLOC
            .self_alloc = 0,
          #endif
        };
      # ifdef XSTR_DEBUG
        fprintf(stderr, "XSTR: ...cloned(offer=%p, base=%p, f=%zu)\n", ts, s->base, (size_t) s->size);
        xstr_print_state(" ...", &ret);
      # endif
        return ret;
    }
    else {
      # ifdef XSTR_DEBUG
        fprintf(stderr, "XSTR: taking(offer=%p, base=%p, size=%zu)...\n", ts, s->base, (size_t) s->size);
      # endif
        S ret = *s;
      #ifdef XSTR_USE_ALLOC
        ret.self_alloc = 0;
      #endif
        xstr_finish_or_ignore(s, 0)
      # ifdef XSTR_DEBUG
        fprintf(stderr, "XSTR: ...taken(offer=%p, base=%p, f=%zu)\n", ts, s->base, (size_t) s->size);
        xstr_print_state(" ...", &ret);
      # endif
        return ret;
    }
}

static inline char const *
xstr_cs(S *s) {
    xstr_assert_valid(s);
    return s->base;
}

static inline char *
xstr_csw(S *s) {
    xstr_assert_valid(s);
    if (!s->writable)
        xstr_panic("Must not convert unwritable to writable");
    return s->base;
}

#define LS(lit_str) xstr_new((lit_str), (sizeof (lit_str))-1, 0)
#define AS(array,len) xstr_new((array), (len), 1)
#define CS(X)       xstr_cs(&(X))
#define CSW(X)      xstr_csw(&(X))
#define CL(X)       xstr_len(&(X))
#define CSL(X)      CS(&(X)), CL(&(X))
#define CSWL(X)     CSW(&(X)), CL(&(X))

#define Loan(X)     xstr_loan(&(X))
#define Borrow(X)   xstr_borrow(&(X))

#define Give(X)     xstr_give_or_return(&(X), 0)
#define Take(X)     xstr_take(&(X))

#define Finish(X)   xstr_finish_or_ignore(&(X), 0)
#define S2B(X)      xstr_to_xbuf(&(X))

#define Return(X)   return xstr_give_or_return(&(X), 1)
#define Ignore(X)   xstr_finish_or_ignore(&(X)->offer, 1)

static inline B
xbuf_new( size_t cap ) {
    void *p = cap > 0 ? malloc(cap) : NULL;
    return (B){
        .s = (S){
            .base = p,
            .is_buffer = 1,
            .signature = xstr_valid,
            .size = 0,
        },
        .len = 0,
    };
}

#define NewBuffer xbuf_new
