/* Module contract
 * Owns: pure OR two-word immediate accumulator family bindings over the shared
 * logical-immediate transaction. Catalog-bound aliases validate their exact
 * row identity before delegation and never mutate an effect on rejection.
 * Does not own: shared operand validation or computation, catalog inventory,
 * generated lookup, decode tables, accumulator storage, status mutation,
 * sequencing, CPU lifecycle, timing, memory, repeat/interrupt policy,
 * diagnostics, or public registration.
 */

#include "op_or.h"

#include <stddef.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../common/op_logical_immediate.h"

ti_c54x_op_status ti_c54x_op_or_immediate(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return ti_c54x_op_logical_immediate(
        TI_C54X_LOGICAL_IMMEDIATE_OR, instruction, input, effect);
}

ti_c54x_op_status ti_c54x_op_or_immediate_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    /* Preserve the common helper's NULL precedence before inspecting id. */
    if (instruction == NULL || input == NULL || effect == NULL) {
        return ti_c54x_op_or_immediate(instruction, input, effect);
    }
    if (instruction->id != TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return ti_c54x_op_logical_immediate(
        TI_C54X_LOGICAL_IMMEDIATE_OR, instruction, input, effect);
}

ti_c54x_op_status ti_c54x_op_or_immediate_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    /* Preserve the common helper's NULL precedence before inspecting id. */
    if (instruction == NULL || input == NULL || effect == NULL) {
        return ti_c54x_op_or_immediate(instruction, input, effect);
    }
    if (instruction->id != TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT16) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return ti_c54x_op_logical_immediate(
        TI_C54X_LOGICAL_IMMEDIATE_OR, instruction, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_or_immediate_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;
    return ti_c54x_op_logical_immediate_values(
        TI_C54X_LOGICAL_IMMEDIATE_OR,
        (uint8_t)operands->values[0].raw_value,
        (uint8_t)operands->values[1].raw_value,
        (uint8_t)operands->values[2].raw_value,
        (uint16_t)operands->values[3].raw_value, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_or_immediate_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;
    return ti_c54x_op_logical_immediate_values(
        TI_C54X_LOGICAL_IMMEDIATE_OR,
        (uint8_t)operands->values[0].raw_value,
        (uint8_t)operands->values[1].raw_value, UINT8_C(16),
        (uint16_t)operands->values[2].raw_value, input, effect);
}
