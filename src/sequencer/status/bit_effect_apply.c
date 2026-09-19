/*
 * Module contract
 *
 * Owns: atomic application of one value-owned ST0/ST1 status-bit effect.
 * Does not own: opcode decoding, handler semantics, CPU lifecycle, effect
 * routing, PC effects, timing, memory, allocation, callbacks, diagnostics,
 * or global state.
 */

#include "bit_effect_apply.h"

#include <stddef.h>

static ti_c54x_status_bit_effect_status apply_status_bit_write(
    const ti_c54x_op_status_bit_write_effect *write,
    ti_c54x_status_register_state *state)
{
    ti_c54x_status_register_state candidate;
    ti_c54x_status_register_target target;

    if (write == NULL || state == NULL) {
        return TI_C54X_STATUS_BIT_EFFECT_INVALID_ARGUMENT;
    }
    switch (write->target_register) {
    case TI_C54X_OP_STATUS_REGISTER_ST0:
        target = TI_C54X_STATUS_REGISTER_TARGET_ST0;
        break;
    case TI_C54X_OP_STATUS_REGISTER_ST1:
        target = TI_C54X_STATUS_REGISTER_TARGET_ST1;
        break;
    case TI_C54X_OP_STATUS_REGISTER_INVALID:
    default:
        return TI_C54X_STATUS_BIT_EFFECT_INVALID_EFFECT;
    }

    candidate = *state;
    if (ti_c54x_status_register_write_bit(&candidate, target,
                                          write->bit_index,
                                          write->raw_value) !=
        TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_STATUS_BIT_EFFECT_INVALID_EFFECT;
    }
    *state = candidate;
    return TI_C54X_STATUS_BIT_EFFECT_OK;
}

ti_c54x_status_bit_effect_status
ti_c54x_status_bit_effect_apply(const ti_c54x_op_effect *effect,
                                ti_c54x_status_register_state *state)
{
    if (effect == NULL || state == NULL) {
        return TI_C54X_STATUS_BIT_EFFECT_INVALID_ARGUMENT;
    }
    switch (effect->kind) {
    case TI_C54X_OP_EFFECT_NONE:
        return TI_C54X_STATUS_BIT_EFFECT_OK;
    case TI_C54X_OP_EFFECT_STATUS_BIT_WRITE:
        return apply_status_bit_write(&effect->payload.status_bit_write,
                                      state);
    case TI_C54X_OP_EFFECT_REPEAT_START:
    case TI_C54X_OP_EFFECT_REPEAT_BLOCK_START:
    case TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE:
    case TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE:
    case TI_C54X_OP_EFFECT_T_REGISTER_WRITE:
    case TI_C54X_OP_EFFECT_MEMORY:
    default:
        return TI_C54X_STATUS_BIT_EFFECT_UNSUPPORTED;
    }
}
