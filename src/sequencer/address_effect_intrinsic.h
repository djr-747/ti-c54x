/*
 * Module contract
 *
 * Owns: state-free validation of one borrowed address effect, including
 * address-owned validators, operand/source shape, update shape, policy,
 * phase, and operand/update relations. It produces the private selected-AR
 * commit description consumed by the sequencer.
 * Does not own: architectural state reads or writes, address planning,
 * opcode decoding, CPU lifecycle, buses, memory, timing, diagnostics, or
 * public ABI. The effect is borrowed for the duration of the call and is
 * never retained or modified; the output is caller-owned and zeroed before
 * validation.
 */

#ifndef TI_C54X_SEQUENCER_ADDRESS_EFFECT_INTRINSIC_H
#define TI_C54X_SEQUENCER_ADDRESS_EFFECT_INTRINSIC_H

#include "address_effect_validate.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Validate all effect facts that do not require architectural state. */
ti_c54x_address_apply_status
ti_c54x_sequencer_validate_address_effect_intrinsic(
    const ti_c54x_address_effect *effect,
    ti_c54x_address_update_phase requested_phase,
    ti_c54x_sequencer_address_commit *out_commit);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_ADDRESS_EFFECT_INTRINSIC_H */
