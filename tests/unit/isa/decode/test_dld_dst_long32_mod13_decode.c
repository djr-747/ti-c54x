/*
 * Module contract
 *
 * Owns: executable proof that long32 DLD/DST first words complete through the
 * checked decoder and expose the shared D/S, ARF, extension, and MOD13 fields
 * in the documented order and widths. It also compares checked extraction to
 * the trusted plan extraction. Does not own: opcode semantics, memory effects,
 * CPU/state mutation, sequencing, timing, or bus behavior.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../../src/isa/decode/operand_layout_internal.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct long32_case {
    ti_c54x_opcode_id id;
    uint16_t base;
} long32_case;

static const long32_case cases[] = {
    {TI_C54X_OPCODE_ID_DLD_LONG32_MOD13, UINT16_C(0x56E8)},
    {TI_C54X_OPCODE_ID_DST_LONG32_MOD13, UINT16_C(0x4EE8)},
};

static void
check_value(const ti_c54x_operand_field_value *value, uint64_t expected,
            uint8_t width)
{
    TI_C54X_TEST_CHECK_U64(value->raw_value, expected);
    TI_C54X_TEST_CHECK(value->width_bits == width);
}

static void
test_checked_and_trusted_extraction_agree(void)
{
    for (size_t case_index = 0U;
         case_index < sizeof(cases) / sizeof(cases[0]); ++case_index) {
        const long32_case *test_case = &cases[case_index];

        for (uint16_t ds = 0U; ds < UINT16_C(2); ++ds) {
            for (uint16_t arf = 0U; arf < UINT16_C(8); ++arf) {
                const uint16_t first_word = (uint16_t)(
                    test_case->base | (uint16_t)(ds << 8U) | arf);
                const uint16_t extension = (uint16_t)(
                    UINT16_C(0xA500) | (uint16_t)(case_index << 8U) | arf);
                const ti_c54x_decode_result first =
                    ti_c54x_decode_first_word(first_word);
                const ti_c54x_instruction_word_bundle bundle = {
                    .program_words = {first_word, extension},
                    .program_word_count = UINT8_C(2)};
                const ti_c54x_program_words_view words = {
                    bundle.program_words, bundle.program_word_count};
                ti_c54x_decoded_instruction instruction;
                ti_c54x_operand_layout_set_view view;
                ti_c54x_operand_field_value checked[4] = {{0U, 0U},
                                                           {0U, 0U},
                                                           {0U, 0U},
                                                           {0U, 0U}};
                ti_c54x_operand_field_value trusted[4] = {{0U, 0U},
                                                           {0U, 0U},
                                                           {0U, 0U},
                                                           {0U, 0U}};
                uint8_t trusted_count = UINT8_C(0);

                TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
                TI_C54X_TEST_CHECK(first.id == test_case->id);
                TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
                TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                                       &first, &bundle, &instruction) ==
                                   TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
                TI_C54X_TEST_CHECK(instruction.operand_layout ==
                                   TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4);
                TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                       instruction.operand_layout, 2U, &view) ==
                                   TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                TI_C54X_TEST_CHECK(view.descriptor->operand_count == 4U);
                for (uint8_t operand = 0U; operand < UINT8_C(4); ++operand) {
                    TI_C54X_TEST_CHECK(
                        ti_c54x_operand_layout_set_extract_operand(
                            &view, &words, operand, &checked[operand]) ==
                        TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                }
                TI_C54X_TEST_CHECK(ti_c54x_operand_layout_extract_all_trusted(
                                       &ti_c54x_operand_layout_plans[
                                           TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4],
                                       &bundle, trusted, &trusted_count));
                TI_C54X_TEST_CHECK(trusted_count == UINT8_C(4));
                for (uint8_t operand = 0U; operand < UINT8_C(4); ++operand) {
                    TI_C54X_TEST_CHECK(checked[operand].raw_value ==
                                       trusted[operand].raw_value);
                    TI_C54X_TEST_CHECK(checked[operand].width_bits ==
                                       trusted[operand].width_bits);
                }
                check_value(&checked[0], ds, UINT8_C(1));
                check_value(&checked[1], arf, UINT8_C(3));
                check_value(&checked[2], extension, UINT8_C(16));
                check_value(&checked[3], UINT16_C(13), UINT8_C(4));
            }
        }
    }
}

static void
test_malformed_completion_is_transactional(void)
{
    const uint16_t first_word = UINT16_C(0x56E8);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle valid = {
        .program_words = {first_word, UINT16_C(0x1234)},
        .program_word_count = UINT8_C(2)};
    ti_c54x_instruction_word_bundle malformed = valid;
    ti_c54x_decoded_instruction output;
    unsigned char before[sizeof(output)];

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    malformed.program_words[0] = UINT16_C(0x56E0);
    (void)memset(&output, 0xA5, sizeof(output));
    (void)memcpy(before, &output, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &malformed, &output) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);
    TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);

    malformed = valid;
    malformed.program_word_count = UINT8_C(1);
    (void)memset(&output, 0x5A, sizeof(output));
    (void)memcpy(before, &output, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &malformed, &output) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);
    TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
}

int
main(void)
{
    test_checked_and_trusted_extraction_agree();
    test_malformed_completion_is_transactional();
    return 0;
}
