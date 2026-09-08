#ifndef TI_C54X_SEQUENCER_INTERRUPT_REPEAT_PENDING_REQUEST_H
#define TI_C54X_SEQUENCER_INTERRUPT_REPEAT_PENDING_REQUEST_H

/*
 * Module contract
 *
 * Owns: one caller-supplied maskable IFR pending bit and its EMPTY/LATCHED
 * lifecycle, including repeat-gate deferral and explicit count-zero take.
 * Does not own: interrupt sources, vectors, priority, NMI, predecode, PC, CPU
 * execution, timing, buses, callbacks, allocation, observers, or diagnostics.
 * State and pending objects are borrowed for one call; no pointer is retained.
 * Rejected operations publish no partial mutation.
 */

#include <stdint.h>

#include "sequencer/repeat/repeat_request_gate.h"
#include "state/arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_repeat_pending_request_phase {
    TI_C54X_REPEAT_PENDING_REQUEST_EMPTY = 0,
    TI_C54X_REPEAT_PENDING_REQUEST_LATCHED = 1
} ti_c54x_repeat_pending_request_phase;

typedef struct ti_c54x_repeat_pending_request {
    uint16_t mask;
    ti_c54x_repeat_pending_request_phase phase;
} ti_c54x_repeat_pending_request;

typedef enum ti_c54x_repeat_pending_request_status {
    TI_C54X_REPEAT_PENDING_REQUEST_OK = 0,
    TI_C54X_REPEAT_PENDING_REQUEST_DEFER = 1,
    TI_C54X_REPEAT_PENDING_REQUEST_INVALID_ARGUMENT = 2,
    TI_C54X_REPEAT_PENDING_REQUEST_INVALID_MASK = 3,
    TI_C54X_REPEAT_PENDING_REQUEST_INVALID_COUNT = 4,
    TI_C54X_REPEAT_PENDING_REQUEST_UNKNOWN_IFR = 5,
    TI_C54X_REPEAT_PENDING_REQUEST_INVALID_PENDING = 6,
    TI_C54X_REPEAT_PENDING_REQUEST_STALE_PENDING = 7,
    TI_C54X_REPEAT_PENDING_REQUEST_NO_PENDING = 8
} ti_c54x_repeat_pending_request_status;

/* Receive one maskable request, latch IFR even when decision is DEFER. */
ti_c54x_repeat_pending_request_status
ti_c54x_repeat_pending_request_receive(
    ti_c54x_arch_state *state,
    ti_c54x_repeat_pending_request *pending,
    uint16_t mask,
    uint32_t body_executions_remaining,
    ti_c54x_sequencer_repeat_request_decision *decision);

/* Take a latched request only when the supplied repeat count is zero. */
ti_c54x_repeat_pending_request_status
ti_c54x_repeat_pending_request_take(
    ti_c54x_arch_state *state,
    ti_c54x_repeat_pending_request *pending,
    uint32_t body_executions_remaining);

/* Model documented IFR write-one clear and INTR clear paths separately. */
ti_c54x_repeat_pending_request_status
ti_c54x_repeat_pending_request_on_ifr_write_one(
    ti_c54x_arch_state *state,
    ti_c54x_repeat_pending_request *pending,
    uint16_t mask);

ti_c54x_repeat_pending_request_status
ti_c54x_repeat_pending_request_on_intr(
    ti_c54x_arch_state *state,
    ti_c54x_repeat_pending_request *pending,
    uint16_t mask);

/* Clear all IFR flags and the one pending latch through the reset path. */
ti_c54x_repeat_pending_request_status
ti_c54x_repeat_pending_request_on_reset(
    ti_c54x_arch_state *state,
    ti_c54x_repeat_pending_request *pending);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INTERRUPT_REPEAT_PENDING_REQUEST_H */
