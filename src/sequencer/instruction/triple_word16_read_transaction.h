#ifndef TI_C54X_SEQUENCER_INSTRUCTION_TRIPLE_WORD16_READ_TRANSACTION_H
#define TI_C54X_SEQUENCER_INSTRUCTION_TRIPLE_WORD16_READ_TRANSACTION_H

/*
 * Module contract
 *
 * Owns: one fixed-size, allocation-free reducer for two
 * already-constructed DATA WORD16 READ requests plus one
 * already-constructed PROGRAM WORD16 READ request (the FIRS
 * dual-data-plus-coefficient triple). It runs the shared dual
 * reducer for the X/Y DATA legs, then one single WORD16
 * transaction for the PROGRAM leg, and publishes all three
 * values only after every leg is ready. IDs must be fresh,
 * nonzero, and strictly increasing X, Y, program.
 * Does not own: endpoint callbacks, physical memory or visibility, bus
 * arbitration, timing, CPU state, opcode or effect publication, observers,
 * diagnostics, environment, globals, heap, or I/O. Request and observation
 * pointers are borrowed only for the duration of a call; accepted values are
 * copied into caller-owned transaction storage.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "bus/transaction/word16_transaction.h"
#include "sequencer/instruction/dual_word16_read_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_triple_word16_read_status {
    TI_C54X_TRIPLE_WORD16_READ_STATUS_OK = 0,
    TI_C54X_TRIPLE_WORD16_READ_STATUS_WAIT = 1,
    TI_C54X_TRIPLE_WORD16_READ_STATUS_FAULT = 2,
    TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_ARGUMENT = 3,
    TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE = 4,
    TI_C54X_TRIPLE_WORD16_READ_STATUS_STALE_TRANSACTION = 5
} ti_c54x_triple_word16_read_status;

typedef enum ti_c54x_triple_word16_read_state {
    TI_C54X_TRIPLE_WORD16_READ_STATE_IDLE = 0,
    TI_C54X_TRIPLE_WORD16_READ_STATE_DUAL = 1,
    TI_C54X_TRIPLE_WORD16_READ_STATE_PROGRAM = 2,
    TI_C54X_TRIPLE_WORD16_READ_STATE_COMPLETION_READY = 3,
    TI_C54X_TRIPLE_WORD16_READ_STATE_COMPLETE = 4,
    TI_C54X_TRIPLE_WORD16_READ_STATE_FAULT = 5
} ti_c54x_triple_word16_read_state;

typedef struct ti_c54x_triple_word16_read_request {
    ti_c54x_bus_word16_request x_request;
    ti_c54x_bus_word16_request y_request;
    ti_c54x_bus_word16_request program_request;
    uint64_t x_transaction_id;
    uint64_t y_transaction_id;
    uint64_t program_transaction_id;
} ti_c54x_triple_word16_read_request;

/*
 * Persistent reducer copy: the dual reducer owns the X/Y DATA
 * legs while one single WORD16 transaction owns the PROGRAM
 * leg; staged values publish only at triple completion.
 */
typedef struct ti_c54x_triple_word16_read_request_snapshot {
    struct {
        uint32_t address_word;
    } x_request;
    struct {
        uint32_t address_word;
    } y_request;
    struct {
        uint32_t address_word;
    } program_request;
    uint64_t x_transaction_id;
    uint64_t y_transaction_id;
    uint64_t program_transaction_id;
} ti_c54x_triple_word16_read_request_snapshot;

typedef struct ti_c54x_triple_word16_read_transaction {
    ti_c54x_triple_word16_read_state state;
    ti_c54x_triple_word16_read_request_snapshot request;
    ti_c54x_dual_word16_read_transaction dual;
    ti_c54x_bus_word16_transaction program;
    uint16_t staged_x_value_word;
    uint16_t staged_y_value_word;
    uint16_t staged_program_value_word;
    ti_c54x_bus_status staged_bus_status;
} ti_c54x_triple_word16_read_transaction;

typedef struct ti_c54x_triple_word16_read_issue_result {
    ti_c54x_triple_word16_read_status status;
    uint64_t transaction_id;
    ti_c54x_bus_word16_request request;
} ti_c54x_triple_word16_read_issue_result;

typedef struct ti_c54x_triple_word16_read_completion_result {
    ti_c54x_triple_word16_read_status status;
    uint16_t x_value_word;
    uint16_t y_value_word;
    uint16_t program_value_word;
    ti_c54x_bus_status bus_status;
} ti_c54x_triple_word16_read_completion_result;

/* The request is borrowed read-only until return and is never retained. */
bool ti_c54x_triple_word16_read_request_is_valid(
    const ti_c54x_triple_word16_read_request *request);

/* Exposed for contract checks; this performs read-only canonical validation. */
bool ti_c54x_triple_word16_read_transaction_is_canonical(
    const ti_c54x_triple_word16_read_transaction *transaction);

/* Copy three valid READ requests and prepare the X child. */
ti_c54x_triple_word16_read_status
ti_c54x_triple_word16_read_transaction_prepare(
    ti_c54x_triple_word16_read_transaction *transaction,
    const ti_c54x_triple_word16_read_request *request);

/* Authorize exactly one external issue for the active X, Y, or program child. */
ti_c54x_triple_word16_read_issue_result
ti_c54x_triple_word16_read_transaction_begin(
    ti_c54x_triple_word16_read_transaction *transaction);

/* Reduce one copied-ID observation; WAIT never reissues or mutates state. */
ti_c54x_triple_word16_read_status ti_c54x_triple_word16_read_transaction_poll(
    ti_c54x_triple_word16_read_transaction *transaction,
    const ti_c54x_bus_word16_observation *observation);

/* Cancel only matching in-flight work and clear the whole triple. */
ti_c54x_triple_word16_read_status ti_c54x_triple_word16_read_transaction_cancel(
    ti_c54x_triple_word16_read_transaction *transaction,
    uint64_t transaction_id);

/* Abandon staged pair/program progress without touching the frozen IDs. */
ti_c54x_triple_word16_read_status
ti_c54x_triple_word16_read_transaction_abandon_staged(
    ti_c54x_triple_word16_read_transaction *transaction);

/* Publish all three values, or return WAIT/FAULT without inventing values. */
ti_c54x_triple_word16_read_completion_result
ti_c54x_triple_word16_read_transaction_complete(
    ti_c54x_triple_word16_read_transaction *transaction);

/* Clear only IDLE, COMPLETE, or FAULT; active work is never reset. */
ti_c54x_triple_word16_read_status ti_c54x_triple_word16_read_transaction_reset(
    ti_c54x_triple_word16_read_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_TRIPLE_WORD16_READ_TRANSACTION_H */
