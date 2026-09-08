/*
 * Module contract
 *
 * Owns: pure checked/trusted DST LONG32 MOD13 semantic proofs.
 * Does not own: catalog tables, address resolution, CPU sequencing, buses,
 * or architectural state.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/store/op_dst_long32.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t dst_word(uint8_t source, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x4EE8) |
                      ((uint16_t)(source & 1U) << 8U) |
                      (uint16_t)(arf & 7U));
}

static ti_c54x_decoded_instruction decode(uint8_t source, uint8_t arf,
                                          uint16_t extension)
{
    const uint16_t word = dst_word(source, arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, extension}, .program_word_count = 2U};
    ti_c54x_decoded_instruction instruction;
    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_DST_LONG32_MOD13);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static ti_c54x_op_input_snapshot input_for(uint64_t a, uint64_t b,
                                           uint8_t sxm, uint8_t sst)
{
    ti_c54x_op_input_snapshot input = {0};
    input.accumulator_a_raw40 = a;
    input.accumulator_b_raw40 = b;
    input.accumulator_a_known = true;
    input.accumulator_b_known = true;
    input.status_sxm = sxm;
    input.status_sxm_known = true;
    input.status_sst = sst;
    input.status_sst_known = true;
    return input;
}

static ti_c54x_trusted_operand_values operands_for(uint8_t source, uint8_t arf,
                                                   uint16_t lk)
{
    ti_c54x_trusted_operand_values operands = {0};
    operands.count = 4U;
    operands.values[0] = (ti_c54x_operand_field_value){source, 1U};
    operands.values[1] = (ti_c54x_operand_field_value){arf, 3U};
    operands.values[2] = (ti_c54x_operand_field_value){lk, 16U};
    operands.values[3] = (ti_c54x_operand_field_value){13U, 4U};
    return operands;
}

static uint32_t expected_store(uint64_t raw, uint8_t sxm, uint8_t sst)
{
    if (sst == 0U) {
        return (uint32_t)raw;
    }
    if (sxm == 0U) {
        return raw > UINT64_C(0xFFFFFFFF) ? UINT32_MAX : (uint32_t)raw;
    }
    {
        const int64_t signed_raw = ti_c54x_u40_sign_extend(raw);
        if (signed_raw > INT64_C(0x7FFFFFFF)) {
            return UINT32_C(0x7FFFFFFF);
        }
        if (signed_raw < -INT64_C(0x80000000)) {
            return UINT32_C(0x80000000);
        }
        return (uint32_t)raw;
    }
}

static void check_forms_and_saturation(void)
{
    static const uint64_t values[] = {0U, 1U, UINT64_C(0x7FFFFFFF),
                                      UINT64_C(0x80000000),
                                      UINT64_C(0xFFFFFFFF),
                                      UINT64_C(0x100000000),
                                      UINT64_C(0xFF7FFFFFFF),
                                      UINT64_C(0xFF80000000),
                                      UINT64_C(0x7FFFFFFFFF),
                                      UINT64_C(0x8000000000),
                                      UINT64_C(0xFFFFFFFFFF)};
    for (uint8_t source = 0U; source <= 1U; ++source) {
        for (uint8_t arf = 0U; arf < 8U; ++arf) {
            const uint16_t lk = (uint16_t)(0x2000U + arf);
            const ti_c54x_decoded_instruction instruction = decode(source, arf, lk);
            const ti_c54x_trusted_operand_values operands =
                operands_for(source, arf, lk);
            for (uint8_t sxm = 0U; sxm <= 1U; ++sxm) {
                for (uint8_t sst = 0U; sst <= 1U; ++sst) {
                    for (size_t index = 0U; index < sizeof(values) / sizeof(values[0]);
                         ++index) {
                        const uint64_t selected = values[index];
                        const ti_c54x_op_input_snapshot input = input_for(
                            source == 0U ? selected : 1U,
                            source == 1U ? selected : 2U, sxm, sst);
                        ti_c54x_op_effect direct = {0};
                        ti_c54x_op_effect trusted = {0};
                        const uint32_t expected = expected_store(selected, sxm, sst);
                        TI_C54X_TEST_CHECK(ti_c54x_op_dst_long32_mod13(
                                               &instruction, &input, &direct) ==
                                           TI_C54X_OP_STATUS_OK);
                        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dst_long32_mod13(
                                               &instruction, &operands, &input,
                                               &trusted) == TI_C54X_OP_STATUS_OK);
                        TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) == 0);
                        TI_C54X_TEST_CHECK(direct.kind == TI_C54X_OP_EFFECT_MEMORY);
                        TI_C54X_TEST_CHECK(direct.payload.memory.address_spec.kind ==
                                           TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32);
                        TI_C54X_TEST_CHECK(direct.payload.memory.address_spec.arf == arf);
                        TI_C54X_TEST_CHECK(direct.payload.memory.address_spec.long_offset_word == lk);
                        TI_C54X_TEST_CHECK(direct.payload.memory.access ==
                                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
                        TI_C54X_TEST_CHECK(direct.payload.memory.first_value_msw ==
                                           (uint16_t)(expected >> 16U));
                        TI_C54X_TEST_CHECK(direct.payload.memory.second_value_lsw ==
                                           (uint16_t)expected);
                        TI_C54X_TEST_CHECK(direct.payload.memory.complete_long32 == NULL);
                        TI_C54X_TEST_CHECK(input.accumulator_a_raw40 ==
                                           (source == 0U ? selected : 1U));
                        TI_C54X_TEST_CHECK(input.accumulator_b_raw40 ==
                                           (source == 1U ? selected : 2U));
                    }
                }
            }
        }
    }
}

static void check_unknown_and_malformed_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(0U, 0U, 0x4321U);
    const ti_c54x_trusted_operand_values operands = operands_for(0U, 0U, 0x4321U);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = 0xCAFEU}};
    ti_c54x_op_effect effect = before;
    ti_c54x_op_input_snapshot input = input_for(1U, 2U, 0U, 0U);

    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dst_long32_mod13(
                           NULL, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    input.status_sst_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_dst_long32_mod13(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    input = input_for(1U, 2U, 2U, 0U);
    TI_C54X_TEST_CHECK(ti_c54x_op_dst_long32_mod13(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    input = input_for(1U, 2U, 0U, 2U);
    TI_C54X_TEST_CHECK(ti_c54x_op_dst_long32_mod13(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    input = input_for(1U, 2U, 0U, 0U);
    input.status_sxm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_dst_long32_mod13(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    input = input_for(TI_C54X_U40_MASK + 1U, 2U, 0U, 0U);
    TI_C54X_TEST_CHECK(ti_c54x_op_dst_long32_mod13(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    input = input_for(1U, 2U, 0U, 0U);
    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_dst_long32_mod13(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    {
        ti_c54x_trusted_operand_values malformed = operands;
        malformed.values[3].raw_value = 12U;
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dst_long32_mod13(
                               &instruction, &malformed, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        malformed = operands;
        malformed.count = 5U;
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dst_long32_mod13(
                               &instruction, &malformed, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        malformed = operands;
        malformed.values[1].width_bits = 2U;
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dst_long32_mod13(
                               &instruction, &malformed, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.id = TI_C54X_OPCODE_ID_NOP;
        input = input_for(1U, 2U, 0U, 0U);
        TI_C54X_TEST_CHECK(ti_c54x_op_dst_long32_mod13(
                               &forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int main(void)
{
    check_forms_and_saturation();
    check_unknown_and_malformed_is_transactional();
    return 0;
}
