/* Module contract
 * Owns: executable decode and operand-layout extraction coverage for the
 * bounded one-word dual-memory ADD/SUB MOD0/MOD0 forms.
 * Does not own: catalog declaration, address resolution, arithmetic effects,
 * CPU execution, or inventory policy.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

static void check_row(uint16_t base, ti_c54x_opcode_id id)
{
    uint8_t destination;
    uint8_t xar;
    uint8_t yar;

    for (destination = 0U; destination < 2U; ++destination) {
        for (xar = 0U; xar < 4U; ++xar) {
            for (yar = 0U; yar < 4U; ++yar) {
                const uint16_t word = (uint16_t)(base |
                    ((uint16_t)destination << 8U) |
                    ((uint16_t)xar << 4U) | yar);
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
                                   TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_C_SXM_OVM);
                TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                       instruction.operand_layout, 1U, &view) ==
                                   TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                for (uint8_t index = 0U; index < 3U; ++index) {
                    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                                           &view, &words, index, &fields[index]) ==
                                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                }
                TI_C54X_TEST_CHECK(fields[0].raw_value == destination);
                TI_C54X_TEST_CHECK(fields[1].raw_value == xar);
                TI_C54X_TEST_CHECK(fields[2].raw_value == yar);
                TI_C54X_TEST_CHECK(fields[0].width_bits == 1U);
                TI_C54X_TEST_CHECK(fields[1].width_bits == 2U);
                TI_C54X_TEST_CHECK(fields[2].width_bits == 2U);
            }
        }
    }
}

int main(void)
{
    check_row(UINT16_C(0xA000), TI_C54X_OPCODE_ID_ADD_DUAL_MOD0);
    check_row(UINT16_C(0xA200), TI_C54X_OPCODE_ID_SUB_DUAL_MOD0);
    /* Non-MOD0/MOD0 combinations live in sibling rows; MOD3 neighborhoods
     * stay undecoded. */
    TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0xA0CC)).status ==
                       TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
    TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0xA2CC)).status ==
                       TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
    return 0;
}
