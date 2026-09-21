/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the parallel
 * ST src,Ymem || ADD/SUB Xmem,dst dual rows (TI SPRU172C
 * §ST||ADD p. 4-177, opcode 1100_00SD_XXXX_YYYY; §ST||SUB
 * p. 4-185, opcode 1100_01SD_XXXX_YYYY). The leaves stage one
 * dual-indirect WORD16 WRITE effect over the MOVE_PAIR arm with
 * the leaf-frozen ST value and the single-read completion set;
 * the store-compute root issues the X READ, then the Y WRITE
 * with the frozen value, then the completion. The ST half
 * stores the STH-by-ASM value of src (S11 leaf shape: the
 * 40-bit accumulator shifted by the signed ASM count with bits
 * 31-16 extracted, proven identical on all seven parallel-ST
 * worked examples). The ADD half accumulates dst_ plus the
 * SXM-widened X word shifted left 16 into dst; the SUB half
 * subtracts dst_ from it. Carry/borrow drives C and 32-bit
 * signed overflow drives OVdst with OVM saturation (mirrors
 * the dual-arithmetic core); TC is preserved. S selects the
 * store source (0 = A, 1 = B) and D the compute destination,
 * with the sibling accumulator as the addend/subtrahend; src
 * is preserved (the §ST||ADD example's changed A is recorded
 * as a one-nibble TI typo against the SUB/MAC/MAS/MPY
 * examples). T and FRCT are never consumed.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution or mutation, buses, CPU/state mutation,
 * sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained;
 * failed calls leave the caller-owned effect unchanged.
 */

#include "op_st_parallel.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "../../catalog/opcode_catalog.h"
#include "../../decode/operand_layout.h"
#include "../../../foundation/arithmetic40.h"
#include "../../../foundation/arithmetic40_addsub.h"
#include "../dispatch/trusted_semantics.h"

typedef enum st_parallel_select {
    ST_PARALLEL_SELECT_ADD = 0,
    ST_PARALLEL_SELECT_SUBTRACT = 1
} st_parallel_select;

typedef struct st_parallel_fields {
    uint8_t src;
    uint8_t destination;
    uint8_t x_mode;
    uint8_t x_ar;
    uint8_t y_mode;
    uint8_t y_ar;
} st_parallel_fields;

