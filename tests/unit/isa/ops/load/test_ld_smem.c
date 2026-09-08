/*
 * Module contract
 *
 * Owns: exhaustive RED proof of pure MOD2 WORD16 LD memory effects,
 * including D/ARF selection, SXM completion, trusted equivalence, and
 * transactional rejection. Does not own: catalog rows, operand-layout
 * descriptors, address resolution, buses, CPU/state mutation, sequencing,
 * timing, observers, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/load/op_ld_smem.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
ld_word(uint8_t destination, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x1090) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_decoded_instruction
decode(uint8_t destination, uint8_t arf)
{
    const uint16_t word = ld_word(destination, arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_SMEM);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint8_t sxm, bool sxm_known)
{
    ti_c54x_op_input_snapshot input = {0};
    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    return input;
}

static uint64_t
expected_value(uint16_t read_value, uint8_t sxm)
{
    if (sxm == UINT8_C(0) || (read_value & UINT16_C(0x8000)) == 0U) {
        return (uint64_t)read_value;
    }
    return (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) | (uint64_t)read_value;
}

static void
assert_memory_read(const ti_c54x_op_effect *effect, uint8_t arf)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD2);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.write_value_word ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
}

static void
assert_accumulator_write(const ti_c54x_op_effect *effect, uint8_t destination,
                         uint64_t value)
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

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t destination, uint8_t arf)
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

static void
check_valid_forms_and_completion(void)
{
    static const uint16_t reads[] = {UINT16_C(0x0000), UINT16_C(0x0001),
                                     UINT16_C(0x7FFF), UINT16_C(0x8000),
                                     UINT16_C(0xFFFF)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1); ++destination) {
        for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
            const ti_c54x_decoded_instruction instruction =
                decode(destination, arf);
            const ti_c54x_op_input_snapshot input = input_for(0U, true);

            for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
                const ti_c54x_op_input_snapshot sxm_input =
                    input_for(sxm, true);
                ti_c54x_op_effect direct = {0};
                ti_c54x_op_effect trusted = {0};
                const ti_c54x_trusted_operand_values operands =
                    trusted_operands(destination, arf);

                TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem(
                                       &instruction, &sxm_input, &direct) ==
                                   TI_C54X_OP_STATUS_OK);
                assert_memory_read(&direct, arf);
                TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_smem(
                                       &instruction, &operands, &sxm_input,
                                       &trusted) == TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) ==
                                   0);

                for (size_t read_index = 0U;
                     read_index < sizeof(reads) / sizeof(reads[0]);
                     ++read_index) {
                    ti_c54x_op_effect completed = {
                        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
                    TI_C54X_TEST_CHECK(direct.payload.memory.complete(
                                           &instruction, &sxm_input,
                                           reads[read_index], &completed) ==
                                       TI_C54X_OP_STATUS_OK);
                    assert_accumulator_write(
                        &completed, destination,
                        expected_value(reads[read_index], sxm));
                }
            }

            (void)input;
        }
    }
}

static void
check_unknown_and_malformed_input_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(0U, 0U);
    const ti_c54x_op_input_snapshot unknown = input_for(0U, false);
    const ti_c54x_op_input_snapshot malformed = input_for(2U, true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;
    unsigned char before_bytes[sizeof(effect)];
    (void)memcpy(before_bytes, &before, sizeof(before_bytes));

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem(NULL, &unknown, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem(&instruction, &unknown, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, before_bytes, sizeof(effect)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem(&instruction, &unknown, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem(&instruction, &malformed, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;
        const ti_c54x_op_input_snapshot valid = input_for(0U, true);
        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_ACCUMULATORS;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0040);
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void
check_completion_rejects_bad_inputs_transactionally(void)
{
    const ti_c54x_decoded_instruction instruction = decode(1U, 7U);
    const ti_c54x_op_input_snapshot valid = input_for(1U, true);
    const ti_c54x_op_input_snapshot unknown = input_for(0U, false);
    const ti_c54x_op_input_snapshot malformed = input_for(3U, true);
    ti_c54x_op_effect read_effect = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem(&instruction, &valid, &read_effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           NULL, &valid, UINT16_C(0x8000), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, NULL, UINT16_C(0x8000), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, &valid, UINT16_C(0x8000), NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, &unknown, UINT16_C(0x8000), &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, &malformed, UINT16_C(0x8000), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    check_valid_forms_and_completion();
    check_unknown_and_malformed_input_is_transactional();
    check_completion_rejects_bad_inputs_transactionally();
    return 0;
}
