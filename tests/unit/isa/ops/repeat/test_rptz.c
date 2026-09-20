/*
 * Module contract
 *
 * Owns: RED proofs for the two-word `RPTZ dst,#lk` repeat start,
 * including D selection, lk extraction, the staged REPEAT_START
 * carrying RC plus the dst-zero, checked/trusted/dispatch
 * parity, and transactional rejection. Does not own: catalog
 * rows, operand-layout descriptors, RC or accumulator state,
 * buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, or the production semantic leaf.
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
#include "../../../../../src/isa/ops/repeat/op_rptz.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* TI SPRU172C section RPTZ: 1111 000D 0111 0001 plus the 16-bit
 * lk, with D selecting the cleared accumulator. */
static ti_c54x_decoded_instruction decode(uint16_t first_word, uint16_t lk)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, lk}, .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_RPTZ_DST_LK);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_RPTZ_DST_LK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == lk);
    return instruction;
}

static ti_c54x_op_input_snapshot ignored_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_b_raw40 = UINT64_C(0x0FEDCBA987);
    input.accumulator_a_known = true;
    input.accumulator_b_known = false;
    return input;
}

static void check_case(uint8_t destination, uint16_t lk)
{
    const uint16_t first_word =
        (uint16_t)(UINT16_C(0xF071) | ((uint16_t)destination << 8));
    const ti_c54x_decoded_instruction instruction =
        decode(first_word, lk);
    const ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect dispatched = {0};
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_RPTZ_DST_LK);

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = destination;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = lk;
    operands.values[1].width_bits = UINT8_C(16);

    TI_C54X_TEST_CHECK(ti_c54x_op_rptz(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(checked.kind == TI_C54X_OP_EFFECT_REPEAT_START);
    TI_C54X_TEST_CHECK(checked.payload.repeat_start.operand == lk);
    TI_C54X_TEST_CHECK(checked.payload.repeat_start.zero_destination);
    TI_C54X_TEST_CHECK(
        checked.payload.repeat_start.zero_destination_index == destination);

    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_rptz(&instruction, &operands,
                                              &input, &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_rptz);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_opcode_binding_is_canonical(
        binding));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&dispatched, &checked, sizeof(checked)) == 0);
}

static void test_cases(void)
{
    /* Both accumulators with the TI example lk plus corners. */
    check_case(UINT8_C(0), UINT16_C(1023));
    check_case(UINT8_C(1), UINT16_C(1023));
    check_case(UINT8_C(0), UINT16_C(0));
    check_case(UINT8_C(1), UINT16_C(0xFFFF));
}

static void test_null_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT16_C(0xF071), UINT16_C(0));
    const ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_rptz(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_rptz(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_rptz(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_rptz(&instruction, NULL, &input,
                                              &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_rptz(&instruction, &operands,
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
