/*
 * Module contract
 *
 * Owns: exhaustive IDLE decode proof for the 0xF4E1/0xFCFF
 * masked row (P3-S88, TI SPRU172C power-down IDLE K, 1 word):
 * first-word resolution, K extraction (bits 9-8) through
 * the IDLE_K layout, and the four-encoding population with
 * neighbors resolving elsewhere.
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
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_IDLE);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(UINT16_C(0xF4E1));
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {UINT16_C(0xF4E1)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xF4E1));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFCFF));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_IDLE);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_IDLE_K);
    {
        ti_c54x_operand_layout_set_view view;
        ti_c54x_operand_field_value field = {0U, 0U};
        const ti_c54x_program_words_view words = {
            bundle.program_words, bundle.program_word_count};
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                               TI_C54X_OPERAND_LAYOUT_IDLE_K, UINT8_C(1),
                               &view) == TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, UINT8_C(0), &field) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(field.width_bits == UINT8_C(2));
        TI_C54X_TEST_CHECK(field.raw_value == UINT8_C(0));
    }

    /* Masked row: exactly the four NN encodings resolve here. */
    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);
        const bool idle = decoded.id == TI_C54X_OPCODE_ID_IDLE;
        TI_C54X_TEST_CHECK(idle ==
                           ((word & UINT16_C(0xFCFF)) == UINT16_C(0xF4E1)));
    }

    {
        /* Neighbors keep their rows. */
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0xF480)).id ==
                           TI_C54X_OPCODE_ID_ADD_SRC_ASM);
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0xF4A8)).id ==
                           TI_C54X_OPCODE_ID_CMPR_AR);
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0xF4B0)).id ==
                           TI_C54X_OPCODE_ID_RSBX_STATUS);
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0xF4EB)).id ==
                           TI_C54X_OPCODE_ID_RETE);
    }
    return 0;
}
