/* Module contract
 *
 * Owns: exhaustive LTD Smem decode and indirect/addr7
 * extraction proof.
 * Does not own: opcode semantics, CPU state, sequencing, or timing.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct ltd_case {
    uint16_t word;
    uint8_t indirect;
    uint8_t address;
} ltd_case;

static const ltd_case cases[] = {
    {UINT16_C(0x4C00), UINT8_C(0), UINT8_C(0x00)},
    {UINT16_C(0x4C85), UINT8_C(1), UINT8_C(0x05)},
    {UINT16_C(0x4C43), UINT8_C(0), UINT8_C(0x43)},
};

int main(void)
{
    unsigned valid_ltd = 0U;
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LTD_SMEM);
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(cases[i].word);
        const ti_c54x_instruction_word_bundle bundle = {
            .program_words = {cases[i].word},
            .program_word_count = UINT8_C(1)};
        ti_c54x_decoded_instruction instruction;

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFF00));
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
        TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LTD_SMEM);
        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &first, &bundle, &instruction) ==
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(instruction.operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7);
        {
            ti_c54x_operand_layout_set_view view;
            ti_c54x_operand_field_value field = {0U, 0U};
            const ti_c54x_program_words_view words = {
                bundle.program_words, bundle.program_word_count};
            TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                   TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7,
                                   UINT8_C(1), &view) ==
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
            TI_C54X_TEST_CHECK(field.raw_value == cases[i].address);
        }
    }

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);
        const bool exact = ((uint16_t)(word & UINT16_C(0xFF00)) ==
                            UINT16_C(0x4C00));
        const bool ltd =
            decoded.id == TI_C54X_OPCODE_ID_LTD_SMEM;
        TI_C54X_TEST_CHECK(ltd == exact);
        if (exact) {
            ++valid_ltd;
        }
    }
    TI_C54X_TEST_CHECK(valid_ltd == 256U);

    {
        /* Below-neighbor is unbound; above-neighbor is the pinned
         * DELAY sibling. */
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x4BFF)).id ==
                           TI_C54X_OPCODE_ID_UNKNOWN);
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x4D00)).id ==
                           TI_C54X_OPCODE_ID_DELAY_SMEM);
    }
    return 0;
}
