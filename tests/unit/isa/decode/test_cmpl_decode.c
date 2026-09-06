/* Module contract
 * Owns: exhaustive CMPL first-word classification and source/destination
 * operand-layout extraction proof.
 * Does not own: CMPL arithmetic, architectural mutation, sequencing, or
 * timing.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct cmpl_case {
    uint16_t word;
    uint8_t source;
    uint8_t destination;
} cmpl_case;

static const cmpl_case cases[] = {
    {UINT16_C(0xF493), UINT8_C(0), UINT8_C(0)},
    {UINT16_C(0xF593), UINT8_C(0), UINT8_C(1)},
    {UINT16_C(0xF693), UINT8_C(1), UINT8_C(0)},
    {UINT16_C(0xF493) + UINT16_C(0x0300), UINT8_C(1), UINT8_C(1)},
};

int main(void)
{
    unsigned valid = 0U;

    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_CMPL_ACC);
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(cases[i].word);
        const ti_c54x_instruction_word_bundle bundle = {
            .program_words = {cases[i].word, UINT16_C(0)},
            .program_word_count = UINT8_C(1)};
        ti_c54x_decoded_instruction instruction;
        ti_c54x_operand_layout_set_view view;
        ti_c54x_operand_field_value source = {0U, 0U};
        ti_c54x_operand_field_value destination = {0U, 0U};
        const ti_c54x_program_words_view words = {
            bundle.program_words, bundle.program_word_count};

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xF493));
        TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFCFF));
        TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_CMPL_ACC);
        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &first, &bundle, &instruction) ==
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(instruction.operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                               instruction.operand_layout, UINT8_C(1), &view) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, UINT8_C(0), &source) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, UINT8_C(1), &destination) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(source.width_bits == UINT8_C(1));
        TI_C54X_TEST_CHECK(destination.width_bits == UINT8_C(1));
        TI_C54X_TEST_CHECK(source.raw_value == cases[i].source);
        TI_C54X_TEST_CHECK(destination.raw_value == cases[i].destination);
    }

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);
        const bool matches =
            (word & UINT16_C(0xFCFF)) == UINT16_C(0xF493);
        TI_C54X_TEST_CHECK((decoded.id == TI_C54X_OPCODE_ID_CMPL_ACC) ==
                           matches);
        if (matches) {
            ++valid;
        }
    }
    TI_C54X_TEST_CHECK(valid == 4U);

    {
        const uint16_t near_misses[] = {
            UINT16_C(0xF492), UINT16_C(0xF493) ^ UINT16_C(0x0400),
            UINT16_C(0xF493) ^ UINT16_C(0x0800),
            UINT16_C(0xF493) ^ UINT16_C(0x1000)};
        for (size_t i = 0U; i < sizeof(near_misses) / sizeof(near_misses[0]);
             ++i) {
            TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(near_misses[i]).id !=
                               TI_C54X_OPCODE_ID_CMPL_ACC);
        }
    }
    return 0;
}
