/*
 * Module contract
 *
 * Owns: exhaustive DLD absolute Smem decode proof for the
 * 0x56F8 exact row (P3-S78, TI SPRU172C section DLD with
 * absolute addressing): first-word resolution, D/I/abs
 * extraction through the reused layout, and the single-encoding
 * population with no shadow word.
 * Does not own: opcode semantics, CPU state, sequencing, or timing.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int main(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(
            TI_C54X_OPCODE_ID_DLD_SMEM_ABSOLUTE);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(UINT16_C(0x56F8));
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {UINT16_C(0x56F8), UINT16_C(0x0060)},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0x56F8));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id ==
                       TI_C54X_OPCODE_ID_DLD_SMEM_ABSOLUTE);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16);
    {
        ti_c54x_operand_layout_set_view view;
        ti_c54x_operand_field_value field = {0U, 0U};
        const ti_c54x_program_words_view words = {
            bundle.program_words, bundle.program_word_count};
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                               TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16,
                               UINT8_C(2), &view) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, UINT8_C(0), &field) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(field.width_bits == UINT8_C(1));
        TI_C54X_TEST_CHECK(field.raw_value == UINT8_C(0));
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, UINT8_C(1), &field) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(field.width_bits == UINT8_C(1));
        TI_C54X_TEST_CHECK(field.raw_value == UINT8_C(1));
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, UINT8_C(2), &field) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(field.width_bits == UINT8_C(7));
        TI_C54X_TEST_CHECK(field.raw_value == UINT8_C(0x78));
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, UINT8_C(3), &field) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(field.width_bits == UINT8_C(16));
        TI_C54X_TEST_CHECK(field.raw_value == UINT16_C(0x0060));
    }

    /* Exact row: only 0x56F8 resolves here. */
    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);
        const bool absolute =
            decoded.id == TI_C54X_OPCODE_ID_DLD_SMEM_ABSOLUTE;
        TI_C54X_TEST_CHECK(absolute == (word == UINT16_C(0x56F8)));
    }

    {
        /* The MOD13 long-offset row keeps its words; the
         * uncovered 0x5600 direct half resolves nowhere. */
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x56E8)).id ==
                           TI_C54X_OPCODE_ID_DLD_LONG32_MOD13);
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x5600)).id !=
                           TI_C54X_OPCODE_ID_DLD_SMEM_ABSOLUTE);
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x5680)).id !=
                           TI_C54X_OPCODE_ID_DLD_SMEM_ABSOLUTE);
    }
    return 0;
}
