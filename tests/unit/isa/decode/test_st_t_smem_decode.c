/*
 * Module contract
 *
 * Owns: executable proof of standard-indirect ST T-to-memory first-word decode,
 * copied one-word bundles and scalar metadata, exhaustive ARF extraction
 * through the single three-bit operand, fixed-bit near-miss rejection, and
 * transactional rejection of forged completion inputs. Does not own: catalog
 * generation, address resolution, memory effects, CPU/state mutation,
 * sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t ST_T_VALUE = UINT16_C(0x8C90);

static void
check_case(uint8_t mode, uint8_t arf)
{
    const uint16_t word = (uint16_t)((ST_T_VALUE &
                                     (uint16_t)~UINT16_C(0x0018)) |
                                     (uint16_t)(mode << 3U) | arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value arf_field = {0U, 0U};
    ti_c54x_operand_field_value mod_field = {0U, 0U};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_ST_T_SMEM);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_ST_T_SMEM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_T_REGISTER);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(0), &arf_field) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(arf_field.width_bits == UINT8_C(3));
    TI_C54X_TEST_CHECK(arf_field.raw_value == arf);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(1), &mod_field) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(mod_field.width_bits == UINT8_C(2));
    TI_C54X_TEST_CHECK(mod_field.raw_value == mode);
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
test_fixed_bit_near_misses_are_unknown(void)
{
    /* Bit 15 (0x8000) leaves in P3-S19: 0x8C90 ^ 0x8000 is 0x0C90, a
     * valid SUB_SMEM_TS word asserted below. */
    /* Clearing bit 7 lands the pinned direct sibling; every other
     * fixed-bit flip stays unbound. */
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x0100), UINT16_C(0x0200),
        UINT16_C(0x0400), UINT16_C(0x0800),
        UINT16_C(0x1000), UINT16_C(0x2000), UINT16_C(0x4000)};

    for (size_t index = 0U;
         index < sizeof(fixed_bit_masks) / sizeof(fixed_bit_masks[0]);
         ++index) {
        const ti_c54x_decode_result result = ti_c54x_decode_first_word(
            (uint16_t)(ST_T_VALUE ^ fixed_bit_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }
    {
        const ti_c54x_decode_result direct_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(ST_T_VALUE ^ UINT16_C(0x0080)));

        TI_C54X_TEST_CHECK(direct_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(direct_sibling.id ==
                           TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
    }

    {
        const ti_c54x_decode_result circular_sibling =
            ti_c54x_decode_first_word((uint16_t)(ST_T_VALUE ^
                                                 UINT16_C(0x0040)));
        TI_C54X_TEST_CHECK(circular_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(circular_sibling.id ==
                           TI_C54X_OPCODE_ID_ST_T_SMEM_MOD10);
    }

    {
        const ti_c54x_decode_result indexed_sibling =
            ti_c54x_decode_first_word((uint16_t)(ST_T_VALUE ^
                                                 UINT16_C(0x0020)));
        TI_C54X_TEST_CHECK(indexed_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(indexed_sibling.id ==
                           TI_C54X_OPCODE_ID_ST_T_SMEM_MOD6);
    }

    /* The adjacent STL sibling is valid and must not be captured by ST T. */
    {
        const ti_c54x_decode_result sibling =
            ti_c54x_decode_first_word(UINT16_C(0x8090));
        TI_C54X_TEST_CHECK(sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(sibling.id == TI_C54X_OPCODE_ID_STL_SMEM);
    }

    /* Flipping bit 15 lands on 0x0C90, claimed by the P3-S19
     * SUB_SMEM_TS row: a valid cross-row near miss, not an
     * overaccept. */
    {
        const ti_c54x_decode_result sub_sibling = ti_c54x_decode_first_word(
            (uint16_t)(ST_T_VALUE ^ UINT16_C(0x8000)));
        TI_C54X_TEST_CHECK(sub_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(sub_sibling.id == TI_C54X_OPCODE_ID_SUB_SMEM_TS);
    }
}

static void
test_transactional_completion_rejects_forged_inputs(void)
{
    const uint16_t word = (uint16_t)(ST_T_VALUE | UINT16_C(7));
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_ST_T_SMEM);

    forged.status = TI_C54X_DECODE_STATUS_UNKNOWN_WORD;
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_NOT_OK);

    forged = valid;
    forged.id = TI_C54X_OPCODE_ID_UNKNOWN;
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_UNKNOWN_OPCODE_ID);

    forged = valid;
    forged.id = TI_C54X_OPCODE_ID_STL_SMEM;
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);

    forged = valid;
    forged.first_word = (uint16_t)(word ^ UINT16_C(0x0008));
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    forged = valid;
    forged.program_word_count = UINT8_C(2);
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH);

    bundle = valid_bundle;
    bundle.program_word_count = UINT8_C(2);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);
}

int
main(void)
{
    for (uint8_t mode = 0U; mode < UINT8_C(4); ++mode) {
        for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
            check_case(mode, arf);
        }
    }
    test_fixed_bit_near_misses_are_unknown();
    test_transactional_completion_rejects_forged_inputs();
    return 0;
}
