#ifndef TI_C54X_SEQUENCER_ADDRESS_EFFECT_APPLY_H
#define TI_C54X_SEQUENCER_ADDRESS_EFFECT_APPLY_H

/*
 * Module contract
 *
 * Owns: checked, transactional application of one value-owned indirect
 * address effect to caller-owned private architectural AR state. Canonical
 * data-operand shape is an intrinsic producer contract validated before any
 * phase relation, mode-state, or AR access.
 * Does not own: address planning, opcode decoding, CPU lifecycle, buses,
 * memory, timing, or diagnostics. It owns validation of the private state
 * precondition carried by an address effect, but not the storage or layout of
 * ARP/CMPT fields and not the public CPU ABI. For a tagged effect, all
 * required mode fields are read through state accessors and must be known
 * before their value constraints or the selected AR is consulted; policy
 * failures never repair or partially mutate state.
 * The state pointer is caller-owned and must remain valid for the duration of
 * the call. The address result pointer is borrowed for the duration of the
 * call and is never retained or modified. A rejected call leaves the complete
 * state object byte-for-byte unchanged; a successful NONE effect does not
 * mutate it.
 */

#include "address/address_effect.h"
#include "state/arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_address_apply_status {
    TI_C54X_ADDRESS_APPLY_OK = 0,
    TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT = 1,
    TI_C54X_ADDRESS_APPLY_REJECTED_RESULT = 2,
    TI_C54X_ADDRESS_APPLY_INVALID_EFFECT = 3,
    TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH = 4,
    TI_C54X_ADDRESS_APPLY_UNKNOWN_AR = 5,
    TI_C54X_ADDRESS_APPLY_STALE_AR = 6,
    TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE = 7,
    TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION = 8
} ti_c54x_address_apply_status;

/*
 * Validate and apply one planner result at its recorded PRE_ACCESS,
 * POST_ACCESS, or NONE phase. The result is borrowed and must have a stable
 * lifetime until the call returns. The state remains caller-owned. A valid
 * `CMPT0_ARP0` policy is checked after intrinsic/phase/address validation,
 * with all required mode fields known before requiring CMPT=0 and ARP=0; this
 * occurs before selected-AR knownness or staleness checks. No arithmetic is
 * inferred: value_before/value_after are checked against the effective
 * address descriptor's first word and the current AR, but are not required to
 * differ by one or to follow a particular address mode. The second word is
 * validated as part of the canonical operand shape and is not used to infer
 * deltas or sequencing.
 */
ti_c54x_address_apply_status ti_c54x_sequencer_apply_address_result(
    ti_c54x_arch_state *state,
    const ti_c54x_address_result *result,
    ti_c54x_address_update_phase requested_phase);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_ADDRESS_EFFECT_APPLY_H */
