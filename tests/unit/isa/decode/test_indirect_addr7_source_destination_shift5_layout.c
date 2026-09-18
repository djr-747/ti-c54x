/*
 * Module contract
 *
 * Owns: executable proof that the INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5
 * layout resolves a two-word plan with four operands (source select
 * S[9:9], destination select D[8:8], one 8-bit Smem specifier cookie
 * with indirect flag I[7:7] over the 7-bit address cookie A[6:0]
 * from word 0, signed 5-bit SHIFT[4:0] from word 1) and extracts
 * every field without touching the catalog internals.
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
check_case(uint16_t word0, uint16_t word1, uint8_t source,
           uint8_t destination, uint8_t smem, uint8_t shift)
{
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word0, word1}, .program_word_count = UINT8_C(2)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5,
                           UINT8_C(2), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(4));

    /* Operand order is source, destination, smem cookie, shift. */
    check_operand(&view, &words, UINT8_C(0), source, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), destination, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(2), smem, UINT8_C(8));
    check_operand(&view, &words, UINT8_C(3), shift, UINT8_C(5));
}

static void
test_field_vectors(void)
{
    check_case(UINT16_C(0x6F00), UINT16_C(0x0C00), UINT8_C(0),
               UINT8_C(0), UINT8_C(0x00), UINT8_C(0));
    check_case(UINT16_C(0x6FFF), UINT16_C(0x0FFF), UINT8_C(1),
               UINT8_C(1), UINT8_C(0xFF), UINT8_C(31));
    check_case(UINT16_C(0x6F82), UINT16_C(0x0D1F), UINT8_C(0),
               UINT8_C(1), UINT8_C(0x82), UINT8_C(31));
    check_case(UINT16_C(0x6F10), UINT16_C(0x0E10), UINT8_C(1),
               UINT8_C(0), UINT8_C(0x10), UINT8_C(16));
}

static void
test_word_count_mismatch_rejects(void)
{
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {UINT16_C(0x6F00), UINT16_C(0x0C00)},
        .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_operand_layout_set_view view;

    (void)memset(&view, 0xA5, sizeof(view));
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5,
                           UINT8_C(1), &view) !=
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    (void)words;
}

int
main(void)
{
    test_field_vectors();
    test_word_count_mismatch_rejects();
    return 0;
}
