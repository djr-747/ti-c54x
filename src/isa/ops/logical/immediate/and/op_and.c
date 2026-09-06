/*
 * Module contract
 *
 * Owns: the AND immediate opcode-family bindings. The generic helper delegates
 * exactly once to the common logical-immediate builder with the AND operation
 * kind; catalog-bound aliases validate their exact row identity before that
 * delegation and never mutate an effect on rejection.
 * Does not own: shared immediate layouts, catalog metadata, logical
 * arithmetic, accumulator or status storage, effect commit, sequencing,
 * timing, memory, repeat/IRQ policy, diagnostics, or build registration.
 */

#include "op_and.h"

#include <stddef.h>

#include "isa/catalog/opcode_catalog.h"
#include "../common/op_logical_immediate.h"

ti_c54x_op_status ti_c54x_op_and_immediate(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return ti_c54x_op_logical_immediate(
        TI_C54X_LOGICAL_IMMEDIATE_AND, instruction, input, effect);
}

ti_c54x_op_status ti_c54x_op_and_immediate_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    /* Preserve the common helper's NULL precedence before inspecting id. */
    if (instruction == NULL || input == NULL || effect == NULL) {
        return ti_c54x_op_and_immediate(instruction, input, effect);
    }
    if (instruction->id != TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return ti_c54x_op_logical_immediate(
        TI_C54X_LOGICAL_IMMEDIATE_AND, instruction, input, effect);
}

ti_c54x_op_status ti_c54x_op_and_immediate_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    /* Preserve the common helper's NULL precedence before inspecting id. */
    if (instruction == NULL || input == NULL || effect == NULL) {
        return ti_c54x_op_and_immediate(instruction, input, effect);
    }
    if (instruction->id != TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT16) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return ti_c54x_op_logical_immediate(
        TI_C54X_LOGICAL_IMMEDIATE_AND, instruction, input, effect);
}
