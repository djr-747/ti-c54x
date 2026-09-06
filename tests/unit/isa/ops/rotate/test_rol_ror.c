/* Module contract
 * Owns: pure ROL/ROR accumulator-handler and generated-dispatch proofs.
 * Does not own: decode tables, accumulator mutation, status mutation,
 * sequencing, CPU lifecycle, timing, or repeat/interrupt policy.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/rotate/op_rotate.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

static ti_c54x_op_status direct_handler(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    if (instruction->id == TI_C54X_OPCODE_ID_ROL_ACC) {
        return ti_c54x_op_rol_acc(instruction, input, effect);
    }
    return ti_c54x_op_ror_acc(instruction, input, effect);
}

static uint64_t expected_value(ti_c54x_opcode_id id, uint64_t source,
                               uint8_t status_c)
{
    const uint64_t low32 = source & UINT64_C(0xFFFFFFFF);

    if (id == TI_C54X_OPCODE_ID_ROL_ACC) {
        return ((low32 << 1U) & UINT64_C(0xFFFFFFFF)) |
               (uint64_t)status_c;
    }
    return (low32 >> 1U) | ((uint64_t)status_c << 31U);
}

static uint8_t expected_c(ti_c54x_opcode_id id, uint64_t source)
{
    const uint64_t low32 = source & UINT64_C(0xFFFFFFFF);

    return id == TI_C54X_OPCODE_ID_ROL_ACC
               ? (uint8_t)((low32 >> 31U) & UINT64_C(1))
               : (uint8_t)(low32 & UINT64_C(1));
}

static void assert_effect(const ti_c54x_op_effect *effect,
                          uint64_t value,
                          uint8_t destination,
                          uint8_t c_value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK_U64(effect->payload.accumulator_write.value_raw40,
                           value);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.c_action ==
                       TI_C54X_OP_ACCUMULATOR_C_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.c_value == c_value);
}

static void check_case(uint16_t word,
                       uint64_t a,
                       uint64_t b,
                       bool a_known,
                       bool b_known,
                       uint8_t status_c,
                       bool status_c_known,
                       ti_c54x_op_status expected_status)
{
    const ti_c54x_decoded_instruction instruction = decode(word);
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = a_known,
        .accumulator_b_known = b_known,
        .status_c = status_c,
        .status_c_known = status_c_known};
    ti_c54x_op_effect direct = {
        .kind = TI_C54X_OP_EFFECT_NONE,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect dispatched = direct;
    const ti_c54x_op_effect before = direct;
    const ti_c54x_op_status direct_status =
        direct_handler(&instruction, &input, &direct);
    const ti_c54x_op_status dispatch_status =
        ti_c54x_dispatch_decoded_with_input_snapshot(
            &instruction, &input, &dispatched);

    TI_C54X_TEST_CHECK(direct_status == expected_status);
    TI_C54X_TEST_CHECK(dispatch_status == expected_status);
    if (expected_status != TI_C54X_OP_STATUS_OK) {
        TI_C54X_TEST_CHECK(memcmp(&direct, &before, sizeof(direct)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&dispatched, &before, sizeof(dispatched)) ==
                           0);
        return;
    }

    {
        const uint8_t source = (uint8_t)((word >> 8U) & UINT16_C(1));
        const uint64_t source_value = source == 0U ? a : b;
        const uint8_t expected_destination = source;
        assert_effect(&direct, expected_value(instruction.id, source_value,
                                               status_c),
                      expected_destination, expected_c(instruction.id,
                                                      source_value));
        TI_C54X_TEST_CHECK(memcmp(&direct, &dispatched, sizeof(direct)) == 0);
    }
}

static void check_form_vectors(uint16_t word)
{
    static const uint64_t vectors[] = {
        UINT64_C(0x0000000000), UINT64_C(0x0000000001),
        UINT64_C(0x0000000007), UINT64_C(0x000000007FFFFFFF),
        UINT64_C(0x0000000080000000), UINT64_C(0x00000000FFFFFFFF),
        UINT64_C(0xFF00000000), UINT64_C(0xFF80000000),
        UINT64_C(0x123456789A), UINT64_C(0x00ABCDEF01)};
    const uint8_t source = (uint8_t)((word >> 8U) & UINT16_C(1));
    size_t index;
    uint8_t c;

    for (c = UINT8_C(0); c <= UINT8_C(1); ++c) {
        for (index = 0U; index < sizeof(vectors) / sizeof(vectors[0]);
             ++index) {
            const uint64_t selected = vectors[index];
            const uint64_t other = vectors[(index + 3U) %
                                           (sizeof(vectors) / sizeof(vectors[0]))];
            check_case(word, source == 0U ? selected : other,
                       source == 1U ? selected : other, true, true, c, true,
                       TI_C54X_OP_STATUS_OK);
        }
    }
}

int main(void)
{
    static const uint16_t forms[] = {
        UINT16_C(0xF491), UINT16_C(0xF591),
        UINT16_C(0xF490), UINT16_C(0xF590)};
    size_t index;

    for (index = 0U; index < sizeof(forms) / sizeof(forms[0]); ++index) {
        check_form_vectors(forms[index]);
    }

    /* An unselected accumulator may be unknown; the selected lane may not. */
    check_case(UINT16_C(0xF491), UINT64_C(3), UINT64_C(0x12345), true,
               false, UINT8_C(1), true, TI_C54X_OP_STATUS_OK);
    check_case(UINT16_C(0xF590), UINT64_C(0x12345), UINT64_C(3), false,
               true, UINT8_C(0), true, TI_C54X_OP_STATUS_OK);

    check_case(UINT16_C(0xF491), UINT64_C(3), UINT64_C(0), false, true,
               UINT8_C(1), true, TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    check_case(UINT16_C(0xF590), UINT64_C(0), UINT64_C(3), true, false,
               UINT8_C(0), true, TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    check_case(UINT16_C(0xF491), UINT64_C(3), UINT64_C(0), true, true,
               UINT8_C(0), false, TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    check_case(UINT16_C(0xF590), UINT64_C(0), UINT64_C(3), true, true,
               UINT8_C(0), false, TI_C54X_OP_STATUS_UNKNOWN_INPUT);

    check_case(UINT16_C(0xF491), TI_C54X_U40_MASK + UINT64_C(1), 0U, true,
               true, UINT8_C(0), true, TI_C54X_OP_STATUS_INVALID_OPERAND);
    check_case(UINT16_C(0xF590), 0U, TI_C54X_U40_MASK + UINT64_C(1), true,
               true, UINT8_C(0), true, TI_C54X_OP_STATUS_INVALID_OPERAND);
    check_case(UINT16_C(0xF491), 3U, 0U, true, true, UINT8_C(2), true,
               TI_C54X_OP_STATUS_INVALID_OPERAND);

    {
        const ti_c54x_decoded_instruction instruction =
            decode(UINT16_C(0xF491));
        const ti_c54x_op_input_snapshot input = {
            .accumulator_a_raw40 = 1U,
            .accumulator_b_raw40 = 2U,
            .accumulator_a_known = true,
            .accumulator_b_known = true,
            .status_c = UINT8_C(0),
            .status_c_known = true};
        ti_c54x_op_effect effect = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
        const ti_c54x_op_effect before = effect;

        {
            ti_c54x_decoded_instruction forged = instruction;
            forged.bundle.program_words[0] ^= UINT16_C(0x0010);
            TI_C54X_TEST_CHECK(ti_c54x_op_rol_acc(&forged, &input, &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        }
        {
            ti_c54x_decoded_instruction forged = instruction;
            forged.operand_layout =
                TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION;
            TI_C54X_TEST_CHECK(ti_c54x_op_rol_acc(&forged, &input, &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        }
        {
            ti_c54x_decoded_instruction forged = instruction;
            forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
            TI_C54X_TEST_CHECK(ti_c54x_op_rol_acc(&forged, &input, &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        }
        {
            ti_c54x_decoded_instruction forged = instruction;
            forged.bundle.program_word_count = UINT8_C(2);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                                   &forged, &input, &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        }
    }
    return 0;
}
