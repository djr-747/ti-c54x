/*
 * Module contract
 *
 * Owns: one maskable IFR pending latch and its checked repeat-gated lifecycle.
 * Does not own: interrupt source wiring, vectors, priority, NMI, predecode,
 * PC, CPU execution, timing, buses, callbacks, allocation, observers, or
 * logging.
 */

#include "repeat_pending_request.h"

#include "state/interrupt_flag_register.h"
#include "sequencer/repeat/repeat_cardinality.h"

#include <stddef.h>
#include <stdint.h>

static int
is_one_bit(uint16_t mask)
{
    return mask != UINT16_C(0) && (mask & (uint16_t)(mask - UINT16_C(1))) ==
                                      UINT16_C(0);
}

static int
pending_is_canonical(const ti_c54x_repeat_pending_request *pending)
{
    if (pending == NULL) {
        return 0;
    }
    if (pending->phase == TI_C54X_REPEAT_PENDING_REQUEST_EMPTY) {
        return pending->mask == UINT16_C(0);
    }
    if (pending->phase == TI_C54X_REPEAT_PENDING_REQUEST_LATCHED) {
        return is_one_bit(pending->mask);
    }
    return 0;
}

static ti_c54x_repeat_pending_request_status
validate_common(ti_c54x_arch_state *state,
                ti_c54x_repeat_pending_request *pending,
                uint16_t mask)
{
    uint16_t ifr;

    if (state == NULL || pending == NULL) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_ARGUMENT;
    }
    if (!is_one_bit(mask)) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_MASK;
    }
    if (!pending_is_canonical(pending)) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_PENDING;
    }
    if (ti_c54x_arch_state_read_ifr(state, &ifr) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_REPEAT_PENDING_REQUEST_UNKNOWN_IFR;
    }
    return TI_C54X_REPEAT_PENDING_REQUEST_OK;
}

ti_c54x_repeat_pending_request_status
ti_c54x_repeat_pending_request_receive(
    ti_c54x_arch_state *state,
    ti_c54x_repeat_pending_request *pending,
    uint16_t mask,
    uint32_t body_executions_remaining,
    ti_c54x_sequencer_repeat_request_decision *decision)
{
    ti_c54x_sequencer_repeat_request_decision staged_decision;
    ti_c54x_repeat_pending_request_status status;

    if (state == NULL || pending == NULL || decision == NULL) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_ARGUMENT;
    }
    if (!is_one_bit(mask)) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_MASK;
    }
    if (body_executions_remaining >
        TI_C54X_SEQUENCER_REPEAT_MAX_BODY_EXECUTION_COUNT) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_COUNT;
    }
    if (!pending_is_canonical(pending)) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_PENDING;
    }
    if (pending->phase == TI_C54X_REPEAT_PENDING_REQUEST_LATCHED) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_PENDING;
    }
    status = validate_common(state, pending, mask);
    if (status != TI_C54X_REPEAT_PENDING_REQUEST_OK) {
        return status;
    }
    if (ti_c54x_sequencer_repeat_request_gate_evaluate(
            body_executions_remaining,
            TI_C54X_SEQUENCER_REPEAT_REQUEST_MASKABLE_INTERRUPT,
            &staged_decision) != TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_OK) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_COUNT;
    }
    if (ti_c54x_arch_state_set_ifr_bit(state, mask) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_REPEAT_PENDING_REQUEST_UNKNOWN_IFR;
    }
    pending->mask = mask;
    pending->phase = TI_C54X_REPEAT_PENDING_REQUEST_LATCHED;
    *decision = staged_decision;
    return TI_C54X_REPEAT_PENDING_REQUEST_OK;
}

