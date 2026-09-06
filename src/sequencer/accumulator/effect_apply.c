/* Module contract
 * Owns: structural validation and atomic accumulator+C effect commit.
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
    const ti_c54x_op_accumulator_select_effect *select;

    if (effect == NULL || accumulators == NULL || status == NULL) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT;
    }
    if (effect->kind != TI_C54X_OP_EFFECT_ACCUMULATOR_SELECT) {
        return effect->kind == TI_C54X_OP_EFFECT_NONE
                   ? TI_C54X_ACCUMULATOR_EFFECT_OK
                   : TI_C54X_ACCUMULATOR_EFFECT_UNSUPPORTED;
    }
    select = &effect->payload.accumulator_select;
    if (select->destination >= TI_C54X_ACCUMULATOR_INVALID ||
        select->selected_raw40 > TI_C54X_U40_MASK ||
        select->c_value > 1U) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    candidate_accumulators = *accumulators;
    candidate_status = *status;
    if (ti_c54x_accumulator_write(&candidate_accumulators,
                                  select->destination,
                                  select->selected_raw40) != TI_C54X_ACCUMULATOR_OK ||
        ti_c54x_status_register_write_c(&candidate_status, select->c_value) !=
            TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    *accumulators = candidate_accumulators;
    *status = candidate_status;
    return TI_C54X_ACCUMULATOR_EFFECT_OK;
}
