#ifndef TI_C54X_SEQUENCER_STATUS_FIELD_EFFECT_APPLY_H
#define TI_C54X_SEQUENCER_STATUS_FIELD_EFFECT_APPLY_H

/*
 * Module contract
 *
 * Owns: transactional application of one value-owned status-field effect to
 * caller-owned private status storage.
 * Does not own: opcode decoding, CPU lifecycle, PC effects, timing, memory,
 * allocation, callbacks, diagnostics, or global state. Unsupported effects
 * and invalid payloads leave the supplied status bank unchanged.
 */

#include "../../isa/ops/op_effect.h"
#include "../../state/status/registers.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_status_field_effect_status {
    TI_C54X_STATUS_FIELD_EFFECT_OK = 0,
    TI_C54X_STATUS_FIELD_EFFECT_INVALID_ARGUMENT = 1,
    TI_C54X_STATUS_FIELD_EFFECT_UNSUPPORTED = 2,
    TI_C54X_STATUS_FIELD_EFFECT_INVALID_EFFECT = 3
} ti_c54x_status_field_effect_status;

ti_c54x_status_field_effect_status
ti_c54x_status_field_effect_apply(
    const ti_c54x_op_effect *effect,
    ti_c54x_status_register_state *state);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_STATUS_FIELD_EFFECT_APPLY_H */
