/*
 * Module contract
 *
 * Owns: RED proofs for the MOD2 WORD16 LD Smem,TS,dst semantic effect,
 * including old-AR memory staging, SXM widening, signed T[5:0] shifting,
 * destination routing, checked/trusted/dispatch parity, and transactional
 * input/decode rejection. Does not own: catalog rows, operand-layout
 * descriptors, address resolution or mutation, buses, CPU/state mutation,
 * sequencing, timing, observers, diagnostics, or production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/load/op_ld_smem.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t ld_ts_word(uint8_t destination, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x1490) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_decoded_instruction decode(uint8_t destination, uint8_t arf)
{
    const uint16_t word = ld_ts_word(destination, arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_SMEM_TS);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_SMEM_TS);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER);
    return instruction;
}

static ti_c54x_op_input_snapshot input_for(uint8_t sxm, bool sxm_known,
                                           uint16_t t_register,
                                           bool t_known, uint8_t seed)
{
    ti_c54x_op_input_snapshot input = {0};

    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    input.t_register_raw16 = t_register;
    input.t_register_known = t_known;
    input.status_c = (uint8_t)(seed & UINT8_C(1));
    input.status_c_known = (seed & UINT8_C(2)) != UINT8_C(0);
    input.status_tc = (uint8_t)((seed >> 2U) & UINT8_C(1));
    input.status_tc_known = (seed & UINT8_C(8)) != UINT8_C(0);
    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_b_raw40 = UINT64_C(0x2ABCDEF012);
    input.accumulator_a_known = true;
    input.accumulator_b_known = true;
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t destination,
                                                       uint8_t arf)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = destination;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = arf;
    operands.values[1].width_bits = UINT8_C(3);
    operands.values[2].raw_value = TI_C54X_INDIRECT_SINGLE_MOD2;
    operands.values[2].width_bits = UINT8_C(2);
    return operands;
}

static void assert_memory_read(const ti_c54x_op_effect *effect, uint8_t arf)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD2);
    /* MOD2 completion receives the old selected-AR address. */
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
}

static uint64_t expected_value(uint16_t read_value, uint8_t sxm,
                               uint16_t t_register)
{
    uint64_t value = (uint64_t)read_value;
    const uint8_t shift_bits = (uint8_t)(t_register & UINT16_C(0x3F));

    if (sxm != UINT8_C(0) &&
        (read_value & UINT16_C(0x8000)) != UINT16_C(0)) {
        value = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) | value;
    }
    if (shift_bits <= UINT8_C(31)) {
        return ti_c54x_u40_logical_left_shift(value, shift_bits);
    }
    return ti_c54x_u40_arithmetic_right_shift(value,
                                               UINT8_C(64) - shift_bits);
}

static void assert_accumulator_write(const ti_c54x_op_effect *effect,
                                     uint8_t destination, uint64_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK_U64(effect->payload.accumulator_write.value_raw40,
                           value);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
}

static void check_forms_and_signed_shift_completion(void)
{
    static const uint16_t reads[] = {
        UINT16_C(0x0000), UINT16_C(0x7FFF), UINT16_C(0x8000),
        UINT16_C(0xFEDC), UINT16_C(0xFFFF)};
    static const uint16_t t_values[] = {
        UINT16_C(0x0000), UINT16_C(0x0001), UINT16_C(0x0008),
        UINT16_C(0x001F),
        UINT16_C(0x0030), UINT16_C(0x003F), UINT16_C(0xA508)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1); ++destination) {
        for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
            const ti_c54x_decoded_instruction instruction =
                decode(destination, arf);
            const ti_c54x_trusted_opcode_binding *binding =
                ti_c54x_trusted_opcode_binding_for_id(
                    TI_C54X_OPCODE_ID_LD_SMEM_TS);
            ti_c54x_trusted_execution_packet packet;

            TI_C54X_TEST_CHECK(binding != NULL);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                   binding, &instruction.bundle, &packet) ==
                               TI_C54X_TRUSTED_EXECUTION_PACKET_OK);

            for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
                for (size_t t_index = 0U;
                     t_index < sizeof(t_values) / sizeof(t_values[0]);
                     ++t_index) {
                    const ti_c54x_op_input_snapshot input =
                        input_for(sxm, true, t_values[t_index], true,
                                  (uint8_t)(t_index + sxm));
                    const ti_c54x_trusted_operand_values operands =
                        trusted_operands(destination, arf);
                    ti_c54x_op_effect direct = {
                        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
                    ti_c54x_op_effect checked = direct;
                    ti_c54x_op_effect trusted = direct;
                    ti_c54x_op_effect trusted_direct = direct;

                    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_ts(
                                           &instruction, &input, &direct) ==
                                       TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(
                        ti_c54x_dispatch_decoded_with_input_snapshot(
                            &instruction, &input, &checked) ==
                        TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                           &packet, &input, &trusted) ==
                                       TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_smem_ts(
                                           &packet.instruction, &operands,
                                           &input, &trusted_direct) ==
                                       TI_C54X_OP_STATUS_OK);
                    assert_memory_read(&direct, arf);
                    TI_C54X_TEST_CHECK(memcmp(&direct, &checked,
                                              sizeof(direct)) == 0);
                    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted,
                                              sizeof(direct)) == 0);
                    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_direct,
                                              sizeof(direct)) == 0);

                    for (size_t read_index = 0U;
                         read_index < sizeof(reads) / sizeof(reads[0]);
                         ++read_index) {
                        ti_c54x_op_effect completed = {
                            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                            .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};

                        TI_C54X_TEST_CHECK(direct.payload.memory.complete(
                                               &instruction, &input,
                                               reads[read_index], &completed) ==
                                           TI_C54X_OP_STATUS_OK);
                        assert_accumulator_write(
                            &completed, destination,
                            expected_value(reads[read_index], sxm,
                                           t_values[t_index]));
                    }
                }
            }
        }
    }
}

