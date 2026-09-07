/*
 * Module contract
 *
 * Owns: exhaustive RED proof of pure fixed-MOD2 WORD16 STL memory effects,
 * including accumulator/ARF selection, low-word extraction, trusted
 * equivalence, and transactional rejection. Does not own: catalog rows,
 * operand-layout descriptors, address resolution, buses, CPU/state mutation,
 * sequencing, timing, observers, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/store/op_stl_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t
stl_word(uint8_t source, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x8090) |
                      ((uint16_t)(source & UINT8_C(1)) << 8U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_decoded_instruction
decode(uint8_t source, uint8_t arf)
{
    const uint16_t word = stl_word(source, arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_STL_SMEM_MOD2);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint64_t a, uint64_t b, bool a_known, bool b_known)
{
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = a_known,
        .accumulator_b_known = b_known,
        .status_sxm = UINT8_C(0xFF),
        .status_sxm_known = false};
    return input;
}

static void
assert_memory_write(const ti_c54x_op_effect *effect, uint8_t arf,
                    uint16_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD2);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.write_value_word == value);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t source, uint8_t arf)
{
    ti_c54x_trusted_operand_values operands = {0};
    operands.count = UINT8_C(2);
    operands.values[0].raw_value = source;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = arf;
    operands.values[1].width_bits = UINT8_C(3);
    return operands;
}

static void
check_valid_forms_and_low_word(void)
{
    static const uint64_t values[] = {
        UINT64_C(0x0000000000), UINT64_C(0x0000000001),
        UINT64_C(0x000000007F), UINT64_C(0x0000000080),
        UINT64_C(0x00000000FFFF), UINT64_C(0x000000010000),
        UINT64_C(0xFFFFFFFFFF)};

    for (uint8_t source = 0U; source <= UINT8_C(1); ++source) {
        for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
            const ti_c54x_decoded_instruction instruction = decode(source, arf);
            for (size_t value_index = 0U;
                 value_index < sizeof(values) / sizeof(values[0]);
                 ++value_index) {
                const uint64_t selected = values[value_index];
                const uint64_t other =
                    values[(value_index + 2U) % (sizeof(values) / sizeof(values[0]))];
                const ti_c54x_op_input_snapshot input = input_for(
                    source == 0U ? selected : other,
                    source == 1U ? selected : other, true, true);
                const ti_c54x_trusted_operand_values operands =
                    trusted_operands(source, arf);
                ti_c54x_op_effect direct = {0};
                ti_c54x_op_effect trusted = {0};

                TI_C54X_TEST_CHECK(ti_c54x_op_stl_smem(
                                       &instruction, &input, &direct) ==
                                   TI_C54X_OP_STATUS_OK);
                assert_memory_write(&direct, arf, (uint16_t)selected);
                TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stl_smem(
                                       &instruction, &operands, &input,
                                       &trusted) == TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) ==
                                   0);
            }
        }
    }
}

static void
check_unknown_and_malformed_input_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(0U, 0U);
    const ti_c54x_op_input_snapshot selected_unknown =
        input_for(UINT64_C(1), UINT64_C(2), false, true);
    const ti_c54x_op_input_snapshot other_unknown =
        input_for(UINT64_C(1), UINT64_C(2), true, false);
    const ti_c54x_op_input_snapshot malformed = input_for(
        TI_C54X_U40_MASK + UINT64_C(1), UINT64_C(2), true, true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_stl_smem(NULL, &selected_unknown, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_stl_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_stl_smem(&instruction, &selected_unknown,
                                           NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_op_stl_smem(&instruction, &selected_unknown,
                                           &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_stl_smem(&instruction, &other_unknown,
                                           &effect) == TI_C54X_OP_STATUS_OK);
    assert_memory_write(&effect, 0U, UINT16_C(1));
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_stl_smem(&instruction, &malformed, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        const ti_c54x_op_input_snapshot valid =
            input_for(UINT64_C(1), UINT64_C(2), true, true);
        ti_c54x_decoded_instruction forged = instruction;
        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_stl_smem(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_stl_smem(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_STATUS_SXM;
        TI_C54X_TEST_CHECK(ti_c54x_op_stl_smem(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0040);
        TI_C54X_TEST_CHECK(ti_c54x_op_stl_smem(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int
main(void)
{
    check_valid_forms_and_low_word();
    check_unknown_and_malformed_input_is_transactional();
    return 0;
}
