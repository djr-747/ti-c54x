/*
 * Module contract
 *
 * Owns: value-owned RMW transforms and paired-write request preparation.
 * Does not own: address resolution, CPU publication, opcode dispatch,
 * endpoint callbacks, timing, observers, diagnostics, or global state.
 */

#include "memory_instruction_rmw.h"

#include <stddef.h>
#include <stdint.h>

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
ti_c54x_memory_instruction_rmw_transform_add(
    const ti_c54x_op_memory_effect *memory,
    uint16_t read_value_word,
    bool overflow_mode,
    bool sign_extension_mode,
    uint16_t *out_value_word,
    bool *out_carry,
    bool *out_overflow)
{
    uint32_t full_sum;
    bool carry;
    bool overflow;

    if (memory == NULL || out_value_word == NULL || out_carry == NULL ||
        out_overflow == NULL ||
        memory->rmw_operation !=
            (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_ADD) {
        return false;
    }
    full_sum = (uint32_t)read_value_word + (uint32_t)memory->value_word;
    carry = (full_sum & UINT32_C(0x10000)) != UINT32_C(0);
    if (sign_extension_mode) {
        const bool read_negative =
            (read_value_word & UINT16_C(0x8000)) != UINT16_C(0);
        const bool addend_negative =
            (memory->value_word & UINT16_C(0x8000)) != UINT16_C(0);
        const bool sum_negative =
            ((uint16_t)full_sum & UINT16_C(0x8000)) != UINT16_C(0);

        overflow = (read_negative == addend_negative) &&
                   (sum_negative != read_negative);
    } else {
        overflow = carry;
    }
    if (overflow_mode && overflow) {
        if (!sign_extension_mode) {
            *out_value_word = UINT16_C(0xFFFF);
        } else if (((uint16_t)full_sum & UINT16_C(0x8000)) !=
                   UINT16_C(0)) {
            *out_value_word = UINT16_C(0x7FFF);
        } else {
            *out_value_word = UINT16_C(0x8000);
        }
    } else {
        *out_value_word = (uint16_t)full_sum;
    }
    *out_carry = carry;
    *out_overflow = overflow;
    return true;
}

bool
ti_c54x_memory_instruction_rmw_run_transform(
    struct ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_input_snapshot *input;
    bool carry = false;
    bool overflow = false;

    if (transaction == NULL) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    input = &transaction->candidate.input;
    if (memory->rmw_operation !=
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_ADD) {
        transaction->rmw_add_carry = false;
        transaction->rmw_add_overflow = false;
        return ti_c54x_memory_instruction_rmw_transform(
            memory, transaction->bus_transaction.staged_read_value_word,
            &transaction->rmw_write_value_word);
    }
    if (!input->status_ovm_known || !input->status_sxm_known) {
        return false;
    }
    if (!ti_c54x_memory_instruction_rmw_transform_add(
            memory, transaction->bus_transaction.staged_read_value_word,
            input->status_ovm != UINT8_C(0),
            input->status_sxm != UINT8_C(0),
            &transaction->rmw_write_value_word, &carry, &overflow)) {
        return false;
    }
    transaction->rmw_add_carry = carry;
    transaction->rmw_add_overflow = overflow;
    return true;
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
