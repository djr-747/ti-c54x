/* Module contract
 * Owns: pure EXP accumulator-handler proofs over the selected canonical
 * 40-bit input, including sign-redundancy boundaries, T-register effect
 * emission, selector isolation, and transactional validation.
 * Does not own: EXP catalog rows, operand extraction, T-register storage,
 * effect application, CPU sequencing, repeat/interrupt policy, timing, or
 * diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/accumulator/op_exp.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
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
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_EXP_ACC);
    TI_C54X_TEST_CHECK(
        instruction.operand_layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    return instruction;
}

static ti_c54x_op_input_snapshot input_for(uint64_t a, uint64_t b,
                                           bool a_known, bool b_known)
{
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = a_known,
        .accumulator_b_known = b_known,
        /* EXP must not consume status inputs. */
        .status_c = UINT8_C(1),
        .status_c_known = false,
        .status_tc = UINT8_C(1),
        .status_tc_known = false};

    return input;
}

static void assert_t_effect(const ti_c54x_op_effect *effect, uint16_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_T_REGISTER_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.t_register_write.raw_value == value);
}

static void check_case(uint16_t word, uint64_t a, uint64_t b, bool a_known,
                       bool b_known, ti_c54x_op_status expected_status,
                       uint16_t expected_value)
{
    const ti_c54x_decoded_instruction instruction = decode(word);
    const ti_c54x_op_input_snapshot input = input_for(a, b, a_known, b_known);
    const ti_c54x_op_input_snapshot input_before = input;
    ti_c54x_op_effect direct = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect dispatched = direct;
    const ti_c54x_op_effect before = direct;
    const ti_c54x_op_status direct_status =
        ti_c54x_op_exp_acc(&instruction, &input, &direct);
    const ti_c54x_op_status dispatch_status =
        ti_c54x_dispatch_decoded_with_input_snapshot(
            &instruction, &input, &dispatched);

    TI_C54X_TEST_CHECK(direct_status == expected_status);
    TI_C54X_TEST_CHECK(dispatch_status == expected_status);
    TI_C54X_TEST_CHECK(memcmp(&input, &input_before, sizeof(input)) == 0);
    if (expected_status != TI_C54X_OP_STATUS_OK) {
        TI_C54X_TEST_CHECK(memcmp(&direct, &before, sizeof(direct)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&dispatched, &before, sizeof(dispatched)) ==
                           0);
        return;
    }

    assert_t_effect(&direct, expected_value);
    TI_C54X_TEST_CHECK(memcmp(&direct, &dispatched, sizeof(direct)) == 0);
}

static void check_sign_redundancy_vectors(void)
{
    struct exp_vector {
        uint64_t raw40;
        uint16_t exponent;
    };
    static const struct exp_vector vectors[] = {
        {UINT64_C(0x0000000000), UINT16_C(0x0000)},
        {UINT64_C(0x7FFFFFFFFF), UINT16_C(0xFFF8)},
        {UINT64_C(0x8000000000), UINT16_C(0xFFF8)},
        {UINT64_C(0x0000000001), UINT16_C(0x001E)},
        {UINT64_C(0xFFFFFFFFFF), UINT16_C(0x001F)},
        {UINT64_C(0x0000000080000000), UINT16_C(0xFFFF)},
        {UINT64_C(0x0000000100000000), UINT16_C(0xFFFE)},
        {UINT64_C(0x000000FF80000000), UINT16_C(0x0000)},
        {UINT64_C(0xFFFFFFFFCB), UINT16_C(0x0019)},
        {UINT64_C(0x0785432105), UINT16_C(0xFFFC)}};
    static const uint16_t forms[] = {UINT16_C(0xF48E), UINT16_C(0xF58E)};
    size_t form_index;
    size_t vector_index;

    for (form_index = 0U; form_index < sizeof(forms) / sizeof(forms[0]);
         ++form_index) {
        const uint8_t source = (uint8_t)((forms[form_index] >> 8U) & 1U);

        for (vector_index = 0U;
             vector_index < sizeof(vectors) / sizeof(vectors[0]);
             ++vector_index) {
            const uint64_t selected = vectors[vector_index].raw40;
            const uint64_t other =
                vectors[(vector_index + 3U) %
                        (sizeof(vectors) / sizeof(vectors[0]))]
                    .raw40;

            check_case(forms[form_index], source == 0U ? selected : other,
                       source == 1U ? selected : other, true, true,
                       TI_C54X_OP_STATUS_OK, vectors[vector_index].exponent);
        }
    }
}

static void check_selected_input_requirements(void)
{
    /* Unknown and noncanonical selected input are rejected transactionally. */
    check_case(UINT16_C(0xF48E), UINT64_C(1), UINT64_C(2), false, true,
               TI_C54X_OP_STATUS_UNKNOWN_INPUT, UINT16_C(0));
    check_case(UINT16_C(0xF58E), UINT64_C(1), UINT64_C(2), true, false,
               TI_C54X_OP_STATUS_UNKNOWN_INPUT, UINT16_C(0));
    check_case(UINT16_C(0xF48E), TI_C54X_U40_MASK + UINT64_C(1), UINT64_C(2),
               true, true, TI_C54X_OP_STATUS_INVALID_OPERAND, UINT16_C(0));
    check_case(UINT16_C(0xF58E), UINT64_C(1), TI_C54X_U40_MASK + UINT64_C(1),
               true, true, TI_C54X_OP_STATUS_INVALID_OPERAND, UINT16_C(0));

    /* Neither unknownness nor noncanonical data in the unselected input is
     * observable by EXP. */
    check_case(UINT16_C(0xF48E), UINT64_C(1), TI_C54X_U40_MASK + UINT64_C(1),
               true, false, TI_C54X_OP_STATUS_OK, UINT16_C(0x001E));
    check_case(UINT16_C(0xF58E), TI_C54X_U40_MASK + UINT64_C(1), UINT64_C(1),
               false, true, TI_C54X_OP_STATUS_OK, UINT16_C(0x001E));
}

static void check_forged_decode_and_null_rollback(void)
{
    const ti_c54x_decoded_instruction instruction = decode(UINT16_C(0xF48E));
    const ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(1), UINT64_C(2), true, true);
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    const ti_c54x_op_effect before = effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_exp_acc(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_exp_acc(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_exp_acc(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);

    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0010);
        TI_C54X_TEST_CHECK(ti_c54x_op_exp_acc(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.operand_layout =
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION;
        TI_C54X_TEST_CHECK(ti_c54x_op_exp_acc(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C;
        TI_C54X_TEST_CHECK(ti_c54x_op_exp_acc(&forged, &input, &effect) ==
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

int main(void)
{
    check_sign_redundancy_vectors();
    check_selected_input_requirements();
    check_forged_decode_and_null_rollback();
    return 0;
}
