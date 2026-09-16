/*
 * Module contract
 *
 * Owns: RED proofs for the two-word `RPT #lk` repeat-start
 * effect, including lk extraction over extension corners,
 * checked/trusted/dispatch parity, ignored input state, and
 * transactional rejection. Does not own: catalog rows,
 * operand-layout descriptors, RC state, buses, CPU or
 * architectural state, sequencing, timing, observers,
 * diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/repeat/op_rpt_lk.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction decode(uint16_t immediate)
{
    const uint16_t first_word = UINT16_C(0xF070);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, immediate},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_RPT_LK);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_RPT_LK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == immediate);
    return instruction;
}

static ti_c54x_op_input_snapshot ignored_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_b_raw40 = UINT64_C(0x0FEDCBA987);
    input.accumulator_a_known = true;
    input.accumulator_b_known = false;
    input.auxiliary_registers_raw16[3] = UINT16_C(0xAAAA);
    input.auxiliary_registers_known_mask = UINT8_C(0);
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(uint16_t immediate)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(1);
    operands.values[0].raw_value = immediate;
    operands.values[0].width_bits = UINT8_C(16);
    return operands;
}

static void assert_repeat_start(const ti_c54x_op_effect *effect,
                                uint16_t operand)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_REPEAT_START);
    TI_C54X_TEST_CHECK(effect->payload.repeat_start.operand == operand);
}

static void check_case(uint16_t immediate)
{
    const ti_c54x_decoded_instruction instruction = decode(immediate);
    const ti_c54x_op_input_snapshot input = ignored_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(immediate);
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect dispatched = {0};
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_RPT_LK);

    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_lk(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_OK);
    assert_repeat_start(&checked, immediate);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_rpt_lk(&instruction, &operands,
                                                &input, &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_rpt_lk);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_opcode_binding_is_canonical(
                           binding));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&dispatched, &checked, sizeof(checked)) == 0);
}

static void test_cases(void)
{
    /* Zero, the worked #1111 example, and both range edges. */
    check_case(UINT16_C(0x0000));
    check_case(UINT16_C(0x1111));
    check_case(UINT16_C(0x7FFF));
    check_case(UINT16_C(0x8000));
    check_case(UINT16_C(0xFFFF));
}

static void test_null_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT16_C(0x1111));
    const ti_c54x_op_input_snapshot input = ignored_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT16_C(0x1111));
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_lk(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_lk(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_lk(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_rpt_lk(&instruction, NULL, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_rpt_lk(&instruction, &operands,
                                                &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_cases();
    test_null_rejects();
    return 0;
}
