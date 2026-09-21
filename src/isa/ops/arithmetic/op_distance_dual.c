/* Module contract
 * Owns: checked and trusted dual-memory SQDST/ABDST semantic leaves
 * (TI SPRU172C Sec4 SQDST/ABDST: B accumulates the distance term
 * while A stages the SXM-widened (X-Y) shifted left 16) over the
 * one-word 0xE200/0xE300 dual-XMOD/XAR/YMOD/YAR forms. The WORD16
 * DATA read pair stages gated on known accumulators, ARs, SXM,
 * FRCT, and OVM. The dual completion stages one accumulator-write
 * pair: A holds (X-Y)<<16 with OVA on guard mismatch and OVM
 * saturation; B adds |A(32-16)| (ABDST, ABS -32768 edge rule) or
 * the squared A(32-16) (SQDST), FRCT-doubled, to 40-bit B with the
 * B-channel carry driving C and 32-bit signed overflow driving
 * OVB with OVM saturation, as in the ADD-dual precedent. TC is
 * preserved. MOD3 indexed and circular commits on either side ride
 * the shared dual resolver, as in the ADD/SUB-dual rows.
 * Does not own: catalog rows, operand-layout definitions, decode,
 * dispatch binding, address resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_distance_dual.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "../../catalog/opcode_catalog.h"
#include "../../decode/operand_layout.h"
#include "../../../foundation/arithmetic40_addsub.h"
#include "../../../state/accumulator/accumulator.h"
#include "../dispatch/trusted_semantics.h"

#define DISTANCE_FIRST_WORD_MASK UINT16_C(0xFF00)
#define DISTANCE_SQDST_VALUE UINT16_C(0xE200)
#define DISTANCE_ABDST_VALUE UINT16_C(0xE300)

typedef struct ti_c54x_distance_dual_operands {
    uint8_t x_mode;
    uint8_t x_ar;
    uint8_t y_mode;
    uint8_t y_ar;
} ti_c54x_distance_dual_operands;

typedef enum ti_c54x_distance_dual_kind {
    TI_C54X_DISTANCE_DUAL_SQDST = 0,
    TI_C54X_DISTANCE_DUAL_ABDST = 1
} ti_c54x_distance_dual_kind;

static ti_c54x_op_status validate_metadata(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    ti_c54x_distance_dual_operands *output)
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
    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_value != expected_value ||
        metadata->first_word_mask != DISTANCE_FIRST_WORD_MASK ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_YMOD_YAR ||
        metadata->repeatability != TI_C54X_REPEATABILITY_ELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_SXM_FRCT_OVM ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
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

static ti_c54x_op_status validate_trusted(
    const ti_c54x_trusted_operand_values *trusted,
    ti_c54x_distance_dual_operands *output)
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

static ti_c54x_op_status validate_inputs(
    const ti_c54x_distance_dual_operands *operands,
    const ti_c54x_op_input_snapshot *input)
{
    const uint8_t x_bit = (uint8_t)(UINT8_C(1) << operands->x_ar);
    const uint8_t y_bit = (uint8_t)(UINT8_C(1) << operands->y_ar);

    if (!input->accumulator_a_known || !input->accumulator_b_known ||
        !input->status_sxm_known || !input->status_frct_known ||
        !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
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

static int64_t widen_word(uint16_t word, uint8_t sxm)
{
    if (sxm != UINT8_C(0) && (word & UINT16_C(0x8000)) != UINT16_C(0)) {
        return (int64_t)((int32_t)word - INT32_C(65536));
    }
    return (int64_t)word;
}

static int32_t signed16(uint16_t word)
{
    return (word & UINT16_C(0x8000)) != UINT16_C(0)
               ? (int32_t)word - INT32_C(65536)
               : (int32_t)word;
}

/* The B-channel distance term: |A(32-16)| for ABDST (ABS -32768
 * edge rule under OVM), the squared A(32-16) for SQDST. FRCT
 * doubles the term, as in the multiply core. */
