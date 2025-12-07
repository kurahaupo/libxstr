
# XSTR

## Synopsis

Efficient binary strings with "value" semantics and statically provable memory
management.

## Description

Minimize string copies while being provably correct.

C doesn't have automatic constructors & destructors, but we get around this by
making it intentionally awkward to pass untracked strings between functions, and
therefore fairly simple to prove correctness by static analysis.

Tooling to prove or disprove correctness will be provided in a future release.

## Mechanism

Each `S` string object either “owns” the buffer that holds, or has borrowed it
from somewhere else.
This module uses Rust's notion of loan+borrow vs give+take to control who owns
a given string. Essentially the idea is to avoid copying strings when the it's
agreed as a transfer or a loan, but to make a copy if both want ownership.

If neither wants ownership (caller Gives but receiver Borrows), the receiver
gets ownership anyway. If both want ownership, then a copy is made.

Correctness is ensured by tracking the "owner" of a string who will eventually
be responsible for freeing its allocated memory.

## Enforcing correctness

Ordinary variables and `struct` members are declared with the `S` type.

Function parameters and return values should be declared using the proxy type `XS`.
This ensures that callers cannot skip specifying whether to `Give` or `Loan` the value.

Note that even if you have `Borrowed` something you should still `Give` it rather
than `Loan` it if you do not intend to make further use of it.

Given an `XS` value, you should `Take` it if you plan to store the string somewhere with a longer
lifetime than the function scope, `Borrow` it if you just want to use it inside the function, or
`Ignore` it if you don't use it at all.

Correctness is ensured provided:

1. The `XS` type is _only_ used for function parameters and returns, while
   the `S` type is _never_ used for function parameters and returns.
2. Every `S` object is:
   1. initialised in its declaration using either `XsNull`, `LS`, `CBL`, `CLB`, or `Clone`, and
   2. eventually subject to either `Return` or `Finish`.
      * It is permissible to use `Finish` repeatedly. 
3. Every `XS` object is eventually either returned, passed to another function, or
   subject to either `Ignore`, `Take`, or `Borrow`.
   * This includes values returned from functions.
   * Attempting to use an `XS` value again after it has been subject to one of these
     will result in an `XNull` value.
   * It is permissible to use `Ignore` repeatedly.

An uninitialised static `S` object is equivalent to initialising with `XsNull`.
Using `CS` or `CSL` or `CLB` on an `XsNull` value gets a `NULL` pointer.

After using `Finish` on an `S` object, it will contain an `XsNull` value.

After using `Ignore` or `Take` or an `XS` object, it will contain an `XsNull` value.

Using `Finish` or `Ignore` on an `XsNull` value has no effect.
Some tooling may be able to detect this situation but for now it should not be relied upon.

## Conversions

* From `S` to C-style:
  * `CS`  (C-string) - extract the base pointer to use as a parameter to a traditional C function.
  * `CBL` - expand to a comma-separated pair {base, length} that passes two arguments to a function call
  * `CLB` - expand to a comma-separated pair {length, base} that passes two arguments to a function call

  (These functions must be used with care; they are semantically equivalent to `Loan`, as
  the receiving function must not take ownership of the supplied pointer.)

* From C-style to `S` objects:

  * `LS`  (literal string) - the content buffer will not change for the duration of the program; the resulting object is in the “borrowed” state.
  * `XBL` take a base+length pair and copy the indicated memory region to create an `S` value; the resulting object is in the “owned” state.

## Usage

Always include these:
```
#include <assert.h> /* ifdef XSTR_DEBUG assert */
#include <malloc.h> /* malloc, free */
#include <memory.h> /* memcpy, strlen */
#include <stdarg.h> /* ifdef XSTR_DEBUG va_start, va_arg, va_end */
#include <stdint.h> /* SIZE_MAX */
#include <stdlib.h> /* ifdef XSTR_DEBUG exit */

#include "xstr.h"
```

### Debugging
If `XSTR_DEBUG` is defined, copious output will be written to `stderr`; in this case you must also:

```
#include <stdio.h>
```

## Examples

Passing:
```
void Func1( XS string_param ) {
    S string_value = Borrow(string_param);
    printf("Funky %s\n", CS(string_value));
    Finish(string_value);
}

void Func2( XS string_param ) {
    S string_value = Take(string_param);
    if (test)
        Func1(Loan(string_value));
    Finish(string_value);
}

 void Func3(void) {
    S string_value = LS("Hello World");
    if (test)
        Func2(Give(string_value));
    Finish(string_value);
}
```

Returning:
```
XS Func4(void) {
    Return(string_value);
}

void Func5(void) {
    S foo = Take(Func4());
    printf("%s\n", CS(foo));
    Finish(foo);
}
```

Short-circuit returning:
```
XS Func6( XS string_param ) {
    Return(string_param); /* this is legal and encouraged */
}
```
