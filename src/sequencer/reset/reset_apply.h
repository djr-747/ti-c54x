#ifndef TI_C54X_SEQUENCER_RESET_APPLY_H
#define TI_C54X_SEQUENCER_RESET_APPLY_H

/*
 * Module contract
 *
 * Owns: transactional application of the TI SPRU172C software-RESET
 * field list (section RESET, pp. 4-137-4-138) to caller-owned
 * architectural state: IPTR-derived program-counter successor,
 * the eighteen listed ST0/ST1 status fields, and the IFR clear.
 * IPTR is snapshotted before any mutation; unknown IPTR rejects
 * with no state changed. Unlisted state (accumulators, T, ARs,
 * SP, RC, SWWSR, CLKMD, unlisted PMST fields, ST1 reserved bit
 * 10) is preserved bit-for-bit including knownness.
 * Does not own: opcode decoding, handler semantics, CPU lifecycle,
 * repeat/take latch clearing, delayed-branch cancellation,
 * retirement counting, timing, memory, allocation, callbacks,
 * diagnostics, or global state.
 */

#include <stdint.h>

#include "../../state/arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_reset_apply_status {
    TI_C54X_RESET_APPLY_OK = 0,
    TI_C54X_RESET_APPLY_INVALID_ARGUMENT = 1,
    TI_C54X_RESET_APPLY_UNKNOWN_IPTR = 2,
    TI_C54X_RESET_APPLY_INVALID_STATE = 3
} ti_c54x_reset_apply_status;

/* Apply the RESET field list transactionally. On success the state
 * carries the manual's values and pc_word_out carries IPTR << 7;
 * the caller commits pc_word_out to the live program counter. */
ti_c54x_reset_apply_status ti_c54x_reset_apply(
    ti_c54x_arch_state *state, uint16_t *pc_word_out);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_RESET_APPLY_H */
