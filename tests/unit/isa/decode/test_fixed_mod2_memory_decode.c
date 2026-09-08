/*
 * Module contract
 *
 * Owns: executable proof that all standard-indirect WORD16 LD/STL first words
 * classify (with MOD2 retained as the canonical regression),
 * complete transactionally, and expose D/S plus ARF operands through the
 * generic decoder layout. Does not own: memory/address effects, CPU/state
 * mutation, fetch or bus behavior, sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

typedef struct standard_indirect_case {
    ti_c54x_opcode_id id;
    uint16_t value;
} standard_indirect_case;

static const standard_indirect_case cases[] = {
    {TI_C54X_OPCODE_ID_LD_SMEM, UINT16_C(0x1090)},
    {TI_C54X_OPCODE_ID_STL_SMEM, UINT16_C(0x8090)},
};

static void
check_operand(const ti_c54x_operand_layout_set_view *view,
              const ti_c54x_program_words_view *words, uint8_t index,
              uint64_t expected, uint8_t width)
{
    ti_c54x_operand_field_value value = {UINT64_C(0xDEADBEEF), UINT8_C(0xA5)};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           view, words, index, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(value.raw_value == expected);
    TI_C54X_TEST_CHECK(value.width_bits == width);
}

static void
test_exhaustive_d_or_s_and_arf_decode(void)
{
    for (size_t case_index = 0U;
         case_index < sizeof(cases) / sizeof(cases[0]); ++case_index) {
        const standard_indirect_case *test_case = &cases[case_index];

        for (uint16_t mode = 0U; mode < UINT16_C(4); ++mode) {
            const uint16_t base = (uint16_t)(test_case->value &
                                             (uint16_t)~UINT16_C(0x0018));
            for (uint16_t accumulator = 0U; accumulator < UINT16_C(2);
                 ++accumulator) {
                for (uint16_t arf = 0U; arf < UINT16_C(8); ++arf) {
                const uint16_t first_word = (uint16_t)(
                    base | (uint16_t)(mode << 3U) |
                    (uint16_t)(accumulator << 8U) | arf);
                const ti_c54x_decode_result first =
                    ti_c54x_decode_first_word(first_word);
                const ti_c54x_instruction_word_bundle bundle = {
                    .program_words = {first_word, UINT16_C(0)},
                    .program_word_count = UINT8_C(1)};
                const ti_c54x_program_words_view words = {
                    bundle.program_words, bundle.program_word_count};
                ti_c54x_decoded_instruction instruction;
                ti_c54x_operand_layout_set_view view;

                TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
                TI_C54X_TEST_CHECK(first.id == test_case->id);
                TI_C54X_TEST_CHECK(first.first_word == first_word);
                TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
                TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                                       &first, &bundle, &instruction) ==
                                   TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
                TI_C54X_TEST_CHECK(instruction.id == test_case->id);
                TI_C54X_TEST_CHECK(instruction.operand_layout ==
                                   TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD);
                TI_C54X_TEST_CHECK(instruction.bundle.program_word_count ==
                                   UINT8_C(1));
                TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                                   first_word);
                TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                       instruction.operand_layout, UINT8_C(1),
                                       &view) ==
                                   TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                TI_C54X_TEST_CHECK(view.descriptor != NULL);
                TI_C54X_TEST_CHECK(view.descriptor->operand_count ==
                                   UINT8_C(3));
                check_operand(&view, &words, UINT8_C(0), accumulator,
                              UINT8_C(1));
                check_operand(&view, &words, UINT8_C(1), arf, UINT8_C(3));
                check_operand(&view, &words, UINT8_C(2), mode,
                              UINT8_C(2));
                }
            }
        }
    }
}

static void
check_failure(const ti_c54x_decode_result *first,
              const ti_c54x_instruction_word_bundle *bundle,
              ti_c54x_instruction_completion_status expected)
{
    unsigned char before[sizeof(ti_c54x_decoded_instruction)];
    ti_c54x_decoded_instruction output;

    (void)memset(&output, 0xA5, sizeof(output));
    (void)memcpy(before, &output, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           first, bundle, &output) == expected);
    TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
}

static void
test_adjacent_patterns_are_unknown(void)
{
    const uint16_t unknown_words[] = {
        UINT16_C(0x10F8), UINT16_C(0x80F8), UINT16_C(0x11F8),
        UINT16_C(0x81F8)};

    for (size_t index = 0U;
         index < sizeof(unknown_words) / sizeof(unknown_words[0]); ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(unknown_words[index]);

        TI_C54X_TEST_CHECK(result.status == TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
        TI_C54X_TEST_CHECK(result.first_word == unknown_words[index]);
        TI_C54X_TEST_CHECK(result.program_word_count == UINT8_C(0));
    }
}

static void
test_completion_rejects_forged_inputs_transactionally(void)
{
    const uint16_t first_word = UINT16_C(0x1197);
    const ti_c54x_decode_result valid =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {first_word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_LD_SMEM);

    forged.status = TI_C54X_DECODE_STATUS_UNKNOWN_WORD;
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_NOT_OK);

    forged = valid;
    forged.first_word = UINT16_C(0x1196);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    forged = valid;
    forged.id = TI_C54X_OPCODE_ID_STL_SMEM;
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);

    forged = valid;
    forged.program_word_count = UINT8_C(2);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH);

    forged = valid;
    bundle.program_word_count = UINT8_C(2);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);
}

int
main(void)
{
    test_exhaustive_d_or_s_and_arf_decode();
    test_adjacent_patterns_are_unknown();
    test_completion_rejects_forged_inputs_transactionally();
    return 0;
}
