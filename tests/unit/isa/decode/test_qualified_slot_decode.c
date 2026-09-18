/*
 * Module contract
 *
 * Owns: executable proof that instruction completion re-resolves first
 * words owning a qualified slot on the second program word: 0x0C40 words
 * complete to LD_SMEM_SHIFT with LD metadata, 0x0C60 words complete to
 * the STH_SHIFT_SMEM_MODn row selected by first-word bits 6:3 with STH
 * metadata, 0x0C00 words complete to ADD_SMEM_SHIFT and 0x0C20 words to
 * SUB_SMEM_SHIFT with syn4 metadata, and every other second word
 * declines transactionally with SECOND_WORD_UNMATCHED. First-word
 * classification is unchanged.
 * Does not own: catalog generation, operand extraction, address
 * resolution, memory effects, CPU/state mutation, sequencing, timing,
 * or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static uint16_t
first_word(uint8_t mode, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x6F80) |
                      (uint16_t)(((uint16_t)(mode & UINT8_C(3))) << 3U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_instruction_completion_status
complete(uint16_t word0, uint16_t word1, ti_c54x_decoded_instruction *output)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word0);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word0, word1}, .program_word_count = UINT8_C(2)};

    if (first.status != TI_C54X_DECODE_STATUS_OK) {
        return TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_NOT_OK;
    }
    return ti_c54x_decode_complete_instruction(&first, &bundle, output);
}

static void
test_ld_words_complete_to_ld(void)
{
    for (uint8_t mode = UINT8_C(0); mode <= UINT8_C(3); ++mode) {
        for (uint8_t arf = UINT8_C(0); arf <= UINT8_C(7); ++arf) {
            ti_c54x_decoded_instruction instruction;
            const uint16_t word0 = first_word(mode, arf);
            const uint16_t word1 =
                (uint16_t)(UINT16_C(0x0C40) |
                           (uint16_t)(((uint16_t)(arf & UINT8_C(1))) << 8U) |
                           (uint16_t)(arf & UINT8_C(0x1F)));

            TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(word0).id ==
                               TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
            TI_C54X_TEST_CHECK(
                complete(word0, word1, &instruction) ==
                TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
            TI_C54X_TEST_CHECK(instruction.id ==
                               TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
            TI_C54X_TEST_CHECK(
                instruction.operand_layout ==
                TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD_SHIFT5);
            TI_C54X_TEST_CHECK(
                instruction.input_resources ==
                TI_C54X_INPUT_RESOURCES_STATUS_SXM_OVM);
        }
    }
}

static void
test_sth_words_complete_to_mod_rows(void)
{
    static const ti_c54x_opcode_id expected[4] = {
        TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD0,
        TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD1,
        TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD2,
        TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD3};
    for (uint8_t mode = UINT8_C(0); mode <= UINT8_C(3); ++mode) {
        for (uint8_t arf = UINT8_C(0); arf <= UINT8_C(7); ++arf) {
            ti_c54x_decoded_instruction instruction;
            const uint16_t word0 = first_word(mode, arf);
            const uint16_t word1 =
                (uint16_t)(UINT16_C(0x0C60) |
                           (uint16_t)(((uint16_t)(arf & UINT8_C(1))) << 8U) |
                           (uint16_t)(arf & UINT8_C(0x1F)));

            /* First-word lookup stays LD; completion re-resolves. */
            TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(word0).id ==
                               TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
            TI_C54X_TEST_CHECK(
                complete(word0, word1, &instruction) ==
                TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
            TI_C54X_TEST_CHECK(instruction.id == expected[mode]);
            TI_C54X_TEST_CHECK(
                instruction.operand_layout ==
                TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5);
            TI_C54X_TEST_CHECK(
                instruction.input_resources ==
                TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS);
        }
    }
}

static void
test_other_second_words_decline_transactionally(void)
{
    static const uint16_t declined[] = {
        UINT16_C(0x0000),
        UINT16_C(0x0C80), UINT16_C(0x0CE0), UINT16_C(0xFFFF)};
    for (uint8_t mode = UINT8_C(0); mode <= UINT8_C(3); ++mode) {
        const uint16_t word0 = first_word(mode, UINT8_C(0));
        for (size_t index = 0U;
             index < sizeof(declined) / sizeof(declined[0]); ++index) {
            ti_c54x_decoded_instruction output;
            unsigned char before[sizeof(output)];

            (void)memset(&output, 0xA5, sizeof(output));
            (void)memcpy(before, &output, sizeof(before));
            TI_C54X_TEST_CHECK(
                complete(word0, declined[index], &output) ==
                TI_C54X_INSTRUCTION_COMPLETION_STATUS_SECOND_WORD_UNMATCHED);
            TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
        }
    }
}

static void
test_syn4_words_complete_to_syn4_rows(void)
{
    static const ti_c54x_opcode_id expected[2] = {
        TI_C54X_OPCODE_ID_ADD_SMEM_SHIFT,
        TI_C54X_OPCODE_ID_SUB_SMEM_SHIFT};
    static const uint16_t word1[2] = {UINT16_C(0x0C00),
                                      UINT16_C(0x0C20)};
    size_t variant;

    for (variant = 0U; variant < 2U; ++variant) {
        for (uint8_t mode = UINT8_C(0); mode <= UINT8_C(3); ++mode) {
            ti_c54x_decoded_instruction instruction;
            const uint16_t word0 = first_word(mode, UINT8_C(0));

            TI_C54X_TEST_CHECK(
                complete(word0, word1[variant], &instruction) ==
                TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
            TI_C54X_TEST_CHECK(instruction.id == expected[variant]);
            TI_C54X_TEST_CHECK(
                instruction.operand_layout ==
                TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_SOURCE_DESTINATION_SHIFT5);
            TI_C54X_TEST_CHECK(
                instruction.input_resources ==
                TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM);
        }
    }
}

int
main(void)
{
    test_ld_words_complete_to_ld();
    test_sth_words_complete_to_mod_rows();
    test_syn4_words_complete_to_syn4_rows();
    test_other_second_words_decline_transactionally();
    return 0;
}
