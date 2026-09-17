/*
 * Module contract
 *
 * Owns: executable proof that the DESTINATION_INDIRECT_ADDR7_PMAD16
 * layout resolves a two-word plan with four operands (destination
 * select S[8:8], indirect flag I[7:7], 7-bit Smem address A[6:0]
 * from word 0, verbatim 16-bit pmad from word 1) and extracts all
 * four fields without touching the catalog internals.
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
#include <string.h>

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
check_case(uint16_t word0, uint16_t pmad, uint8_t dst, uint8_t indirect,
           uint8_t addr7)
{
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word0, pmad}, .program_word_count = UINT8_C(2)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7_PMAD16,
                           UINT8_C(2), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(4));

    /* Operand order is destination, indirect, addr7, pmad. */
    check_operand(&view, &words, UINT8_C(0), dst, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), indirect, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(2), addr7, UINT8_C(7));
    check_operand(&view, &words, UINT8_C(3), pmad, UINT8_C(16));
}

static void
test_field_vectors(void)
{
    check_case(UINT16_C(0x7800), UINT16_C(0x0100), UINT8_C(0), UINT8_C(0),
               UINT8_C(0x00));
    check_case(UINT16_C(0x79FF), UINT16_C(0xFFFF), UINT8_C(1), UINT8_C(1),
               UINT8_C(0x7F));
    check_case(UINT16_C(0x7895), UINT16_C(0x0000), UINT8_C(0), UINT8_C(1),
               UINT8_C(0x15));
    check_case(UINT16_C(0x7940), UINT16_C(0xABCD), UINT8_C(1), UINT8_C(0),
               UINT8_C(0x40));
}

static void
test_word_count_mismatch(void)
{
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7_PMAD16,
                           UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_MISMATCH);
}

int
main(void)
{
    test_field_vectors();
    test_word_count_mismatch();
    return 0;
}