ti_c54x_repeat_pending_request_status
ti_c54x_repeat_pending_request_take(
    ti_c54x_arch_state *state,
    ti_c54x_repeat_pending_request *pending,
    uint32_t body_executions_remaining)
{
    ti_c54x_sequencer_repeat_request_decision decision;
    uint16_t ifr;

    if (state == NULL || pending == NULL) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_ARGUMENT;
    }
    if (body_executions_remaining >
        TI_C54X_SEQUENCER_REPEAT_MAX_BODY_EXECUTION_COUNT) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_COUNT;
    }
    if (!pending_is_canonical(pending)) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_PENDING;
    }
    if (pending->phase == TI_C54X_REPEAT_PENDING_REQUEST_EMPTY) {
        return TI_C54X_REPEAT_PENDING_REQUEST_NO_PENDING;
    }
    if (ti_c54x_arch_state_read_ifr(state, &ifr) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_REPEAT_PENDING_REQUEST_UNKNOWN_IFR;
    }
    if ((ifr & pending->mask) == UINT16_C(0)) {
        return TI_C54X_REPEAT_PENDING_REQUEST_STALE_PENDING;
    }
    if (ti_c54x_sequencer_repeat_request_gate_evaluate(
            body_executions_remaining,
            TI_C54X_SEQUENCER_REPEAT_REQUEST_MASKABLE_INTERRUPT,
            &decision) != TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_OK) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_COUNT;
    }
    if (decision == TI_C54X_SEQUENCER_REPEAT_REQUEST_DEFER) {
        return TI_C54X_REPEAT_PENDING_REQUEST_DEFER;
    }
    if (ti_c54x_arch_state_clear_ifr_bit(state, pending->mask) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_REPEAT_PENDING_REQUEST_UNKNOWN_IFR;
    }
    pending->mask = UINT16_C(0);
    pending->phase = TI_C54X_REPEAT_PENDING_REQUEST_EMPTY;
    return TI_C54X_REPEAT_PENDING_REQUEST_OK;
}

static ti_c54x_repeat_pending_request_status
clear_path(ti_c54x_arch_state *state,
           ti_c54x_repeat_pending_request *pending,
           uint16_t mask)
{
    ti_c54x_repeat_pending_request_status status;

    status = validate_common(state, pending, mask);
    if (status != TI_C54X_REPEAT_PENDING_REQUEST_OK) {
        return status;
    }
    if (pending->phase == TI_C54X_REPEAT_PENDING_REQUEST_LATCHED &&
        (state->ifr & pending->mask) == UINT16_C(0)) {
        return TI_C54X_REPEAT_PENDING_REQUEST_STALE_PENDING;
    }
    if (ti_c54x_arch_state_clear_ifr_bit(state, mask) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_REPEAT_PENDING_REQUEST_UNKNOWN_IFR;
    }
    if (pending->phase == TI_C54X_REPEAT_PENDING_REQUEST_LATCHED &&
        mask == pending->mask) {
        pending->mask = UINT16_C(0);
        pending->phase = TI_C54X_REPEAT_PENDING_REQUEST_EMPTY;
    }
    return TI_C54X_REPEAT_PENDING_REQUEST_OK;
}

ti_c54x_repeat_pending_request_status
ti_c54x_repeat_pending_request_on_ifr_write_one(
    ti_c54x_arch_state *state,
    ti_c54x_repeat_pending_request *pending,
    uint16_t mask)
{
    return clear_path(state, pending, mask);
}

ti_c54x_repeat_pending_request_status
ti_c54x_repeat_pending_request_on_intr(
    ti_c54x_arch_state *state,
    ti_c54x_repeat_pending_request *pending,
    uint16_t mask)
{
    return clear_path(state, pending, mask);
}

ti_c54x_repeat_pending_request_status
ti_c54x_repeat_pending_request_on_reset(
    ti_c54x_arch_state *state,
    ti_c54x_repeat_pending_request *pending)
{
    if (state == NULL || pending == NULL) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_ARGUMENT;
    }
    if (!pending_is_canonical(pending)) {
        return TI_C54X_REPEAT_PENDING_REQUEST_INVALID_PENDING;
    }
    if (ti_c54x_arch_state_write_ifr(state, UINT16_C(0)) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_REPEAT_PENDING_REQUEST_UNKNOWN_IFR;
    }
    pending->mask = UINT16_C(0);
    pending->phase = TI_C54X_REPEAT_PENDING_REQUEST_EMPTY;
    return TI_C54X_REPEAT_PENDING_REQUEST_OK;
}
