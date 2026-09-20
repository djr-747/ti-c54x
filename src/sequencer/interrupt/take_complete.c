/*
 * Module contract
 *
 * Owns: the INTM-setting push completion described in
 * take_complete.h.
 * Does not own: catalog rows, operand extraction, effect commit,
 * stack writes, program-counter or stack-pointer commit, CPU
 * sequencing, timing, buses, memory, diagnostics, or public ABI
 * declarations.
 */

#include "take_complete.h"

#include <stddef.h>
#include <stdint.h>

ti_c54x_op_status ti_c54x_take_complete_intm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t staged_value_word,
    ti_c54x_op_effect *next_effect)
{
    (void)instruction;
    (void)input;
    (void)staged_value_word;
    if (next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    *next_effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_STATUS_BIT_WRITE,
        .payload.status_bit_write = {
            .target_register = TI_C54X_OP_STATUS_REGISTER_ST1,
            .bit_index = TI_C54X_TAKE_INTM_BIT_INDEX,
            .raw_value = UINT8_C(1)}};
    return TI_C54X_OP_STATUS_OK;
}
