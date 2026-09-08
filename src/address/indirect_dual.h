#ifndef TI_C54X_INDIRECT_DUAL_H
#define TI_C54X_INDIRECT_DUAL_H

/*
 * Module contract
 *
 * Owns: value-owned planning for a dual-indirect WORD16 operand pair. The
 * raw dual mode is deliberately represented as MOD0 through MOD3 so future
 * mode-specific planners can extend this seam without changing its shape;
 * this slice implements only MOD0.
 * Does not own: CPU/ARP/CMPT state, dual-opcode decoding, register mutation,
 * bus transfers, sequencing, timing, diagnostics, or memory. Inputs are
 * copied scalar snapshots. Returned X/Y results own no borrowed storage and
 * remain valid after return. Every rejected call carries all-zero X/Y effects.
 */

#include <stdint.h>

#include "address_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Keep all four architectural raw selectors representable even though only
 * MOD0 has an admitted effect in this first dual-indirect slice. */
typedef enum ti_c54x_indirect_dual_mode {
    TI_C54X_INDIRECT_DUAL_MOD0 = 0,
    TI_C54X_INDIRECT_DUAL_MOD1 = 1,
    TI_C54X_INDIRECT_DUAL_MOD2 = 2,
    TI_C54X_INDIRECT_DUAL_MOD3 = 3
} ti_c54x_indirect_dual_mode;

/* A dual plan retains one ordinary address result per issued operand. */
typedef struct ti_c54x_indirect_dual_result {
    ti_c54x_address_status status;
    ti_c54x_address_result x;
    ti_c54x_address_result y;
} ti_c54x_indirect_dual_result;

/*
 * Plan two dual-indirect WORD16 READ operands from copied old AR snapshots.
 * W50A admits only actual AR2..AR5 selectors and MOD0 in both raw mode
 * fields. The same selector is legal when both snapshots are equal; it
 * yields two independent same-address results and no update. A same-selector
 * mismatch, selector outside AR2..AR5, malformed raw mode, or invalid access
 * is rejected transactionally. MOD1--MOD3 are represented but return
 * TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS until their evidence and effects
 * are implemented.
 */
ti_c54x_indirect_dual_result ti_c54x_indirect_dual_word16(
    uint8_t x_ar_index,
    uint16_t x_ar_value,
    uint8_t y_ar_index,
    uint16_t y_ar_value,
    ti_c54x_indirect_dual_mode x_mode,
    ti_c54x_indirect_dual_mode y_mode,
    ti_c54x_address_access access);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_INDIRECT_DUAL_H */
