/*
 * Module contract
 *
 * Owns: the shared INTM-clearing pop completion for the RETE
 * family. See the header contract for the ownership split.
 * Does not own: catalog lookup generation, TOS value derivation,
 * completion commit, program-counter or stack-pointer commit, CPU
 * sequencing, timing, buses, memory, diagnostics, or public ABI
 * declarations.
 */

#include "op_rete_common.h"

#include <stddef.h>
#include <stdint.h>

ti_c54x_op_status ti_c54x_rete_complete_intm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    (void)instruction;
    (void)input;
    (void)read_value_word;
    if (next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    *next_effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_STATUS_BIT_WRITE,
        .payload.status_bit_write = {
            .target_register = TI_C54X_OP_STATUS_REGISTER_ST1,
            .bit_index = TI_C54X_RETE_INTM_BIT_INDEX,
            .raw_value = UINT8_C(0)}};
    return TI_C54X_OP_STATUS_OK;
}
