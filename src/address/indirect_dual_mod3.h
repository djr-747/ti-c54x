#ifndef TI_C54X_INDIRECT_DUAL_MOD3_H
#define TI_C54X_INDIRECT_DUAL_MOD3_H

/*
 * Module contract
 *
 * Owns: the shared dual-indirect MOD3 (`*ARx+0%`) post-access AR computation
 * from copied AR, AR0, and BK snapshots. BK=0 selects plain signed ARx+AR0
 * addition; nonzero BK selects section-5.5.3.4 circular movement with a
 * signed AR0 step bounded by BK and a start-inside-buffer requirement.
 * Does not own: operand planning, selector/mode validation, register
 * storage, sequencing, timing, diagnostics, or any other addressing mode.
 * Inputs are copied scalar values; the output is caller-owned and
 * unchanged on every failure. No pointer, allocator, or I/O is used.
 */

#include <stdint.h>

#include "address_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Compute the post-access AR value for one MOD3 side. A NULL output is
 * INVALID_ARGUMENT. BK=0 with AR0=0 computes the identity value; the
 * caller decides whether that is a no-update plan. Out-of-range signed
 * results, oversized steps, out-of-buffer starts, and results above
 * 0xffff are UNRESOLVED_BOUNDARY. OK carries no other meaning. */
ti_c54x_address_status ti_c54x_indirect_dual_mod3_after(
    uint16_t ar_value,
    uint16_t ar0_value,
    uint16_t bk_value,
    uint16_t *value_after);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_INDIRECT_DUAL_MOD3_H */
