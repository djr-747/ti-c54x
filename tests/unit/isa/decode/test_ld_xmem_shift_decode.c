/*
 * Module contract
 *
 * Owns: executable proof that all 512 one-word LD Xmem,SHFT,dst
 * first words classify through the generic decoder, copy their
 * complete bundles and scalar metadata, extract destination,
 * Xmod, Xar, then the unsigned 4-bit SHFT in layout order, and
 * reject malformed inputs transactionally.
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

static const uint16_t LD_XMEM_SHIFT_VALUE = UINT16_C(0x9400);

static uint16_t
first_word(uint8_t destination, uint8_t xmod, uint8_t xar_field,
           uint8_t shift)
{
    return (uint16_t)(LD_XMEM_SHIFT_VALUE |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (uint16_t)(((uint16_t)(xmod & UINT8_C(3))) << 6U) |
                      (uint16_t)(((uint16_t)(xar_field & UINT8_C(3)))
                                 << 4U) |
                      (uint16_t)(shift & UINT8_C(0x0F)));
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
check_case(uint8_t destination, uint8_t xmod, uint8_t xar_field,
           uint8_t shift)
{
    const uint16_t word = first_word(destination, xmod, xar_field, shift);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_SXM_OVM);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(4));

    /* Layout order is destination, Xmod, raw Xar field, SHFT. The
     * handler maps the raw field to AR2..AR5; decode exposes raw bits. */
    check_operand(&view, &words, UINT8_C(0), destination, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), xmod, UINT8_C(2));
    check_operand(&view, &words, UINT8_C(2), xar_field, UINT8_C(2));
    check_operand(&view, &words, UINT8_C(3), shift, UINT8_C(4));
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
    const uint16_t word = first_word(UINT8_C(1), UINT8_C(2), UINT8_C(1),
                                     UINT8_C(15));
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);

    /* 0xF0E0 is the SFTL_ACC sibling: valid row, wrong identity. */
    bundle.program_words[0] = UINT16_C(0xF0E0);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0x9000);
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
    /* Mask 0xFE00 leaves D(8), XXXX(7:4), and SHFT(3:0) variable.
     * Flipping fixed bit 13 lands on the P3-S3g MAC_DUAL_M0_M0
     * sibling, flipping fixed bit 10 lands on the P3-S28
     * ADD_XMEM sibling, flipping fixed bit 11 lands on the P3-S49
     * STRCD_XMEM_COND sibling (pinned below), and flipping fixed
     * bit 12 lands on the P3-S46 STL_ASM_SMEM_DIRECT sibling
     * (pinned below), and flipping fixed bit 9 lands on the P3-S101
     * BIT_XMEM row (pinned below); all other fixed-bit 15:9 flips
     * miss every catalog row. */
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x4000),
        UINT16_C(0x8000)};

    /* 0xB400 (fixed bit 13 set) is the MAC_DUAL_M0_M0 encoding with
     * R=1, S=0 (src A), D=0 (dst A), XMOD0 XAR=AR2, YMOD0 YAR=AR2. */
    {
        const ti_c54x_decode_result mac_dual_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_XMEM_SHIFT_VALUE ^ UINT16_C(0x2000)));

        TI_C54X_TEST_CHECK(mac_dual_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(mac_dual_sibling.id ==
                           TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0);
    }

    /* 0x9000 (fixed bit 10 cleared) is the ADD_XMEM encoding with
     * S=0 (src A), XMOD0, XAR=AR2, SHFT=0. */
    {
        const ti_c54x_decode_result add_xmem_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_XMEM_SHIFT_VALUE ^ UINT16_C(0x0400)));

        TI_C54X_TEST_CHECK(add_xmem_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(add_xmem_sibling.id ==
                           TI_C54X_OPCODE_ID_ADD_XMEM);
    }

    /* 0x9C00 (fixed bit 11 set) is the STRCD_XMEM_COND first word
     * with S=0, XMOD0, XAR=AR2 (COND=0x0 rejects at the leaf). */
    {
        const ti_c54x_decode_result strcd_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_XMEM_SHIFT_VALUE ^ UINT16_C(0x0800)));

        TI_C54X_TEST_CHECK(strcd_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(strcd_sibling.id ==
                           TI_C54X_OPCODE_ID_STRCD_XMEM_COND);
    }

    /* 0x8400 (fixed bit 12 cleared) is the STL_ASM_SMEM_DIRECT
     * encoding with S=0 (src A), I=0, dma=0. */
    {
        const ti_c54x_decode_result stl_asm_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_XMEM_SHIFT_VALUE ^ UINT16_C(0x1000)));

        TI_C54X_TEST_CHECK(stl_asm_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(stl_asm_sibling.id ==
                           TI_C54X_OPCODE_ID_STL_ASM_SMEM_DIRECT);
    }

    /* 0x9600 (fixed bit 9 set) is the BIT_XMEM encoding with
     * XMOD0, XAR=AR2, BITC=0. */
    {
        const ti_c54x_decode_result bit_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_XMEM_SHIFT_VALUE ^ UINT16_C(0x0200)));

        TI_C54X_TEST_CHECK(bit_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(bit_sibling.id == TI_C54X_OPCODE_ID_BIT_XMEM);
    }

    for (size_t index = 0U;
         index < sizeof(fixed_bit_masks) / sizeof(fixed_bit_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_XMEM_SHIFT_VALUE ^ fixed_bit_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }
}

int
main(void)
{
    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        for (uint8_t xmod = 0U; xmod <= UINT8_C(3); ++xmod) {
            for (uint8_t xar_field = 0U; xar_field <= UINT8_C(3);
                 ++xar_field) {
                for (uint16_t shift = 0U; shift <= UINT16_C(15); ++shift) {
                    check_case(destination, xmod, xar_field,
                               (uint8_t)shift);
                }
            }
        }
    }
    test_malformed_inputs_reject_transactionally();
    test_fixed_bit_near_misses();
    return 0;
}
