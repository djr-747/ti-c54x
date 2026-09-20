/*
 * Module contract
 *
 * Owns: RED proof of the pure IDLE power-down effect
 * (P3-S88, TI SPRU172C power-down IDLE K, 1 word):
 * decode identity, K extraction (bits 9-8, K = NN+1),
 * reserved-selector rejection, trusted equivalence,
 * input-independence, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors,
 * address resolution, buses, CPU/state mutation,
 * sequencing, timing, observers, or diagnostics.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../src/isa/ops/op_effect.h"
#include "../../../../src/isa/ops/op_idle.h"
#include "../../../../src/isa/ops/op_input.h"
#include "../../../../src/isa/ops/op_status.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction
decode_idle(uint16_t word)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_IDLE);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_IDLE_K);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    return instruction;
}

static ti_c54x_trusted_operand_values
trusted_idle_operands(uint8_t k)
{
    ti_c54x_trusted_operand_values operands = {0};

    /* The trusted path takes the raw NN field; the leaf adds one. */
    operands.count = UINT8_C(1);
    operands.values[0].raw_value = (uint64_t)k - UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(2);
    return operands;
}

static void
assert_idle_effect(const ti_c54x_op_effect *effect, uint8_t k)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_IDLE);
    TI_C54X_TEST_CHECK(effect->payload.idle.power_down_mode == k);
}

static void
test_effect_and_parity(void)
{
    /* Evidenced K=1 plus the K=2/K=3 encodings. */
    static const uint16_t words[] = {UINT16_C(0xF4E1),
                                     UINT16_C(0xF5E1),
                                     UINT16_C(0xF6E1)};
    static const uint8_t modes[] = {UINT8_C(1), UINT8_C(2), UINT8_C(3)};

    for (size_t w = 0U; w < sizeof(words) / sizeof(words[0]); ++w) {
        const ti_c54x_decoded_instruction instruction =
            decode_idle(words[w]);
        const ti_c54x_trusted_operand_values operands =
            trusted_idle_operands(modes[w]);
        const ti_c54x_op_input_snapshot input = {0};
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
        ti_c54x_op_effect checked = before;
        ti_c54x_op_effect trusted = before;

        TI_C54X_TEST_CHECK(ti_c54x_op_idle(&instruction, &input,
                                          &checked) ==
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_idle(
                               &instruction, &operands, &input,
                               &trusted) == TI_C54X_OP_STATUS_OK);
        assert_idle_effect(&checked, modes[w]);
        assert_idle_effect(&trusted, modes[w]);
        TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                                  sizeof(checked)) == 0);
    }
}

static void
test_reserved_selector_rejects(void)
{
    /* NN=3 (K=4) is reserved: both paths reject with
     * effects untouched. */
    const ti_c54x_decoded_instruction instruction =
        decode_idle(UINT16_C(0xF7E1));
    const ti_c54x_op_input_snapshot input = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x5A5A)}};
    ti_c54x_op_effect checked = before;
    ti_c54x_op_effect trusted = before;
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(1);
    operands.values[0].raw_value = UINT8_C(3);
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_idle(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&checked, &before, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_idle(
                           &instruction, &operands, &input,
                           &trusted) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &before, sizeof(trusted)) == 0);
}

static void
test_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_idle(UINT16_C(0xF4E1));
    const ti_c54x_op_input_snapshot input = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;

    TI_C54X_TEST_CHECK(ti_c54x_op_idle(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_idle(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_idle(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.id = TI_C54X_OPCODE_ID_NOP;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_idle(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    test_effect_and_parity();
    test_reserved_selector_rejects();
    test_rejection_is_transactional();
    return 0;
}
