/*
 * Module contract
 *
 * Owns: executable proof that the N_CONDITION8 layout resolves a
 * one-word plan with two operands (execute-count select N[9:9]
 * and 8-bit condition code C[7:0]) and extracts both fields
 * without touching the catalog internals.
 * Does not own: catalog generation, first-word classification,
 * accumulator/status state, CPU sequencing, fetch or bus
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
check_case(uint16_t word, uint8_t select, uint8_t cond)
{
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_N_CONDITION8,
                           UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(2));

    /* Operand order is select, condition. */
    check_operand(&view, &words, UINT8_C(0), select, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), cond, UINT8_C(8));
}

static void
test_field_vectors(void)
{
    check_case(UINT16_C(0xFD00), UINT8_C(0), UINT8_C(0x00));
    check_case(UINT16_C(0xFF47), UINT8_C(1), UINT8_C(0x47));
    check_case(UINT16_C(0xFD30), UINT8_C(0), UINT8_C(0x30));
    check_case(UINT16_C(0xFFFF), UINT8_C(1), UINT8_C(0xFF));
}

static void
test_descriptor_identity(void)
{
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_N_CONDITION8,
                           UINT8_C(2), &view) !=
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
}

int
main(void)
{
    test_field_vectors();
    test_descriptor_identity();
    return 0;
}
