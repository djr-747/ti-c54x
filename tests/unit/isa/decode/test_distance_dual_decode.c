/*
 * Module contract
 *
 * Owns: executable proof that all 512 one-word SQDST/ABDST
 * Xmem,Ymem first words classify through the generic decoder,
 * copy their complete bundles and scalar metadata, extract
 * Xmod, raw Xar, Ymod, then raw Yar in layout order, and reject
 * malformed inputs transactionally.
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

static uint16_t
first_word(uint16_t base, uint8_t xmod, uint8_t xar_field, uint8_t ymod,
           uint8_t yar_field)
{
    return (uint16_t)(base |
                      (uint16_t)(((uint16_t)(xmod & UINT8_C(3))) << 6U) |
                      (uint16_t)(((uint16_t)(xar_field & UINT8_C(3)))
                                 << 4U) |
                      (uint16_t)(((uint16_t)(ymod & UINT8_C(3))) << 2U) |
                      (uint16_t)(yar_field & UINT8_C(3)));
}

static void
check_operand(const ti_c54x_operand_layout_set_view *view,
              const ti_c54x_program_words_view *words, uint8_t index,
              uint64_t expected)
{
    ti_c54x_operand_field_value value = {
        UINT64_C(0xDEADBEEF), UINT8_C(0xA5)};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           view, words, index, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(value.raw_value == expected);
    TI_C54X_TEST_CHECK(value.width_bits == UINT8_C(2));
}

static void
check_case(uint16_t base, ti_c54x_opcode_id expected_id, uint8_t xmod,
           uint8_t xar_field, uint8_t ymod, uint8_t yar_field)
{
    const uint16_t word = first_word(base, xmod, xar_field, ymod,
                                     yar_field);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == expected_id);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == expected_id);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_YMOD_YAR);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_SXM_FRCT_OVM);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(4));

    /* Layout order is Xmod, raw Xar, Ymod, raw Yar. The handlers
     * map the raw fields to AR2..AR5; decode exposes raw bits. */
    check_operand(&view, &words, UINT8_C(0), xmod);
    check_operand(&view, &words, UINT8_C(1), xar_field);
    check_operand(&view, &words, UINT8_C(2), ymod);
    check_operand(&view, &words, UINT8_C(3), yar_field);
}