static void check_pinned_vectors(void)
{
    TI_C54X_TEST_CHECK_U64(
        expected_value(UINT16_C(0xFEDC), UINT8_C(1), UINT16_C(8)),
        UINT64_C(0xFFFFFEDC00));
    TI_C54X_TEST_CHECK_U64(
        expected_value(UINT16_C(0x8000), UINT8_C(0), UINT16_C(0x30)),
        UINT64_C(0x0000000000));
    TI_C54X_TEST_CHECK_U64(
        expected_value(UINT16_C(0x8000), UINT8_C(1), UINT16_C(0x30)),
        UINT64_C(0xFFFFFFFFFF));
    TI_C54X_TEST_CHECK_U64(
        expected_value(UINT16_C(0x8000), UINT8_C(0), UINT16_C(0x3F)),
        UINT64_C(0x0000004000));
    TI_C54X_TEST_CHECK_U64(
        expected_value(UINT16_C(0x8000), UINT8_C(1), UINT16_C(0x3F)),
        UINT64_C(0xFFFFFFC000));
    TI_C54X_TEST_CHECK_U64(
        expected_value(UINT16_C(0x7FFF), UINT8_C(0), UINT16_C(0x1F)),
        UINT64_C(0xFF80000000));
}

static void check_unknown_and_malformed_input_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(0U, 0U);
    const ti_c54x_op_input_snapshot unknown_sxm =
        input_for(0U, false, UINT16_C(8), true, 0U);
    const ti_c54x_op_input_snapshot unknown_t =
        input_for(0U, true, UINT16_C(8), false, 0U);
    const ti_c54x_op_input_snapshot malformed_sxm =
        input_for(2U, true, UINT16_C(8), true, 0U);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_ts(NULL, &unknown_t, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_ts(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_ts(&instruction, &unknown_t, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_ts(&instruction, &unknown_sxm,
                                             &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_ts(&instruction, &unknown_t, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_ts(&instruction, &malformed_sxm,
                                             &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;
        const ti_c54x_op_input_snapshot valid =
            input_for(0U, true, UINT16_C(8), true, 0U);

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_ts(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_ts(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_STATUS_SXM;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_ts(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0020);
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_ts(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void check_completion_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(1U, 7U);
    const ti_c54x_op_input_snapshot valid =
        input_for(1U, true, UINT16_C(0x30), true, 0U);
    const ti_c54x_op_input_snapshot unknown =
        input_for(0U, true, UINT16_C(0x30), false, 0U);
    const ti_c54x_op_input_snapshot malformed =
        input_for(3U, true, UINT16_C(0x30), true, 0U);
    const ti_c54x_op_input_snapshot unsupported_t =
        input_for(1U, true, UINT16_C(0x20), true, 0U);
    ti_c54x_op_effect read_effect = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x1234)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_ts(&instruction, &valid,
                                             &read_effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           NULL, &valid, UINT16_C(0xFEDC), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, NULL, UINT16_C(0xFEDC), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, &valid, UINT16_C(0xFEDC), NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, &unknown, UINT16_C(0xFEDC), &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, &malformed, UINT16_C(0xFEDC), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, &unsupported_t, UINT16_C(0xFEDC),
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                               &forged, &valid, UINT16_C(0xFEDC), &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0020);
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                               &forged, &valid, UINT16_C(0xFEDC), &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int main(void)
{
    check_pinned_vectors();
    check_forms_and_signed_shift_completion();
    check_unknown_and_malformed_input_is_transactional();
    check_completion_rejection_is_transactional();
    return 0;
}
