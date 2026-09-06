#ifndef TI_C54X_BUS_TRANSACTION_WORD16_TRANSACTION_H
#define TI_C54X_BUS_TRANSACTION_WORD16_TRANSACTION_H

/*
 * Module contract
 *
 * Owns: the private callback-free, allocation-free lifecycle reducer for one
 * caller-owned WORD16 bus request.
 * Does not own: external issue/completion callbacks, physical memory or
 * visibility, arbitration, device timing, address-effect phases, CPU state,
 * public ABI, observers, diagnostics, environment, or globals.
 *
 * The caller declares storage as canonical {0}, provides a fresh nonzero ID
 * for each reuse while stale observations may exist, and must not create a
 * second authoritative copy of active storage. Ordinary pointer aliases to
 * the same storage are permitted under caller synchronization. Requests and
 * observations are borrowed only until the call returns; issue and completion
 * results own their named values.
 */

#include <stdint.h>

#include "../request/word16_request.h"
#include <ti_c54x/bus.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_bus_word16_transaction_status {
    TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK = 0,
    TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT = 1,
    TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT = 2,
    TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT = 3,
    TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE = 4,
    TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION = 5
} ti_c54x_bus_word16_transaction_status;

typedef enum ti_c54x_bus_word16_transaction_state {
    TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE = 0,
    TI_C54X_BUS_WORD16_TRANSACTION_STATE_ISSUE_READY = 1,
    TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT = 2,
    TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETION_READY = 3,
    TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE = 4,
    TI_C54X_BUS_WORD16_TRANSACTION_STATE_FAULT = 5
} ti_c54x_bus_word16_transaction_state;

typedef enum ti_c54x_bus_word16_transaction_observation_kind {
    TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_INVALID = 0,
    TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_WAIT = 1,
    TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_READY = 2,
    TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_FAULT = 3
} ti_c54x_bus_word16_transaction_observation_kind;

typedef struct ti_c54x_bus_word16_transaction_observation {
    uint64_t transaction_id;
    ti_c54x_bus_word16_transaction_observation_kind kind;
    uint16_t read_value_word;
    ti_c54x_bus_status bus_status;
} ti_c54x_bus_word16_transaction_observation;

typedef struct ti_c54x_bus_word16_transaction {
    ti_c54x_bus_word16_transaction_state state;
    uint64_t transaction_id;
    ti_c54x_bus_word16_request request;
    uint16_t staged_read_value_word;
    ti_c54x_bus_status staged_bus_status;
} ti_c54x_bus_word16_transaction;

typedef struct ti_c54x_bus_word16_transaction_issue_result {
    ti_c54x_bus_word16_transaction_status status;
    uint64_t transaction_id;
    ti_c54x_bus_word16_request request;
} ti_c54x_bus_word16_transaction_issue_result;

typedef struct ti_c54x_bus_word16_transaction_completion_result {
    ti_c54x_bus_word16_transaction_status status;
    uint16_t read_value_word;
    ti_c54x_bus_status bus_status;
} ti_c54x_bus_word16_transaction_completion_result;

/* Copy a valid request and fresh nonzero ID from IDLE into ISSUE_READY. */
ti_c54x_bus_word16_transaction_status
ti_c54x_bus_word16_transaction_prepare(
    ti_c54x_bus_word16_transaction *transaction,
    const ti_c54x_bus_word16_request *request,
    uint64_t transaction_id);

/* Authorize exactly one external issue from ISSUE_READY. */
ti_c54x_bus_word16_transaction_issue_result
ti_c54x_bus_word16_transaction_begin(
    ti_c54x_bus_word16_transaction *transaction);

/* Reduce one copied-ID observation; WAIT may be repeated without reissue. */
ti_c54x_bus_word16_transaction_status
ti_c54x_bus_word16_transaction_poll(
    ti_c54x_bus_word16_transaction *transaction,
    const ti_c54x_bus_word16_transaction_observation *observation);

/* Publish one staged completion, or WAIT while an issue remains in flight. */
ti_c54x_bus_word16_transaction_completion_result
ti_c54x_bus_word16_transaction_complete(
    ti_c54x_bus_word16_transaction *transaction,
    uint64_t transaction_id);

/* Clear only IDLE, COMPLETE, or FAULT; active work is never cancelled. */
ti_c54x_bus_word16_transaction_status
ti_c54x_bus_word16_transaction_reset(
    ti_c54x_bus_word16_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_BUS_TRANSACTION_WORD16_TRANSACTION_H */
