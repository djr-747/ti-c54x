#ifndef TI_C54X_SEQUENCER_REPEAT_PROGRESS_H
#define TI_C54X_SEQUENCER_REPEAT_PROGRESS_H

/*
 * Module contract
 *
 * Owns: value-owned repeat progress derived from a REPEAT_START effect and
 * checked RC observations for each retired body. Does not own: CPU, PC, IRQ,
 * bus, memory, timing, opcode decode, effect dispatch, allocation, callbacks,
 * or observers. State and progress pointers are borrowed for one call; no
 * pointer is retained. Publication is transactional on every rejected call.
 */

#include <stdint.h>

#include "isa/ops/op_effect.h"
#include "state/arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_repeat_progress_phase {
    TI_C54X_REPEAT_PROGRESS_PHASE_IDLE = 0,
    TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE = 1,
    TI_C54X_REPEAT_PROGRESS_PHASE_COMPLETE = 2
} ti_c54x_repeat_progress_phase;

typedef enum ti_c54x_repeat_progress_status {
    TI_C54X_REPEAT_PROGRESS_OK = 0,
    TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT = 1,
    TI_C54X_REPEAT_PROGRESS_INVALID_EFFECT = 2,
    TI_C54X_REPEAT_PROGRESS_ACTIVE = 3,
    TI_C54X_REPEAT_PROGRESS_INVALID_PHASE = 4,
    TI_C54X_REPEAT_PROGRESS_INVALID_STATE = 5,
    TI_C54X_REPEAT_PROGRESS_UNKNOWN_RC = 6,
    TI_C54X_REPEAT_PROGRESS_STALE_RC = 7,
    TI_C54X_REPEAT_PROGRESS_COMPLETE = 8
} ti_c54x_repeat_progress_status;

typedef struct ti_c54x_repeat_progress {
    uint16_t loaded_rc_operand;
    uint32_t body_executions_remaining;
    ti_c54x_repeat_progress_phase phase;
} ti_c54x_repeat_progress;

/* Admit a copied REPEAT_START effect and stage its RC/cardinality. */
ti_c54x_repeat_progress_status
ti_c54x_repeat_progress_start(const ti_c54x_op_effect *effect,
                              ti_c54x_arch_state *state,
                              ti_c54x_repeat_progress *progress);

/* Record one successfully retired body after validating the observed RC. */
ti_c54x_repeat_progress_status
ti_c54x_repeat_progress_record_body_retired(
    ti_c54x_arch_state *state,
    ti_c54x_repeat_progress *progress);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_REPEAT_PROGRESS_H */
