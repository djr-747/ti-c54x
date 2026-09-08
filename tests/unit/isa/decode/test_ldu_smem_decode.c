/*
 * Module contract
 *
 * Owns: executable proof of LDU one-word decode classification, copied
 * metadata, exhaustive D/ARF extraction, fixed-bit near-miss rejection, and
 * transactional rejection of forged completion inputs. Does not own: catalog
 * generation, unsigned-load effects, address resolution, memory/bus access,
 * CPU/state mutation, sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t LDU_VALUE = UINT16_C(0x1290);
static const uint16_t LDU_MASK = UINT16_C(0xFEE0);

static void
check_case(uint8_t mode, uint8_t destination, uint8_t arf)
{
    const uint16_t word = (uint16_t)((LDU_VALUE &
                                     (uint16_t)~UINT16_C(0x0018)) |
                                     (uint16_t)(mode << 3U) |
                                     ((uint16_t)destination << 8U) | arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value arf_field = {0U, 0U};
    ti_c54x_operand_field_value mod_field = {0U, 0U};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LDU_SMEM);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LDU_SMEM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(3));
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(0), &destination_field) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(1), &arf_field) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(destination_field.width_bits == UINT8_C(1));
    TI_C54X_TEST_CHECK(destination_field.raw_value == destination);
    TI_C54X_TEST_CHECK(arf_field.width_bits == UINT8_C(3));
    TI_C54X_TEST_CHECK(arf_field.raw_value == arf);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(2), &mod_field) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(mod_field.width_bits == UINT8_C(2));
    TI_C54X_TEST_CHECK(mod_field.raw_value == mode);
}

static void
check_transactional_failure(const ti_c54x_decode_result *first,
                            const ti_c54x_instruction_word_bundle *bundle)
{
    ti_c54x_decoded_instruction output;
    unsigned char before[sizeof(output)];

    (void)memset(&output, 0xA5, sizeof(output));
    (void)memcpy(before, &output, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           first, bundle, &output) !=
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
}

static void
test_fixed_bit_near_misses_and_forged_completion(void)
{
    static const uint16_t unknown_near_misses[] = {
        UINT16_C(0x0040), UINT16_C(0x0080), UINT16_C(0x0400),
        UINT16_C(0x0800), UINT16_C(0x1000),
        UINT16_C(0x4000), UINT16_C(0x8000)};
    const uint16_t word = LDU_VALUE;
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_LDU_SMEM);
    TI_C54X_TEST_CHECK((word & LDU_MASK) == (LDU_VALUE & LDU_MASK));

    for (size_t index = 0U;
         index < sizeof(unknown_near_misses) /
                    sizeof(unknown_near_misses[0]);
         ++index) {
        const ti_c54x_decode_result result = ti_c54x_decode_first_word(
            (uint16_t)(word ^ unknown_near_misses[index]));
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }

    /* This former near-miss is the W43B indexed LDU MOD6 sibling. */
    {
        const ti_c54x_decode_result sibling = ti_c54x_decode_first_word(
            (uint16_t)(word ^ UINT16_C(0x0020)));
        TI_C54X_TEST_CHECK(sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(sibling.id == TI_C54X_OPCODE_ID_LDU_SMEM_MOD6);
    }

    /* This former near-miss is the W43B indexed LD ASM MOD6 sibling. */
    {
        const ti_c54x_decode_result sibling = ti_c54x_decode_first_word(
            (uint16_t)(word ^ UINT16_C(0x2020)));
        TI_C54X_TEST_CHECK(sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(sibling.id == TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD6);
    }

    /* Fixed bit 9 is the existing LD sibling, so it must not alias LDU. */
    {
        const ti_c54x_decode_result sibling = ti_c54x_decode_first_word(
            (uint16_t)(word ^ UINT16_C(0x0200)));
        TI_C54X_TEST_CHECK(sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(sibling.id == TI_C54X_OPCODE_ID_LD_SMEM);
    }

    {
        ti_c54x_decode_result forged = valid;
        forged.id = TI_C54X_OPCODE_ID_LD_SMEM;
        check_transactional_failure(&forged, &bundle);

        forged = valid;
        forged.first_word = (uint16_t)(word ^ UINT16_C(0x0008));
        check_transactional_failure(&forged, &bundle);

        forged = valid;
        forged.program_word_count = UINT8_C(2);
        check_transactional_failure(&forged, &bundle);

        {
            ti_c54x_instruction_word_bundle malformed = bundle;
            malformed.program_word_count = UINT8_C(2);
            check_transactional_failure(&valid, &malformed);
        }
    }
}

int
main(void)
{
    for (uint8_t mode = 0U; mode < UINT8_C(4); ++mode) {
        for (uint8_t destination = 0U; destination <= UINT8_C(1);
             ++destination) {
            for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
                check_case(mode, destination, arf);
            }
        }
    }
    test_fixed_bit_near_misses_and_forged_completion();
    return 0;
}
