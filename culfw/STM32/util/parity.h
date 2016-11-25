#ifndef _UTIL_PARITY_H_
#define _UTIL_PARITY_H_

/** \file */
/** \defgroup util_parity <util/parity.h>: Parity bit generation
    \code #include <util/parity.h> \endcode

    This header file contains optimized assembler code to calculate
    the parity bit for a byte.
*/
/** \def parity_even_bit
    \ingroup util_parity
    \returns 1 if \c val has an odd number of bits set. */

typedef enum {EVEN = 0, ODD = 1} parity_t;

parity_t parity_even_bit (unsigned int x);

#endif /* _UTIL_PARITY_H_ */
