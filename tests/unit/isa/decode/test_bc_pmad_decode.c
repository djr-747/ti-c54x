/*
 * Module contract
 *
 * Owns: executable proof that all 256 two-word BC pmad,cond first
 * words classify through the generic decoder, copy their complete
 * bundles and scalar metadata, extract the condition code then the
 * raw 16-bit pmad extension, pin the delayed BCD sibling word,
 * and reject malformed inputs transactionally.
 * Does not own: catalog generation, condition evaluation, PC writes,
 * CPU/state mutation, fetch or bus behavior, sequencing, timing, or
 * handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t BC_PMAD_VALUE = UINT16_C(0xF800);
/* Bit 9 set selects the BCD sibling row bound in W69. */
static const uint16_t BC_DELAYED_WORD = UINT16_C(0xFA00);

static uint16_t
first_word(uint8_t cond)
{
    return (uint16_t)(BC_PMAD_VALUE | cond);
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
check_case(uint8_t cond, uint16_t pmad)
{
    const uint16_t word = first_word(cond);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, pmad}, .program_word_count = UINT8_C(2)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_BC_PMAD);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_BC_PMAD);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == pmad);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(2), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(2));

    /* Layout order is condition code, pmad extension. */
    check_operand(&view, &words, UINT8_C(0), cond, UINT8_C(8));
    check_operand(&view, &words, UINT8_C(1), pmad, UINT8_C(16));
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
        ti_c54x_decode_first_word(first_word(UINT8_C(0x46)));
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {first_word(UINT8_C(0x46)), UINT16_C(0x2000)},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_BC_PMAD);

    /* 0xF0E0 is the SFTL_ACC sibling: valid row, wrong identity. */
    bundle.program_words[0] = UINT16_C(0xF0E0);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0xF000);
    bundle.program_words[0] = forged.first_word;
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);

    forged = valid;
    forged.program_word_count = UINT8_C(1);
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
test_delayed_word_decodes_bcd(void)
{
    /* The W69 BCD row claims the bit-9 word; the W63 UNKNOWN pin
     * updated here under the cross-slice hazard rule. */
    const ti_c54x_decode_result result =
        ti_c54x_decode_first_word(BC_DELAYED_WORD);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_BCD);
    TI_C54X_TEST_CHECK(result.first_word == BC_DELAYED_WORD);
    TI_C54X_TEST_CHECK(result.program_word_count == UINT8_C(2));
}

static void
test_fixed_bit_near_misses(void)
{
    /* Mask 0xFF00 leaves only the condition byte variable. Flipping
     * fixed bit 12 lands on the LD_K sibling, flipping fixed bit 10
     * lands on the RET sibling, flipping fixed bit 9 lands on
     * the W69 BCD sibling, and flipping fixed bit 8 lands on the
     * W73 CC sibling; flipping fixed bit 15 lands on the S3q MACP
     * row, and all other fixed-bit flips must miss
     * every row. */
    static const uint16_t unknown_masks[] = {
        UINT16_C(0x0800), UINT16_C(0x2000)};
    const ti_c54x_decode_result ld_k_sibling =
        ti_c54x_decode_first_word((uint16_t)(BC_PMAD_VALUE ^ UINT16_C(0x1000)));
    const ti_c54x_decode_result ret_sibling =
        ti_c54x_decode_first_word((uint16_t)(BC_PMAD_VALUE ^ UINT16_C(0x0400)));
    const ti_c54x_decode_result bcd_sibling =
        ti_c54x_decode_first_word((uint16_t)(BC_PMAD_VALUE ^ UINT16_C(0x0200)));
    const ti_c54x_decode_result cc_sibling =
        ti_c54x_decode_first_word((uint16_t)(BC_PMAD_VALUE ^ UINT16_C(0x0100)));

    TI_C54X_TEST_CHECK(ld_k_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(ld_k_sibling.id == TI_C54X_OPCODE_ID_LD_K);
    TI_C54X_TEST_CHECK(ret_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(ret_sibling.id == TI_C54X_OPCODE_ID_RET);
    TI_C54X_TEST_CHECK(bcd_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(bcd_sibling.id == TI_C54X_OPCODE_ID_BCD);
    TI_C54X_TEST_CHECK(cc_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(cc_sibling.id == TI_C54X_OPCODE_ID_CC);

    for (size_t index = 0U;
         index < sizeof(unknown_masks) / sizeof(unknown_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(BC_PMAD_VALUE ^ unknown_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }

    /* Clearing fixed bit 14 lands on the S3i MAS dual row. */
    {
        const ti_c54x_decode_result mas_dual_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(BC_PMAD_VALUE ^ UINT16_C(0x4000)));

        TI_C54X_TEST_CHECK(mas_dual_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(mas_dual_sibling.id == TI_C54X_OPCODE_ID_MAS_DUAL_M0_M0);
    }

    /* Flipping fixed bit 15 lands on the S3q MACP row. */
    {
        const ti_c54x_decode_result macp_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(BC_PMAD_VALUE ^ UINT16_C(0x8000)));

        TI_C54X_TEST_CHECK(macp_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(macp_sibling.id == TI_C54X_OPCODE_ID_MACP_SMEM);
    }
}

int
main(void)
{
    static const uint16_t pmads[] = {
        UINT16_C(0x0000), UINT16_C(0x0002), UINT16_C(0x1000),
        UINT16_C(0x2000), UINT16_C(0xFFFF)};

    /* Exhaustive over the condition byte for representative targets. */
    for (uint32_t cond = 0U; cond <= UINT32_C(0xFF); ++cond) {
        for (size_t index = 0U;
             index < sizeof(pmads) / sizeof(pmads[0]); ++index) {
            check_case((uint8_t)cond, pmads[index]);
        }
    }
    test_malformed_inputs_reject_transactionally();
    test_delayed_word_decodes_bcd();
    test_fixed_bit_near_misses();
    return 0;
}
