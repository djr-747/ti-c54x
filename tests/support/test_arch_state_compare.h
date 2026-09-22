#ifndef TI_C54X_TEST_ARCH_STATE_COMPARE_H
#define TI_C54X_TEST_ARCH_STATE_COMPARE_H

/* Module contract
 * Owns: a padding-safe equality check over ti_c54x_arch_state for
 * test-case use. Production commit paths leave struct padding
 * stale, which the optimizer exposes under -O3, so byte-wise
 * memcmp of two architecturally identical states can fail; this
 * comparator checks every architectural field individually and
 * never reads padding.
 * Does not own: production assertions, test fixtures, failure
 * reporting, or test-case discovery. Inputs are borrowed
 * read-only.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "state/arch_state.h"

static inline bool
ti_c54x_test_arch_state_equal(const ti_c54x_arch_state *actual,
                              const ti_c54x_arch_state *expected)
{
    size_t index;

    if (actual == NULL || expected == NULL) {
        return false;
    }
    if (actual->pc_word != expected->pc_word ||
        actual->ifr != expected->ifr || actual->rc != expected->rc ||
        actual->bk != expected->bk || actual->sp != expected->sp ||
        actual->rtn != expected->rtn ||
        actual->xpc != expected->xpc) {
        return false;
    }
    for (index = 0U; index < 8U; ++index) {
        if (actual->ar[index] != expected->ar[index]) {
            return false;
        }
    }
    if (actual->status.st0 != expected->status.st0 ||
        actual->status.st1 != expected->status.st1 ||
        actual->status.pmst != expected->status.pmst ||
        actual->status.st0_known_mask !=
            expected->status.st0_known_mask ||
        actual->status.st1_known_mask !=
            expected->status.st1_known_mask ||
        actual->status.pmst_known_mask !=
            expected->status.pmst_known_mask) {
        return false;
    }
    if (actual->mmr_file.imr != expected->mmr_file.imr ||
        actual->mmr_file.trn != expected->mmr_file.trn ||
        actual->mmr_file.brc != expected->mmr_file.brc ||
        actual->mmr_file.rsa != expected->mmr_file.rsa ||
        actual->mmr_file.rea != expected->mmr_file.rea ||
        actual->mmr_file.known_mask !=
            expected->mmr_file.known_mask) {
        return false;
    }
    if (actual->swwsr.value != expected->swwsr.value ||
        actual->swwsr.known != expected->swwsr.known ||
        actual->clkmd.value != expected->clkmd.value ||
        actual->clkmd.known != expected->clkmd.known ||
        actual->scalar_known_mask != expected->scalar_known_mask ||
        actual->ar_known_mask != expected->ar_known_mask) {
        return false;
    }
    return true;
}

#endif /* TI_C54X_TEST_ARCH_STATE_COMPARE_H */
