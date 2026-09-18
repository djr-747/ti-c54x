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

/*
 * Value-owned 16-bit RMW-ADD reducer. Serves only carriers whose
 * rmw_operation is ADD; the addend is the carrier value_word and the
 * OVM/SXM modes arrive as frozen values from the candidate input
 * snapshot. Reports the wrapped or saturated write word plus the raw
 * bit-16 carry and the SXM-selected overflow the commit path turns
 * into sticky C/OVA writes.
 */
bool ti_c54x_memory_instruction_rmw_transform_add(
    const ti_c54x_op_memory_effect *memory,
    uint16_t read_value_word,
    bool overflow_mode,
    bool sign_extension_mode,
    uint16_t *out_value_word,
    bool *out_carry,
    bool *out_overflow);

/*
 * Runs the frozen candidate's RMW transform over the staged READ value.
 * The ADD reducer additionally freezes the OVM/SXM modes from the
 * candidate input snapshot and stages the resulting carry/overflow
 * beside the write word for the commit path. Transaction storage is
 * borrowed for one call.
 */
bool ti_c54x_memory_instruction_rmw_run_transform(
    struct ti_c54x_memory_instruction_transaction *transaction);

bool ti_c54x_memory_instruction_rmw_next_id(
    struct ti_c54x_memory_instruction_transaction *transaction,
    uint64_t *out_id);

bool ti_c54x_memory_instruction_rmw_prepare_write(
    struct ti_c54x_memory_instruction_transaction *transaction,
    uint64_t transaction_id,
    uint16_t write_value_word);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_RMW_H */
