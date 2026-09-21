/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for MPY Xmem,Ymem
 * (TI SPRU172C syntax 2, opcode 1010 010D XXXX YYYY) across X/Y
 * MOD0/MOD1/MOD2 addressing. The leaves stage one dual WORD16 read
 * effect with read-phase T load of the X-side word and, on pair
 * completion, one accumulator write of (Xmem)x(Ymem) through the
 * multiply core with FRCT/OVM handling and STORE overwrite.
 * The destination accumulator is overwritten, so accumulator
 * knownness is not required; only FRCT, OVM, and both ARs gate
 * staging.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution or mutation, buses, CPU/state mutation,
 * sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained;
 * failed calls leave the caller-owned effect unchanged.
 */

#include "op_mpy_dual.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_multiply.h"

typedef struct mpy_dual_fields {
    uint8_t destination;
    uint8_t x_ar;
    uint8_t y_ar;
} mpy_dual_fields;

static const uint8_t mpy_dual_widths[3] = {UINT8_C(1), UINT8_C(2),
                                           UINT8_C(2)};

static ti_c54x_op_status mpy_dual_check_fields(
    const ti_c54x_operand_field_value *fields, mpy_dual_fields *output)
{
    if (fields == NULL || output == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (fields[0].width_bits != mpy_dual_widths[0] ||
        fields[1].width_bits != mpy_dual_widths[1] ||
        fields[2].width_bits != mpy_dual_widths[2] ||
        fields[0].raw_value > UINT64_C(1) ||
        fields[1].raw_value > UINT64_C(3) ||
        fields[2].raw_value > UINT64_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    output->destination = (uint8_t)fields[0].raw_value;
    output->x_ar = (uint8_t)(fields[1].raw_value + UINT64_C(2));
    output->y_ar = (uint8_t)(fields[2].raw_value + UINT64_C(2));
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mpy_dual_extract(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    mpy_dual_fields *fields)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value unpacked[3] = {{0U, 0U},
                                              {0U, 0U},
                                              {0U, 0U}};
    ti_c54x_program_words_view words;
    uint8_t index;

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_FRCT_OVM ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_DUAL_DESTINATION_XAR_YAR ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value ||
        metadata->first_word_value != expected_value ||
        metadata->first_word_mask != UINT16_C(0xFECC)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    for (index = UINT8_C(0); index < UINT8_C(3); ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index,
                &unpacked[index]) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    return mpy_dual_check_fields(unpacked, fields);
}

static ti_c54x_op_status mpy_dual_extract_trusted(
    const ti_c54x_trusted_operand_values *trusted, mpy_dual_fields *fields)
{
    if (trusted == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (trusted->count != UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (trusted->values[0].width_bits != mpy_dual_widths[0] ||
        trusted->values[1].width_bits != mpy_dual_widths[1] ||
        trusted->values[2].width_bits != mpy_dual_widths[2] ||
        trusted->values[0].raw_value > UINT64_C(1) ||
        trusted->values[1].raw_value > UINT64_C(3) ||
        trusted->values[2].raw_value > UINT64_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields->destination = (uint8_t)trusted->values[0].raw_value;
    fields->x_ar = (uint8_t)(trusted->values[1].raw_value + UINT64_C(2));
    fields->y_ar = (uint8_t)(trusted->values[2].raw_value + UINT64_C(2));
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mpy_dual_validate_inputs(
    const mpy_dual_fields *fields, const ti_c54x_op_input_snapshot *input)
{
    const uint8_t x_bit = (uint8_t)(UINT8_C(1) << fields->x_ar);
    const uint8_t y_bit = (uint8_t)(UINT8_C(1) << fields->y_ar);

    if (!input->status_frct_known || !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_frct > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if ((input->auxiliary_registers_known_mask & x_bit) == UINT8_C(0) ||
        (input->auxiliary_registers_known_mask & y_bit) == UINT8_C(0)) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mpy_dual_stage(
    const mpy_dual_fields *fields,
    ti_c54x_op_memory_complete_dual_fn completion, uint8_t x_mode,
    uint8_t y_mode, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    if (fields == NULL || completion == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16,
                .dual = {.x_mode = x_mode,
                          .x_ar = fields->x_ar,
                          .y_mode = y_mode,
                          .y_ar = fields->y_ar,
                          .reserved_word0 = UINT32_C(0),
                          .reserved_word1 = UINT32_C(0),
                          .reserved_word2 = UINT32_C(0),
                          .reserved_word3 = UINT32_C(0)}},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = UINT16_C(0),
            .rmw_operation =
                (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
            .transfer_reserved = UINT8_C(0),
            /* Read-phase T load: T takes the X-side word once the
             * pair completion has committed. */
            .load_t_with_read = true,
            .complete_dual = completion}};

    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mpy_dual_complete_for_id(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word,
    uint16_t y_word, ti_c54x_opcode_id expected_id,
    uint16_t expected_value, ti_c54x_op_effect *next_effect)
{
    mpy_dual_fields fields;
    ti_c54x_op_status status;
    ti_c54x_multiply_operands operands;
    ti_c54x_multiply_result result;
    ti_c54x_multiply_status multiply_status;
    ti_c54x_op_effect staged = {0};

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = mpy_dual_extract(instruction, expected_id, expected_value,
                              &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = mpy_dual_validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* The X-side word feeds the multiplier T input; the read-phase
     * T load commits the same word to the T register. */
    operands.t_raw16 = x_word;
    operands.t_known = true;
    operands.multiplicand_raw16 = y_word;
    operands.multiplicand_known = true;
    operands.accumulator_raw40 = UINT64_C(0);
    operands.accumulator_known = false;
    operands.frct = input->status_frct;
    operands.frct_known = true;
    operands.ovm = input->status_ovm;
    operands.ovm_known = true;
    operands.unsigned_operands = false;
    operands.unsigned_t_only = false;
    operands.rounding = false;
    operands.select = TI_C54X_MULTIPLY_STORE;
    multiply_status = ti_c54x_multiply_accumulate(&operands, &result);
    if (multiply_status == TI_C54X_MULTIPLY_UNKNOWN) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (multiply_status != TI_C54X_MULTIPLY_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields.destination;
    staged.payload.accumulator_write.value_raw40 = result.accumulator_raw40;
    staged.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.destination_overflow_action =
        result.overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                        : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mpy_dual_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    ti_c54x_op_memory_complete_dual_fn completion, uint8_t x_mode,
    uint8_t y_mode)
{
    mpy_dual_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = mpy_dual_extract(instruction, expected_id, expected_value,
                              &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = mpy_dual_validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return mpy_dual_stage(&fields, completion, x_mode, y_mode, effect);
}

static ti_c54x_op_status mpy_dual_trusted(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_op_memory_complete_dual_fn completion, uint8_t x_mode,
    uint8_t y_mode)
{
    mpy_dual_fields fields;
    ti_c54x_op_status status;

    (void)instruction;
    if (trusted == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = mpy_dual_extract_trusted(trusted, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = mpy_dual_validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return mpy_dual_stage(&fields, completion, x_mode, y_mode, effect);
}

#define DEFINE_MPY_DUAL_COMPLETE(suffix, id, value)                           \
    static ti_c54x_op_status complete_##suffix(                                 \
        const ti_c54x_decoded_instruction *instruction,                        \
        const ti_c54x_op_input_snapshot *input, uint16_t x_word,               \
        uint16_t y_word, ti_c54x_op_effect *next_effect)                       \
    {                                                                          \
        return mpy_dual_complete_for_id(instruction, input, x_word,           \
                                        y_word, id, value, next_effect);      \
    }

DEFINE_MPY_DUAL_COMPLETE(mpy_dual_m0_m0, TI_C54X_OPCODE_ID_MPY_DUAL_M0_M0,
                         UINT16_C(0xA400))
DEFINE_MPY_DUAL_COMPLETE(mpy_dual_m0_m1, TI_C54X_OPCODE_ID_MPY_DUAL_M0_M1,
                         UINT16_C(0xA404))
DEFINE_MPY_DUAL_COMPLETE(mpy_dual_m0_m2, TI_C54X_OPCODE_ID_MPY_DUAL_M0_M2,
                         UINT16_C(0xA408))
DEFINE_MPY_DUAL_COMPLETE(mpy_dual_m1_m0, TI_C54X_OPCODE_ID_MPY_DUAL_M1_M0,
                         UINT16_C(0xA440))
DEFINE_MPY_DUAL_COMPLETE(mpy_dual_m1_m1, TI_C54X_OPCODE_ID_MPY_DUAL_M1_M1,
                         UINT16_C(0xA444))
DEFINE_MPY_DUAL_COMPLETE(mpy_dual_m1_m2, TI_C54X_OPCODE_ID_MPY_DUAL_M1_M2,
                         UINT16_C(0xA448))
DEFINE_MPY_DUAL_COMPLETE(mpy_dual_m2_m0, TI_C54X_OPCODE_ID_MPY_DUAL_M2_M0,
                         UINT16_C(0xA480))
DEFINE_MPY_DUAL_COMPLETE(mpy_dual_m2_m1, TI_C54X_OPCODE_ID_MPY_DUAL_M2_M1,
                         UINT16_C(0xA484))
DEFINE_MPY_DUAL_COMPLETE(mpy_dual_m2_m2, TI_C54X_OPCODE_ID_MPY_DUAL_M2_M2,
                         UINT16_C(0xA488))

#undef DEFINE_MPY_DUAL_COMPLETE

#define DEFINE_MPY_DUAL_WRAPPERS(suffix, id, value, x_mode, y_mode)           \
    ti_c54x_op_status ti_c54x_op_##suffix(                                     \
        const ti_c54x_decoded_instruction *instruction,                       \
        const ti_c54x_op_input_snapshot *input,                                \
        ti_c54x_op_effect *effect)                                            \
    {                                                                          \
        return mpy_dual_checked(instruction, input, effect, id, value,        \
                                complete_##suffix, x_mode, y_mode);            \
    }                                                                          \
    ti_c54x_op_status ti_c54x_trusted_op_##suffix(                             \
        const ti_c54x_decoded_instruction *instruction,                       \
        const ti_c54x_trusted_operand_values *operands,                       \
        const ti_c54x_op_input_snapshot *input,                                \
        ti_c54x_op_effect *effect)                                            \
    {                                                                          \
        return mpy_dual_trusted(instruction, operands, input, effect,         \
                                complete_##suffix, x_mode, y_mode);            \
    }

DEFINE_MPY_DUAL_WRAPPERS(mpy_dual_m0_m0, TI_C54X_OPCODE_ID_MPY_DUAL_M0_M0,
                         UINT16_C(0xA400), UINT8_C(0), UINT8_C(0))
DEFINE_MPY_DUAL_WRAPPERS(mpy_dual_m0_m1, TI_C54X_OPCODE_ID_MPY_DUAL_M0_M1,
                         UINT16_C(0xA404), UINT8_C(0), UINT8_C(1))
DEFINE_MPY_DUAL_WRAPPERS(mpy_dual_m0_m2, TI_C54X_OPCODE_ID_MPY_DUAL_M0_M2,
                         UINT16_C(0xA408), UINT8_C(0), UINT8_C(2))
DEFINE_MPY_DUAL_WRAPPERS(mpy_dual_m1_m0, TI_C54X_OPCODE_ID_MPY_DUAL_M1_M0,
                         UINT16_C(0xA440), UINT8_C(1), UINT8_C(0))
DEFINE_MPY_DUAL_WRAPPERS(mpy_dual_m1_m1, TI_C54X_OPCODE_ID_MPY_DUAL_M1_M1,
                         UINT16_C(0xA444), UINT8_C(1), UINT8_C(1))
DEFINE_MPY_DUAL_WRAPPERS(mpy_dual_m1_m2, TI_C54X_OPCODE_ID_MPY_DUAL_M1_M2,
                         UINT16_C(0xA448), UINT8_C(1), UINT8_C(2))
DEFINE_MPY_DUAL_WRAPPERS(mpy_dual_m2_m0, TI_C54X_OPCODE_ID_MPY_DUAL_M2_M0,
                         UINT16_C(0xA480), UINT8_C(2), UINT8_C(0))
DEFINE_MPY_DUAL_WRAPPERS(mpy_dual_m2_m1, TI_C54X_OPCODE_ID_MPY_DUAL_M2_M1,
                         UINT16_C(0xA484), UINT8_C(2), UINT8_C(1))
DEFINE_MPY_DUAL_WRAPPERS(mpy_dual_m2_m2, TI_C54X_OPCODE_ID_MPY_DUAL_M2_M2,
                         UINT16_C(0xA488), UINT8_C(2), UINT8_C(2))

#undef DEFINE_MPY_DUAL_WRAPPERS
