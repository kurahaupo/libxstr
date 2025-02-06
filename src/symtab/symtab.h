/* symtab.h
 *
 * Used for
 *  - global variables
 *  - function-local variables
 *  - expression-local variables (including parameters)
 *  - hashmaps
 */

#include "xstr.h"

typedef struct syment SYMENT;
typedef struct symtab SYMTAB;

SYMENT *getsym(SYMTAB *t, SS s) {
}
