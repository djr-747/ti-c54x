#ifndef TI_C54X_SEQUENCER_TEMPORARY_EFFECT_APPLY_H
#define TI_C54X_SEQUENCER_TEMPORARY_EFFECT_APPLY_H

/*
 * Module contract
 *
 * Owns: transactional application of the value-owned T-register effect to
 * caller-owned private T-register storage. The effect and state pointers are
 * borrowed for the duration of the call; no pointer is retained.
 * Does not own: opcode semantics, decode, effect routing, CPU lifecycle,
 * program-counter changes, MMR or memory access, timing, diagnostics,
 * allocation, or observers. NONE is a successful no-op. Unsupported effect
 * kinds and invalid arguments leave the supplied T state unchanged.
 */

#include "../../isa/ops/op_effect.h"
#include "../../state/temporary/t_register.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_t_register_effect_status {
    TI_C54X_T_REGISTER_EFFECT_OK = 0,
    TI_C54X_T_REGISTER_EFFECT_INVALID_ARGUMENT = 1,
    TI_C54X_T_REGISTER_EFFECT_UNSUPPORTED = 2
} ti_c54x_t_register_effect_status;

ti_c54x_t_register_effect_status
ti_c54x_t_register_effect_apply(const ti_c54x_op_effect *effect,
                                ti_c54x_t_register_state *state);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_TEMPORARY_EFFECT_APPLY_H */
