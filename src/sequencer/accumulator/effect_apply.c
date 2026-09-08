/* Module contract
 * Owns: structural validation and atomic accumulator-write effect commit,
 * including conditional C/TC and sticky destination-overflow mutation.
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
        write->status_delta.c_action < TI_C54X_OP_STATUS_BIT_PRESERVE ||
        write->status_delta.c_action >= TI_C54X_OP_STATUS_BIT_ACTION_COUNT ||
        write->status_delta.tc_action < TI_C54X_OP_STATUS_BIT_PRESERVE ||
        write->status_delta.tc_action >= TI_C54X_OP_STATUS_BIT_ACTION_COUNT ||
        write->status_delta.destination_overflow_action <
            TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE ||
        write->status_delta.destination_overflow_action >=
            TI_C54X_OP_DESTINATION_OVERFLOW_ACTION_COUNT) {
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
    if (write->status_delta.c_action != TI_C54X_OP_STATUS_BIT_PRESERVE &&
        ti_c54x_status_register_write_c(
            &candidate_status,
            write->status_delta.c_action == TI_C54X_OP_STATUS_BIT_WRITE_1
                ? UINT8_C(1)
                : UINT8_C(0)) != TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    if (write->status_delta.tc_action != TI_C54X_OP_STATUS_BIT_PRESERVE &&
        ti_c54x_status_register_write_tc(
            &candidate_status,
            write->status_delta.tc_action == TI_C54X_OP_STATUS_BIT_WRITE_1
                ? UINT8_C(1)
                : UINT8_C(0)) != TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    if (write->status_delta.destination_overflow_action ==
            TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1 &&
        (write->destination == TI_C54X_ACCUMULATOR_A
             ? ti_c54x_status_register_write_ova(&candidate_status, 1U)
             : ti_c54x_status_register_write_ovb(&candidate_status, 1U)) !=
            TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    *accumulators = candidate_accumulators;
    *status = candidate_status;
    return TI_C54X_ACCUMULATOR_EFFECT_OK;
}
