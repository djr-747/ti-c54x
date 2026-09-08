#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_RMW_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_RMW_H

/*
 * Module contract
 *
 * Owns: the private, allocation-free WORD16 read-transform-write helpers
 * used by the root memory transaction.
 * Does not own: CPU publication, address resolution, endpoint callbacks,
 * opcode dispatch, timing, observers, or diagnostics. Transaction storage is
 * borrowed for each call.
 */

#include <stdbool.h>
#include <stdint.h>

#include "isa/ops/memory/memory_effect.h"

struct ti_c54x_memory_instruction_transaction;

bool ti_c54x_memory_instruction_memory_is_rmw(
    const ti_c54x_op_memory_effect *memory);

bool ti_c54x_memory_instruction_rmw_transform(
    const ti_c54x_op_memory_effect *memory,
    uint16_t read_value_word,
    uint16_t *out_value_word);

bool ti_c54x_memory_instruction_rmw_next_id(
    struct ti_c54x_memory_instruction_transaction *transaction,
    uint64_t *out_id);

bool ti_c54x_memory_instruction_rmw_prepare_write(
    struct ti_c54x_memory_instruction_transaction *transaction,
    uint64_t transaction_id,
    uint16_t write_value_word);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_RMW_H */
