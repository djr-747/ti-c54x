/*
 * Module contract
 *
 * Owns: transactional application of the value-owned TRN-shift
 * effect to caller-owned private MMR-file and status-register
 * storage. Applies to candidates so staging never mutates live
 * state; both destinations validate before either is written.
 * Does not own: opcode semantics, decode, effect routing, CPU
 * lifecycle, program-counter changes, memory or bus access,
 * timing, diagnostics, allocation, observers, or logging.
 */

#include "trn_shift_apply.h"

#include <stddef.h>
#include <stdint.h>

#include "../../isa/ops/op_effect.h"
#include "../../state/mmr_file.h"
#include "../../state/status/registers.h"

#define TRN_SHIFT_TC_BIT_INDEX UINT8_C(12)

ti_c54x_trn_shift_effect_status
ti_c54x_trn_shift_effect_apply(const ti_c54x_op_effect *effect,
                               ti_c54x_mmr_file_state *mmr_file,
                               ti_c54x_status_register_state *status)
{
    ti_c54x_mmr_file_state mmr_candidate;
    ti_c54x_status_register_state status_candidate;

    if (effect == NULL || mmr_file == NULL || status == NULL) {
        return TI_C54X_TRN_SHIFT_EFFECT_INVALID_ARGUMENT;
    }
    switch (effect->kind) {
    case TI_C54X_OP_EFFECT_NONE:
        return TI_C54X_TRN_SHIFT_EFFECT_OK;
    case TI_C54X_OP_EFFECT_TRN_SHIFT_WRITE:
        break;
    default:
        return TI_C54X_TRN_SHIFT_EFFECT_UNSUPPORTED;
    }
    if (effect->payload.trn_shift_write.tc_bit > UINT8_C(1)) {
        return TI_C54X_TRN_SHIFT_EFFECT_INVALID_EFFECT;
    }

    mmr_candidate = *mmr_file;
    status_candidate = *status;
    if (ti_c54x_mmr_file_write_trn(
            &mmr_candidate,
            effect->payload.trn_shift_write.trn_word) !=
        TI_C54X_MMR_FILE_OK) {
        return TI_C54X_TRN_SHIFT_EFFECT_UNSUPPORTED;
    }
    if (ti_c54x_status_register_write_bit(
            &status_candidate, TI_C54X_STATUS_REGISTER_TARGET_ST0,
            TRN_SHIFT_TC_BIT_INDEX,
            effect->payload.trn_shift_write.tc_bit) !=
        TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_TRN_SHIFT_EFFECT_UNSUPPORTED;
    }
    *mmr_file = mmr_candidate;
    *status = status_candidate;
    return TI_C54X_TRN_SHIFT_EFFECT_OK;
}
