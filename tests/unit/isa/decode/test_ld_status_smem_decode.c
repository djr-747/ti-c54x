/*
 * Module contract
 *
 * Owns: executable proof of standard-indirect LD Smem,DP and LD Smem,ASM first-word
 * classification, copied metadata, exhaustive ARF extraction, fixed-bit
 * rejection, and transactional completion/layout rejection. Does not own:
 * catalog generation, address resolution, memory effects, CPU/state
 * mutation, sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct ld_status_case {
    ti_c54x_opcode_id id;
    ti_c54x_opcode_id other_id;
    uint16_t value;
    ti_c54x_repeatability_class repeatability;
} ld_status_case;

static const ld_status_case cases[] = {
    {TI_C54X_OPCODE_ID_LD_DP_SMEM,
     TI_C54X_OPCODE_ID_LD_ASM_SMEM, UINT16_C(0x4690),
     TI_C54X_REPEATABILITY_INELIGIBLE},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM,
     TI_C54X_OPCODE_ID_LD_DP_SMEM, UINT16_C(0x3290),
     TI_C54X_REPEATABILITY_UNRESOLVED},
};

static void
check_case(const ld_status_case *test_case, uint8_t mode, uint8_t arf)
{
    const uint16_t word = (uint16_t)((test_case->value &
                                     (uint16_t)~UINT16_C(0x0018)) |
                                     (uint16_t)(mode << 3U) | arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value arf_field = {0U, 0U};
    ti_c54x_operand_field_value mod_field = {0U, 0U};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == test_case->id);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == test_case->id);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       test_case->repeatability);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(0), &arf_field) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(arf_field.width_bits == UINT8_C(3));
    TI_C54X_TEST_CHECK(arf_field.raw_value == arf);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(1), &mod_field) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(mod_field.width_bits == UINT8_C(2));
    TI_C54X_TEST_CHECK(mod_field.raw_value == mode);
}

static void
check_transactional_failure(
    const ti_c54x_decode_result *first,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_instruction_completion_status expected)
{
    ti_c54x_decoded_instruction output;
    unsigned char before[sizeof(output)];

    (void)memset(&output, 0xA5, sizeof(output));
    (void)memcpy(before, &output, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           first, bundle, &output) == expected);
    TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
}

static void
test_fixed_bit_near_misses(void)
{
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x0020),
        UINT16_C(0x0040), UINT16_C(0x0080), UINT16_C(0x0100),
        UINT16_C(0x0200), UINT16_C(0x0400), UINT16_C(0x0800),
        UINT16_C(0x1000), UINT16_C(0x2000), UINT16_C(0x4000),
        UINT16_C(0x8000)};

    for (size_t case_index = 0U;
         case_index < sizeof(cases) / sizeof(cases[0]); ++case_index) {
        const ld_status_case *test_case = &cases[case_index];

        for (size_t mask_index = 0U;
             mask_index < sizeof(fixed_bit_masks) /
                               sizeof(fixed_bit_masks[0]);
             ++mask_index) {
            const ti_c54x_decode_result result = ti_c54x_decode_first_word(
                (uint16_t)(test_case->value ^ fixed_bit_masks[mask_index]));

            TI_C54X_TEST_CHECK(result.id != test_case->id);
            if (fixed_bit_masks[mask_index] == UINT16_C(0x0100)) {
                if (test_case->id == TI_C54X_OPCODE_ID_LD_ASM_SMEM) {
                    /* Flipping fixed bit 8 lands on the S3w MASA_SMEM
                     * row (0x3290 ^ 0x0100 = 0x3390). */
                    TI_C54X_TEST_CHECK(result.status ==
                                       TI_C54X_DECODE_STATUS_OK);
                    TI_C54X_TEST_CHECK(result.id ==
                                       TI_C54X_OPCODE_ID_MASA_SMEM);
                } else {
                    TI_C54X_TEST_CHECK(result.status ==
                                       TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
                    TI_C54X_TEST_CHECK(result.id ==
                                       TI_C54X_OPCODE_ID_UNKNOWN);
                }
            }
        }
    }
}

static void
test_transactional_completion_failures(void)
{
    for (size_t case_index = 0U;
         case_index < sizeof(cases) / sizeof(cases[0]); ++case_index) {
        const ld_status_case *test_case = &cases[case_index];
        const uint16_t word = (uint16_t)(test_case->value | UINT16_C(7));
        const ti_c54x_decode_result valid =
            ti_c54x_decode_first_word(word);
        const ti_c54x_instruction_word_bundle valid_bundle = {
            .program_words = {word, UINT16_C(0)},
            .program_word_count = UINT8_C(1)};
        ti_c54x_decode_result forged = valid;
        ti_c54x_instruction_word_bundle bundle = valid_bundle;

        TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(valid.id == test_case->id);

        forged.status = TI_C54X_DECODE_STATUS_UNKNOWN_WORD;
        check_transactional_failure(
            &forged, &bundle,
            TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_NOT_OK);

        forged = valid;
        forged.first_word = (uint16_t)(word ^ UINT16_C(0x0008));
        check_transactional_failure(
            &forged, &bundle,
            TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

        forged = valid;
        forged.id = test_case->other_id;
        check_transactional_failure(
            &forged, &bundle,
            TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);

        forged = valid;
        forged.first_word = (uint16_t)(word ^ UINT16_C(0x0100));
        bundle.program_words[0] = forged.first_word;
        check_transactional_failure(
            &forged, &bundle,
            TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);

        forged = valid;
        bundle = valid_bundle;
        forged.program_word_count = UINT8_C(2);
        check_transactional_failure(
            &forged, &bundle,
            TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH);

        forged = valid;
        bundle = valid_bundle;
        bundle.program_word_count = UINT8_C(2);
        check_transactional_failure(
            &forged, &bundle,
            TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);
    }
}

static void
test_existing_arf_layout_rejects_wrong_width_transactionally(void)
{
    ti_c54x_operand_layout_set_view view;
    unsigned char before[sizeof(view)];

    (void)memset(&view, 0xA5, sizeof(view));
    (void)memcpy(before, &view, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_ARF_MOD, UINT8_C(2), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_MISMATCH);
    TI_C54X_TEST_CHECK(memcmp(before, &view, sizeof(view)) == 0);
}

int
main(void)
{
    for (size_t case_index = 0U;
         case_index < sizeof(cases) / sizeof(cases[0]); ++case_index) {
        for (uint8_t mode = 0U; mode < UINT8_C(4); ++mode) {
            for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
                check_case(&cases[case_index], mode, arf);
            }
        }
    }
    test_fixed_bit_near_misses();
    test_transactional_completion_failures();
    test_existing_arf_layout_rejects_wrong_width_transactionally();
    return 0;
}
