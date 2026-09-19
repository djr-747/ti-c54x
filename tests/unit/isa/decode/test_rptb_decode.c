/*
 * Module contract
 *
 * Owns: exhaustive RPTB block-repeat decode proof for the
 * 0xF072 exact row (P3-S79, TI SPRU172C section RPTB[D]
 * with Z=0): first-word resolution, pmad extraction
 * through the reused branch layout, and the
 * single-encoding population with no shadow word.
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
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_RPTB);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(UINT16_C(0xF072));
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {UINT16_C(0xF072), UINT16_C(0x803A)},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xF072));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_RPTB);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16);
    {
        ti_c54x_operand_layout_set_view view;
        ti_c54x_operand_field_value field = {0U, 0U};
        const ti_c54x_program_words_view words = {
            bundle.program_words, bundle.program_word_count};
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                               TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16,
                               UINT8_C(2), &view) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, UINT8_C(0), &field) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(field.width_bits == UINT8_C(16));
        TI_C54X_TEST_CHECK(field.raw_value == UINT16_C(0x803A));
    }

    /* Exact row: only 0xF072 resolves here. */
    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);
        const bool block =
            decoded.id == TI_C54X_OPCODE_ID_RPTB;
        TI_C54X_TEST_CHECK(block == (word == UINT16_C(0xF072)));
    }

    {
        /* Neighbors keep their rows: RPT_LK, the delayed
         * RPTBD first word (own slice), B, and CALL. */
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0xF070)).id ==
                           TI_C54X_OPCODE_ID_RPT_LK);
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0xF272)).id !=
                           TI_C54X_OPCODE_ID_RPTB);
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0xF073)).id ==
                           TI_C54X_OPCODE_ID_B_PMAD);
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0xF074)).id ==
                           TI_C54X_OPCODE_ID_CALL_PMAD);
    }
    return 0;
}
