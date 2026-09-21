/*
 * Module contract
 *
 * Owns: executable proof that all 256 one-word BIT Xmem first
 * words classify through the generic decoder, copy their complete
 * bundles and scalar metadata, extract Xmod, raw Xar, then the
 * unsigned 4-bit BITC in layout order, and reject malformed
 * inputs transactionally.
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

static const uint16_t BIT_XMEM_VALUE = UINT16_C(0x9600);

static uint16_t
first_word(uint8_t xmod, uint8_t xar_field, uint8_t bitc)
{
    return (uint16_t)(BIT_XMEM_VALUE |
                      (uint16_t)(((uint16_t)(xmod & UINT8_C(3))) << 6U) |
                      (uint16_t)(((uint16_t)(xar_field & UINT8_C(3)))
                                 << 4U) |
                      (uint16_t)(bitc & UINT8_C(0x0F)));
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
check_case(uint8_t xmod, uint8_t xar_field, uint8_t bitc)
{
    const uint16_t word = first_word(xmod, xar_field, bitc);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_BIT_XMEM);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_BIT_XMEM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_BITC);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(3));

    /* Layout order is Xmod, raw Xar field, BITC. The handler maps
     * the raw field to AR2..AR5; decode exposes raw bits. */
    check_operand(&view, &words, UINT8_C(0), xmod, UINT8_C(2));
    check_operand(&view, &words, UINT8_C(1), xar_field, UINT8_C(2));
    check_operand(&view, &words, UINT8_C(2), bitc, UINT8_C(4));
}

static void
test_all_first_words_classify(void)
{
    for (uint16_t xmod = 0U; xmod < 4U; ++xmod) {
        for (uint16_t xar_field = 0U; xar_field < 4U; ++xar_field) {
            for (uint16_t bitc = 0U; bitc < 16U; ++bitc) {
                check_case((uint8_t)xmod, (uint8_t)xar_field,
                           (uint8_t)bitc);
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
    const uint16_t word =
        first_word(UINT8_C(1), UINT8_C(3), UINT8_C(12));
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_BIT_XMEM);

    /* 0x977C keeps the low byte but flips fixed bit 8: no row. */
    bundle.program_words[0] = UINT16_C(0x977C);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0x9400);
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
    /* Mask 0xFF00 leaves XMOD(7:6), XAR(5:4), BITC(3:0)
     * variable. Flipping fixed bit 9 lands on the LD_XMEM_SHIFT
     * sibling, flipping fixed bit 10 lands on the SUB_XMEM
     * sibling, flipping fixed bit 11 lands on the SACCD_XMEM_COND
     * sibling, and flipping fixed bit 13 lands on the MAC_DUAL_M0_M0
     * sibling (both pinned below); all other fixed-bit 15:8 flips
     * miss every catalog row. */
    {
        const ti_c54x_decode_result ld_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(BIT_XMEM_VALUE ^ UINT16_C(0x0200)));

        TI_C54X_TEST_CHECK(ld_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(ld_sibling.id ==
                           TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);
    }

    /* 0x927C (fixed bit 10 cleared) is the SUB_XMEM encoding with
     * XMOD1, XAR=AR5, SHFT=12. */
    {
        const ti_c54x_decode_result sub_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(BIT_XMEM_VALUE ^ UINT16_C(0x0400)));

        TI_C54X_TEST_CHECK(sub_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(sub_sibling.id == TI_C54X_OPCODE_ID_SUB_XMEM);
    }

    /* 0x9E7C (fixed bit 11 set) is the SACCD_XMEM_COND first word
     * with XMOD1, XAR=AR5 (COND=0xC rejects at the leaf). */
    {
        const ti_c54x_decode_result saccd_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(BIT_XMEM_VALUE ^ UINT16_C(0x0800)));

        TI_C54X_TEST_CHECK(saccd_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(saccd_sibling.id ==
                           TI_C54X_OPCODE_ID_SACCD_XMEM_COND);
    }

    /* 0xB600 (fixed bit 13 set) is the MAC_DUAL_M0_M0 encoding
     * with R=1, S=0 (src A), D=0 (dst A), XMOD0 XAR=AR2, YMOD0
     * YAR=AR2. */
    {
        const ti_c54x_decode_result mac_dual_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(BIT_XMEM_VALUE ^ UINT16_C(0x2000)));

        TI_C54X_TEST_CHECK(mac_dual_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(mac_dual_sibling.id ==
                           TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0);
    }

    {
        static const uint16_t misses[] = {
            UINT16_C(0x0100), UINT16_C(0x1000),
            UINT16_C(0x4000), UINT16_C(0x8000)};
        size_t index;

        for (index = 0U; index < sizeof(misses) / sizeof(misses[0]);
             ++index) {
            const ti_c54x_decode_result miss =
                ti_c54x_decode_first_word(
                    (uint16_t)(BIT_XMEM_VALUE ^ misses[index]));

            TI_C54X_TEST_CHECK(miss.status != TI_C54X_DECODE_STATUS_OK ||
                               miss.id == TI_C54X_OPCODE_ID_UNKNOWN);
        }
    }
}

int
main(void)
{
    test_all_first_words_classify();
    test_malformed_inputs_reject_transactionally();
    test_fixed_bit_near_misses();
    return 0;
}
