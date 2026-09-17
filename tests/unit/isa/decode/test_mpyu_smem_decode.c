/* Module contract
 * Owns: exhaustive MPYU decode and D/I/addr7 extraction proof.
 * Does not own: opcode semantics, CPU state, sequencing, or timing.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct mpyu_smem_case {
    uint16_t word;
    uint8_t select;
    uint8_t indirect;
    uint8_t addr7;
} mpyu_smem_case;

static const mpyu_smem_case cases[] = {
    {UINT16_C(0x2400), UINT8_C(0), UINT8_C(0), UINT8_C(0x00)},
    {UINT16_C(0x2582), UINT8_C(1), UINT8_C(1), UINT8_C(0x02)},
};

int main(void)
{
    unsigned valid_count = 0U;
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MPYU_SMEM);
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(cases[i].word);
        const ti_c54x_instruction_word_bundle bundle = {
            .program_words = {cases[i].word, UINT16_C(0)},
            .program_word_count = UINT8_C(1)};
        ti_c54x_decoded_instruction instruction;

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFE00));
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
        TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MPYU_SMEM);
        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &first, &bundle, &instruction) ==
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(instruction.operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7);
        {
            ti_c54x_operand_layout_set_view view;
            ti_c54x_operand_field_value field = {0U, 0U};
            const ti_c54x_program_words_view words = {
                bundle.program_words, bundle.program_word_count};
            const uint8_t expected[3] = {cases[i].select,
                                         cases[i].indirect, cases[i].addr7};
            const uint8_t widths[3] = {UINT8_C(1), UINT8_C(1), UINT8_C(7)};
            TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                   TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7,
                                   UINT8_C(1), &view) ==
                               TI_C54X_OPERAND_LAYOUT_STATUS_OK);
            for (uint8_t index = UINT8_C(0); index < UINT8_C(3); ++index) {
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
        const bool mpyu =
            (uint16_t)(word & UINT16_C(0xFE00)) == UINT16_C(0x2400);
        TI_C54X_TEST_CHECK((decoded.id == TI_C54X_OPCODE_ID_MPYU_SMEM) ==
                           mpyu);
        if (mpyu) {
            ++valid_count;
        }
    }
    TI_C54X_TEST_CHECK(valid_count == 512U);

    {
        const uint16_t invalid[] = {UINT16_C(0x3000), UINT16_C(0x0000)};
        for (size_t i = 0U; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
            TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(invalid[i]).status ==
                               TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        }
    }
    return 0;
}
