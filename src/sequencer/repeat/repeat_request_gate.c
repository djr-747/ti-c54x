/*
 * Module contract
 *
 * Owns: transactional evaluation of the request gate imposed by a supplied
 * single-repeat remaining-body count.
 * Does not own: request sampling, general interrupt admission, IFR mutation,
 * acknowledgement, vectors, priority, devices, RC, PC, timing, buses, memory,
 * allocation, observers, or diagnostics.
 */

#include "repeat_request_gate.h"

#include "repeat_cardinality.h"

#include <stddef.h>
#include <stdint.h>

ti_c54x_sequencer_repeat_request_gate_status
ti_c54x_sequencer_repeat_request_gate_evaluate(
    uint32_t body_executions_remaining,
    ti_c54x_sequencer_repeat_request_kind request_kind,
    ti_c54x_sequencer_repeat_request_decision *decision)
{
    ti_c54x_sequencer_repeat_request_decision staged;

    if (decision == NULL) {
        return TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_ARGUMENT;
    }
    if (body_executions_remaining >
        TI_C54X_SEQUENCER_REPEAT_MAX_BODY_EXECUTION_COUNT) {
        return
            TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_BODY_EXECUTION_COUNT;
    }

    switch (request_kind) {
    case TI_C54X_SEQUENCER_REPEAT_REQUEST_MASKABLE_INTERRUPT:
    case TI_C54X_SEQUENCER_REPEAT_REQUEST_NMI:
        staged = body_executions_remaining == UINT32_C(0)
                     ? TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED
                     : TI_C54X_SEQUENCER_REPEAT_REQUEST_DEFER;
        break;
    case TI_C54X_SEQUENCER_REPEAT_REQUEST_LITERAL_RS_RESET:
        staged = TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED;
        break;
    case TI_C54X_SEQUENCER_REPEAT_REQUEST_INVALID:
    case TI_C54X_SEQUENCER_REPEAT_REQUEST_KIND_COUNT:
    default:
        return TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_REQUEST_KIND;
    }

    *decision = staged;
    return TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_OK;
}
