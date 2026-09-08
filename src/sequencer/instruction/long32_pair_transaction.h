#ifndef TI_C54X_SEQUENCER_INSTRUCTION_LONG32_PAIR_TRANSACTION_H
#define TI_C54X_SEQUENCER_INSTRUCTION_LONG32_PAIR_TRANSACTION_H

/*
 * Module contract
 *
 * Owns: one private, allocation-free reducer for a copied LONG32 data
 * operand represented by two ordered WORD16 bus requests.
 * Does not own: endpoint callbacks, physical memory or visibility, bus
 * arbitration, timing, CPU state, opcode or effect publication, observers,
 * diagnostics, environment, globals, or public ABI. Request and observation
 * pointers are borrowed only for the duration of a call; all accepted values
 * are copied into caller-owned transaction storage.
 */

#include <stdbool.h>
#include <stdint.h>

#include "address/data_operand.h"
#include "bus/transaction/word16_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_long32_pair_status {
    TI_C54X_LONG32_PAIR_STATUS_OK = 0,
    TI_C54X_LONG32_PAIR_STATUS_WAIT = 1,
    TI_C54X_LONG32_PAIR_STATUS_FAULT = 2,
    TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT = 3,
    TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE = 4,
    TI_C54X_LONG32_PAIR_STATUS_STALE_TRANSACTION = 5
} ti_c54x_long32_pair_status;

typedef enum ti_c54x_long32_pair_state {
    TI_C54X_LONG32_PAIR_STATE_IDLE = 0,
    TI_C54X_LONG32_PAIR_STATE_FIRST = 1,
    TI_C54X_LONG32_PAIR_STATE_SECOND = 2,
    TI_C54X_LONG32_PAIR_STATE_COMPLETION_READY = 3,
    TI_C54X_LONG32_PAIR_STATE_COMPLETE = 4,
    TI_C54X_LONG32_PAIR_STATE_FAULT = 5
} ti_c54x_long32_pair_state;

typedef struct ti_c54x_long32_pair_request {
    ti_c54x_data_operand operand;
    ti_c54x_bus_word16_request_access access;
    uint16_t first_value_msw;
    uint16_t second_value_lsw;
    uint64_t first_transaction_id;
    uint64_t second_transaction_id;
} ti_c54x_long32_pair_request;

typedef struct ti_c54x_long32_pair_transaction {
    ti_c54x_long32_pair_state state;
    ti_c54x_long32_pair_request request;
    ti_c54x_bus_word16_transaction word16_transaction;
    uint16_t staged_first_msw;
    uint16_t staged_second_lsw;
    ti_c54x_bus_status staged_bus_status;
} ti_c54x_long32_pair_transaction;

typedef struct ti_c54x_long32_pair_issue_result {
    ti_c54x_long32_pair_status status;
    uint64_t transaction_id;
    ti_c54x_bus_word16_request request;
} ti_c54x_long32_pair_issue_result;

typedef struct ti_c54x_long32_pair_completion_result {
    ti_c54x_long32_pair_status status;
    uint16_t first_msw;
    uint16_t second_lsw;
    ti_c54x_bus_status bus_status;
} ti_c54x_long32_pair_completion_result;

/* The request is borrowed read-only until return and is never retained. */
bool ti_c54x_long32_pair_request_is_valid(
    const ti_c54x_long32_pair_request *request);

/* Copy a valid request and prepare its first/MSW WORD16 child. */
ti_c54x_long32_pair_status ti_c54x_long32_pair_transaction_prepare(
    ti_c54x_long32_pair_transaction *transaction,
    const ti_c54x_long32_pair_request *request);

/* Authorize exactly one external issue for the active word. */
ti_c54x_long32_pair_issue_result ti_c54x_long32_pair_transaction_begin(
    ti_c54x_long32_pair_transaction *transaction);

/* Reduce one copied-ID observation; WAIT never reissues or mutates state. */
ti_c54x_long32_pair_status ti_c54x_long32_pair_transaction_poll(
    ti_c54x_long32_pair_transaction *transaction,
    const ti_c54x_bus_word16_observation *observation);

/* Cancel only the matching in-flight word and clear the complete pair. */
ti_c54x_long32_pair_status ti_c54x_long32_pair_transaction_cancel(
    ti_c54x_long32_pair_transaction *transaction,
    uint64_t transaction_id);

/* Abandon only canonical staged, unpublished work; no endpoint is called. */
ti_c54x_long32_pair_status
ti_c54x_long32_pair_transaction_abandon_staged(
    ti_c54x_long32_pair_transaction *transaction);

/* Publish a staged pair, or return WAIT/FAULT without inventing values. */
ti_c54x_long32_pair_completion_result
ti_c54x_long32_pair_transaction_complete(
    ti_c54x_long32_pair_transaction *transaction);

/* Clear only IDLE, COMPLETE, or FAULT; active work is never reset. */
ti_c54x_long32_pair_status ti_c54x_long32_pair_transaction_reset(
    ti_c54x_long32_pair_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_LONG32_PAIR_TRANSACTION_H */
