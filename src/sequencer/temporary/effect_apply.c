/*
 * Module contract
 *
 * Owns: atomic application of one value-owned T-register effect. A successful
 * write copies the effect's uint16_t value and publishes T as known through
 * the checked state owner. Every rejected call leaves the state byte-identical.
 * Does not own: opcode semantics, decode, effect routing, CPU lifecycle,
 * program-counter changes, MMR or memory access, timing, diagnostics,
 * allocation, or observers.
 */

#include "effect_apply.h"

#include <stddef.h>

ti_c54x_t_register_effect_status
ti_c54x_t_register_effect_apply(const ti_c54x_op_effect *effect,
                                ti_c54x_t_register_state *state)
{
    ti_c54x_t_register_state candidate;

    if (effect == NULL || state == NULL) {
        return TI_C54X_T_REGISTER_EFFECT_INVALID_ARGUMENT;
    }

    switch (effect->kind) {
    case TI_C54X_OP_EFFECT_NONE:
        return TI_C54X_T_REGISTER_EFFECT_OK;
    case TI_C54X_OP_EFFECT_T_REGISTER_WRITE:
        candidate = *state;
        if (ti_c54x_t_register_write(
                &candidate, effect->payload.t_register_write.raw_value) !=
            TI_C54X_T_REGISTER_OK) {
            return TI_C54X_T_REGISTER_EFFECT_INVALID_ARGUMENT;
        }
        *state = candidate;
        return TI_C54X_T_REGISTER_EFFECT_OK;
    case TI_C54X_OP_EFFECT_REPEAT_START:
    case TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE:
    case TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE:
    default:
        return TI_C54X_T_REGISTER_EFFECT_UNSUPPORTED;
    }
}
