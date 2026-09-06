/* Module contract
 * Owns: pure full-width MAX/MIN value-owned effect proof.
 * Does not own: state mutation, sequencing, CPU lifecycle, or timing.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/compare/op_compare.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../support/test_harness.h"

#include <string.h>
#include <stdint.h>

static ti_c54x_decoded_instruction decode(uint16_t word)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static void check(uint16_t word, uint64_t a, uint64_t b, uint64_t selected,
                  uint8_t c)
{
    const ti_c54x_decoded_instruction instruction = decode(word);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a, .accumulator_b_raw40 = b,
        .accumulator_a_known = true, .accumulator_b_known = true};
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect dispatched = {0};
    const ti_c54x_op_status status =
        (instruction.id == TI_C54X_OPCODE_ID_MAX_ACC
             ? ti_c54x_op_max_acc(&instruction, &input, &effect)
             : ti_c54x_op_min_acc(&instruction, &input, &effect));

    TI_C54X_TEST_CHECK(status == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       (uint8_t)((word >> 8U) & 1U));
    TI_C54X_TEST_CHECK_U64(effect.payload.accumulator_write.value_raw40,
                           selected);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.c_action ==
                       TI_C54X_OP_ACCUMULATOR_C_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.c_value == c);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(dispatched.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(dispatched.payload.accumulator_write.destination ==
                       (uint8_t)((word >> 8U) & 1U));
    TI_C54X_TEST_CHECK_U64(dispatched.payload.accumulator_write.value_raw40,
                           selected);
    TI_C54X_TEST_CHECK(dispatched.payload.accumulator_write.c_action ==
                       TI_C54X_OP_ACCUMULATOR_C_WRITE);
    TI_C54X_TEST_CHECK(dispatched.payload.accumulator_write.c_value == c);
}

int main(void)
{
    check(UINT16_C(0xF486), UINT64_C(5), UINT64_C(3), UINT64_C(5), 0U);
    check(UINT16_C(0xF486), UINT64_C(2), UINT64_C(3), UINT64_C(3), 1U);
    check(UINT16_C(0xF586), UINT64_C(2), UINT64_C(3), UINT64_C(3), 1U);
    check(UINT16_C(0xF487), UINT64_C(2), UINT64_C(3), UINT64_C(2), 0U);
    check(UINT16_C(0xF587), UINT64_C(2), UINT64_C(3), UINT64_C(2), 0U);
    check(UINT16_C(0xF486), UINT64_C(0x8000000000), UINT64_C(0x7FFFFFFFFF),
          UINT64_C(0x7FFFFFFFFF), 1U);
    check(UINT16_C(0xF487), UINT64_C(0x8000000000), UINT64_C(0x7FFFFFFFFF),
          UINT64_C(0x8000000000), 0U);
    check(UINT16_C(0xF486), UINT64_C(0xFFFFFFFFFE), UINT64_C(0xFFFFFFFFFE),
          UINT64_C(0xFFFFFFFFFE), 0U);
    check(UINT16_C(0xF587), UINT64_C(0xFFFFFFFFFE), UINT64_C(0xFFFFFFFFFE),
          UINT64_C(0xFFFFFFFFFE), 0U);
    /* The complete relation matrix for both operations and destinations. */
    check(UINT16_C(0xF586), UINT64_C(7), UINT64_C(7), UINT64_C(7), 0U);
    check(UINT16_C(0xF586), UINT64_C(7), UINT64_C(3), UINT64_C(7), 0U);
    check(UINT16_C(0xF487), UINT64_C(7), UINT64_C(7), UINT64_C(7), 0U);
    check(UINT16_C(0xF487), UINT64_C(7), UINT64_C(3), UINT64_C(3), 1U);
    check(UINT16_C(0xF587), UINT64_C(7), UINT64_C(3), UINT64_C(3), 1U);
    check(UINT16_C(0xF486), UINT64_C(0xFFFFFFFFFF), UINT64_C(0),
          UINT64_C(0), 1U);
    check(UINT16_C(0xF586), UINT64_C(0xFFFFFFFFFF), UINT64_C(0),
          UINT64_C(0), 1U);
    check(UINT16_C(0xF487), UINT64_C(0x8000000000), UINT64_C(0x7FFFFFFFFF),
          UINT64_C(0x8000000000), 0U);
    check(UINT16_C(0xF587), UINT64_C(0x8000000000), UINT64_C(0x7FFFFFFFFF),
          UINT64_C(0x8000000000), 0U);

    {
        const ti_c54x_decoded_instruction instruction = decode(UINT16_C(0xF486));
        ti_c54x_op_input_snapshot input = {
            .accumulator_a_raw40 = 1U,
            .accumulator_b_raw40 = 2U,
            .accumulator_a_known = false,
            .accumulator_b_known = true};
        ti_c54x_op_effect effect = {
            .kind = TI_C54X_OP_EFFECT_NONE,
            .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
        const ti_c54x_op_effect before = effect;
        TI_C54X_TEST_CHECK(ti_c54x_op_max_acc(&instruction, &input, &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        input.accumulator_a_known = true;
        input.accumulator_a_raw40 = TI_C54X_U40_MASK + UINT64_C(1);
        TI_C54X_TEST_CHECK(ti_c54x_op_max_acc(&instruction, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        input.accumulator_a_raw40 = 1U;
        input.accumulator_b_known = false;
        TI_C54X_TEST_CHECK(ti_c54x_op_max_acc(&instruction, &input, &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        input.accumulator_b_known = true;
        input.accumulator_b_raw40 = TI_C54X_U40_MASK + UINT64_C(1);
        TI_C54X_TEST_CHECK(ti_c54x_op_max_acc(&instruction, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        {
            ti_c54x_decoded_instruction forged = instruction;
            forged.bundle.program_words[0] ^= UINT16_C(0x0010);
            TI_C54X_TEST_CHECK(ti_c54x_op_max_acc(&forged, &input, &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        }
        {
            ti_c54x_decoded_instruction forged = instruction;
            forged.repeatability = TI_C54X_REPEATABILITY_COUNT;
            TI_C54X_TEST_CHECK(ti_c54x_op_max_acc(&forged, &input, &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        }
        {
            ti_c54x_decoded_instruction forged = instruction;
            forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                                   &forged, &input, &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        }
        {
            ti_c54x_decoded_instruction forged = instruction;
            forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
            TI_C54X_TEST_CHECK(ti_c54x_op_max_acc(&forged, &input, &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
            forged.input_resources = (ti_c54x_input_resource_mask)0x80U;
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                                   &forged, &input, &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        }
    }
    return 0;
}
