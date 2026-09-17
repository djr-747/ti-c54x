/*
 * Module contract
 *
 * Owns: executable proof that the shared ACCUMULATOR_SOURCE layout
 * resolves a one-word plan with one operand (accumulator select
 * S[8:8]) and extracts the select for BACC-family first words
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
check_case(uint16_t word, uint8_t select)
{
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value value = {
        UINT64_C(0xDEADBEEF), UINT8_C(0xA5)};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE,
                           UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(0), &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(value.raw_value == select);
    TI_C54X_TEST_CHECK(value.width_bits == UINT8_C(1));
}

static void
test_field_vectors(void)
{
    /* BACC A (S=0) and BACCD B (S=1) first words. */
    check_case(UINT16_C(0xF4E2), UINT8_C(0));
    check_case(UINT16_C(0xF7E2), UINT8_C(1));
    /* CALA B (S=1) and CALAD A (S=0) first words share the layout. */
    check_case(UINT16_C(0xF5E3), UINT8_C(1));
    check_case(UINT16_C(0xF6E3), UINT8_C(0));
}

static void
test_descriptor_identity(void)
{
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE,
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
