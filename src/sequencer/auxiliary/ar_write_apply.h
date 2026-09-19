#ifndef TI_C54X_SEQUENCER_AUXILIARY_AR_WRITE_APPLY_H
#define TI_C54X_SEQUENCER_AUXILIARY_AR_WRITE_APPLY_H

/*
 * Module contract
 *
 * Owns: transactional application of the value-owned AR_WRITE
 * effect to caller-owned private architectural state: the copied
 * value-after replaces the addressed auxiliary register through
 * the checked state writer (which marks that AR known). The
 * effect and state pointers are borrowed for the duration of
 * the call; no pointer is retained.
 * Does not own: opcode semantics, decode, effect routing, CPU
 * lifecycle, program-counter changes, memory or bus access,
 * timing, diagnostics, allocation, or observers. NONE is a
 * successful no-op. Unsupported effect kinds and invalid
 * arguments leave the supplied state unchanged.
 */

#include "../../isa/ops/op_effect.h"
#include "../../state/arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_ar_write_effect_status {
    TI_C54X_AR_WRITE_EFFECT_OK = 0,
    TI_C54X_AR_WRITE_EFFECT_INVALID_ARGUMENT = 1,
    TI_C54X_AR_WRITE_EFFECT_UNSUPPORTED = 2
} ti_c54x_ar_write_effect_status;

ti_c54x_ar_write_effect_status
ti_c54x_ar_write_effect_apply(const ti_c54x_op_effect *effect,
                              ti_c54x_arch_state *state);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_AUXILIARY_AR_WRITE_APPLY_H */
