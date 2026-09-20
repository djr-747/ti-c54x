#ifndef TI_C54X_SEQUENCER_INTERRUPT_TAKE_ARBITRATION_H
#define TI_C54X_SEQUENCER_INTERRUPT_TAKE_ARBITRATION_H

/*
 * Module contract
 *
 * Owns: the pure predecode take-arbitration decision for a
 * latched NMI take (P3-S94): TAKE when pending with no repeat
 * ACTIVE and no armed delayed-branch countdown, DEFER while a
 * repeat (single or block) or countdown holds the edge, FETCH
 * otherwise. TI SPRU131G section 6.7 disables NMI until the
 * repeat loop completes; multicycle delay slots likewise delay
 * it. INTM never gates NMI by TI construction, so it is not an
 * input. Maskable arbitration (IMR, priority) belongs to a
 * later slice.
 * Does not own: pending state, repeat/delay state, vectors,
 * the take itself, CPU flow, timing, buses, or diagnostics.
 * All inputs are plain caller-observed booleans; there are no
 * state reads and no side effects.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_take_arbitration_decision {
    TI_C54X_TAKE_ARBITRATION_FETCH = 0,
    TI_C54X_TAKE_ARBITRATION_TAKE_NMI = 1,
    TI_C54X_TAKE_ARBITRATION_DEFER = 2
} ti_c54x_take_arbitration_decision;

ti_c54x_take_arbitration_decision ti_c54x_take_arbitrate_nmi(
    bool nmi_pending, bool repeat_active, bool delayed_branch_armed);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INTERRUPT_TAKE_ARBITRATION_H */
