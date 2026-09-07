/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the fixed-MOD2 WORD16 LD Smem,16,dst
 * effect, including D/ARF selection, SXM-controlled sign/zero extension and
 * logical left shift, checked/trusted/dispatch parity, and transactional
 * rejection. Does not own: catalog rows, operand-layout descriptors, address
 * resolution, buses, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or the production implementation.
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

static uint16_t ld_word(uint8_t destination, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x4490) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_decoded_instruction decode(uint8_t destination, uint8_t arf)
{
    const uint16_t word = ld_word(destination, arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD2);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id ==
                       TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD2);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM);
    return instruction;
}

static ti_c54x_op_input_snapshot input_for(uint8_t sxm, bool sxm_known,
                                           uint8_t seed)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_b_raw40 = UINT64_C(0x2ABCDEF012);
    input.accumulator_a_known = (seed & UINT8_C(1)) != 0U;
    input.accumulator_b_known = (seed & UINT8_C(2)) != 0U;
    input.status_c = (uint8_t)(seed & UINT8_C(1));
    input.status_c_known = (seed & UINT8_C(4)) != 0U;
    input.status_tc = (uint8_t)((seed >> 1U) & UINT8_C(1));
    input.status_tc_known = (seed & UINT8_C(8)) != 0U;
    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    for (uint8_t index = 0U; index < UINT8_C(8); ++index) {
        input.auxiliary_registers_raw16[index] =
            (uint16_t)(UINT16_C(0xA500) | index);
    }
    input.auxiliary_registers_known_mask = seed;
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t destination,
                                                       uint8_t arf)
{
    ti_c54x_trusted_operand_values operands = {0};
    operands.count = UINT8_C(2);
    operands.values[0].raw_value = destination;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = arf;
    operands.values[1].width_bits = UINT8_C(3);
    return operands;
}

static void assert_memory_read(const ti_c54x_op_effect *effect, uint8_t arf)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD2);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.write_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
}

static uint64_t expected_value(uint16_t read_value, uint8_t sxm)
{
    uint64_t value = (uint64_t)read_value;

    if (sxm != UINT8_C(0) && (read_value & UINT16_C(0x8000)) != UINT16_C(0)) {
        value = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) | value;
    }
    return (value << 16U) & TI_C54X_U40_MASK;
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

static void check_metadata_and_exact_forms(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(
            TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD2);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id ==
                       TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD2);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0x4490));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFEF8));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM);

    for (uint8_t destination = 0U; destination <= UINT8_C(1); ++destination) {
        for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
            const ti_c54x_decoded_instruction instruction =
                decode(destination, arf);
            TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                               ld_word(destination, arf));
        }
    }
}

