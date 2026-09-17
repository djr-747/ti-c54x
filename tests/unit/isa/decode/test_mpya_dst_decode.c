/* Module contract
 * Owns: exhaustive MPYA-dst decode and D-bit extraction proof.
 * Does not own: opcode semantics, CPU state, sequencing, or timing.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct mpya_dst_case {
    uint16_t word;
    uint8_t destination;
} mpya_dst_case;

static const mpya_dst_case cases[] = {
    {UINT16_C(0xF48C), UINT8_C(0)},
    {UINT16_C(0xF58C), UINT8_C(1)},
};

int main(void)
{
    unsigned valid_mpya_dst = 0U;
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MPYA_DST);
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(cases[i].word);
        const ti_c54x_instruction_word_bundle bundle = {
            .program_words = {cases[i].word, UINT16_C(0)},
            .program_word_count = UINT8_C(1)};
        ti_c54x_decoded_instruction instruction;

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFEFF));
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
        TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MPYA_DST);
        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &first, &bundle, &instruction) ==
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(instruction.operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_MAX_MIN_ACC);
        {
            ti_c54x_operand_layout_set_view view;
            ti_c54x_operand_field_value field = {0U, 0U};
            const ti_c54x_program_words_view words = {
                bundle.program_words, bundle.program_word_count};
            TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                   TI_C54X_OPERAND_LAYOUT_MAX_MIN_ACC,
                                   UINT8_C(1), &view) ==
                               TI_C54X_OPERAND_LAYOUT_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                                   &view, &words, UINT8_C(0), &field) ==
                               TI_C54X_OPERAND_LAYOUT_STATUS_OK);
            TI_C54X_TEST_CHECK(field.width_bits == UINT8_C(1));
            TI_C54X_TEST_CHECK(field.raw_value == cases[i].destination);
        }
    }

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);
        const bool exact = word == UINT16_C(0xF48C) ||
                           word == UINT16_C(0xF58C);
        const bool mpya_dst =
            decoded.id == TI_C54X_OPCODE_ID_MPYA_DST;
        TI_C54X_TEST_CHECK(mpya_dst == exact);
        if (exact) {
            ++valid_mpya_dst;
        }
    }
    TI_C54X_TEST_CHECK(valid_mpya_dst == 2U);

    {
        /* 0xF48D/0xF58D belonged here until the S3l SQUR_ADST slice
         * claimed them; the recorded W57->W58 cross-slice update
         * replaces them with verified-unknown neighbors. */
        const uint16_t invalid[] = {UINT16_C(0xF49C), UINT16_C(0xF59C),
                                    UINT16_C(0xF68C), UINT16_C(0xE48C)};
        for (size_t i = 0U; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
            TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(invalid[i]).status ==
                               TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        }
    }
    return 0;
}
