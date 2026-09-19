/* Module contract
 * Owns: exhaustive PSHD/POPD/PSHM/POPM decode and I/cookie
 * extraction proof, including the unbound neighborhood on both
 * sides of each row.
 * Does not own: opcode semantics, CPU state, sequencing, or timing.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct pair_decode_case {
    ti_c54x_opcode_id id;
    uint16_t word;
    uint8_t indirect;
    uint8_t cookie;
} pair_decode_case;

static const pair_decode_case cases[] = {
    {TI_C54X_OPCODE_ID_PSHD_DIRECT, UINT16_C(0x4B00), UINT8_C(0),
     UINT8_C(0x00)},
    {TI_C54X_OPCODE_ID_PSHD_DIRECT, UINT16_C(0x4B60), UINT8_C(0),
     UINT8_C(0x60)},
    {TI_C54X_OPCODE_ID_PSHD_DIRECT, UINT16_C(0x4BFF), UINT8_C(1),
     UINT8_C(0x7F)},
    {TI_C54X_OPCODE_ID_POPD_DIRECT, UINT16_C(0x8B10), UINT8_C(0),
     UINT8_C(0x10)},
    {TI_C54X_OPCODE_ID_POPD_DIRECT, UINT16_C(0x8B80), UINT8_C(1),
     UINT8_C(0x00)},
    {TI_C54X_OPCODE_ID_PSHM_DIRECT, UINT16_C(0x4A1A), UINT8_C(0),
     UINT8_C(0x1A)},
    {TI_C54X_OPCODE_ID_PSHM_DIRECT, UINT16_C(0x4AFF), UINT8_C(1),
     UINT8_C(0x7F)},
    {TI_C54X_OPCODE_ID_POPM_DIRECT, UINT16_C(0x8A15), UINT8_C(0),
     UINT8_C(0x15)},
    {TI_C54X_OPCODE_ID_POPM_DIRECT, UINT16_C(0x8A80), UINT8_C(1),
     UINT8_C(0x00)},
};

static void
check_case(const pair_decode_case *c)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(c->word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {c->word},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == c->id);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7);
    {
        ti_c54x_operand_layout_set_view view;
        ti_c54x_operand_field_value indirect_field = {0U, 0U};
        ti_c54x_operand_field_value addr_field = {0U, 0U};
        const ti_c54x_program_words_view words = {
            bundle.program_words, bundle.program_word_count};
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                               TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7,
                               UINT8_C(1), &view) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, UINT8_C(0),
                               &indirect_field) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, UINT8_C(1), &addr_field) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(indirect_field.width_bits == UINT8_C(1));
        TI_C54X_TEST_CHECK(indirect_field.raw_value == c->indirect);
        TI_C54X_TEST_CHECK(addr_field.width_bits == UINT8_C(7));
        TI_C54X_TEST_CHECK(addr_field.raw_value == c->cookie);
    }
}

int main(void)
{
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        check_case(&cases[i]);
    }

    /* Each row owns exactly its 256 first words. */
    {
        unsigned valid_pshd = 0U;
        unsigned valid_popd = 0U;
        unsigned valid_pshm = 0U;
        unsigned valid_popm = 0U;
        for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
            const uint16_t word = (uint16_t)raw;
            const ti_c54x_decode_result decoded =
                ti_c54x_decode_first_word(word);
            if (decoded.id == TI_C54X_OPCODE_ID_PSHD_DIRECT) {
                TI_C54X_TEST_CHECK(((uint16_t)(word & UINT16_C(0xFF00)) ==
                                    UINT16_C(0x4B00)));
                ++valid_pshd;
            }
            if (decoded.id == TI_C54X_OPCODE_ID_POPD_DIRECT) {
                TI_C54X_TEST_CHECK(((uint16_t)(word & UINT16_C(0xFF00)) ==
                                    UINT16_C(0x8B00)));
                ++valid_popd;
            }
            if (decoded.id == TI_C54X_OPCODE_ID_PSHM_DIRECT) {
                TI_C54X_TEST_CHECK(((uint16_t)(word & UINT16_C(0xFF00)) ==
                                    UINT16_C(0x4A00)));
                ++valid_pshm;
            }
            if (decoded.id == TI_C54X_OPCODE_ID_POPM_DIRECT) {
                TI_C54X_TEST_CHECK(((uint16_t)(word & UINT16_C(0xFF00)) ==
                                    UINT16_C(0x8A00)));
                ++valid_popm;
            }
        }
        TI_C54X_TEST_CHECK(valid_pshd == 256U);
        TI_C54X_TEST_CHECK(valid_popd == 256U);
        TI_C54X_TEST_CHECK(valid_pshm == 256U);
        TI_C54X_TEST_CHECK(valid_popm == 256U);
    }

    {
        /* The outer neighbors are unbound. */
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x49FF)).id ==
                           TI_C54X_OPCODE_ID_UNKNOWN);
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x89FF)).id ==
                           TI_C54X_OPCODE_ID_UNKNOWN);
    }
    return 0;
}
