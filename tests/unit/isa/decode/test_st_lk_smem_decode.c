/* Module contract
 * Owns: exhaustive ST #lk direct Smem decode and I/A/lk extraction
 * proof, including the ST_IMMEDIATE_SMEM carve-out boundary.
 * Does not own: opcode semantics, CPU state, sequencing, or timing.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct st_lk_case {
    uint16_t word0;
    uint16_t word1;
    uint8_t addr7;
} st_lk_case;

static const st_lk_case cases[] = {
    {UINT16_C(0x7600), UINT16_C(0x0000), UINT8_C(0x00)},
    {UINT16_C(0x7654), UINT16_C(0x1234), UINT8_C(0x54)},
    {UINT16_C(0x767F), UINT16_C(0xFFFF), UINT8_C(0x7F)},
};

int main(void)
{
    unsigned valid_st_lk = 0U;
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ST_LK_SMEM);
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(cases[i].word0);
        const ti_c54x_instruction_word_bundle bundle = {
            .program_words = {cases[i].word0, cases[i].word1},
            .program_word_count = UINT8_C(2)};
        ti_c54x_decoded_instruction instruction;

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFF80));
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
        TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_ST_LK_SMEM);
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
            TI_C54X_TEST_CHECK(field.raw_value == UINT8_C(0));
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
        const bool exact = ((uint16_t)(word & UINT16_C(0xFF80)) ==
                            UINT16_C(0x7600));
        const bool st_lk =
            decoded.id == TI_C54X_OPCODE_ID_ST_LK_SMEM;
        TI_C54X_TEST_CHECK(st_lk == exact);
        if (exact) {
            ++valid_st_lk;
        }
    }
    TI_C54X_TEST_CHECK(valid_st_lk == 128U);

    {
        /* The carve-out and deferred neighbors belong elsewhere. */
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x7690)).id ==
                           TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x7700)).id ==
                           TI_C54X_OPCODE_ID_STM);
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x76E0)).status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
    }
    return 0;
}