static ti_c54x_op_status st_parallel_check_operands(
    uint64_t src, uint64_t destination, uint64_t dualxy,
    st_parallel_fields *output)
{
    if (output == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (src > UINT64_C(1) || destination > UINT64_C(1) ||
        dualxy > UINT64_C(0xFF)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    output->src = (uint8_t)src;
    output->destination = (uint8_t)destination;
    output->x_mode = (uint8_t)((dualxy >> 6U) & UINT64_C(3));
    output->x_ar =
        (uint8_t)(((dualxy >> 4U) & UINT64_C(3)) + UINT64_C(2));
    output->y_mode = (uint8_t)((dualxy >> 2U) & UINT64_C(3));
    output->y_ar = (uint8_t)((dualxy & UINT64_C(3)) + UINT64_C(2));
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_parallel_extract(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    st_parallel_fields *fields)
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
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_STATUS_SXM_OVM ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_SRC_DEST_DUALXY ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->repeatability != TI_C54X_REPEATABILITY_ELIGIBLE ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value ||
        metadata->first_word_value != expected_value ||
        metadata->first_word_mask != UINT16_C(0xFC00)) {
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
    if (unpacked[0].width_bits != UINT8_C(1) ||
        unpacked[1].width_bits != UINT8_C(1) ||
        unpacked[2].width_bits != UINT8_C(8)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return st_parallel_check_operands(unpacked[0].raw_value,
                                      unpacked[1].raw_value,
                                      unpacked[2].raw_value, fields);
}

static ti_c54x_op_status st_parallel_extract_trusted(
    const ti_c54x_trusted_operand_values *trusted,
    st_parallel_fields *fields)
{
    if (trusted == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (trusted->count != UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (trusted->values[0].width_bits != UINT8_C(1) ||
        trusted->values[1].width_bits != UINT8_C(1) ||
        trusted->values[2].width_bits != UINT8_C(8)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (st_parallel_check_operands(trusted->values[0].raw_value,
                                   trusted->values[1].raw_value,
                                   trusted->values[2].raw_value,
                                   fields) != TI_C54X_OP_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_parallel_validate_inputs(
    const st_parallel_fields *fields, const ti_c54x_op_input_snapshot *input)
{
    const uint8_t x_bit = (uint8_t)(UINT8_C(1) << fields->x_ar);
    const uint8_t y_bit = (uint8_t)(UINT8_C(1) << fields->y_ar);

    if (!input->accumulator_a_known || !input->accumulator_b_known ||
        !input->status_asm_known || !input->status_sxm_known ||
        !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->accumulator_a_raw40 > TI_C54X_U40_MASK ||
        input->accumulator_b_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (input->status_asm > UINT8_C(31) ||
        input->status_sxm > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if ((input->auxiliary_registers_known_mask & x_bit) == UINT8_C(0) ||
        (input->auxiliary_registers_known_mask & y_bit) == UINT8_C(0)) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    return TI_C54X_OP_STATUS_OK;
}

/* STH-by-ASM value (S11 leaf shape): shift the 40-bit
 * accumulator by the signed ASM count (0-15 left, 16-31 right
 * by 32-ASM) and extract bits 31-16. Proven identical on all
 * seven parallel-ST worked examples. */
static uint16_t st_parallel_store_value(uint64_t source_raw40,
                                        uint8_t asm_value)
{
    ti_c54x_u40_t shifted;

    if (asm_value <= UINT8_C(15)) {
        shifted = ti_c54x_u40_logical_left_shift(source_raw40, asm_value);
    } else {
        shifted = ti_c54x_u40_arithmetic_right_shift(
            source_raw40, (uint64_t)(UINT8_C(32) - asm_value));
    }
    return (uint16_t)((shifted >> 16U) & UINT64_C(0xFFFF));
}

static ti_c54x_u40_t st_parallel_extend_word(uint16_t word, uint8_t sxm)
{
    ti_c54x_u40_t value = ti_c54x_u40_from_unsigned(word);

    if (sxm != UINT8_C(0) && (word & UINT16_C(0x8000)) != UINT16_C(0)) {
        value = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) | value;
    }
    return value;
}

static ti_c54x_op_status st_parallel_stage(
    const st_parallel_fields *fields,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_memory_complete_fn completion, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;
    uint64_t src_raw40;

    if (fields == NULL || input == NULL || completion == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    src_raw40 = fields->src == UINT8_C(0) ? input->accumulator_a_raw40
                                          : input->accumulator_b_raw40;
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_MOVE_PAIR_WORD16,
                .dual = {.x_mode = fields->x_mode,
                         .x_ar = fields->x_ar,
                         .y_mode = fields->y_mode,
                         .y_ar = fields->y_ar,
                         .reserved_word0 = UINT32_C(0),
                         .reserved_word1 = UINT32_C(0),
                         .reserved_word2 = UINT32_C(0),
                         .reserved_word3 = UINT32_C(0)}},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .value_word =
                st_parallel_store_value(src_raw40, input->status_asm),
            .rmw_operation =
                (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
            .transfer_reserved = UINT8_C(0),
            .load_t_with_read = false,
            .complete = completion}};

    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_parallel_complete_for_id(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    st_parallel_select select, ti_c54x_op_effect *next_effect)
{
    st_parallel_fields fields;
    ti_c54x_op_status status;
    ti_c54x_u40_t x_value;
    ti_c54x_u40_t sibling_value;
    ti_c54x_u40_add_result add_result;
    ti_c54x_u40_sub_result sub_result;
    ti_c54x_u40_t value;
    ti_c54x_signed32_overflow_direction overflow;
    bool carry_or_no_borrow;
    ti_c54x_op_effect staged = {0};
    uint8_t destination;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = st_parallel_extract(instruction, expected_id, expected_value,
                                 &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = st_parallel_validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* The compute half reads the sibling (non-D) accumulator and
     * the SXM-widened X word shifted left 16 (dual-arithmetic
     * core shape). */
    x_value = ti_c54x_u40_logical_left_shift(
        st_parallel_extend_word(x_word, input->status_sxm), UINT64_C(16));
    sibling_value = fields.destination == UINT8_C(0)
                        ? input->accumulator_b_raw40
                        : input->accumulator_a_raw40;
    if (select == ST_PARALLEL_SELECT_ADD) {
        if (ti_c54x_u40_add_checked(sibling_value, x_value, &add_result) !=
            TI_C54X_ARITHMETIC40_ADDSUB_OK) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        value = add_result.value;
        carry_or_no_borrow = add_result.low32_carry;
        overflow = add_result.signed32_overflow;
    } else {
        if (ti_c54x_u40_sub_checked(x_value, sibling_value, &sub_result) !=
            TI_C54X_ARITHMETIC40_ADDSUB_OK) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        value = sub_result.value;
        carry_or_no_borrow = sub_result.low32_no_borrow;
        overflow = sub_result.signed32_overflow;
    }
    if (input->status_ovm != UINT8_C(0)) {
        if (overflow == TI_C54X_SIGNED32_OVERFLOW_POSITIVE) {
            value = UINT64_C(0x007fffffff);
        } else if (overflow == TI_C54X_SIGNED32_OVERFLOW_NEGATIVE) {
            value = UINT64_C(0xff80000000);
        }
    }
    destination =
        fields.destination == UINT8_C(0) ? UINT8_C(0) : UINT8_C(1);
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = destination;
    staged.payload.accumulator_write.value_raw40 = value;
    if (select == ST_PARALLEL_SELECT_ADD) {
        staged.payload.accumulator_write.status_delta.c_action =
            carry_or_no_borrow ? TI_C54X_OP_STATUS_BIT_WRITE_1
                               : TI_C54X_OP_STATUS_BIT_PRESERVE;
    } else {
        staged.payload.accumulator_write.status_delta.c_action =
            carry_or_no_borrow ? TI_C54X_OP_STATUS_BIT_PRESERVE
                               : TI_C54X_OP_STATUS_BIT_WRITE_0;
    }
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta
        .destination_overflow_action =
        overflow == TI_C54X_SIGNED32_OVERFLOW_NONE
            ? TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE
            : TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1;
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_parallel_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    ti_c54x_op_memory_complete_fn completion)
{
    st_parallel_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = st_parallel_extract(instruction, expected_id, expected_value,
                                 &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = st_parallel_validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_parallel_stage(&fields, input, completion, effect);
}

static ti_c54x_op_status st_parallel_trusted(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_op_memory_complete_fn completion)
{
    st_parallel_fields fields;
    ti_c54x_op_status status;

    (void)instruction;
    if (trusted == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = st_parallel_extract_trusted(trusted, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = st_parallel_validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_parallel_stage(&fields, input, completion, effect);
}

#define DEFINE_ST_PARALLEL_COMPLETE(suffix, id, value, select)             \
    static ti_c54x_op_status complete_##suffix(                             \
        const ti_c54x_decoded_instruction *instruction,                    \
        const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,  \
        ti_c54x_op_effect *next_effect)                                    \
    {                                                                      \
        return st_parallel_complete_for_id(instruction, input,             \
                                           read_value_word, id, value,    \
                                           select, next_effect);          \
    }

DEFINE_ST_PARALLEL_COMPLETE(st_parallel_add,
                            TI_C54X_OPCODE_ID_ST_PARALLEL_ADD,
                            UINT16_C(0xC000), ST_PARALLEL_SELECT_ADD)
DEFINE_ST_PARALLEL_COMPLETE(st_parallel_sub,
                            TI_C54X_OPCODE_ID_ST_PARALLEL_SUB,
                            UINT16_C(0xC400), ST_PARALLEL_SELECT_SUBTRACT)

#undef DEFINE_ST_PARALLEL_COMPLETE

#define DEFINE_ST_PARALLEL_WRAPPERS(suffix, id, value)                    \
    ti_c54x_op_status ti_c54x_op_##suffix(                                 \
        const ti_c54x_decoded_instruction *instruction,                   \
        const ti_c54x_op_input_snapshot *input,                           \
        ti_c54x_op_effect *effect)                                        \
    {                                                                      \
        return st_parallel_checked(instruction, input, effect, id,        \
                                   value, complete_##suffix);             \
    }                                                                      \
    ti_c54x_op_status ti_c54x_trusted_op_##suffix(                         \
        const ti_c54x_decoded_instruction *instruction,                   \
        const ti_c54x_trusted_operand_values *operands,                   \
        const ti_c54x_op_input_snapshot *input,                           \
        ti_c54x_op_effect *effect)                                        \
    {                                                                      \
        return st_parallel_trusted(instruction, operands, input,          \
                                   effect, complete_##suffix);            \
    }

DEFINE_ST_PARALLEL_WRAPPERS(st_parallel_add,
                            TI_C54X_OPCODE_ID_ST_PARALLEL_ADD,
                            UINT16_C(0xC000))
DEFINE_ST_PARALLEL_WRAPPERS(st_parallel_sub,
                            TI_C54X_OPCODE_ID_ST_PARALLEL_SUB,
                            UINT16_C(0xC400))

#undef DEFINE_ST_PARALLEL_WRAPPERS
