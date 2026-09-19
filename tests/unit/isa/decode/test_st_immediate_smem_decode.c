/*
 * Module contract
 *
 * Owns: executable proof that all standard-indirect two-word ST immediate
 * first words classify through the generic decoder, copy their complete
 * bundles and scalar metadata, extract ARF then raw extension16, and reject
 * malformed inputs transactionally. Does not own: catalog generation,
 * address resolution, memory effects, CPU/state mutation, fetch or bus
 * behavior, sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t ST_IMMEDIATE_VALUE = UINT16_C(0x7690);

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
check_case(uint8_t mode, uint8_t arf, uint16_t extension)
{
    const uint16_t first_word = (uint16_t)((ST_IMMEDIATE_VALUE &
                                           (uint16_t)~UINT16_C(0x0018)) |
                                           (uint16_t)(mode << 3U) | arf);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, extension},
        .program_word_count = UINT8_C(2)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id ==
                       TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == first_word);
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == extension);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(2), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(3));

    /* Layout order is ARF in word 0, then the raw extension16 word. */
    check_operand(&view, &words, UINT8_C(0), arf, UINT8_C(3));
    check_operand(&view, &words, UINT8_C(1), extension, UINT8_C(16));
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
test_malformed_two_word_inputs_reject_transactionally(void)
{
    const uint16_t first_word = ST_IMMEDIATE_VALUE;
    const ti_c54x_decode_result valid =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {first_word, UINT16_C(0x8000)},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);

    /* Incomplete extension bundle. */
    bundle.program_word_count = UINT8_C(1);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);

    /* Wrong first word in an otherwise complete bundle. */
    bundle = valid_bundle;
    bundle.program_words[0] = UINT16_C(0x7691);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    /* Forged first-word result must not be accepted as this opcode. */
    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0x76B8);
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
test_fixed_bit_near_misses_are_unknown(void)
{
    /* Flipping fixed bit 12 lands in the MAC_SMEM_LK row
     * (0x6690, pinned below); flipping fixed bit 11 lands in the
     * P3-S64 READA Smem row (0x7E90, pinned below); all other
     * fixed-bit flips must miss every row. */
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x0400),
        UINT16_C(0x4000), UINT16_C(0x8000)};

    for (size_t index = 0U;
         index < sizeof(fixed_bit_masks) / sizeof(fixed_bit_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(ST_IMMEDIATE_VALUE ^ fixed_bit_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }

    {
        const ti_c54x_decode_result stm_sibling =
            ti_c54x_decode_first_word((uint16_t)(ST_IMMEDIATE_VALUE ^
                                                 UINT16_C(0x0100)));
        TI_C54X_TEST_CHECK(stm_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(stm_sibling.id == TI_C54X_OPCODE_ID_STM);
    }

    /* Flipping fixed bit 7 lands on 0x7610, claimed by the P3-S36
     * ST_LK_SMEM row (direct dmad 0x10): a valid cross-row near
     * miss, not an overaccept. */
    {
        const ti_c54x_decode_result st_sibling =
            ti_c54x_decode_first_word((uint16_t)(ST_IMMEDIATE_VALUE ^
                                                 UINT16_C(0x0080)));
        TI_C54X_TEST_CHECK(st_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(st_sibling.id == TI_C54X_OPCODE_ID_ST_LK_SMEM);
    }

    /* Flipping fixed bit 12 lands on 0x6690, claimed by the P3-S33
     * MAC_SMEM_LK row: a valid cross-row near miss, not an
     * overaccept. */
    {
        const ti_c54x_decode_result mac_sibling =
            ti_c54x_decode_first_word((uint16_t)(ST_IMMEDIATE_VALUE ^
                                                 UINT16_C(0x1000)));
        TI_C54X_TEST_CHECK(mac_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(mac_sibling.id == TI_C54X_OPCODE_ID_MAC_SMEM_LK);
    }

    /* Flipping fixed bit 11 lands on 0x7E90, claimed by the P3-S64
     * READA_SMEM row: a valid cross-row near miss, not an
     * overaccept. */
    {
        const ti_c54x_decode_result reada_sibling =
            ti_c54x_decode_first_word((uint16_t)(ST_IMMEDIATE_VALUE ^
                                                 UINT16_C(0x0800)));
        TI_C54X_TEST_CHECK(reada_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(reada_sibling.id == TI_C54X_OPCODE_ID_READA_SMEM);
    }

    {
        const ti_c54x_decode_result circular_sibling =
            ti_c54x_decode_first_word((uint16_t)(ST_IMMEDIATE_VALUE ^
                                                 UINT16_C(0x0040)));
        TI_C54X_TEST_CHECK(circular_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(circular_sibling.id ==
                           TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD10);
    }

    {
        const ti_c54x_decode_result indexed_sibling =
            ti_c54x_decode_first_word((uint16_t)(ST_IMMEDIATE_VALUE ^
                                                 UINT16_C(0x0020)));
        TI_C54X_TEST_CHECK(indexed_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(indexed_sibling.id ==
                           TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD6);
    }
}

int
main(void)
{
    static const uint16_t extension_values[] = {
        UINT16_C(0x0000), UINT16_C(0x7FFF), UINT16_C(0x8000),
        UINT16_C(0xFFFF)};

    for (uint8_t mode = 0U; mode < UINT8_C(4); ++mode) {
        for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
            for (size_t index = 0U;
                 index < sizeof(extension_values) / sizeof(extension_values[0]);
                 ++index) {
                check_case(mode, arf, extension_values[index]);
            }
        }
    }
    test_malformed_two_word_inputs_reject_transactionally();
    test_fixed_bit_near_misses_are_unknown();
    return 0;
}
