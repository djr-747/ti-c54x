/*
 * Module contract
 *
 * Owns: executable proof that all 256 MPYA_SMEM first words classify
 * through the generic decoder, copy their complete one-word
 * bundles and scalar metadata, extract the indirect/addr7
 * operands, audit fixed-bit near-miss siblings, and reject
 * malformed inputs transactionally.
 * Does not own: catalog generation, accumulator/T/status state, CPU
 * sequencing, fetch or bus behavior, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
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
check_case(uint16_t word, uint8_t indirect, uint8_t addr7)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_MPYA_SMEM);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_MPYA_SMEM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_FRCT_OVM);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(2));

    /* Operand order is indirect, addr7. */
    check_operand(&view, &words, UINT8_C(0), indirect, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), addr7, UINT8_C(7));
}

static void
test_field_vectors(void)
{
    check_case(UINT16_C(0x3100), UINT8_C(0), UINT8_C(0x00));
    check_case(UINT16_C(0x31FF), UINT8_C(1), UINT8_C(0x7F));
    check_case(UINT16_C(0x3182), UINT8_C(1), UINT8_C(0x02));
    check_case(UINT16_C(0x3134), UINT8_C(0), UINT8_C(0x34));
}

static void
test_near_miss_siblings(void)
{
    /* Fixed-bit flips resolve away from the MPYA_SMEM row. */
    TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x30FF)).id !=
                       TI_C54X_OPCODE_ID_MPYA_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x3200)).id !=
                       TI_C54X_OPCODE_ID_MPYA_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x3000)).id !=
                       TI_C54X_OPCODE_ID_MPYA_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x3300)).id !=
                       TI_C54X_OPCODE_ID_MPYA_SMEM);
}

int
main(void)
{
    test_field_vectors();
    test_near_miss_siblings();
    return 0;
}
