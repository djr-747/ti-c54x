/*
 * Module contract
 *
 * Owns: value-owned RMW transforms and paired-write request preparation.
 * Does not own: address resolution, CPU publication, opcode dispatch,
 * endpoint callbacks, timing, observers, diagnostics, or global state.
 */

#include "memory_instruction_rmw.h"

#include <stddef.h>

#include "bus/request/word16_request.h"
#include "bus/transaction/word16_transaction.h"
#include "memory_instruction_transaction.h"

bool
ti_c54x_memory_instruction_memory_is_rmw(
    const ti_c54x_op_memory_effect *memory)
{
    return memory != NULL &&
           memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           memory->rmw_operation !=
               (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
           memory->complete == NULL;
}

bool
ti_c54x_memory_instruction_rmw_transform(
    const ti_c54x_op_memory_effect *memory,
    uint16_t read_value_word,
    uint16_t *out_value_word)
{
    if (memory == NULL || out_value_word == NULL) {
        return false;
    }
    switch ((ti_c54x_op_memory_rmw_operation)memory->rmw_operation) {
    case TI_C54X_OP_MEMORY_RMW_OPERATION_AND:
        *out_value_word = (uint16_t)(read_value_word & memory->value_word);
        return true;
    case TI_C54X_OP_MEMORY_RMW_OPERATION_OR:
        *out_value_word = (uint16_t)(read_value_word | memory->value_word);
        return true;
    case TI_C54X_OP_MEMORY_RMW_OPERATION_XOR:
        *out_value_word = (uint16_t)(read_value_word ^ memory->value_word);
        return true;
    case TI_C54X_OP_MEMORY_RMW_OPERATION_NONE:
    default:
        return false;
    }
}

bool
ti_c54x_memory_instruction_rmw_next_id(
    struct ti_c54x_memory_instruction_transaction *transaction,
    uint64_t *out_id)
{
    uint64_t next_id;

    if (transaction == NULL || out_id == NULL ||
        transaction->last_transaction_id == UINT64_MAX) {
        return false;
    }
    next_id = transaction->last_transaction_id + UINT64_C(1);
    if (next_id == UINT64_C(0)) {
        return false;
    }
    transaction->last_transaction_id = next_id;
    *out_id = next_id;
    return true;
}

bool
ti_c54x_memory_instruction_rmw_prepare_write(
    struct ti_c54x_memory_instruction_transaction *transaction,
    uint64_t transaction_id,
    uint16_t write_value_word)
{
    ti_c54x_bus_word16_request_result request;

    if (transaction == NULL || transaction_id == UINT64_C(0)) {
        return false;
    }
    request = ti_c54x_bus_word16_request_write(
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        transaction->candidate.address_resolution.request.address_word,
        write_value_word);
    if (request.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK ||
        ti_c54x_bus_word16_transaction_reset(&transaction->bus_transaction) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK ||
        ti_c54x_bus_word16_transaction_prepare(
            &transaction->bus_transaction, &request.request, transaction_id) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return false;
    }
    return true;
}
