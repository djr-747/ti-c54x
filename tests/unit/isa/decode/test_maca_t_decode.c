/* Module contract
 * Owns: exhaustive MACA_T decode and S/D/R extraction proof.
 * Does not own: opcode semantics, CPU state, sequencing, or timing.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct maca_t_case {
    uint16_t word;
    uint8_t src;
    uint8_t dst;
    uint8_t round;
} maca_t_case;

static const maca_t_case cases[] = {
    {UINT16_C(0xF488), UINT8_C(0), UINT8_C(0), UINT8_C(0)},
    {UINT16_C(0xF789), UINT8_C(1), UINT8_C(1), UINT8_C(1)},
};

int main(void)
{
    unsigned valid_count = 0U;
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MACA_T);
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(cases[i].word);
        const ti_c54x_instruction_word_bundle bundle = {
            .program_words = {cases[i].word, UINT16_C(0)},
            .program_word_count = UINT8_C(1)};
        ti_c54x_decoded_instruction instruction;

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFCFE));
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
        TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MACA_T);
        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &first, &bundle, &instruction) ==
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(instruction.operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_ROUND);
        {
            ti_c54x_operand_layout_set_view view;
            ti_c54x_operand_field_value field = {0U, 0U};
            const ti_c54x_program_words_view words = {
                bundle.program_words, bundle.program_word_count};
            const uint8_t expected[3] = {cases[i].src,
                                         cases[i].dst, cases[i].round};
            const uint8_t widths[3] = {UINT8_C(1), UINT8_C(1), UINT8_C(1)};
            TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                   TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_ROUND,
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
        const bool maca =
            (uint16_t)(word & UINT16_C(0xFCFE)) == UINT16_C(0xF488);
        TI_C54X_TEST_CHECK((decoded.id == TI_C54X_OPCODE_ID_MACA_T) ==
                           maca);
        if (maca) {
            ++valid_count;
        }
    }
    TI_C54X_TEST_CHECK(valid_count == 8U);

    {
        const uint16_t invalid[] = {UINT16_C(0xF48F)};
        for (size_t i = 0U; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
            TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(invalid[i]).status ==
                               TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        }
    }

    /* 0xF480 is claimed by the P3-S29 ADD_SRC_ASM row with S=0
     * (src A), D=0 (dst A): a valid cross-row word, not an
     * overaccept. */
    {
        const ti_c54x_decode_result sibling =
            ti_c54x_decode_first_word(UINT16_C(0xF480));
        TI_C54X_TEST_CHECK(sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(sibling.id == TI_C54X_OPCODE_ID_ADD_SRC_ASM);
    }
    return 0;
}
