/* Module contract
 * Owns: pure CMPR CC,ARx semantic proofs.  The tests cover all four unsigned
 * conditions, every AR selector, representative 16-bit boundaries, TC-only
 * STATUS_BIT_WRITE effects, checked/trusted/dispatch parity, and transactional
 * rejection of unknown or forged inputs.
 * Does not own: catalog/decode production, architectural state collection,
 * status mutation, sequencing, CPU lifecycle, memory, timing, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/compare/op_cmpr.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction decode(uint8_t cc, uint8_t arx)
{
    const uint16_t word = (uint16_t)(UINT16_C(0xF4A8) |
                                     ((uint16_t)(cc & UINT8_C(3)) << 8U) |
                                     (uint16_t)(arx & UINT8_C(7)));
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_CMPR_AR);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static bool expected_condition(uint8_t cc, uint16_t selected, uint16_t ar0)
{
    switch (cc & UINT8_C(3)) {
    case 0U:
        return selected == ar0;
    case 1U:
        return selected < ar0;
    case 2U:
        return selected > ar0;
    default:
        return selected != ar0;
    }
}

static ti_c54x_op_input_snapshot make_input(uint8_t arx, uint16_t selected,
                                             uint16_t ar0)
{
    ti_c54x_op_input_snapshot input = {0};

    input.auxiliary_registers_raw16[0] = ar0;
    if ((arx & UINT8_C(7)) != UINT8_C(0)) {
        input.auxiliary_registers_raw16[arx & UINT8_C(7)] = selected;
    }
    input.auxiliary_registers_known_mask = UINT8_C(0xFF);
    return input;
}

static void check_effect(const ti_c54x_op_effect *effect, bool condition)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_STATUS_BIT_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.status_bit_write.target_register ==
                       TI_C54X_OP_STATUS_REGISTER_ST0);
    TI_C54X_TEST_CHECK(effect->payload.status_bit_write.bit_index ==
                       UINT8_C(12));
    TI_C54X_TEST_CHECK(effect->payload.status_bit_write.raw_value ==
                       (condition ? UINT8_C(1) : UINT8_C(0)));
}

static void check_one(uint8_t cc, uint8_t arx, uint16_t selected,
                      uint16_t ar0)
{
    const ti_c54x_decoded_instruction instruction = decode(cc, arx);
    const ti_c54x_op_input_snapshot input =
        make_input(arx, selected, ar0);
    const uint16_t selected_value =
        (arx & UINT8_C(7)) == UINT8_C(0) ? ar0 : selected;
    const bool condition = expected_condition(cc, selected_value, ar0);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_CMPR_AR);
    const uint16_t word = instruction.bundle.program_words[0];
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_trusted_execution_packet packet;
    ti_c54x_op_effect direct;
    ti_c54x_op_effect checked;
    ti_c54x_op_effect trusted;

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_cmpr(&instruction, &input, &direct) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(&packet, &input, &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    check_effect(&direct, condition);
    TI_C54X_TEST_CHECK(memcmp(&direct, &checked, sizeof(direct)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) == 0);
}

static void test_unknown_inputs_and_forged_decode_are_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(0U, 1U);
    ti_c54x_op_input_snapshot input = make_input(1U, UINT16_C(0x0001),
                                                  UINT16_C(0x0002));
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    const ti_c54x_op_effect before = effect;

    input.auxiliary_registers_known_mask =
        (uint8_t)(input.auxiliary_registers_known_mask &
                  (uint8_t)~(UINT8_C(1) << 0));
    TI_C54X_TEST_CHECK(ti_c54x_op_cmpr(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    input = make_input(1U, UINT16_C(0x0001), UINT16_C(0x0002));
    input.auxiliary_registers_known_mask =
        (uint8_t)(input.auxiliary_registers_known_mask &
                  (uint8_t)~(UINT8_C(1) << 1));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0010);
        TI_C54X_TEST_CHECK(ti_c54x_op_cmpr(&forged, &input, &effect) ==
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
        TI_C54X_TEST_CHECK(ti_c54x_op_cmpr(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int main(void)
{
    static const uint16_t boundaries[] = {
        UINT16_C(0x0000), UINT16_C(0x0001), UINT16_C(0x7FFF),
        UINT16_C(0x8000), UINT16_C(0xFFFE), UINT16_C(0xFFFF)};

    for (uint8_t cc = 0U; cc < UINT8_C(4); ++cc) {
        for (uint8_t arx = 0U; arx < UINT8_C(8); ++arx) {
            for (size_t selected = 0U;
                 selected < sizeof(boundaries) / sizeof(boundaries[0]);
                 ++selected) {
                for (size_t ar0 = 0U;
                     ar0 < sizeof(boundaries) / sizeof(boundaries[0]); ++ar0) {
                    check_one(cc, arx, boundaries[selected], boundaries[ar0]);
                }
            }
        }
    }
    test_unknown_inputs_and_forged_decode_are_transactional();
    return 0;
}
