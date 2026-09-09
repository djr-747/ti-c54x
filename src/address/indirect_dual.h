#ifndef TI_C54X_INDIRECT_DUAL_H
#define TI_C54X_INDIRECT_DUAL_H

/*
 * Module contract
 *
 * Owns: value-owned planning for a dual-indirect WORD16 operand pair. The
 * raw dual mode is deliberately represented as MOD0 through MOD3. MOD0,
 * MOD1, and MOD2 plan WORD16 READs as before; MOD3 sides plan `*ARx+0%`
 * from copied AR0/BK snapshots (BK=0 indexed addition, nonzero-BK circular
 * movement). MOD3 sides still observe no CMPT/ARP state: selectors stay
 * directly encoded and snapshots stay caller-copied scalars.
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

/* Keep all four architectural raw selectors representable. W51A admits
 * MOD0 through MOD2; MOD3 remains an explicit deferred boundary. */
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
 * Plan two dual-indirect WORD16 READ operands from copied old AR snapshots
 * plus copied AR0/BK snapshots consumed only by MOD3 sides. W51A admits
 * actual AR2..AR5 selectors and MOD0/MOD1/MOD2. MOD0 reads the old address
 * without an update; MOD1 and MOD2 read the old address and stage one
 * post-access decrement/increment update. A MOD3 side reads the old address
 * and stages one post-access AR0/BK update, or no update for the BK=0/AR0=0
 * documented no-modification case. The same selector is legal when both
 * snapshots are equal: both operands retain that old address, X mode owns
 * at most one update, and Y carries no update. A same-selector mismatch,
 * selector outside AR2..AR5, malformed raw mode, invalid access, an
 * unresolved decrement/increment boundary, or an unresolved MOD3 AR0/BK
 * boundary is rejected transactionally.
 */
ti_c54x_indirect_dual_result ti_c54x_indirect_dual_word16(
    uint8_t x_ar_index,
    uint16_t x_ar_value,
    uint8_t y_ar_index,
    uint16_t y_ar_value,
    ti_c54x_indirect_dual_mode x_mode,
    ti_c54x_indirect_dual_mode y_mode,
    uint16_t ar0_value,
    uint16_t bk_value,
    ti_c54x_address_access access);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_INDIRECT_DUAL_H */
