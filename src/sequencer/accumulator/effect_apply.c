/* Module contract
 * Owns: structural validation and atomic accumulator-write effect commit,
 * including conditional C mutation.
 * Does not own: compare semantics, decode, CPU lifecycle, or timing.
 */

#include "effect_apply.h"

#include <stddef.h>

ti_c54x_accumulator_effect_status ti_c54x_accumulator_effect_apply(
    const ti_c54x_op_effect *effect,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_status_register_state *status)
{
    ti_c54x_accumulator_state candidate_accumulators;
    ti_c54x_status_register_state candidate_status;
    const ti_c54x_op_accumulator_write_effect *write;

    if (effect == NULL || accumulators == NULL || status == NULL) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT;
    }
    if (effect->kind != TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE) {
        return effect->kind == TI_C54X_OP_EFFECT_NONE
                   ? TI_C54X_ACCUMULATOR_EFFECT_OK
                   : TI_C54X_ACCUMULATOR_EFFECT_UNSUPPORTED;
    }
    write = &effect->payload.accumulator_write;
    if (write->destination >= TI_C54X_ACCUMULATOR_INVALID ||
        write->value_raw40 > TI_C54X_U40_MASK ||
        write->c_action < TI_C54X_OP_ACCUMULATOR_C_PRESERVE ||
        write->c_action >= TI_C54X_OP_ACCUMULATOR_C_ACTION_COUNT ||
        write->c_value > 1U ||
        (write->c_action == TI_C54X_OP_ACCUMULATOR_C_PRESERVE &&
         write->c_value != 0U)) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    candidate_accumulators = *accumulators;
    candidate_status = *status;
    if (ti_c54x_accumulator_write(&candidate_accumulators,
                                  write->destination,
                                  write->value_raw40) !=
        TI_C54X_ACCUMULATOR_OK) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    if (write->c_action == TI_C54X_OP_ACCUMULATOR_C_WRITE &&
        ti_c54x_status_register_write_c(&candidate_status, write->c_value) !=
            TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    *accumulators = candidate_accumulators;
    *status = candidate_status;
    return TI_C54X_ACCUMULATOR_EFFECT_OK;
}
