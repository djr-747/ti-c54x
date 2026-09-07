/*
 * Module contract
 *
 * Owns: executable proof of SSBX/RSBX one-word classification, copied
 * completion metadata, exhaustive N/SBIT extraction, and rejection of
 * sibling or forged metadata. Every encoded SBIT, including ST1 bit 10, is
 * admitted as a value-owned field.
 * Does not own: status mutation, opcode semantics, CPU/state integration,
 * sequencing, timing, generated lookup production, or registration.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static void check_case(uint16_t base, ti_c54x_opcode_id id, uint8_t n,
                       uint8_t sbit)
{
    const uint16_t word = (uint16_t)(base | ((uint16_t)n << 9U) | sbit);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value n_field = {0U, 0U};
    ti_c54x_operand_field_value sbit_field = {0U, 0U};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == id);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == id);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_BIT);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(0), &n_field) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(1), &sbit_field) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(n_field.width_bits == UINT8_C(1));
    TI_C54X_TEST_CHECK(n_field.raw_value == n);
    TI_C54X_TEST_CHECK(sbit_field.width_bits == UINT8_C(4));
    TI_C54X_TEST_CHECK(sbit_field.raw_value == sbit);
}

static void test_near_misses_and_forged_completion(void)
{
    static const uint16_t near_misses[] = {
        UINT16_C(0xF4A0), UINT16_C(0xF5A0), UINT16_C(0xF4B1) ^ UINT16_C(0x0010),
        UINT16_C(0xF5B0) ^ UINT16_C(0x0020), UINT16_C(0xF5B0) ^ UINT16_C(0x0400)};
    const uint16_t word = UINT16_C(0xF5B0);
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction output;
    unsigned char before[sizeof(output)];

    for (size_t i = 0U; i < sizeof(near_misses) / sizeof(near_misses[0]); ++i) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(near_misses[i]);
        TI_C54X_TEST_CHECK(result.id != TI_C54X_OPCODE_ID_SSBX_STATUS);
        TI_C54X_TEST_CHECK(result.id != TI_C54X_OPCODE_ID_RSBX_STATUS);
    }

    (void)memset(&output, 0xA5, sizeof(output));
    (void)memcpy(before, &output, sizeof(before));
    {
        ti_c54x_decode_result forged = valid;
        forged.id = TI_C54X_OPCODE_ID_RSBX_STATUS;
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
    const uint16_t bases[] = {UINT16_C(0xF5B0), UINT16_C(0xF4B0)};
    const ti_c54x_opcode_id ids[] = {TI_C54X_OPCODE_ID_SSBX_STATUS,
                                     TI_C54X_OPCODE_ID_RSBX_STATUS};

    for (size_t row = 0U; row < 2U; ++row) {
        check_case(bases[row], ids[row], UINT8_C(0), UINT8_C(0));
        check_case(bases[row], ids[row], UINT8_C(0), UINT8_C(15));
        check_case(bases[row], ids[row], UINT8_C(1), UINT8_C(0));
        check_case(bases[row], ids[row], UINT8_C(1), UINT8_C(10));
        check_case(bases[row], ids[row], UINT8_C(1), UINT8_C(15));
    }
    test_near_misses_and_forged_completion();
    return 0;
}
