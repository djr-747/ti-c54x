/*
 * Module contract
 *
 * Owns: transactional application of the value-owned AR_WRITE
 * effect to caller-owned private architectural state. Applies
 * to a candidate so staging never mutates live state; the
 * checked state writer marks the addressed AR known on
 * success.
 * Does not own: opcode semantics, decode, effect routing, CPU
 * lifecycle, program-counter changes, memory or bus access,
 * timing, diagnostics, allocation, observers, or logging.
 */

#include "ar_write_apply.h"

#include <stddef.h>
#include <stdint.h>

#include "../../isa/ops/op_effect.h"
#include "../../state/arch_state.h"

ti_c54x_ar_write_effect_status
ti_c54x_ar_write_effect_apply(const ti_c54x_op_effect *effect,
                              ti_c54x_arch_state *state)
{
    uint8_t ar_index;
    uint16_t candidate;

    if (effect == NULL || state == NULL) {
        return TI_C54X_AR_WRITE_EFFECT_INVALID_ARGUMENT;
    }
    switch (effect->kind) {
    case TI_C54X_OP_EFFECT_NONE:
        return TI_C54X_AR_WRITE_EFFECT_OK;
    case TI_C54X_OP_EFFECT_AR_WRITE:
        break;
    default:
        return TI_C54X_AR_WRITE_EFFECT_UNSUPPORTED;
    }
    ar_index = effect->payload.ar_write.ar_index;
    candidate = effect->payload.ar_write.value_after;
    if (ti_c54x_arch_state_write_ar(state, ar_index, candidate) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_AR_WRITE_EFFECT_UNSUPPORTED;
    }
    return TI_C54X_AR_WRITE_EFFECT_OK;
}
