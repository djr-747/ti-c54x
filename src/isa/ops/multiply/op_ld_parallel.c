/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the parallel
 * LD Xmem,dst || MAC[R]/MAS[R] Ymem,dst_ dual rows (TI SPRU172C
 * §LD||MAC[R] p. 4-74, opcode 1010_10RD_XXXX_YYYY; §LD||MAS[R]
 * p. 4-76, opcode 1010_11RD_XXXX_YYYY). The leaves stage one
 * dual WORD16 read effect with no read-phase T load and, on pair
 * completion, one accumulator-write pair: the LD half writes the
 * SXM-widened X word shifted left 16 to dst while the MAC/MAS
 * half accumulates live-T times Y into dst_ through the multiply
 * core with FRCT/OVM handling and R rounding. D selects the LD
 * destination (0 = A, 1 = B); the MAC/MAS half takes the other
 * accumulator. R rounding is shared per row, so MACR/MASR bind
 * to the plain rows.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution or mutation, buses, CPU/state mutation,
 * sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained;
 * failed calls leave the caller-owned effect unchanged.
 */

#include "op_ld_parallel.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "state/accumulator/accumulator.h"

#include "op_multiply.h"

typedef enum ld_parallel_select {
    LD_PARALLEL_SELECT_ADD = 0,
    LD_PARALLEL_SELECT_SUBTRACT = 1
} ld_parallel_select;

typedef struct ld_parallel_fields {
    uint8_t round;
    uint8_t destination;
    uint8_t x_mode;
    uint8_t x_ar;
    uint8_t y_mode;
    uint8_t y_ar;
} ld_parallel_fields;

