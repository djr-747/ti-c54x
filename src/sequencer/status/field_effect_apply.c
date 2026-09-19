/*
 * Module contract
 *
 * Owns: transactional application of one value-owned status-field effect.
 * Does not own: opcode decoding, CPU lifecycle, PC effects, timing, memory,
 * allocation, callbacks, diagnostics, or global state.
 */

#include "field_effect_apply.h"

#include <stddef.h>

static ti_c54x_status_field_effect_status apply_status_field_write(
    const ti_c54x_op_status_field_write_effect *write,
    ti_c54x_status_register_state *state)
{
    ti_c54x_status_register_state candidate;
    ti_c54x_status_register_status status;

    if (write == NULL || state == NULL) {
        return TI_C54X_STATUS_FIELD_EFFECT_INVALID_ARGUMENT;
    }
    candidate = *state;
    switch (write->target) {
    case TI_C54X_OP_STATUS_FIELD_TARGET_DP:
        status = ti_c54x_status_register_write_dp(&candidate,
                                                   write->raw_value);
        break;
    case TI_C54X_OP_STATUS_FIELD_TARGET_ASM:
        status = ti_c54x_status_register_write_asm(&candidate,
                                                    write->raw_value);
        break;
    case TI_C54X_OP_STATUS_FIELD_TARGET_ARP:
        if (write->raw_value > UINT16_C(7)) {
            return TI_C54X_STATUS_FIELD_EFFECT_INVALID_EFFECT;
        }
        status = ti_c54x_status_register_write_arp(
            &candidate, (uint8_t)write->raw_value);
        break;
    case TI_C54X_OP_STATUS_FIELD_TARGET_INVALID:
    default:
        return TI_C54X_STATUS_FIELD_EFFECT_INVALID_EFFECT;
    }
    if (status != TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_STATUS_FIELD_EFFECT_INVALID_EFFECT;
    }
    *state = candidate;
    return TI_C54X_STATUS_FIELD_EFFECT_OK;
}

ti_c54x_status_field_effect_status
ti_c54x_status_field_effect_apply(const ti_c54x_op_effect *effect,
                                   ti_c54x_status_register_state *state)
{
    if (effect == NULL || state == NULL) {
        return TI_C54X_STATUS_FIELD_EFFECT_INVALID_ARGUMENT;
    }
    switch (effect->kind) {
    case TI_C54X_OP_EFFECT_NONE:
        return TI_C54X_STATUS_FIELD_EFFECT_OK;
    case TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE:
        return apply_status_field_write(&effect->payload.status_field_write,
                                        state);
    case TI_C54X_OP_EFFECT_REPEAT_START:
    case TI_C54X_OP_EFFECT_REPEAT_BLOCK_START:
    default:
        return TI_C54X_STATUS_FIELD_EFFECT_UNSUPPORTED;
    }
}
