#ifndef TI_C54X_SEQUENCER_TEMPORARY_TRN_SHIFT_APPLY_H
#define TI_C54X_SEQUENCER_TEMPORARY_TRN_SHIFT_APPLY_H

/*
 * Module contract
 *
 * Owns: transactional application of the value-owned TRN-shift
 * effect to caller-owned private MMR-file and status-register
 * storage: the copied TRN word replaces the MMR TRN cell and
 * the companion TC bit writes ST0 bit 12. The effect and state
 * pointers are borrowed for the duration of the call; no
 * pointer is retained.
 * Does not own: opcode semantics, decode, effect routing, CPU
 * lifecycle, program-counter changes, memory or bus access,
 * timing, diagnostics, allocation, or observers. NONE is a
 * successful no-op. Unsupported effect kinds and invalid
 * arguments leave both supplied states unchanged, as does a
 * non-binary TC bit (reported as an invalid effect).
 */

#include "../../isa/ops/op_effect.h"
#include "../../state/mmr_file.h"
#include "../../state/status/registers.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_trn_shift_effect_status {
    TI_C54X_TRN_SHIFT_EFFECT_OK = 0,
    TI_C54X_TRN_SHIFT_EFFECT_INVALID_ARGUMENT = 1,
    TI_C54X_TRN_SHIFT_EFFECT_UNSUPPORTED = 2,
    TI_C54X_TRN_SHIFT_EFFECT_INVALID_EFFECT = 3
} ti_c54x_trn_shift_effect_status;

ti_c54x_trn_shift_effect_status
ti_c54x_trn_shift_effect_apply(const ti_c54x_op_effect *effect,
                               ti_c54x_mmr_file_state *mmr_file,
                               ti_c54x_status_register_state *status);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_TEMPORARY_TRN_SHIFT_APPLY_H */
