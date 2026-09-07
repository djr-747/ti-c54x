/*
 * Module contract
 *
 * Owns: executable proof of CMPR one-word decode classification, copied
 * completion metadata, exhaustive CC/ARx extraction, fixed-bit near-miss
 * rejection, and transactional rejection of forged completion inputs.
 * Does not own: catalog generation, compare semantics, architectural state,
 * status mutation, sequencing, timing, or CPU integration.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static void check_case(uint8_t cc, uint8_t arx)
{
    const uint16_t word = (uint16_t)(UINT16_C(0xF4A8) |
                                     ((uint16_t)cc << 8U) | arx);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value cc_field = {0U, 0U};
    ti_c54x_operand_field_value arx_field = {0U, 0U};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_CMPR_AR);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_CMPR_AR);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_CONDITION_ARF);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(0), &cc_field) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(1), &arx_field) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(cc_field.width_bits == UINT8_C(2));
    TI_C54X_TEST_CHECK(cc_field.raw_value == cc);
    TI_C54X_TEST_CHECK(arx_field.width_bits == UINT8_C(3));
    TI_C54X_TEST_CHECK(arx_field.raw_value == arx);
}

static void test_near_misses_and_forged_completion(void)
{
    static const uint16_t near_misses[] = {
        UINT16_C(0xF4A8) ^ UINT16_C(0x0008),
        UINT16_C(0xF4A8) ^ UINT16_C(0x0010),
        UINT16_C(0xF4A8) ^ UINT16_C(0x0400),
        UINT16_C(0xF4A8) ^ UINT16_C(0x0800),
        UINT16_C(0xF4A8) ^ UINT16_C(0x1000)};
    const uint16_t word = UINT16_C(0xF4A8);
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction output;
    unsigned char before[sizeof(output)];

    for (size_t i = 0U; i < sizeof(near_misses) / sizeof(near_misses[0]); ++i) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(near_misses[i]);
        TI_C54X_TEST_CHECK(result.id != TI_C54X_OPCODE_ID_CMPR_AR);
    }

    (void)memset(&output, 0xA5, sizeof(output));
    (void)memcpy(before, &output, sizeof(before));
    {
        ti_c54x_decode_result forged = valid;
        forged.id = TI_C54X_OPCODE_ID_UNKNOWN;
        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &forged, &bundle, &output) !=
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
    }
    {
        ti_c54x_instruction_word_bundle malformed = bundle;
        malformed.program_word_count = UINT8_C(2);
        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &valid, &malformed, &output) !=
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
    }
}

int main(void)
{
    for (uint8_t cc = 0U; cc < UINT8_C(4); ++cc) {
        for (uint8_t arx = 0U; arx < UINT8_C(8); ++arx) {
            check_case(cc, arx);
        }
    }
    test_near_misses_and_forged_completion();
    return 0;
}
