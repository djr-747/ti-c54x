/* Module contract
 * Owns: exhaustive MACP_SMEM decode and S/I/addr7/pmad extraction proof.
 * Does not own: opcode semantics, CPU state, sequencing, or timing.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct macp_smem_case {
    uint16_t word;
    uint16_t pmad;
    uint8_t dst;
    uint8_t indirect;
    uint8_t addr7;
} macp_smem_case;

static const macp_smem_case cases[] = {
    {UINT16_C(0x7800), UINT16_C(0x0100), UINT8_C(0), UINT8_C(0),
     UINT8_C(0x00)},
    {UINT16_C(0x79FF), UINT16_C(0xFFFF), UINT8_C(1), UINT8_C(1),
     UINT8_C(0x7F)},
    {UINT16_C(0x7895), UINT16_C(0x0000), UINT8_C(0), UINT8_C(1),
     UINT8_C(0x15)},
};

int main(void)
{
    unsigned valid_count = 0U;
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MACP_SMEM);
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(cases[i].word);
        const ti_c54x_instruction_word_bundle bundle = {
            .program_words = {cases[i].word, cases[i].pmad},
            .program_word_count = UINT8_C(2)};
        ti_c54x_decoded_instruction instruction;

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFE00));
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
        TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MACP_SMEM);
        TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &first, &bundle, &instruction) ==
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(instruction.operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7_PMAD16);
        TI_C54X_TEST_CHECK(instruction.repeatability ==
                           TI_C54X_REPEATABILITY_UNRESOLVED);
        TI_C54X_TEST_CHECK(instruction.input_resources ==
                           TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_FRCT_OVM);
        {
            ti_c54x_operand_layout_set_view view;
            ti_c54x_operand_field_value field = {0U, 0U};
            const ti_c54x_program_words_view words = {
                bundle.program_words, bundle.program_word_count};
            const uint64_t expected[4] = {cases[i].dst,
                                          cases[i].indirect,
                                          cases[i].addr7, cases[i].pmad};
            const uint8_t widths[4] = {UINT8_C(1), UINT8_C(1), UINT8_C(7),
                                       UINT8_C(16)};
            TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                   TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7_PMAD16,
                                   UINT8_C(2), &view) ==
                               TI_C54X_OPERAND_LAYOUT_STATUS_OK);
            for (uint8_t index = UINT8_C(0); index < UINT8_C(4); ++index) {
                TI_C54X_TEST_CHECK(
                    ti_c54x_operand_layout_set_extract_operand(
                        &view, &words, index, &field) ==
                    TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                TI_C54X_TEST_CHECK(field.width_bits == widths[index]);
                TI_C54X_TEST_CHECK(field.raw_value == expected[index]);
            }
        }
    }

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);
        const bool macp =
            (uint16_t)(word & UINT16_C(0xFE00)) == UINT16_C(0x7800);
        TI_C54X_TEST_CHECK((decoded.id == TI_C54X_OPCODE_ID_MACP_SMEM) ==
                           macp);
        if (macp) {
            ++valid_count;
        }
    }
    TI_C54X_TEST_CHECK(valid_count == 512U);

    {
        /* 0x7600 is the P3-S36 ST_LK_SMEM base word (pinned in the
         * STM/ST decode sibling tests), so only 0x7C00 stays here. */
        const uint16_t invalid[] = {UINT16_C(0x7C00)};
        for (size_t i = 0U; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
            TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(invalid[i]).status ==
                               TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        }
    }
    return 0;
}