static void
test_all_first_words_classify(void)
{
    static const uint16_t bases[] = {UINT16_C(0xE200),
                                     UINT16_C(0xE300)};
    static const ti_c54x_opcode_id ids[] = {
        TI_C54X_OPCODE_ID_SQDST_XMEM_YMEM,
        TI_C54X_OPCODE_ID_ABDST_XMEM_YMEM};
    size_t row;

    for (row = 0U; row < sizeof(bases) / sizeof(bases[0]); ++row) {
        for (uint16_t xmod = 0U; xmod < 4U; ++xmod) {
            for (uint16_t xar_field = 0U; xar_field < 4U; ++xar_field) {
                for (uint16_t ymod = 0U; ymod < 4U; ++ymod) {
                    for (uint16_t yar_field = 0U; yar_field < 4U;
                         ++yar_field) {
                        check_case(bases[row], ids[row], (uint8_t)xmod,
                                   (uint8_t)xar_field, (uint8_t)ymod,
                                   (uint8_t)yar_field);
                    }
                }
            }
        }
    }
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
    const uint16_t word = first_word(UINT16_C(0xE200), UINT8_C(2),
                                     UINT8_C(1), UINT8_C(2), UINT8_C(2));
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_SQDST_XMEM_YMEM);

    /* 0xE3A6 keeps the low byte but flips fixed bit 8: ABDST. */
    bundle.program_words[0] = UINT16_C(0xE3A6);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0xE300);
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
check_sibling(uint16_t word, ti_c54x_opcode_id expected)
{
    const ti_c54x_decode_result sibling =
        ti_c54x_decode_first_word(word);

    TI_C54X_TEST_CHECK(sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(sibling.id == expected);
}

static void
check_miss(uint16_t word)
{
    const ti_c54x_decode_result miss = ti_c54x_decode_first_word(word);

    TI_C54X_TEST_CHECK(miss.status != TI_C54X_DECODE_STATUS_OK ||
                       miss.id == TI_C54X_OPCODE_ID_UNKNOWN);
}

static void
test_fixed_bit_near_misses(void)
{
    /* Mask 0xFF00 leaves XMOD(7:6), XAR(5:4), YMOD(3:2),
     * YAR(1:0) variable. Flipping fixed bit 8 crosses the two
     * distance rows (pinned below); flipping fixed bit 9 lands on
     * the landed FIRS row and on the landed LMS row; flipping
     * fixed bit 10 lands on the
     * unclaimed 0xE6 slot for SQDST and on MVMM for ABDST;
     * flipping fixed bit 13 lands on the landed ST_PARALLEL_ADD
     * row for both bases; all other fixed-bit 15:8 flips land on
     * the pinned siblings or miss every row. */
    check_sibling((uint16_t)(UINT16_C(0xE2A6) ^ UINT16_C(0x0100)),
                  TI_C54X_OPCODE_ID_ABDST_XMEM_YMEM);
    check_sibling((uint16_t)(UINT16_C(0xE3A6) ^ UINT16_C(0x0100)),
                  TI_C54X_OPCODE_ID_SQDST_XMEM_YMEM);
    check_sibling((uint16_t)(UINT16_C(0xE2A6) ^ UINT16_C(0x0200)),
                  TI_C54X_OPCODE_ID_FIRS_XMEM_YMEM_PMAD);
    check_sibling((uint16_t)(UINT16_C(0xE3A6) ^ UINT16_C(0x0200)),
                  TI_C54X_OPCODE_ID_LMS_XMEM_YMEM);
    check_miss((uint16_t)(UINT16_C(0xE2A6) ^ UINT16_C(0x0400)));
    check_sibling((uint16_t)(UINT16_C(0xE3A6) ^ UINT16_C(0x0400)),
                  TI_C54X_OPCODE_ID_MVMM);
    check_sibling((uint16_t)(UINT16_C(0xE2A6) ^ UINT16_C(0x0800)),
                  TI_C54X_OPCODE_ID_LD_IMMEDIATE_DP);
    check_sibling((uint16_t)(UINT16_C(0xE3A6) ^ UINT16_C(0x0800)),
                  TI_C54X_OPCODE_ID_LD_IMMEDIATE_DP);
    check_sibling((uint16_t)(UINT16_C(0xE2A6) ^ UINT16_C(0x1000)),
                  TI_C54X_OPCODE_ID_OR_ACC_SHIFT);
    check_sibling((uint16_t)(UINT16_C(0xE3A6) ^ UINT16_C(0x1000)),
                  TI_C54X_OPCODE_ID_OR_ACC_SHIFT);
    check_sibling((uint16_t)(UINT16_C(0xE2A6) ^ UINT16_C(0x2000)),
                  TI_C54X_OPCODE_ID_ST_PARALLEL_ADD);
    check_sibling((uint16_t)(UINT16_C(0xE3A6) ^ UINT16_C(0x2000)),
                  TI_C54X_OPCODE_ID_ST_PARALLEL_ADD);
    check_sibling((uint16_t)(UINT16_C(0xE2A6) ^ UINT16_C(0x4000)),
                  TI_C54X_OPCODE_ID_SUB_DUAL_M2_M1);
    check_sibling((uint16_t)(UINT16_C(0xE3A6) ^ UINT16_C(0x4000)),
                  TI_C54X_OPCODE_ID_SUB_DUAL_M2_M1);
    check_sibling((uint16_t)(UINT16_C(0xE2A6) ^ UINT16_C(0x8000)),
                  TI_C54X_OPCODE_ID_MPY_SMEM_LK);
    check_sibling((uint16_t)(UINT16_C(0xE3A6) ^ UINT16_C(0x8000)),
                  TI_C54X_OPCODE_ID_MPY_SMEM_LK);
}

int
main(void)
{
    test_all_first_words_classify();
    test_malformed_inputs_reject_transactionally();
    test_fixed_bit_near_misses();
    return 0;
}
