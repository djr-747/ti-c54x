/* Module contract
 * Owns: checked and trusted pure semantic leaves for the ST
 * parallel-MAC/MAS/MPY rows (TI SPRU172C Table 2-25). The
 * leaves stage one dual-indirect WORD16 WRITE effect over the
 * MOVE_PAIR arm with the leaf-frozen ST value and the
 * single-read completion set; the store-compute root issues
 * the X READ, then the Y WRITE with the frozen value, then
 * the completion. The ST half stores the STH-by-ASM value of
 * src (S11 leaf shape). The compute half multiplies live T by
 * the X-side word and accumulates into the D-selected
 * accumulator itself (MAC/MAS, R rounding per the S114 shape)
 * or replaces dst (MPY); C preserves (standalone MAC/MPY
 * precedent, whose core reports no carry) and 32-bit overflow
 * drives OVdst with OVM saturation. S selects the store
 * source (0 = A, 1 = B) and D the compute destination; src is
 * preserved. T and SXM are never consumed; T is read live.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution or mutation, buses, CPU/state mutation,
 * sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained;
 * failed calls leave the caller-owned effect unchanged.
 */

#include "op_st_parallel_mac.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "../../catalog/opcode_catalog.h"
#include "../../decode/operand_layout.h"
#include "../../../foundation/arithmetic40.h"
#include "../dispatch/trusted_semantics.h"
#include "../memory/memory_effect.h"
#include "../multiply/op_multiply.h"

typedef enum st_parallel_mac_select {
    ST_PARALLEL_MAC_SELECT_MAC = 0,
    ST_PARALLEL_MAC_SELECT_MAS = 1,
    ST_PARALLEL_MAC_SELECT_MPY = 2
} st_parallel_mac_select;

typedef struct st_parallel_mac_fields {
    uint8_t round;
    uint8_t src;
    uint8_t dst;
    uint8_t x_mode;
    uint8_t x_ar;
    uint8_t y_mode;
    uint8_t y_ar;
} st_parallel_mac_fields;

