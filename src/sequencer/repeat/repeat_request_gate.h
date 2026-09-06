#ifndef TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_H
#define TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_H

/*
 * Module contract
 *
 * Owns: the pure request-blocking decision attributable only to an active
 * single-repeat window and the literal RS/reset exception.
 * Does not own: general interrupt admission, request sampling, IFR state,
 * acknowledgement, priority, vectors, device wiring, RC, PC, timing, buses,
 * memory, allocation, observers, or diagnostics. The caller owns the output;
 * no pointer is retained after the call.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_sequencer_repeat_request_kind {
    TI_C54X_SEQUENCER_REPEAT_REQUEST_INVALID = 0,
    TI_C54X_SEQUENCER_REPEAT_REQUEST_MASKABLE_INTERRUPT = 1,
    TI_C54X_SEQUENCER_REPEAT_REQUEST_NMI = 2,
    TI_C54X_SEQUENCER_REPEAT_REQUEST_LITERAL_RS_RESET = 3,
    TI_C54X_SEQUENCER_REPEAT_REQUEST_KIND_COUNT
} ti_c54x_sequencer_repeat_request_kind;

typedef enum ti_c54x_sequencer_repeat_request_decision {
    TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID = 0,
    TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED = 1,
    TI_C54X_SEQUENCER_REPEAT_REQUEST_DEFER = 2
} ti_c54x_sequencer_repeat_request_decision;

typedef enum ti_c54x_sequencer_repeat_request_gate_status {
    TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_OK = 0,
    TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_ARGUMENT = 1,
    TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_BODY_EXECUTION_COUNT = 2,
    TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_REQUEST_KIND = 3
} ti_c54x_sequencer_repeat_request_gate_status;

/* Report only whether repeat blocks this request; NOT_BLOCKED is not service. */
ti_c54x_sequencer_repeat_request_gate_status
ti_c54x_sequencer_repeat_request_gate_evaluate(
    uint32_t body_executions_remaining,
    ti_c54x_sequencer_repeat_request_kind request_kind,
    ti_c54x_sequencer_repeat_request_decision *decision);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_H */
