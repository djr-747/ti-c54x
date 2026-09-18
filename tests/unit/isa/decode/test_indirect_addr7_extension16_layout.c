/*
 * Module contract
 *
 * Owns: executable proof that the INDIRECT_ADDR7_EXTENSION16 layout
 * resolves a two-word plan with three operands (indirect flag
 * I[7:7] and 7-bit Smem address A[6:0] from word 0 with the
 * copied 16-bit immediate from word 1) and extracts all three
 * fields without touching the catalog internals.
 * Does not own: catalog generation, first-word classification,
 * accumulator/T/status state, CPU sequencing, fetch or bus
 * behavior, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

static void
check_operand(const ti_c54x_operand_layout_set_view *view,
              const ti_c54x_program_words_view *words, uint8_t index,
              uint64_t expected, uint8_t width)
{
    ti_c54x_operand_field_value value = {
        UINT64_C(0xDEADBEEF), UINT8_C(0xA5)};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           view, words, index, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(value.raw_value == expected);
    TI_C54X_TEST_CHECK(value.width_bits == width);
}

static void
check_case(uint16_t first_word, uint16_t second_word, uint8_t indirect,
           uint8_t addr7, uint16_t immediate)
{
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, second_word},
        .program_word_count = UINT8_C(2)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_EXTENSION16,
                           UINT8_C(2), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(3));

    /* Operand order is indirect, addr7, immediate. */
    check_operand(&view, &words, UINT8_C(0), indirect, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), addr7, UINT8_C(7));
    check_operand(&view, &words, UINT8_C(2), immediate, UINT8_C(16));
}

static void
test_field_vectors(void)
{
    /* BITF-style direct word with a full mask immediate. */
    check_case(UINT16_C(0x6100), UINT16_C(0x00FF), UINT8_C(0),
               UINT8_C(0x00), UINT16_C(0x00FF));
    /* CMPM-style indirect word with an equality immediate. */
    check_case(UINT16_C(0x60A5), UINT16_C(0x0404), UINT8_C(1),
               UINT8_C(0x25), UINT16_C(0x0404));
    /* ST-style direct word with a store immediate. */
    check_case(UINT16_C(0x7654), UINT16_C(0x1234), UINT8_C(0),
               UINT8_C(0x54), UINT16_C(0x1234));
}

static void
test_word_count_mismatch_rejects(void)
{
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_EXTENSION16,
                           UINT8_C(1), &view) !=
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
}

int
main(void)
{
    test_field_vectors();
    test_word_count_mismatch_rejects();
    return 0;
}