static ti_c54x_op_status ld_parallel_check_operands(
    uint64_t round, uint64_t destination, uint64_t dualxy,
    ld_parallel_fields *output)
{
    uint8_t x_ar;
    uint8_t y_ar;

    if (output == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (round > UINT64_C(1) || destination > UINT64_C(1) ||
        dualxy > UINT64_C(0xFF)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    x_ar = (uint8_t)(((dualxy >> 4U) & UINT64_C(3)) + UINT64_C(2));
    y_ar = (uint8_t)((dualxy & UINT64_C(3)) + UINT64_C(2));
    output->round = (uint8_t)round;
    output->destination = (uint8_t)destination;
    output->x_mode = (uint8_t)((dualxy >> 6U) & UINT64_C(3));
    output->x_ar = x_ar;
    output->y_mode = (uint8_t)((dualxy >> 2U) & UINT64_C(3));
    output->y_ar = y_ar;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_parallel_extract(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    ld_parallel_fields *fields)
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
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_T_REGISTER_STATUS_SXM_FRCT_OVM ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ROUND_DESTINATION_DUALXY ||
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
    return ld_parallel_check_operands(unpacked[0].raw_value,
                                      unpacked[1].raw_value,
                                      unpacked[2].raw_value, fields);
}

static ti_c54x_op_status ld_parallel_extract_trusted(
    const ti_c54x_trusted_operand_values *trusted,
    ld_parallel_fields *fields)
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
    if (ld_parallel_check_operands(trusted->values[0].raw_value,
                                   trusted->values[1].raw_value,
                                   trusted->values[2].raw_value,
                                   fields) != TI_C54X_OP_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_parallel_validate_inputs(
    const ld_parallel_fields *fields, const ti_c54x_op_input_snapshot *input)
{
    const uint8_t x_bit = (uint8_t)(UINT8_C(1) << fields->x_ar);
    const uint8_t y_bit = (uint8_t)(UINT8_C(1) << fields->y_ar);

    if (!input->accumulator_a_known || !input->accumulator_b_known ||
        !input->t_register_known || !input->status_sxm_known ||
        !input->status_frct_known || !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->accumulator_a_raw40 > TI_C54X_U40_MASK ||
        input->accumulator_b_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (input->status_sxm > UINT8_C(1) ||
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

static ti_c54x_op_status ld_parallel_stage(
    const ld_parallel_fields *fields,
    ti_c54x_op_memory_complete_dual_fn completion,
    ti_c54x_op_effect *effect)
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
                .dual = {.x_mode = fields->x_mode,
                         .x_ar = fields->x_ar,
                         .y_mode = fields->y_mode,
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
            /* T is a live validated input here, never a
             * read-phase load: the examples keep T across the
             * instruction. */
            .load_t_with_read = false,
            .complete_dual = completion}};

    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_parallel_complete_for_id(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word,
    uint16_t y_word, ti_c54x_opcode_id expected_id,
    uint16_t expected_value, ld_parallel_select select,
    ti_c54x_op_effect *next_effect)
{
    ld_parallel_fields fields;
    ti_c54x_op_status status;
    ti_c54x_multiply_operands operands;
    ti_c54x_multiply_result result;
    ti_c54x_multiply_status multiply_status;
    ti_c54x_u40_t load_value;
    ti_c54x_op_effect staged = {0};
    uint8_t load_destination;
    uint8_t mac_destination;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_parallel_extract(instruction, expected_id, expected_value,
                                 &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_parallel_validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* The MAC/MAS half multiplies live T by the Y-side word and
     * accumulates into the non-D accumulator. */
    operands.t_raw16 = input->t_register_raw16;
    operands.t_known = true;
    operands.multiplicand_raw16 = y_word;
    operands.multiplicand_known = true;
    operands.accumulator_raw40 =
        fields.destination == UINT8_C(0) ? input->accumulator_b_raw40
                                         : input->accumulator_a_raw40;
    operands.accumulator_known = true;
    operands.frct = input->status_frct;
    operands.frct_known = true;
    operands.ovm = input->status_ovm;
    operands.ovm_known = true;
    operands.unsigned_operands = false;
    operands.unsigned_t_only = false;
    operands.rounding = fields.round != UINT8_C(0);
    operands.select = select == LD_PARALLEL_SELECT_ADD
                          ? TI_C54X_MULTIPLY_ADD
                          : TI_C54X_MULTIPLY_SUBTRACT;
    multiply_status = ti_c54x_multiply_accumulate(&operands, &result);
    if (multiply_status == TI_C54X_MULTIPLY_UNKNOWN) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (multiply_status != TI_C54X_MULTIPLY_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    /* The LD half loads the SXM-widened X word shifted left 16
     * (shift16-load precedent): widening first preserves the
     * sign through the shift, and the full-40 write zeroes the
     * low 16 as the TI examples pin. A shift never
     * overflows. */
    load_value = ti_c54x_u40_from_unsigned(x_word);
    if (input->status_sxm != UINT8_C(0) &&
        (x_word & UINT16_C(0x8000)) != UINT16_C(0)) {
        load_value = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                     (uint64_t)x_word;
    }
    load_value = ti_c54x_u40_logical_left_shift(load_value, UINT8_C(16));
    load_destination =
        fields.destination == UINT8_C(0) ? TI_C54X_ACCUMULATOR_A
                                         : TI_C54X_ACCUMULATOR_B;
    mac_destination =
        fields.destination == UINT8_C(0) ? TI_C54X_ACCUMULATOR_B
                                         : TI_C54X_ACCUMULATOR_A;
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE_PAIR;
    staged.payload.accumulator_write_pair.first.destination =
        load_destination;
    staged.payload.accumulator_write_pair.first.value_raw40 = load_value;
    staged.payload.accumulator_write_pair.first.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write_pair.first.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write_pair.first.status_delta
        .destination_overflow_action =
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    staged.payload.accumulator_write_pair.second.destination =
        mac_destination;
    staged.payload.accumulator_write_pair.second.value_raw40 =
        result.accumulator_raw40;
    staged.payload.accumulator_write_pair.second.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write_pair.second.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write_pair.second.status_delta
        .destination_overflow_action =
        result.overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                        : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_parallel_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    ti_c54x_op_memory_complete_dual_fn completion)
{
    ld_parallel_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_parallel_extract(instruction, expected_id, expected_value,
                                 &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_parallel_validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ld_parallel_stage(&fields, completion, effect);
}

static ti_c54x_op_status ld_parallel_trusted(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_op_memory_complete_dual_fn completion)
{
    ld_parallel_fields fields;
    ti_c54x_op_status status;

    (void)instruction;
    if (trusted == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_parallel_extract_trusted(trusted, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_parallel_validate_inputs(&fields, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ld_parallel_stage(&fields, completion, effect);
}

#define DEFINE_LD_PARALLEL_COMPLETE(suffix, id, value, select)             \
    static ti_c54x_op_status complete_##suffix(                             \
        const ti_c54x_decoded_instruction *instruction,                    \
        const ti_c54x_op_input_snapshot *input, uint16_t x_word,           \
        uint16_t y_word, ti_c54x_op_effect *next_effect)                   \
    {                                                                      \
        return ld_parallel_complete_for_id(instruction, input, x_word,    \
                                           y_word, id, value, select,     \
                                           next_effect);                  \
    }

DEFINE_LD_PARALLEL_COMPLETE(ld_parallel_mac,
                            TI_C54X_OPCODE_ID_LD_PARALLEL_MAC,
                            UINT16_C(0xA800), LD_PARALLEL_SELECT_ADD)
DEFINE_LD_PARALLEL_COMPLETE(ld_parallel_mas,
                            TI_C54X_OPCODE_ID_LD_PARALLEL_MAS,
                            UINT16_C(0xAC00), LD_PARALLEL_SELECT_SUBTRACT)

#undef DEFINE_LD_PARALLEL_COMPLETE

#define DEFINE_LD_PARALLEL_WRAPPERS(suffix, id, value)                    \
    ti_c54x_op_status ti_c54x_op_##suffix(                                 \
        const ti_c54x_decoded_instruction *instruction,                   \
        const ti_c54x_op_input_snapshot *input,                           \
        ti_c54x_op_effect *effect)                                        \
    {                                                                      \
        return ld_parallel_checked(instruction, input, effect, id,        \
                                   value, complete_##suffix);             \
    }                                                                      \
    ti_c54x_op_status ti_c54x_trusted_op_##suffix(                         \
        const ti_c54x_decoded_instruction *instruction,                   \
        const ti_c54x_trusted_operand_values *operands,                   \
        const ti_c54x_op_input_snapshot *input,                           \
        ti_c54x_op_effect *effect)                                        \
    {                                                                      \
        return ld_parallel_trusted(instruction, operands, input,          \
                                   effect, complete_##suffix);            \
    }

DEFINE_LD_PARALLEL_WRAPPERS(ld_parallel_mac,
                            TI_C54X_OPCODE_ID_LD_PARALLEL_MAC,
                            UINT16_C(0xA800))
DEFINE_LD_PARALLEL_WRAPPERS(ld_parallel_mas,
                            TI_C54X_OPCODE_ID_LD_PARALLEL_MAS,
                            UINT16_C(0xAC00))

#undef DEFINE_LD_PARALLEL_WRAPPERS
