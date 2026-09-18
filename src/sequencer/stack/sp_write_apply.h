#ifndef TI_C54X_SEQUENCER_STACK_SP_WRITE_APPLY_H
#define TI_C54X_SEQUENCER_STACK_SP_WRITE_APPLY_H

/*
 * Module contract
 *
 * Owns: transactional application of the value-owned SP_WRITE
 * effect to caller-owned private architectural state: the copied
 * value-after replaces SP through the checked state writer
 * (which marks SP known). The effect and state pointers are
 * borrowed for the duration of the call; no pointer is retained.
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

typedef enum ti_c54x_sp_write_effect_status {
    TI_C54X_SP_WRITE_EFFECT_OK = 0,
    TI_C54X_SP_WRITE_EFFECT_INVALID_ARGUMENT = 1,
    TI_C54X_SP_WRITE_EFFECT_UNSUPPORTED = 2
} ti_c54x_sp_write_effect_status;

ti_c54x_sp_write_effect_status
ti_c54x_sp_write_effect_apply(const ti_c54x_op_effect *effect,
                              ti_c54x_arch_state *state);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_STACK_SP_WRITE_APPLY_H */