static void check_effects_and_parity(void)
{
    static const uint16_t reads[] = {UINT16_C(0x0000), UINT16_C(0x7FFF),
                                     UINT16_C(0x8000), UINT16_C(0xFFFF),
                                     UINT16_C(0xFEDC)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1); ++destination) {
        for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
            const ti_c54x_decoded_instruction instruction =
                decode(destination, arf);
            const ti_c54x_trusted_opcode_binding *binding =
                ti_c54x_trusted_opcode_binding_for_id(
                    TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD2);
            ti_c54x_trusted_execution_packet packet;

            TI_C54X_TEST_CHECK(binding != NULL);
            TI_C54X_TEST_CHECK(binding->handler ==
                               ti_c54x_trusted_op_ld_smem_shift16);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                   binding, &instruction.bundle, &packet) ==
                               TI_C54X_TRUSTED_EXECUTION_PACKET_OK);

            for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
                ti_c54x_op_effect baseline_read = {0};
                ti_c54x_op_effect baseline_completed[5] = {{0}};
                for (uint8_t seed = 0U; seed < UINT8_C(16); ++seed) {
                    const ti_c54x_op_input_snapshot input =
                        input_for(sxm, true, seed);
                    const ti_c54x_trusted_operand_values operands =
                        trusted_operands(destination, arf);
                    const ti_c54x_op_effect before = {
                        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
                    ti_c54x_op_effect direct = before;
                    ti_c54x_op_effect checked = before;
                    ti_c54x_op_effect trusted = before;
                    ti_c54x_op_effect trusted_direct = before;

                    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(
                                           &instruction, &input, &direct) ==
                                       TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(
                        ti_c54x_dispatch_decoded_with_input_snapshot(
                            &instruction, &input, &checked) ==
                        TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                           &packet, &input, &trusted) ==
                                       TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_smem_shift16(
                                           &packet.instruction, &operands,
                                           &input, &trusted_direct) ==
                                       TI_C54X_OP_STATUS_OK);
                    assert_memory_read(&direct, arf);
                    if (seed == UINT8_C(0)) {
                        baseline_read = direct;
                    } else {
                        TI_C54X_TEST_CHECK(memcmp(&direct, &baseline_read,
                                                  sizeof(direct)) == 0);
                    }
                    TI_C54X_TEST_CHECK(memcmp(&direct, &checked,
                                              sizeof(direct)) == 0);
                    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted,
                                              sizeof(direct)) == 0);
                    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_direct,
                                              sizeof(direct)) == 0);

                    for (size_t index = 0U;
                         index < sizeof(reads) / sizeof(reads[0]); ++index) {
                        ti_c54x_op_effect completed = before;

                        TI_C54X_TEST_CHECK(direct.payload.memory.complete(
                                               &instruction, &input, reads[index],
                                               &completed) ==
                                           TI_C54X_OP_STATUS_OK);
                        assert_accumulator_write(
                            &completed, destination,
                            expected_value(reads[index], sxm));
                        if (seed == UINT8_C(0)) {
                            baseline_completed[index] = completed;
                        } else {
                            TI_C54X_TEST_CHECK(
                                memcmp(&completed, &baseline_completed[index],
                                       sizeof(completed)) == 0);
                        }
                    }
                }
            }
        }
    }
}

static void check_input_and_decode_rejection(void)
{
    const ti_c54x_decoded_instruction instruction = decode(1U, 7U);
    const ti_c54x_op_input_snapshot valid = input_for(1U, true, UINT8_C(0xA5));
    const ti_c54x_op_input_snapshot unknown = input_for(0U, false, 0U);
    const ti_c54x_op_input_snapshot malformed = input_for(2U, true, 0U);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(&instruction, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(&instruction, &unknown,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(&instruction, &malformed,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        const ti_c54x_decoded_instruction *original = &instruction;
        ti_c54x_decoded_instruction forged;

        forged = *original;
        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = *original;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = *original;
        forged.repeatability = TI_C54X_REPEATABILITY_INELIGIBLE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = *original;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = *original;
        forged.bundle.program_word_count = UINT8_C(2);
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = *original;
        forged.bundle.program_words[0] ^= UINT16_C(0x0008);
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void check_trusted_operand_rejection(void)
{
    const ti_c54x_decoded_instruction instruction = decode(1U, 7U);
    const ti_c54x_op_input_snapshot input = input_for(0U, true, 0U);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD2);
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands = trusted_operands(1U, 7U);

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_smem_shift16(
                           &packet.instruction, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_smem_shift16(
                           &packet.instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_smem_shift16(
                           &packet.instruction, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_smem_shift16(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(1U, 7U);
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_smem_shift16(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(1U, 7U);
    operands.values[1].raw_value = UINT64_C(8);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_smem_shift16(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void check_completion_rejection(void)
{
    const ti_c54x_decoded_instruction instruction = decode(0U, 0U);
    const ti_c54x_op_input_snapshot valid = input_for(1U, true, 0U);
    const ti_c54x_op_input_snapshot unknown = input_for(0U, false, 0U);
    const ti_c54x_op_input_snapshot malformed = input_for(2U, true, 0U);
    ti_c54x_op_effect read_effect = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x1234)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_shift16(&instruction, &valid,
                                                  &read_effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           NULL, &valid, UINT16_C(0xFFFF), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, NULL, UINT16_C(0xFFFF), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, &valid, UINT16_C(0xFFFF), NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, &unknown, UINT16_C(0xFFFF), &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, &malformed, UINT16_C(0xFFFF), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                               &forged, &valid, UINT16_C(0xFFFF), &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0008);
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                               &forged, &valid, UINT16_C(0xFFFF), &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int main(void)
{
    check_metadata_and_exact_forms();
    check_effects_and_parity();
    check_input_and_decode_rejection();
    check_trusted_operand_rejection();
    check_completion_rejection();
    return 0;
}
