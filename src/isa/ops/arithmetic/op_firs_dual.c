/* Module contract
 * Owns: checked and trusted dual-memory FIRS semantic leaves
 * (TI SPRU172C Sec4 FIRS: A stages (X+Y)<<16 while B
 * accumulates the signed old-A(32-16) times the Pmem(pmad)
 * coefficient, FRCT-doubled) over the two-word 0xE000
 * dual-XMOD/XAR/YMOD/YAR+PMAD form. The WORD16 triple read
 * (dual DATA plus absolute PROGRAM coefficient) stages gated
 * on known accumulators, ARs, SXM, FRCT, and OVM. The triple
 * completion stages one accumulator-write pair: A holds the
 * SXM-widened (X+Y) shifted left 16 with OVA on guard
 * mismatch and OVM saturation; B adds the signed product to
 * 40-bit B with the B-channel carry driving C and 32-bit
 * signed overflow driving OVB with OVM saturation, as in the
 * ADD-dual precedent. TC is preserved; T is never written.
 * MOD3 indexed and circular commits on either side ride the
 * shared dual resolver, as in the ADD/SUB-dual rows.
 * Does not own: catalog rows, operand-layout descriptors, decode,
 * dispatch binding, address resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_firs_dual.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "../../catalog/opcode_catalog.h"
#include "../../decode/operand_layout.h"
#include "../../../foundation/arithmetic40_addsub.h"
#include "../../../state/accumulator/accumulator.h"
#include "../dispatch/trusted_semantics.h"

#define FIRS_FIRST_WORD_MASK UINT16_C(0xFF00)
#define FIRS_XMEM_YMEM_PMAD_VALUE UINT16_C(0xE000)

typedef struct ti_c54x_firs_dual_operands {
    uint8_t x_mode;
    uint8_t x_ar;
    uint8_t y_mode;
    uint8_t y_ar;
    uint16_t pmad;
} ti_c54x_firs_dual_operands;

static ti_c54x_op_status validate_metadata(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_firs_dual_operands *output)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value fields[2] = {{0U, 0U},
                                             {0U, 0U}};
    const ti_c54x_program_words_view words = {
        instruction == NULL ? NULL : instruction->bundle.program_words,
        instruction == NULL ? 0U : instruction->bundle.program_word_count};
    uint8_t index;
    uint16_t dualxy;

    if (instruction == NULL || output == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_FIRS_XMEM_YMEM_PMAD);
    if (metadata == NULL ||
        metadata->id != TI_C54X_OPCODE_ID_FIRS_XMEM_YMEM_PMAD ||
        metadata->first_word_value != FIRS_XMEM_YMEM_PMAD_VALUE ||
        metadata->first_word_mask != FIRS_FIRST_WORD_MASK ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_YMOD_YAR_PMAD16 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_ELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_SXM_FRCT_OVM ||
        instruction->id != TI_C54X_OPCODE_ID_FIRS_XMEM_YMEM_PMAD ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    for (index = UINT8_C(0); index < UINT8_C(2); ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &fields[index]) !=
            TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    if (fields[0].width_bits != UINT8_C(8) ||
        fields[1].width_bits != UINT8_C(16) ||
        fields[0].raw_value > UINT64_C(0xFF)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    dualxy = (uint16_t)fields[0].raw_value;
    output->x_mode = (uint8_t)((dualxy >> 6U) & UINT16_C(3));
    output->x_ar = (uint8_t)(((dualxy >> 4U) & UINT16_C(3)) + UINT16_C(2));
    output->y_mode = (uint8_t)((dualxy >> 2U) & UINT16_C(3));
    output->y_ar = (uint8_t)((dualxy & UINT16_C(3)) + UINT16_C(2));
    output->pmad = (uint16_t)fields[1].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status validate_trusted(
    const ti_c54x_trusted_operand_values *trusted,
    ti_c54x_firs_dual_operands *output)
{
    uint16_t dualxy;

    if (trusted == NULL || output == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (trusted->count != UINT8_C(2) ||
        trusted->values[0].width_bits != UINT8_C(8) ||
        trusted->values[1].width_bits != UINT8_C(16) ||
        trusted->values[0].raw_value > UINT64_C(0xFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    dualxy = (uint16_t)trusted->values[0].raw_value;
    output->x_mode = (uint8_t)((dualxy >> 6U) & UINT16_C(3));
    output->x_ar = (uint8_t)(((dualxy >> 4U) & UINT16_C(3)) + UINT16_C(2));
    output->y_mode = (uint8_t)((dualxy >> 2U) & UINT16_C(3));
    output->y_ar = (uint8_t)((dualxy & UINT16_C(3)) + UINT16_C(2));
    output->pmad = (uint16_t)trusted->values[1].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status validate_inputs(
    const ti_c54x_firs_dual_operands *operands,
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

static ti_c54x_op_status complete_values(
    const ti_c54x_firs_dual_operands *operands,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word,
    uint16_t y_word, uint16_t coef_word, ti_c54x_op_effect *next_effect)
{
    /* A stages the SXM-widened (X+Y) shifted left 16. B
     * accumulates the signed old-A(32-16) times the signed
     * coefficient (the multiplier is always signed), doubled
     * under FRCT as in the multiply core. */
    const int64_t sum = widen_word(x_word, input->status_sxm) +
                        widen_word(y_word, input->status_sxm);
    const uint64_t a_staged =
        ((uint64_t)(sum << 16) & (uint64_t)TI_C54X_U40_MASK);
    const uint8_t a_guard = (uint8_t)((a_staged >> 32U) & UINT64_C(0xFF));
    const uint8_t a_bit31 = (uint8_t)((a_staged >> 31U) & UINT64_C(1));
    const bool a_overflow =
        a_guard != (a_bit31 != UINT8_C(0) ? UINT8_C(0xFF)
                                          : UINT8_C(0));
    const int32_t a_hi = signed16(
        (uint16_t)((input->accumulator_a_raw40 >> 16U) & UINT64_C(0xFFFF)));
    int64_t product = (int64_t)a_hi * (int64_t)signed16(coef_word);
    ti_c54x_u40_t b_old = (ti_c54x_u40_t)(input->accumulator_b_raw40 &
                                          (uint64_t)TI_C54X_U40_MASK);
    ti_c54x_u40_add_result b_add;
    ti_c54x_op_effect staged = {0};
    uint64_t a_value = a_staged;
    uint64_t b_value;
    bool b_overflow;
    ti_c54x_op_status status;

    (void)operands;
    status = validate_inputs(operands, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (input->status_frct != UINT8_C(0)) {
        product *= INT64_C(2);
    }
    if (ti_c54x_u40_add_checked(
            b_old,
            (ti_c54x_u40_t)((uint64_t)product &
                            (uint64_t)TI_C54X_U40_MASK),
            &b_add) != TI_C54X_ARITHMETIC40_ADDSUB_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (a_overflow && input->status_ovm != UINT8_C(0)) {
        /* Saturate by the pre-shift sum sign: the shift can
         * carry the sign out of bit 31 (LD-shift precedent). */
        a_value = sum < INT64_C(0) ? UINT64_C(0xff80000000)
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

static ti_c54x_op_status complete_firs(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word,
    uint16_t y_word, uint16_t coef_word, ti_c54x_op_effect *next_effect);

static ti_c54x_op_status stage_memory(
    const ti_c54x_firs_dual_operands *operands,
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
                .kind = TI_C54X_OP_MEMORY_ADDRESS_DUAL_PROGRAM_TRIPLE_WORD16,
                .dual_program = {.x_mode = operands->x_mode,
                                 .x_ar = operands->x_ar,
                                 .y_mode = operands->y_mode,
                                 .y_ar = operands->y_ar,
                                 .program_pmad = operands->pmad,
                                 .reserved_half = UINT16_C(0),
                                 .reserved_word1 = UINT32_C(0),
                                 .reserved_word2 = UINT32_C(0),
                                 .reserved_word3 = UINT32_C(0)}},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = UINT16_C(0),
            .rmw_operation =
                (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
            .transfer_reserved = UINT8_C(0),
            .complete_triple = complete_firs}};

    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status complete_firs(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word,
    uint16_t y_word, uint16_t coef_word, ti_c54x_op_effect *next_effect)
{
    ti_c54x_firs_dual_operands operands;
    ti_c54x_op_status status = validate_metadata(instruction, &operands);

    if (status != TI_C54X_OP_STATUS_OK || next_effect == NULL ||
        input == NULL) {
        return status != TI_C54X_OP_STATUS_OK
                   ? status
                   : TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    return complete_values(&operands, input, x_word, y_word, coef_word,
                           next_effect);
}

static ti_c54x_op_status checked_common(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_firs_dual_operands operands;
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
    return stage_memory(&operands, effect);
}

ti_c54x_op_status ti_c54x_op_firs_xmem_ymem_pmad(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return checked_common(instruction, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_firs_xmem_ymem_pmad(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_firs_dual_operands decoded;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = validate_trusted(operands, &decoded);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = validate_inputs(&decoded, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return stage_memory(&decoded, effect);
}
