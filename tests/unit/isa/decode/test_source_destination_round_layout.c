/*
 * Module contract
 *
 * Owns: executable proof that the SOURCE_DESTINATION_ROUND layout
 * resolves a one-word plan with three 1-bit operands (source
 * select S[9:9], destination select D[8:8], round R[0:0]) and
 * extracts all three fields without touching the catalog
 * internals.
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
check_case(uint16_t word, uint8_t src, uint8_t dst, uint8_t round)
{
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_ROUND,
                           UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(3));

    /* Operand order is source, destination, round. */
    check_operand(&view, &words, UINT8_C(0), src, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), dst, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(2), round, UINT8_C(1));
}

static void
test_field_vectors(void)
{
    check_case(UINT16_C(0xF488), UINT8_C(0), UINT8_C(0), UINT8_C(0));
    check_case(UINT16_C(0xF789), UINT8_C(1), UINT8_C(1), UINT8_C(1));
    check_case(UINT16_C(0xF688), UINT8_C(1), UINT8_C(0), UINT8_C(0));
    check_case(UINT16_C(0xF589), UINT8_C(0), UINT8_C(1), UINT8_C(1));
}

int
main(void)
{
    test_field_vectors();
    return 0;
}
