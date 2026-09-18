/*
 * Module contract
 *
 * Owns: executable proof that all 64 one-word LDR Smem,dst first
 * words classify through the generic decoder, copy their complete
 * bundles and scalar metadata, extract destination, ARF, then MOD
 * in layout order, and reject malformed inputs transactionally.
 * Does not own: catalog generation, AR reads, CPU/state mutation,
 * fetch or bus behavior, sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t LDR_SMEM_VALUE = UINT16_C(0x1680);

static uint16_t
first_word(uint8_t destination, uint8_t mode, uint8_t arf)
{
    return (uint16_t)(LDR_SMEM_VALUE |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (uint16_t)(((uint16_t)(mode & UINT8_C(3))) << 3U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

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
check_case(uint8_t destination, uint8_t mode, uint8_t arf)
{
    const uint16_t word = first_word(destination, mode, arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LDR);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LDR);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(3));

    /* Layout order is destination, ARF, MOD. */
    check_operand(&view, &words, UINT8_C(0), destination, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), arf, UINT8_C(3));
    check_operand(&view, &words, UINT8_C(2), mode, UINT8_C(2));
}

static void
check_transactional_failure(
    const ti_c54x_decode_result *first,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_instruction_completion_status expected)
{
    ti_c54x_decoded_instruction output;
    unsigned char before[sizeof(output)];

    (void)memset(&output, 0xA5, sizeof(output));
    (void)memcpy(before, &output, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           first, bundle, &output) == expected);
    TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
}

static void
test_malformed_inputs_reject_transactionally(void)
{
    const uint16_t word = first_word(UINT8_C(1), UINT8_C(2), UINT8_C(5));
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_LDR);

    /* 0x1280 is the LDU_SMEM sibling: valid row, wrong identity. */
    bundle.program_words[0] = UINT16_C(0x1280);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0x1280);
    bundle.program_words[0] = forged.first_word;
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);

    forged = valid;
    forged.program_word_count = UINT8_C(2);
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH);

    forged = valid;
    forged.id = TI_C54X_OPCODE_ID_UNKNOWN;
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_UNKNOWN_OPCODE_ID);
}

static void
test_fixed_bit_near_misses(void)
{
    /* Mask 0xFEE0 leaves D(8) and Smem(4:0) variable. Flipping fixed
     * bit 10 lands on LDU_SMEM, flipping bit 9 lands on LD_SMEM_TS;
     * every other fixed-bit flip misses every catalog row. */
    static const uint16_t unknown_masks[] = {
        UINT16_C(0x2000),
        UINT16_C(0x4000), UINT16_C(0x8000)};

    for (size_t index = 0U;
         index < sizeof(unknown_masks) / sizeof(unknown_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(LDR_SMEM_VALUE ^ unknown_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }

    {
        const ti_c54x_decode_result bit10 =
            ti_c54x_decode_first_word(UINT16_C(0x1280));

        TI_C54X_TEST_CHECK(bit10.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(bit10.id == TI_C54X_OPCODE_ID_LDU_SMEM);
    }
    {
        const ti_c54x_decode_result bit9 =
            ti_c54x_decode_first_word(UINT16_C(0x1480));

        TI_C54X_TEST_CHECK(bit9.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(bit9.id == TI_C54X_OPCODE_ID_LD_SMEM_TS);
    }

    /* Flipping bit 11 lands on 0x1E80, claimed by the P3-S38
     * SUBC_SMEM row: a valid cross-row near miss, not an overaccept. */
    {
        const ti_c54x_decode_result sibling = ti_c54x_decode_first_word(
            (uint16_t)(LDR_SMEM_VALUE ^ UINT16_C(0x0800)));
        TI_C54X_TEST_CHECK(sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(sibling.id == TI_C54X_OPCODE_ID_SUBC_SMEM);
    }

    /* Flipping bit 12 lands on 0x0680, claimed by the P3-S38
     * ADDC_SMEM row: a valid cross-row near miss, not an overaccept. */
    {
        const ti_c54x_decode_result sibling = ti_c54x_decode_first_word(
            (uint16_t)(LDR_SMEM_VALUE ^ UINT16_C(0x1000)));
        TI_C54X_TEST_CHECK(sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(sibling.id == TI_C54X_OPCODE_ID_ADDC_SMEM);
    }
}

int
main(void)
{
    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        for (uint8_t mode = 0U; mode <= UINT8_C(3); ++mode) {
            for (uint8_t arf = 0U; arf <= UINT8_C(7); ++arf) {
                check_case(destination, mode, arf);
            }
        }
    }
    test_malformed_inputs_reject_transactionally();
    test_fixed_bit_near_misses();
    return 0;
}
