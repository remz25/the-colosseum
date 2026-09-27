/* GCC's modulo helpers, done right.
 *
 * GCC compiles `a % b` to __aeabi_idivmod / __aeabi_uidivmod, which must return the quotient in
 * r0 and the remainder in r1. The FE-CLib reference maps those names onto FE8's agbcc routines
 * __modsi3 / __umodsi3, which return the remainder in r0 - so `%` silently gave wrong results
 * (found by the Phase 7 tests). These definitions take precedence over the reference: the
 * quotient comes from FE8's (correct) division routines, the remainder from it.
 * A 64-bit return value is passed in r0 (low word) and r1 (high word), as the EABI wants. */
#include "colosseum.h"

typedef unsigned long long u64_t;

int __aeabi_idiv(int n, int d);                 /* FE8 __divsi3 */
unsigned __aeabi_uidiv(unsigned n, unsigned d); /* FE8 __udivsi3 */

u64_t __aeabi_idivmod(int n, int d);
u64_t __aeabi_uidivmod(unsigned n, unsigned d);

u64_t __aeabi_idivmod(int n, int d)
{
    int q = __aeabi_idiv(n, d);
    int r = n - q * d;
    return (u64_t)(unsigned)q | ((u64_t)(unsigned)r << 32);
}

u64_t __aeabi_uidivmod(unsigned n, unsigned d)
{
    unsigned q = __aeabi_uidiv(n, d);
    unsigned r = n - q * d;
    return (u64_t)q | ((u64_t)r << 32);
}
