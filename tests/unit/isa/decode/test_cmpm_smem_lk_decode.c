/* Module contract
 * Owns: exhaustive CMPM Smem,#lk decode and I/A/lk extraction proof.
 * Does not own: opcode semantics, CPU state, sequencing, or timing.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct cmpm_case {
    uint16_t word0;
    uint16_t word1;
    uint8_t indirect;
    uint8_t addr7;
} cmpm_case;

static const cmpm_case cases[] = {
    {UINT16_C(0x6000), UINT16_C(0x0404), UINT8_C(0), UINT8_C(0x00)},
    {UINT16_C(0x6095), UINT16_C(0x1234), UINT8_C(1), UINT8_C(0x15)},
    {UINT16_C(0x60FF), UINT16_C(0xFFFF), UINT8_C(1), UINT8_C(0x7F)},
};

int main(void)
{
    unsigned valid_cmpm = 0U;
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_CMPM_SMEM_LK);
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(cases[i].word0);
        const ti_c54x_instruction_word_bundle bundle = {
            .program_words = {cases[i].word0, cases[i].word1},
            .program_word_count = UINT8_C(2)};
        ti_c54x_decoded_instruction instruction;

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFF00));
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
        TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_CMPM_SMEM_LK);
        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &first, &bundle, &instruction) ==
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(instruction.operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_EXTENSION16);
        {
            ti_c54x_operand_layout_set_view view;
            ti_c54x_operand_field_value field = {0U, 0U};
            const ti_c54x_program_words_view words = {
                bundle.program_words, bundle.program_word_count};
            TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                   TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_EXTENSION16,
                                   UINT8_C(2), &view) ==
                               TI_C54X_OPERAND_LAYOUT_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                                   &view, &words, UINT8_C(0), &field) ==
                               TI_C54X_OPERAND_LAYOUT_STATUS_OK);
            TI_C54X_TEST_CHECK(field.width_bits == UINT8_C(1));
            TI_C54X_TEST_CHECK(field.raw_value == cases[i].indirect);
            TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                                   &view, &words, UINT8_C(1), &field) ==
                               TI_C54X_OPERAND_LAYOUT_STATUS_OK);
            TI_C54X_TEST_CHECK(field.width_bits == UINT8_C(7));
            TI_C54X_TEST_CHECK(field.raw_value == cases[i].addr7);
            TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                                   &view, &words, UINT8_C(2), &field) ==
                               TI_C54X_OPERAND_LAYOUT_STATUS_OK);
            TI_C54X_TEST_CHECK(field.width_bits == UINT8_C(16));
            TI_C54X_TEST_CHECK(field.raw_value == cases[i].word1);
        }
    }

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);
        const bool exact = ((uint16_t)(word & UINT16_C(0xFF00)) ==
                            UINT16_C(0x6000));
        const bool cmpm =
            decoded.id == TI_C54X_OPCODE_ID_CMPM_SMEM_LK;
        TI_C54X_TEST_CHECK(cmpm == exact);
        if (exact) {
            ++valid_cmpm;
        }
    }
    TI_C54X_TEST_CHECK(valid_cmpm == 256U);

    {
        /* Neighbors belong to other rows or nothing. */
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x6100)).id ==
                           TI_C54X_OPCODE_ID_BITF_SMEM_LK);
        /* Below-neighbor 0x5FFF is the P3-S100 DSADT row. */
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x5FFF)).id ==
                           TI_C54X_OPCODE_ID_DSADT);
    }
    return 0;
}
