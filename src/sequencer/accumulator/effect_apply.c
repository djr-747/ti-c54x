/* Module contract
 * Owns: structural validation and atomic accumulator-write effect commit,
 * including conditional C/TC and explicit destination-overflow mutation
 * (sticky set via WRITE_1, recompute-to-clear via WRITE_0). The pair
 * entry validates both writes before either commits, then applies
 * first followed by second onto one candidate set.
 * Does not own: compare semantics, decode, CPU lifecycle, or timing.
 */

#include "effect_apply.h"

#include <stddef.h>

static bool
write_is_valid(const ti_c54x_op_accumulator_write_effect *write)
{
    return write->destination < TI_C54X_ACCUMULATOR_INVALID &&
           write->value_raw40 <= TI_C54X_U40_MASK &&
           write->status_delta.c_action >= TI_C54X_OP_STATUS_BIT_PRESERVE &&
           write->status_delta.c_action <
               TI_C54X_OP_STATUS_BIT_ACTION_COUNT &&
           write->status_delta.tc_action >=
               TI_C54X_OP_STATUS_BIT_PRESERVE &&
           write->status_delta.tc_action <
               TI_C54X_OP_STATUS_BIT_ACTION_COUNT &&
           write->status_delta.destination_overflow_action >=
               TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE &&
           write->status_delta.destination_overflow_action <
               TI_C54X_OP_DESTINATION_OVERFLOW_ACTION_COUNT;
}

static ti_c54x_accumulator_effect_status
apply_single_write(const ti_c54x_op_accumulator_write_effect *write,
                   ti_c54x_accumulator_state *accumulators,
                   ti_c54x_status_register_state *status)
{
    if (ti_c54x_accumulator_write(accumulators, write->destination,
                                  write->value_raw40) !=
        TI_C54X_ACCUMULATOR_OK) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    if (write->status_delta.c_action != TI_C54X_OP_STATUS_BIT_PRESERVE &&
        ti_c54x_status_register_write_c(
            status,
            write->status_delta.c_action == TI_C54X_OP_STATUS_BIT_WRITE_1
                ? UINT8_C(1)
                : UINT8_C(0)) != TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    if (write->status_delta.tc_action != TI_C54X_OP_STATUS_BIT_PRESERVE &&
        ti_c54x_status_register_write_tc(
            status,
            write->status_delta.tc_action == TI_C54X_OP_STATUS_BIT_WRITE_1
                ? UINT8_C(1)
                : UINT8_C(0)) != TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    if (write->status_delta.destination_overflow_action !=
            TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE &&
        (write->destination == TI_C54X_ACCUMULATOR_A
             ? ti_c54x_status_register_write_ova(
                   status,
                   write->status_delta.destination_overflow_action ==
                           TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                       ? 1U
                       : 0U)
             : ti_c54x_status_register_write_ovb(
                   status,
                   write->status_delta.destination_overflow_action ==
                           TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                       ? 1U
                       : 0U)) != TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    return TI_C54X_ACCUMULATOR_EFFECT_OK;
}

ti_c54x_accumulator_effect_status ti_c54x_accumulator_effect_apply(
    const ti_c54x_op_effect *effect,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_status_register_state *status)
{
    ti_c54x_accumulator_state candidate_accumulators;
    ti_c54x_status_register_state candidate_status;
    ti_c54x_accumulator_effect_status applied;

    if (effect == NULL || accumulators == NULL || status == NULL) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT;
    }
    if (effect->kind != TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE) {
        return effect->kind == TI_C54X_OP_EFFECT_NONE
                   ? TI_C54X_ACCUMULATOR_EFFECT_OK
                   : TI_C54X_ACCUMULATOR_EFFECT_UNSUPPORTED;
    }
    if (!write_is_valid(&effect->payload.accumulator_write)) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    candidate_accumulators = *accumulators;
    candidate_status = *status;
    applied = apply_single_write(&effect->payload.accumulator_write,
                                 &candidate_accumulators,
                                 &candidate_status);
    if (applied != TI_C54X_ACCUMULATOR_EFFECT_OK) {
        return applied;
    }
    *accumulators = candidate_accumulators;
    *status = candidate_status;
    return TI_C54X_ACCUMULATOR_EFFECT_OK;
}

ti_c54x_accumulator_effect_status ti_c54x_accumulator_effect_apply_pair(
    const ti_c54x_op_effect *effect,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_status_register_state *status)
{
    ti_c54x_accumulator_state candidate_accumulators;
    ti_c54x_status_register_state candidate_status;
    ti_c54x_accumulator_effect_status applied;
    const ti_c54x_op_accumulator_write_pair_effect *pair;

    if (effect == NULL || accumulators == NULL || status == NULL) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT;
    }
    if (effect->kind != TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE_PAIR) {
        return TI_C54X_ACCUMULATOR_EFFECT_UNSUPPORTED;
    }
    pair = &effect->payload.accumulator_write_pair;
    if (!write_is_valid(&pair->first) || !write_is_valid(&pair->second)) {
        return TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT;
    }
    candidate_accumulators = *accumulators;
    candidate_status = *status;
    applied = apply_single_write(&pair->first, &candidate_accumulators,
                                 &candidate_status);
    if (applied != TI_C54X_ACCUMULATOR_EFFECT_OK) {
        return applied;
    }
    applied = apply_single_write(&pair->second, &candidate_accumulators,
                                 &candidate_status);
    if (applied != TI_C54X_ACCUMULATOR_EFFECT_OK) {
        return applied;
    }
    *accumulators = candidate_accumulators;
    *status = candidate_status;
    return TI_C54X_ACCUMULATOR_EFFECT_OK;
}
