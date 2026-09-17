/*
 * Module contract
 *
 * Owns: executable proof that all single-word LD src,ASM[,dst] first
 * words classify through the generic decoder, copy their complete
 * bundles and scalar metadata, extract source then destination, and
 * reject malformed inputs transactionally. The ASM count itself is a
 * status input, not a program-word operand.
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

static const uint16_t LD_SRC_ASM_VALUE = UINT16_C(0xF482);

static uint16_t
first_word(uint8_t source, uint8_t destination)
{
    return (uint16_t)(LD_SRC_ASM_VALUE |
                      (uint16_t)(((uint16_t)(source & UINT8_C(1))) << 9U) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U));
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
    TI_C54X_TEST_CHECK(value.width_bits == UINT8_C(1));
}

static void
check_case(uint8_t source, uint8_t destination)
{
    const uint16_t word = first_word(source, destination);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_SRC_ASM);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_SRC_ASM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM_ASM);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(2));

    /* Layout order is source, destination. */
    check_operand(&view, &words, UINT8_C(0), source);
    check_operand(&view, &words, UINT8_C(1), destination);
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
    const uint16_t word = first_word(UINT8_C(1), UINT8_C(0));
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_LD_SRC_ASM);

    /* 0xF440 is the LD_SRC_SHIFT sibling: valid row, wrong identity. */
    bundle.program_words[0] = UINT16_C(0xF440);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0xF480);
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
    /* Mask 0xFCFF leaves only S(9) and D(8) variable. Flips landing on
     * pre-existing sibling rows pin those identities; all other
     * fixed-bit flips must miss every row. */
    static const struct {
        uint16_t flip;
        ti_c54x_opcode_id id;
    } siblings[] = {
        {UINT16_C(0x0004), TI_C54X_OPCODE_ID_MAX_ACC},
        {UINT16_C(0x0010), TI_C54X_OPCODE_ID_ROLTC_ACC},
        {UINT16_C(0x0020), TI_C54X_OPCODE_ID_LD_IMMEDIATE_ARP},
        {UINT16_C(0x0080), TI_C54X_OPCODE_ID_ADD_ACC_SHIFT},
        {UINT16_C(0x0400), TI_C54X_OPCODE_ID_AND_ACC_SHIFT},
        /* 0xB482 (fixed bit 14 set) is the MAC_DUAL_M2_M0 encoding
         * with R=1, S=0 (src A), D=0 (dst A), XMOD2 XAR=AR2,
         * YMOD0 YAR=AR4. */
        {UINT16_C(0x4000), TI_C54X_OPCODE_ID_MAC_DUAL_M2_M0},
        /* 0xF48A (fixed bit 3 set) is the S3w MASA_T encoding
         * with S=0 (src A), D=0 (dst A), R=0. */
        {UINT16_C(0x0008), TI_C54X_OPCODE_ID_MASA_T},
        /* 0x7482 (fixed bit 15 clear) is the P3-S6 PORTR indirect
         * row with I=1, MOD0, ARF=2 (TI SPRU172C 4-129 pins PORTR
         * at 0x74xx). */
        {UINT16_C(0x8000), TI_C54X_OPCODE_ID_PORTR_SMEM},
    };
    static const uint16_t misses[] = {
        UINT16_C(0x0001), UINT16_C(0x0002),
        UINT16_C(0x0040), UINT16_C(0x1000),
        UINT16_C(0x2000)};
    size_t index;

    for (index = 0U; index < sizeof(siblings) / sizeof(siblings[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SRC_ASM_VALUE ^ siblings[index].flip));

        TI_C54X_TEST_CHECK(result.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(result.id == siblings[index].id);
    }
    /* Flipping fixed bit 11 lands in the RC row. */
    {
        const ti_c54x_decode_result rc_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SRC_ASM_VALUE ^ UINT16_C(0x0800)));

        TI_C54X_TEST_CHECK(rc_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(rc_sibling.id == TI_C54X_OPCODE_ID_RC);
    }
    for (index = 0U; index < sizeof(misses) / sizeof(misses[0]); ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SRC_ASM_VALUE ^ misses[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }
}

int
main(void)
{
    for (uint8_t source = 0U; source <= UINT8_C(1); ++source) {
        for (uint8_t destination = 0U; destination <= UINT8_C(1);
             ++destination) {
            check_case(source, destination);
        }
    }
    test_malformed_inputs_reject_transactionally();
    test_fixed_bit_near_misses();
    return 0;
}
