/*
 * Module contract
 *
 * Owns: executable proof of SFTC first-word classification, copied
 * completion metadata, and source-bit extraction for both encoded source
 * values. It deliberately leaves status and accumulator semantics to the
 * operation-family tests.
 * Does not own: shift semantics, status mutation, CPU/state integration,
 * sequencing, timing, generated lookup production, or opcode registration.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct sftc_case {
    uint16_t word;
    uint8_t source;
} sftc_case;

static const sftc_case cases[] = {
    {UINT16_C(0xF494), UINT8_C(0)},
    {UINT16_C(0xF594), UINT8_C(1)},
};

static void
check_source_and_metadata(const sftc_case *test_case)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_SFTC_ACC);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(test_case->word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {test_case->word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value source = {0U, 0U};

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_SFTC_ACC);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == metadata->id);
    TI_C54X_TEST_CHECK(instruction.operand_layout == metadata->operand_layout);
    TI_C54X_TEST_CHECK(instruction.repeatability == metadata->repeatability);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       metadata->input_resources);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(0), &source) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(source.width_bits == UINT8_C(1));
    TI_C54X_TEST_CHECK(source.raw_value == test_case->source);
}

static void
test_fixed_pattern_near_misses_are_rejected(void)
{
    static const uint16_t near_misses[] = {
        UINT16_C(0xF491), UINT16_C(0xF490), UINT16_C(0xF493),
        UINT16_C(0xF492), UINT16_C(0xF494) ^ UINT16_C(0x0001),
        UINT16_C(0xF494) ^ UINT16_C(0x0200)};

    for (size_t i = 0U; i < sizeof(near_misses) / sizeof(near_misses[0]);
         ++i) {
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(near_misses[i]).id !=
                           TI_C54X_OPCODE_ID_SFTC_ACC);
    }
}

int
main(void)
{
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        check_source_and_metadata(&cases[i]);
    }
    test_fixed_pattern_near_misses_are_rejected();
    return 0;
}
