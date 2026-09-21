/*
 * Module contract
 *
 * Owns: executable proof that all 256 RPT_K first words classify
 * through the generic decoder, copy their complete one-word
 * bundles and scalar metadata, extract the 8-bit K operand,
 * audit fixed-bit near-miss siblings, and reject malformed
 * inputs transactionally.
 * Does not own: catalog generation, RC state, CPU sequencing,
 * fetch or bus behavior, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t RPT_K_VALUE = UINT16_C(0xEC00);

static uint16_t
first_word(uint8_t suffix)
{
    return (uint16_t)(RPT_K_VALUE | suffix);
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
check_case(uint8_t suffix)
{
    const uint16_t word = first_word(suffix);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_RPT_K);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_RPT_K);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(1));

    /* The layout carries the single K operand. */
    check_operand(&view, &words, UINT8_C(0), suffix, UINT8_C(8));
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
    const ti_c54x_decode_result valid =
        ti_c54x_decode_first_word(first_word(UINT8_C(0x02)));
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {first_word(UINT8_C(0x02)), UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_RPT_K);

    bundle.program_word_count = UINT8_C(2);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0xED02);
    bundle.program_words[0] = forged.first_word;
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);

    forged = valid;
    forged.id = TI_C54X_OPCODE_ID_UNKNOWN;
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_UNKNOWN_OPCODE_ID);
}

static void
test_fixed_bit_near_misses(void)
{
    /* Mask 0xFF00 fixes bits 15:8 and leaves the K byte variable
     * (pinned by the exhaustive loop). Flipping bit 8 lands on the
     * LD_IMMEDIATE_ASM row, bit 9 on the P3-S51 FRAME row, bit 10
     * on LD_K, bit 12 on RET (which shadows the RC row at exactly
     * 0xFC00), bit 14 on the P3-S114 LD parallel-MAS row (pinned
     * below), and bit 15 on BANZ (each pinned below); flipping
     * fixed bit 13 lands on the P3-S116 ST_PARALLEL_MPY sibling
     * (pinned below); all other fixed-bit flips must miss every
     * row. */
    static const uint16_t unknown_masks[] = {
        UINT16_C(0x0800)};

    for (size_t index = 0U;
         index < sizeof(unknown_masks) / sizeof(unknown_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_K_VALUE ^ unknown_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }

    /* This former near-miss is the P3-S116 ST_PARALLEL_MPY sibling:
     * 0xEC00 ^ 0x2000 = 0xCC00 classifies into the
     * ST_PARALLEL_MPY row. */
    {
        const ti_c54x_decode_result mpy_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_K_VALUE ^ UINT16_C(0x2000)));

        TI_C54X_TEST_CHECK(mpy_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(mpy_sibling.id == TI_C54X_OPCODE_ID_ST_PARALLEL_MPY);
    }

    /* Flipping fixed bit 14 lands on 0xAC00, the P3-S114 LD
     * parallel-MAS row. */
    {
        const ti_c54x_decode_result parallel_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_K_VALUE ^ UINT16_C(0x4000)));

        TI_C54X_TEST_CHECK(parallel_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(parallel_sibling.id ==
                           TI_C54X_OPCODE_ID_LD_PARALLEL_MAS);
    }

    /* Flipping fixed bit 12 lands on the RET sibling. */
    {
        const ti_c54x_decode_result ret_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_K_VALUE ^ UINT16_C(0x1000)));

        TI_C54X_TEST_CHECK(ret_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(ret_sibling.id == TI_C54X_OPCODE_ID_RET);
    }

    /* Flipping bit 9 lands on the P3-S51 FRAME row. */
    {
        const ti_c54x_decode_result frame_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_K_VALUE ^ UINT16_C(0x0200)));

        TI_C54X_TEST_CHECK(frame_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(frame_sibling.id == TI_C54X_OPCODE_ID_FRAME);
    }

    /* Flipping bit 8 lands on LD_IMMEDIATE_ASM. */
    {
        const ti_c54x_decode_result asm_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_K_VALUE ^ UINT16_C(0x0100)));

        TI_C54X_TEST_CHECK(asm_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(asm_sibling.id ==
                           TI_C54X_OPCODE_ID_LD_IMMEDIATE_ASM);
    }

    /* Flipping bit 10 lands on LD_K. */
    {
        const ti_c54x_decode_result k_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_K_VALUE ^ UINT16_C(0x0400)));

        TI_C54X_TEST_CHECK(k_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(k_sibling.id == TI_C54X_OPCODE_ID_LD_K);
    }

    /* Flipping bit 15 lands on BANZ. */
    {
        const ti_c54x_decode_result banz_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_K_VALUE ^ UINT16_C(0x8000)));

        TI_C54X_TEST_CHECK(banz_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(banz_sibling.id == TI_C54X_OPCODE_ID_BANZ);
    }
}

int
main(void)
{
    /* Exhaustive over the K byte. */
    for (uint32_t suffix = 0U; suffix <= UINT32_C(0xFF); ++suffix) {
        check_case((uint8_t)suffix);
    }
    test_malformed_inputs_reject_transactionally();
    test_fixed_bit_near_misses();
    return 0;
}
