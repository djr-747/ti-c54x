/*
 * Module contract
 *
 * Owns: executable proof that the single one-word RETE first word
 * classifies through the generic decoder, copies its complete bundle
 * and scalar metadata with no operand extraction, pins the deferred
 * RETED word outside this row, audits fixed-bit near-miss siblings,
 * and rejects malformed inputs transactionally.
 * Does not own: catalog generation, stack pops, CPU/state mutation,
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

static const uint16_t RETE_VALUE = UINT16_C(0xF4EB);
/* Bit 9 set selects the deferred RETED variant. */
static const uint16_t RETED_WORD = UINT16_C(0xF6EB);

static void
check_case(void)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(RETE_VALUE);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {RETE_VALUE},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_RETE);
    TI_C54X_TEST_CHECK(first.first_word == RETE_VALUE);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_RETE);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == RETE_VALUE);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(0));
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
        ti_c54x_decode_first_word(RETE_VALUE);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {RETE_VALUE},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_RETE);

    /* 0xF830 is the BC_PMAD sibling: valid row, wrong identity. */
    bundle.program_words[0] = UINT16_C(0xF830);
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
test_delayed_word_is_unknown(void)
{
    /* The deferred delayed RETED variant decodes UNKNOWN_WORD
     * today; a future RETED slice updates this pin under the
     * cross-slice hazard rule. */
    const ti_c54x_decode_result result =
        ti_c54x_decode_first_word(RETED_WORD);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
    TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
}

static void
test_fixed_bit_near_misses(void)
{
    /* Mask 0xFFFF leaves no operand bit variable. Setting fixed
     * bit 9 lands on the deferred RETED word (pinned UNKNOWN
     * above), clearing fixed bit 6 lands on the CMPR_AR sibling,
     * and clearing fixed bit 10 lands on the SFTL_ACC sibling
     * (both pinned below); all other fixed-bit flips must miss
     * every row. */
    static const uint16_t unknown_masks[] = {
        UINT16_C(0x0001), UINT16_C(0x0002), UINT16_C(0x0004),
        UINT16_C(0x0008), UINT16_C(0x0010), UINT16_C(0x0020),
        UINT16_C(0x0080), UINT16_C(0x0100),
        UINT16_C(0x0800), UINT16_C(0x1000), UINT16_C(0x2000),
        UINT16_C(0x4000), UINT16_C(0x8000)};
    const ti_c54x_decode_result cmpr_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(RETE_VALUE ^ UINT16_C(0x0040)));
    const ti_c54x_decode_result sftl_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(RETE_VALUE ^ UINT16_C(0x0400)));

    TI_C54X_TEST_CHECK(cmpr_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(cmpr_sibling.id == TI_C54X_OPCODE_ID_CMPR_AR);
    TI_C54X_TEST_CHECK(sftl_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(sftl_sibling.id == TI_C54X_OPCODE_ID_SFTL_ACC);

    for (size_t index = 0U;
         index < sizeof(unknown_masks) / sizeof(unknown_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(RETE_VALUE ^ unknown_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }
}

int
main(void)
{
    check_case();
    test_malformed_inputs_reject_transactionally();
    test_delayed_word_is_unknown();
    test_fixed_bit_near_misses();
    return 0;
}
