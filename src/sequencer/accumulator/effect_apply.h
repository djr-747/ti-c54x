#ifndef TI_C54X_SEQUENCER_ACCUMULATOR_EFFECT_APPLY_H
#define TI_C54X_SEQUENCER_ACCUMULATOR_EFFECT_APPLY_H

/* Module contract
 * Owns: validation and atomic commit of a value-owned accumulator+C effect.
 * Does not own: compare semantics, decode, CPU lifecycle, or timing.
 */

#include "../../isa/ops/op_effect.h"
#include "../../state/accumulator/accumulator.h"
#include "../../state/status/registers.h"

typedef enum ti_c54x_accumulator_effect_status {
    TI_C54X_ACCUMULATOR_EFFECT_OK = 0,
    TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT = 1,
    TI_C54X_ACCUMULATOR_EFFECT_UNSUPPORTED = 2,
    TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT = 3
} ti_c54x_accumulator_effect_status;

ti_c54x_accumulator_effect_status ti_c54x_accumulator_effect_apply(
    const ti_c54x_op_effect *effect,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_status_register_state *status);

#endif /* TI_C54X_SEQUENCER_ACCUMULATOR_EFFECT_APPLY_H */
