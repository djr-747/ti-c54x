/*
 * Module contract
 *
 * Owns: executable proof of fixed-MOD2 LD Smem,TS,dst first-word
 * classification, copied metadata, exhaustive D/ARF extraction through the
 * existing accumulator-ARF layout, fixed-bit rejection, and transactional
 * rejection of forged completion inputs. Does not own: catalog generation,
 * address resolution, memory effects, CPU/state mutation, sequencing, timing,
 * or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t LD_SMEM_TS_VALUE = UINT16_C(0x1490);

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
check_case(uint8_t d, uint8_t arf)
{
    const uint16_t word = (uint16_t)(LD_SMEM_TS_VALUE |
                                     (uint16_t)(d << 8U) | arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD2);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD2);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(2));
    check_operand(&view, &words, UINT8_C(0), d, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), arf, UINT8_C(3));
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
test_exhaustive_classification_is_disjoint(void)
{
    unsigned encoded_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool expected =
            (word & UINT16_C(0xFEF8)) == LD_SMEM_TS_VALUE;
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);

        TI_C54X_TEST_CHECK((decoded.id ==
                            TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD2) == expected);
        if (expected) {
            ++encoded_count;
            TI_C54X_TEST_CHECK(decoded.status == TI_C54X_DECODE_STATUS_OK);
            TI_C54X_TEST_CHECK(decoded.program_word_count == UINT8_C(1));
        }
    }
    TI_C54X_TEST_CHECK(encoded_count == 16U);
}

static void
test_fixed_bits_and_nearby_unknown_words(void)
{
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x0008), UINT16_C(0x0010), UINT16_C(0x0020),
        UINT16_C(0x0040), UINT16_C(0x0080), UINT16_C(0x0200),
        UINT16_C(0x0400), UINT16_C(0x0800), UINT16_C(0x1000),
        UINT16_C(0x2000), UINT16_C(0x4000), UINT16_C(0x8000)};

    for (size_t index = 0U;
         index < sizeof(fixed_bit_masks) / sizeof(fixed_bit_masks[0]);
         ++index) {
        const ti_c54x_decode_result result = ti_c54x_decode_first_word(
            (uint16_t)(LD_SMEM_TS_VALUE ^ fixed_bit_masks[index]));

        TI_C54X_TEST_CHECK(result.id != TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD2);
    }

    /* Bit 10 is the established LD Smem sibling; it must not classify as TS. */
    {
        const ti_c54x_decode_result sibling = ti_c54x_decode_first_word(
            (uint16_t)(LD_SMEM_TS_VALUE ^ UINT16_C(0x0400)));
        TI_C54X_TEST_CHECK(sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(sibling.id == TI_C54X_OPCODE_ID_LD_SMEM_MOD2);
    }
}

static void
test_transactional_completion_rejects_forged_inputs(void)
{
    const uint16_t word = (uint16_t)(LD_SMEM_TS_VALUE |
                                     UINT16_C(0x0100) | UINT16_C(7));
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD2);

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
    forged.id = TI_C54X_OPCODE_ID_LD_SMEM_MOD2;
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
    for (uint8_t d = 0U; d < UINT8_C(2); ++d) {
        for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
            check_case(d, arf);
        }
    }
    test_exhaustive_classification_is_disjoint();
    test_fixed_bits_and_nearby_unknown_words();
    test_transactional_completion_rejects_forged_inputs();
    return 0;
}