static ti_c54x_op_status st_parallel_mac_check_operands(
    uint64_t round, uint64_t src, uint64_t dst, uint64_t dualxy,
    st_parallel_mac_fields *fields)
{
    uint64_t dual;

    if (fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (round > UINT64_C(1) || src > UINT64_C(1) || dst > UINT64_C(1) ||
        dualxy > UINT64_C(0xFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    dual = dualxy;
    fields->round = (uint8_t)round;
    fields->src = (uint8_t)src;
    fields->dst = (uint8_t)dst;
    fields->x_mode = (uint8_t)((dual >> 6U) & UINT64_C(3));
    fields->x_ar = (uint8_t)(((dual >> 4U) & UINT64_C(3)) + UINT64_C(2));
    fields->y_mode = (uint8_t)((dual >> 2U) & UINT64_C(3));
    fields->y_ar = (uint8_t)((dual & UINT64_C(3)) + UINT64_C(2));
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_parallel_mac_extract(
    const ti_c54x_decoded_instruction *instruction, ti_c54x_opcode_id id,
    uint16_t expected_value, uint16_t expected_mask, bool rounded_row,
    st_parallel_mac_fields *fields)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value unpacked[4] = {{0U, 0U},
                                              {0U, 0U},
                                              {0U, 0U},
                                              {0U, 0U}};
    ti_c54x_program_words_view words;
    uint8_t index;
    uint8_t count = rounded_row ? UINT8_C(4) : UINT8_C(3);

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(id);
    if (metadata == NULL || metadata->id != id ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_T_REGISTER_STATUS_FRCT_OVM ||
        metadata->operand_layout !=
            (rounded_row
                 ? TI_C54X_OPERAND_LAYOUT_ROUND_SRC_DEST_DUALXY
                 : TI_C54X_OPERAND_LAYOUT_SRC_DEST_DUALXY) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->repeatability != TI_C54X_REPEATABILITY_ELIGIBLE ||
        instruction->id != id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value ||
        metadata->first_word_value != expected_value ||
        metadata->first_word_mask != expected_mask) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    for (index = UINT8_C(0); index < count; ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index,
                &unpacked[index]) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    if (rounded_row) {
        if (unpacked[0].width_bits != UINT8_C(1) ||
            unpacked[1].width_bits != UINT8_C(1) ||
            unpacked[2].width_bits != UINT8_C(1) ||
            unpacked[3].width_bits != UINT8_C(8)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        return st_parallel_mac_check_operands(
            unpacked[0].raw_value, unpacked[1].raw_value,
            unpacked[2].raw_value, unpacked[3].raw_value, fields);
    }
    if (unpacked[0].width_bits != UINT8_C(1) ||
        unpacked[1].width_bits != UINT8_C(1) ||
        unpacked[2].width_bits != UINT8_C(8)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return st_parallel_mac_check_operands(
        UINT64_C(0), unpacked[0].raw_value, unpacked[1].raw_value,
        unpacked[2].raw_value, fields);
}

static ti_c54x_op_status st_parallel_mac_extract_trusted(
    const ti_c54x_trusted_operand_values *trusted, bool rounded_row,
    st_parallel_mac_fields *fields)
{
    if (trusted == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (rounded_row) {
        if (trusted->count != UINT8_C(4)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        if (trusted->values[0].width_bits != UINT8_C(1) ||
            trusted->values[1].width_bits != UINT8_C(1) ||
            trusted->values[2].width_bits != UINT8_C(1) ||
            trusted->values[3].width_bits != UINT8_C(8)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        return st_parallel_mac_check_operands(
            trusted->values[0].raw_value, trusted->values[1].raw_value,
            trusted->values[2].raw_value, trusted->values[3].raw_value,
            fields);
    }
    if (trusted->count != UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (trusted->values[0].width_bits != UINT8_C(1) ||
        trusted->values[1].width_bits != UINT8_C(1) ||
        trusted->values[2].width_bits != UINT8_C(8)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return st_parallel_mac_check_operands(
        UINT64_C(0), trusted->values[0].raw_value,
        trusted->values[1].raw_value, trusted->values[2].raw_value, fields);
}

static ti_c54x_op_status st_parallel_mac_validate_inputs(
    const st_parallel_mac_fields *fields,
    const ti_c54x_op_input_snapshot *input)
{
    const uint8_t x_bit = (uint8_t)(UINT8_C(1) << fields->x_ar);
    const uint8_t y_bit = (uint8_t)(UINT8_C(1) << fields->y_ar);

    if (fields == NULL || input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!input->accumulator_a_known || !input->accumulator_b_known ||
        !input->status_asm_known || !input->t_register_known ||
        !input->status_frct_known || !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->accumulator_a_raw40 > TI_C54X_U40_MASK ||
        input->accumulator_b_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (input->status_asm > UINT8_C(31) ||
        input->status_frct > UINT8_C(1) ||
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
 * by 32-ASM) and extract bits 31-16. */
static uint16_t st_parallel_mac_store_value(uint64_t source_raw40,
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

static ti_c54x_op_status st_parallel_mac_stage(
    const st_parallel_mac_fields *fields,
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
                st_parallel_mac_store_value(src_raw40, input->status_asm),
            .rmw_operation =
                (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
            .transfer_reserved = UINT8_C(0),
            .load_t_with_read = false,
            .complete = completion}};

    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_parallel_mac_complete_for_id(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    uint16_t expected_mask, bool rounded_row, st_parallel_mac_select select,
    ti_c54x_op_effect *next_effect)
{
    st_parallel_mac_fields fields;
    ti_c54x_op_status status;
    ti_c54x_multiply_operands operands;
    ti_c54x_multiply_result result;
    ti_c54x_multiply_status multiply_status;
    ti_c54x_op_effect staged = {0};

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = st_parallel_mac_extract(instruction, expected_id,
                                     expected_value, expected_mask,
                                     rounded_row, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = st_parallel_mac_validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* The compute half multiplies live T by the X-side word and
     * accumulates into the D-selected accumulator itself (Table
     * 2-25 dst expressions); MPY replaces dst with the product. */
    operands.t_raw16 = input->t_register_raw16;
    operands.t_known = true;
    operands.multiplicand_raw16 = x_word;
    operands.multiplicand_known = true;
    operands.accumulator_raw40 = fields.dst == UINT8_C(0)
                                     ? input->accumulator_a_raw40
                                     : input->accumulator_b_raw40;
    operands.accumulator_known = true;
    operands.frct = input->status_frct;
    operands.frct_known = true;
    operands.ovm = input->status_ovm;
    operands.ovm_known = true;
    operands.unsigned_operands = false;
    operands.unsigned_t_only = false;
    operands.rounding = fields.round != UINT8_C(0);
    operands.select = select == ST_PARALLEL_MAC_SELECT_MAC
                          ? TI_C54X_MULTIPLY_ADD
                          : (select == ST_PARALLEL_MAC_SELECT_MAS
                                 ? TI_C54X_MULTIPLY_SUBTRACT
                                 : TI_C54X_MULTIPLY_STORE);
    multiply_status = ti_c54x_multiply_accumulate(&operands, &result);
    if (multiply_status == TI_C54X_MULTIPLY_UNKNOWN) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (multiply_status != TI_C54X_MULTIPLY_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields.dst;
    staged.payload.accumulator_write.value_raw40 = result.accumulator_raw40;
    /* The multiply core reports no carry: C preserves on all
     * three compute halves (standalone MAC/MPY precedent). */
    staged.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta
        .destination_overflow_action =
        result.overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                        : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_parallel_mac_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    uint16_t expected_mask, bool rounded_row,
    ti_c54x_op_memory_complete_fn completion)
{
    st_parallel_mac_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = st_parallel_mac_extract(instruction, expected_id,
                                     expected_value, expected_mask,
                                     rounded_row, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = st_parallel_mac_validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_parallel_mac_stage(&fields, input, completion, effect);
}

static ti_c54x_op_status st_parallel_mac_trusted(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    bool rounded_row, ti_c54x_op_memory_complete_fn completion)
{
    st_parallel_mac_fields fields;
    ti_c54x_op_status status;

    (void)instruction;
    if (trusted == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = st_parallel_mac_extract_trusted(trusted, rounded_row, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = st_parallel_mac_validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_parallel_mac_stage(&fields, input, completion, effect);
}

#define DEFINE_ST_PARALLEL_MAC_COMPLETE(suffix, id, value, mask, rounded, \
                                        select)                           \
    static ti_c54x_op_status complete_##suffix(                             \
        const ti_c54x_decoded_instruction *instruction,                    \
        const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,  \
        ti_c54x_op_effect *next_effect)                                    \
    {                                                                      \
        return st_parallel_mac_complete_for_id(                            \
            instruction, input, read_value_word, id, value, mask, rounded, \
            select, next_effect);                                          \
    }

DEFINE_ST_PARALLEL_MAC_COMPLETE(st_parallel_mac,
                                TI_C54X_OPCODE_ID_ST_PARALLEL_MAC,
                                UINT16_C(0xD000), UINT16_C(0xF800), true,
                                ST_PARALLEL_MAC_SELECT_MAC)
DEFINE_ST_PARALLEL_MAC_COMPLETE(st_parallel_mas,
                                TI_C54X_OPCODE_ID_ST_PARALLEL_MAS,
                                UINT16_C(0xD800), UINT16_C(0xF800), true,
                                ST_PARALLEL_MAC_SELECT_MAS)
DEFINE_ST_PARALLEL_MAC_COMPLETE(st_parallel_mpy,
                                TI_C54X_OPCODE_ID_ST_PARALLEL_MPY,
                                UINT16_C(0xCC00), UINT16_C(0xFC00), false,
                                ST_PARALLEL_MAC_SELECT_MPY)

#undef DEFINE_ST_PARALLEL_MAC_COMPLETE

#define DEFINE_ST_PARALLEL_MAC_WRAPPERS(suffix, id, value, mask, rounded) \
    ti_c54x_op_status ti_c54x_op_##suffix(                                 \
        const ti_c54x_decoded_instruction *instruction,                   \
        const ti_c54x_op_input_snapshot *input,                           \
        ti_c54x_op_effect *effect)                                        \
    {                                                                      \
        return st_parallel_mac_checked(instruction, input, effect, id,    \
                                       value, mask, rounded,              \
                                       complete_##suffix);                \
    }                                                                      \
    ti_c54x_op_status ti_c54x_trusted_op_##suffix(                         \
        const ti_c54x_decoded_instruction *instruction,                   \
        const ti_c54x_trusted_operand_values *operands,                   \
        const ti_c54x_op_input_snapshot *input,                           \
        ti_c54x_op_effect *effect)                                        \
    {                                                                      \
        return st_parallel_mac_trusted(instruction, operands, input,      \
                                       effect, rounded,                   \
                                       complete_##suffix);                \
    }

DEFINE_ST_PARALLEL_MAC_WRAPPERS(st_parallel_mac,
                                TI_C54X_OPCODE_ID_ST_PARALLEL_MAC,
                                UINT16_C(0xD000), UINT16_C(0xF800), true)
DEFINE_ST_PARALLEL_MAC_WRAPPERS(st_parallel_mas,
                                TI_C54X_OPCODE_ID_ST_PARALLEL_MAS,
                                UINT16_C(0xD800), UINT16_C(0xF800), true)
DEFINE_ST_PARALLEL_MAC_WRAPPERS(st_parallel_mpy,
                                TI_C54X_OPCODE_ID_ST_PARALLEL_MPY,
                                UINT16_C(0xCC00), UINT16_C(0xFC00), false)

#undef DEFINE_ST_PARALLEL_MAC_WRAPPERS
