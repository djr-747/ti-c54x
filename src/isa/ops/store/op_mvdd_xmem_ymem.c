/*
 * Module contract
 *
 * Owns: checked semantic execution of the MVDD Xmem,Ymem
 * descriptor (TI SPRU172C section MVDD, p. 4-107, 1 word).
 * The leaf gates the 0xE500 first word under mask 0xFF00 with
 * the DUAL_XMOD_XAR_YMOD_YAR layout, ELIGIBLE repeatability,
 * and AUXILIARY_REGISTERS inputs, and stages one
 * DUAL_INDIRECT_MOVE_PAIR MEMORY effect with WRITE access
 * and null completion; the X READ, Y WRITE, and both AR
 * commits belong to the move root. MVDD is absent from Table
 * 2-28 (repeatable).
 * Does not own: opcode inventory, decode, program-counter
 * sequencing, CPU or status state, memory, buses, timing,
 * diagnostics, or observer callbacks. The descriptor is
 * borrowed for the call and the caller-owned effect is
 * unchanged on every failure.
 */

#include "op_mvdd_xmem_ymem.h"

#include <stddef.h>
#include <stdint.h>

#include "../../catalog/opcode_catalog.h"
#include "../../decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/memory/memory_effect.h"

#define MVDD_FIRST_WORD_VALUE UINT16_C(0xE500)
#define MVDD_FIRST_WORD_MASK UINT16_C(0xFF00)

typedef struct ti_c54x_mvdd_operands {
    uint8_t x_mode;
    uint8_t x_ar;
    uint8_t y_mode;
    uint8_t y_ar;
} ti_c54x_mvdd_operands;

static ti_c54x_op_status
validate_metadata(const ti_c54x_decoded_instruction *instruction,
                  ti_c54x_mvdd_operands *output)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value fields[4] = {{0U, 0U},
                                             {0U, 0U},
                                             {0U, 0U},
                                             {0U, 0U}};
    const ti_c54x_program_words_view words = {
        instruction == NULL ? NULL : instruction->bundle.program_words,
        instruction == NULL ? 0U : instruction->bundle.program_word_count};
    uint8_t index;

    if (instruction == NULL || output == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_MVDD_XMEM_YMEM);
    if (metadata == NULL ||
        metadata->id != TI_C54X_OPCODE_ID_MVDD_XMEM_YMEM ||
        metadata->first_word_value != MVDD_FIRST_WORD_VALUE ||
        metadata->first_word_mask != MVDD_FIRST_WORD_MASK ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_YMOD_YAR ||
        metadata->repeatability != TI_C54X_REPEATABILITY_ELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS ||
        instruction->id != TI_C54X_OPCODE_ID_MVDD_XMEM_YMEM ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) !=
            metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    for (index = UINT8_C(0); index < UINT8_C(4); ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &fields[index]) !=
            TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    if (fields[0].width_bits != UINT8_C(2) ||
        fields[1].width_bits != UINT8_C(2) ||
        fields[2].width_bits != UINT8_C(2) ||
        fields[3].width_bits != UINT8_C(2) ||
        fields[0].raw_value > UINT64_C(3) ||
        fields[1].raw_value > UINT64_C(3) ||
        fields[2].raw_value > UINT64_C(3) ||
        fields[3].raw_value > UINT64_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    output->x_mode = (uint8_t)fields[0].raw_value;
    output->x_ar = (uint8_t)(fields[1].raw_value + UINT64_C(2));
    output->y_mode = (uint8_t)fields[2].raw_value;
    output->y_ar = (uint8_t)(fields[3].raw_value + UINT64_C(2));
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status
validate_trusted(const ti_c54x_trusted_operand_values *trusted,
                 ti_c54x_mvdd_operands *output)
{
    if (trusted == NULL || output == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (trusted->count != UINT8_C(4) ||
        trusted->values[0].width_bits != UINT8_C(2) ||
        trusted->values[1].width_bits != UINT8_C(2) ||
        trusted->values[2].width_bits != UINT8_C(2) ||
        trusted->values[3].width_bits != UINT8_C(2) ||
        trusted->values[0].raw_value > UINT64_C(3) ||
        trusted->values[1].raw_value > UINT64_C(3) ||
        trusted->values[2].raw_value > UINT64_C(3) ||
        trusted->values[3].raw_value > UINT64_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    output->x_mode = (uint8_t)trusted->values[0].raw_value;
    output->x_ar = (uint8_t)(trusted->values[1].raw_value + UINT64_C(2));
    output->y_mode = (uint8_t)trusted->values[2].raw_value;
    output->y_ar = (uint8_t)(trusted->values[3].raw_value + UINT64_C(2));
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status
validate_inputs(const ti_c54x_mvdd_operands *operands,
                const ti_c54x_op_input_snapshot *input)
{
    const uint8_t x_bit = (uint8_t)(UINT8_C(1) << operands->x_ar);
    const uint8_t y_bit = (uint8_t)(UINT8_C(1) << operands->y_ar);

    if (operands == NULL || input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if ((input->auxiliary_registers_known_mask & x_bit) == UINT8_C(0) ||
        (input->auxiliary_registers_known_mask & y_bit) == UINT8_C(0)) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status
stage_move(const ti_c54x_mvdd_operands *operands,
           ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    if (effect == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind =
                    TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_MOVE_PAIR_WORD16,
                .dual = {.x_mode = operands->x_mode,
                         .x_ar = operands->x_ar,
                         .y_mode = operands->y_mode,
                         .y_ar = operands->y_ar,
                         .reserved_word0 = UINT32_C(0),
                         .reserved_word1 = UINT32_C(0),
                         .reserved_word2 = UINT32_C(0),
                         .reserved_word3 = UINT32_C(0)}},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .value_word = UINT16_C(0),
            .rmw_operation =
                (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
            .transfer_reserved = UINT8_C(0),
            .complete = NULL}};

    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_mvdd_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_mvdd_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = validate_metadata(instruction, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = validate_inputs(&operands, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return stage_move(&operands, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_mvdd_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_mvdd_operands fields;
    ti_c54x_op_status status;

    if (instruction == NULL || operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = validate_trusted(operands, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return stage_move(&fields, effect);
}
