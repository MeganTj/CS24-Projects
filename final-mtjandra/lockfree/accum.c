#include "cmp_swap.h"
#include "accum.h"


/* Adds "value" to *accum, returning the new total that is held in *accum as
 * a result of this particular addition (i.e. irrespective of other concurrent
 * additions that may be occurring).
 *
 * This function should be thread-safe, allowing calls from multiple threads
 * without generating spurious results.
 */
uint32_t add_to_accum(uint32_t *accum, uint32_t value) {
    // We wait until compare and swap succeeds. When it succeeds, we know that
    // no other concurrent additions have occurred and changed the old value of
    // accum. In this case, it is safe to add accum and value and return our
    // result. Otherwise, we know that accum has changed, so we update the
    // old and new values to be the updated value of accum and the updated
    // value added to the value parameter respectively.
    int old = *accum;
    int new = *accum + value;
    while (compare_and_swap(accum, old, new) != 1) {
        old = *accum;
        new = *accum + value;
    }
    return new;
}

