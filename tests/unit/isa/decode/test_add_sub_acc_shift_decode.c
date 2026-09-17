/* Module contract
 * Owns: executable checked-decoder coverage for all ADD/SUB accumulator
 * source-destination-shift5 encodings and signed shift-field extraction.
 * Does not own: catalog declaration, arithmetic semantics, or CPU execution.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static void check_row(uint16_t base, ti_c54x_opcode_id id)
{
    uint8_t source;
    uint8_t destination;
    uint8_t shift;

    for (source = 0U; source < 2U; ++source) {
        for (destination = 0U; destination < 2U; ++destination) {
            for (shift = 0U; shift < 32U; ++shift) {
                const uint16_t word = (uint16_t)(base |
                    ((uint16_t)source << 9U) |
                    ((uint16_t)destination << 8U) | shift);
                const ti_c54x_decode_result first =
                    ti_c54x_decode_first_word(word);
                const ti_c54x_instruction_word_bundle bundle = {
                    .program_words = {word, 0U}, .program_word_count = 1U};
                ti_c54x_decoded_instruction instruction;
                ti_c54x_operand_layout_set_view view;
                ti_c54x_operand_field_value fields[3];
                const ti_c54x_program_words_view words = {
                    bundle.program_words, bundle.program_word_count};

                TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
                TI_C54X_TEST_CHECK(first.id == id);
                TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                                       &first, &bundle, &instruction) ==
                                   TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
                TI_C54X_TEST_CHECK(instruction.input_resources ==
                                   TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM);
                TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                       instruction.operand_layout, 1U, &view) ==
                                   TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                for (uint8_t index = 0U; index < 3U; ++index) {
                    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                                           &view, &words, index, &fields[index]) ==
                                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                }
                TI_C54X_TEST_CHECK(fields[0].raw_value == source);
                TI_C54X_TEST_CHECK(fields[1].raw_value == destination);
                TI_C54X_TEST_CHECK(fields[2].raw_value == shift);
                TI_C54X_TEST_CHECK(fields[2].width_bits == 5U);
            }
        }
    }
}

/* Clearing fixed bit 15 lands in the P3-S6 PORTR direct row
 * (TI SPRU172C 4-129 pins PORTR at 0x74xx). */
static void check_near_miss_and_completion(uint16_t base)
{
    const uint16_t near_miss = (uint16_t)(base ^ UINT16_C(0x8000));
    const ti_c54x_decode_result portr_sibling =
        ti_c54x_decode_first_word(near_miss);
    const uint16_t valid_word = (uint16_t)(base | UINT16_C(0x0015));
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(valid_word);
    const ti_c54x_instruction_word_bundle malformed = {
        .program_words = {valid_word, 0U}, .program_word_count = 0U};
    ti_c54x_decoded_instruction output;
    ti_c54x_decoded_instruction before;

    TI_C54X_TEST_CHECK(portr_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(portr_sibling.id == TI_C54X_OPCODE_ID_PORTR_DIRECT);
    memset(&output, 0xa5, sizeof(output));
    before = output;
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &malformed, &output) !=
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&output, &before, sizeof(output)) == 0);
}

int main(void)
{
    check_row(UINT16_C(0xF400), TI_C54X_OPCODE_ID_ADD_ACC_SHIFT);
    check_row(UINT16_C(0xF420), TI_C54X_OPCODE_ID_SUB_ACC_SHIFT);
    check_near_miss_and_completion(UINT16_C(0xF400));
    check_near_miss_and_completion(UINT16_C(0xF420));
    return 0;
}