static uint64_t distance_term(ti_c54x_distance_dual_kind kind,
                              int32_t a_hi, uint8_t frct, uint8_t ovm)
{
    uint64_t term;

    if (kind == TI_C54X_DISTANCE_DUAL_ABDST) {
        uint64_t magnitude =
            a_hi < INT32_C(0) ? (uint64_t)(-(int64_t)a_hi)
                              : (uint64_t)a_hi;
        if (a_hi == INT32_C(-32768) && ovm != UINT8_C(0)) {
            magnitude = UINT64_C(32767);
        }
        term = magnitude;
    } else {
        const int64_t factor = (int64_t)a_hi;
        term = (uint64_t)(factor * factor);
    }
    if (frct != UINT8_C(0)) {
        term *= UINT64_C(2);
    }
    return term;
}

static ti_c54x_op_status complete_values(
    const ti_c54x_distance_dual_operands *operands,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word,
    uint16_t y_word, ti_c54x_distance_dual_kind kind,
    ti_c54x_op_effect *next_effect)
{
    const int64_t difference =
        widen_word(x_word, input->status_sxm) -
        widen_word(y_word, input->status_sxm);
    const uint64_t a_staged =
        ((uint64_t)(difference << 16)) & (uint64_t)TI_C54X_U40_MASK;
    const uint8_t a_guard = (uint8_t)((a_staged >> 32U) & UINT64_C(0xFF));
    const uint8_t a_bit31 = (uint8_t)((a_staged >> 31U) & UINT64_C(1));
    const bool a_overflow =
        a_guard != (a_bit31 != UINT8_C(0) ? UINT8_C(0xFF)
                                          : UINT8_C(0));
    const int32_t a_hi = signed16(
        (uint16_t)((input->accumulator_a_raw40 >> 16U) & UINT64_C(0xFFFF)));
    const uint64_t term =
        distance_term(kind, a_hi, input->status_frct, input->status_ovm);
    ti_c54x_u40_t b_old = (ti_c54x_u40_t)(input->accumulator_b_raw40 &
                                          (uint64_t)TI_C54X_U40_MASK);
    ti_c54x_u40_add_result b_add;
    ti_c54x_op_effect staged = {0};
    uint64_t a_value = a_staged;
    uint64_t b_value;
    bool b_overflow;
    ti_c54x_op_status status;

    status = validate_inputs(operands, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (ti_c54x_u40_add_checked(
            b_old, (ti_c54x_u40_t)(term & (uint64_t)TI_C54X_U40_MASK),
            &b_add) != TI_C54X_ARITHMETIC40_ADDSUB_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (a_overflow && input->status_ovm != UINT8_C(0)) {
        /* Saturate by the pre-shift difference sign: the shift can
         * carry the sign out of bit 31 (LD-shift precedent). */
        a_value = difference < INT64_C(0) ? UINT64_C(0xff80000000)
                                          : UINT64_C(0x007fffffff);
    }
    b_value = b_add.value;
    b_overflow = b_add.signed32_overflow !=
                 TI_C54X_SIGNED32_OVERFLOW_NONE;
    if (b_overflow && input->status_ovm != UINT8_C(0)) {
        b_value = b_add.signed32_overflow ==
                          TI_C54X_SIGNED32_OVERFLOW_POSITIVE
                      ? UINT64_C(0x007fffffff)
                      : UINT64_C(0xff80000000);
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE_PAIR;
    staged.payload.accumulator_write_pair.first.destination =
        TI_C54X_ACCUMULATOR_A;
    staged.payload.accumulator_write_pair.first.value_raw40 =
        (ti_c54x_u40_t)a_value;
    staged.payload.accumulator_write_pair.first.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write_pair.first.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write_pair.first.status_delta
        .destination_overflow_action =
        a_overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                   : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    staged.payload.accumulator_write_pair.second.destination =
        TI_C54X_ACCUMULATOR_B;
    staged.payload.accumulator_write_pair.second.value_raw40 =
        (ti_c54x_u40_t)b_value;
    staged.payload.accumulator_write_pair.second.status_delta.c_action =
        b_add.low32_carry ? TI_C54X_OP_STATUS_BIT_WRITE_1
                          : TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write_pair.second.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write_pair.second.status_delta
        .destination_overflow_action =
        b_overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                   : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status stage_memory(
    const ti_c54x_distance_dual_operands *operands,
    ti_c54x_op_memory_complete_dual_fn completion,
    ti_c54x_op_effect *effect)
{
    const uint8_t x_mode = operands->x_mode;
    const uint8_t y_mode = operands->y_mode;
    ti_c54x_op_effect staged;

    if (effect == NULL || operands == NULL || completion == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16,
                .dual = {.x_mode = x_mode,
                         .x_ar = operands->x_ar,
                         .y_mode = y_mode,
                         .y_ar = operands->y_ar,
                         .reserved_word0 = UINT32_C(0),
                         .reserved_word1 = UINT32_C(0),
                         .reserved_word2 = UINT32_C(0),
                         .reserved_word3 = UINT32_C(0)}},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = UINT16_C(0),
            .rmw_operation =
                (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
            .transfer_reserved = UINT8_C(0),
            .complete_dual = completion}};

    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status complete_for_kind(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word,
    uint16_t y_word, ti_c54x_opcode_id expected_id,
    uint16_t expected_value, ti_c54x_distance_dual_kind kind,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_distance_dual_operands operands;
    ti_c54x_op_status status = validate_metadata(
        instruction, expected_id, expected_value, &operands);

    if (status != TI_C54X_OP_STATUS_OK || next_effect == NULL ||
        input == NULL) {
        return status != TI_C54X_OP_STATUS_OK
                   ? status
                   : TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    return complete_values(&operands, input, x_word, y_word, kind,
                           next_effect);
}

static ti_c54x_op_status complete_sqdst(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word,
    uint16_t y_word, ti_c54x_op_effect *next_effect)
{
    return complete_for_kind(instruction, input, x_word, y_word,
                             TI_C54X_OPCODE_ID_SQDST_XMEM_YMEM,
                             DISTANCE_SQDST_VALUE,
                             TI_C54X_DISTANCE_DUAL_SQDST, next_effect);
}

static ti_c54x_op_status complete_abdst(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word,
    uint16_t y_word, ti_c54x_op_effect *next_effect)
{
    return complete_for_kind(instruction, input, x_word, y_word,
                             TI_C54X_OPCODE_ID_ABDST_XMEM_YMEM,
                             DISTANCE_ABDST_VALUE,
                             TI_C54X_DISTANCE_DUAL_ABDST, next_effect);
}

static ti_c54x_op_status checked_common(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    ti_c54x_op_memory_complete_dual_fn completion)
{
    ti_c54x_distance_dual_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = validate_metadata(instruction, expected_id, expected_value,
                               &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = validate_inputs(&operands, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return stage_memory(&operands, completion, effect);
}

static ti_c54x_op_status trusted_common(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_op_memory_complete_dual_fn completion)
{
    ti_c54x_distance_dual_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || trusted == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = validate_trusted(trusted, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = validate_inputs(&operands, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return stage_memory(&operands, completion, effect);
}

ti_c54x_op_status ti_c54x_op_sqdst_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return checked_common(instruction, input, effect,
                          TI_C54X_OPCODE_ID_SQDST_XMEM_YMEM,
                          DISTANCE_SQDST_VALUE, complete_sqdst);
}

ti_c54x_op_status ti_c54x_op_abdst_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return checked_common(instruction, input, effect,
                          TI_C54X_OPCODE_ID_ABDST_XMEM_YMEM,
                          DISTANCE_ABDST_VALUE, complete_abdst);
}

ti_c54x_op_status ti_c54x_trusted_op_sqdst_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return trusted_common(instruction, operands, input, effect,
                          complete_sqdst);
}

ti_c54x_op_status ti_c54x_trusted_op_abdst_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return trusted_common(instruction, operands, input, effect,
                          complete_abdst);
}
