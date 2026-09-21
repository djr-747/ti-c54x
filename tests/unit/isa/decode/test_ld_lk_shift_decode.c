/*
 * Module contract
 *
 * Owns: executable proof that all 32 two-word LD #lk[,SHFT],dst first
 * words classify through the generic decoder, copy their complete
 * bundles and scalar metadata, extract destination then unsigned
 * shift then the raw 16-bit extension immediate, and reject malformed
 * inputs transactionally.
 * Does not own: catalog generation, accumulator reads, CPU/state
 * mutation, fetch or bus behavior, sequencing, timing, or handler
 * bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t LD_LK_SHIFT_VALUE = UINT16_C(0xF020);

static uint16_t
first_word(uint8_t destination, uint8_t shift)
{
    return (uint16_t)(LD_LK_SHIFT_VALUE |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (shift & UINT8_C(0x0F)));
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
check_case(uint8_t destination, uint8_t shift, uint16_t immediate)
{
    const uint16_t word = first_word(destination, shift);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, immediate}, .program_word_count = UINT8_C(2)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_LK_SHIFT);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_LK_SHIFT);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_SHIFT4_LK16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM_OVM);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == immediate);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(2), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(3));

    /* Layout order is destination, shift, extension immediate. */
    check_operand(&view, &words, UINT8_C(0), destination, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), shift, UINT8_C(4));
    check_operand(&view, &words, UINT8_C(2), immediate, UINT8_C(16));
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
    const uint16_t word = first_word(UINT8_C(1), UINT8_C(9));
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {word, UINT16_C(0xFEDC)},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_LD_LK_SHIFT);

    /* 0xF030 is the AND_IMMEDIATE_SHIFT sibling: valid row, wrong
     * identity. */
    bundle.program_words[0] = UINT16_C(0xF030);
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
test_fixed_bit_near_misses(void)
{
    /* Mask 0xFEF0 leaves D(8) and SHFT(3:0) variable. Flipping fixed
     * bit 4 lands on the AND_IMMEDIATE_SHIFT sibling, flipping fixed
     * bit 7 lands on OR_ACC_SHIFT, flipping fixed bit 10 lands on
     * SUB_ACC_SHIFT, and flipping fixed bit 14 lands on the P3-S3g
     * MAC_DUAL_M0_M0 sibling; flipping fixed bit 5 lands on the
     * P3-S27 ADD_LK_SHIFT sibling and flipping fixed bit 6 lands on
     * the P3-S27 ADD_LK_16 sibling; flipping fixed bit 12 lands on
     * the P3-S108 FIRS row; all other fixed-bit flips must
     * miss every row. */
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x0200),
        UINT16_C(0x2000)};
    const ti_c54x_decode_result firs_sibling =
        ti_c54x_decode_first_word((uint16_t)(LD_LK_SHIFT_VALUE ^
                                            UINT16_C(0x1000)));
    const ti_c54x_decode_result and_sibling =
        ti_c54x_decode_first_word((uint16_t)(LD_LK_SHIFT_VALUE ^
                                            UINT16_C(0x0010)));
    const ti_c54x_decode_result or_sibling =
        ti_c54x_decode_first_word((uint16_t)(LD_LK_SHIFT_VALUE ^
                                            UINT16_C(0x0080)));
    const ti_c54x_decode_result sub_sibling =
        ti_c54x_decode_first_word((uint16_t)(LD_LK_SHIFT_VALUE ^
                                            UINT16_C(0x0400)));
    /* This former near-miss is the P3-S61 MVKD direct sibling:
     * 0xF020 ^ 0x8000 = 0x7020 classifies into the MVKD_DIRECT
     * row. */
    {
        const ti_c54x_decode_result mvkd_sibling =
            ti_c54x_decode_first_word((uint16_t)(LD_LK_SHIFT_VALUE ^
                                                UINT16_C(0x8000)));
        TI_C54X_TEST_CHECK(mvkd_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(mvkd_sibling.id ==
                           TI_C54X_OPCODE_ID_MVKD_DIRECT);
    }
    /* 0xF820 (fixed bit 11 set) is the BC_PMAD encoding with
     * cond=0x20 (NTC): valid row, wrong identity. */
    const ti_c54x_decode_result bc_sibling =
        ti_c54x_decode_first_word((uint16_t)(LD_LK_SHIFT_VALUE ^
                                            UINT16_C(0x0800)));

    TI_C54X_TEST_CHECK(and_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(and_sibling.id ==
                       TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT);
    TI_C54X_TEST_CHECK(or_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(or_sibling.id == TI_C54X_OPCODE_ID_OR_ACC_SHIFT);
    TI_C54X_TEST_CHECK(sub_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(sub_sibling.id == TI_C54X_OPCODE_ID_SUB_ACC_SHIFT);
    TI_C54X_TEST_CHECK(firs_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(firs_sibling.id ==
                       TI_C54X_OPCODE_ID_FIRS_XMEM_YMEM_PMAD);
    TI_C54X_TEST_CHECK(bc_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(bc_sibling.id == TI_C54X_OPCODE_ID_BC_PMAD);
    /* 0xB020 (fixed bit 14 set) is the MAC_DUAL_M0_M0 encoding with
     * R=0, S=0 (src A), D=0 (dst A), XMOD0 XAR=AR4, YMOD0 YAR=AR2. */
    const ti_c54x_decode_result mac_dual_sibling =
        ti_c54x_decode_first_word((uint16_t)(LD_LK_SHIFT_VALUE ^
                                            UINT16_C(0x4000)));
    TI_C54X_TEST_CHECK(mac_dual_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(mac_dual_sibling.id == TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0);
    /* 0xF000 (fixed bit 5 cleared) is the ADD_LK_SHIFT encoding with
     * S=0 (src A), D=0 (dst A), SHFT=0: valid row, wrong identity.
     * 0xF060 (fixed bit 6 set) is the ADD_LK_16 encoding with S=0,
     * D=0: valid row, wrong identity. */
    const ti_c54x_decode_result add_lk_shift_sibling =
        ti_c54x_decode_first_word((uint16_t)(LD_LK_SHIFT_VALUE ^
                                            UINT16_C(0x0020)));
    TI_C54X_TEST_CHECK(add_lk_shift_sibling.status ==
                       TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(add_lk_shift_sibling.id ==
                       TI_C54X_OPCODE_ID_ADD_LK_SHIFT);
    const ti_c54x_decode_result add_lk16_sibling =
        ti_c54x_decode_first_word((uint16_t)(LD_LK_SHIFT_VALUE ^
                                            UINT16_C(0x0040)));
    TI_C54X_TEST_CHECK(add_lk16_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(add_lk16_sibling.id == TI_C54X_OPCODE_ID_ADD_LK_16);

    for (size_t index = 0U;
         index < sizeof(fixed_bit_masks) / sizeof(fixed_bit_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_LK_SHIFT_VALUE ^ fixed_bit_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }
}

int
main(void)
{
    static const uint16_t immediates[] = {
        UINT16_C(0x0000), UINT16_C(0x0001), UINT16_C(0x00FF),
        UINT16_C(0x7FFF), UINT16_C(0x8000), UINT16_C(0xFEDC),
        UINT16_C(0xFFFF)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        for (uint8_t shift = 0U; shift <= UINT8_C(15); ++shift) {
            for (size_t index = 0U;
                 index < sizeof(immediates) / sizeof(immediates[0]);
                 ++index) {
                check_case(destination, shift, immediates[index]);
            }
        }
    }
    test_malformed_inputs_reject_transactionally();
    test_fixed_bit_near_misses();
    return 0;
}
