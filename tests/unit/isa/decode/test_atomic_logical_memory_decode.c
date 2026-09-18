/*
 * Module contract
 *
 * Owns: executable proof that ANDM/ORM/XORM/ADDM MOD0 through MOD3 first
 * words complete through the generic two-word ARF_EXTENSION16_MOD decoder
 * and expose copied ARF, extension, and MOD operands. Does not own:
 * catalog generation, address arithmetic, memory effects, CPU sequencing,
 * or handler semantics.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

typedef struct atomic_decode_case {
    ti_c54x_opcode_id id;
    uint16_t base;
} atomic_decode_case;

static const atomic_decode_case cases[] = {
    {TI_C54X_OPCODE_ID_ANDM_SMEM_MOD0_3, UINT16_C(0x6880)},
    {TI_C54X_OPCODE_ID_ORM_SMEM_MOD0_3, UINT16_C(0x6980)},
    {TI_C54X_OPCODE_ID_XORM_SMEM_MOD0_3, UINT16_C(0x6A80)},
    {TI_C54X_OPCODE_ID_ADDM_SMEM_MOD0_3, UINT16_C(0x6B80)},
};

static void
check_operand(const ti_c54x_operand_layout_set_view *view,
              const ti_c54x_program_words_view *words, uint8_t index,
              uint64_t expected, uint8_t width)
{
    ti_c54x_operand_field_value value = {0};
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           view, words, index, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(value.raw_value == expected);
    TI_C54X_TEST_CHECK(value.width_bits == width);
}

static void
test_all_modes_and_operand_shape(void)
{
    for (size_t case_index = 0U;
         case_index < sizeof(cases) / sizeof(cases[0]); ++case_index) {
        const atomic_decode_case *test_case = &cases[case_index];
        for (uint16_t mode = 0U; mode < UINT16_C(4); ++mode) {
            const uint16_t arf = UINT16_C(5);
            const uint16_t extension = UINT16_C(0xC53A);
            const uint16_t first_word = (uint16_t)(
                test_case->base | (uint16_t)(mode << 3U) | arf);
            const ti_c54x_decode_result first =
                ti_c54x_decode_first_word(first_word);
            const ti_c54x_instruction_word_bundle bundle = {
                .program_words = {first_word, extension},
                .program_word_count = UINT8_C(2)};
            const ti_c54x_program_words_view words = {
                bundle.program_words, bundle.program_word_count};
            ti_c54x_decoded_instruction instruction = {0};
            ti_c54x_operand_layout_set_view view;

            TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
            TI_C54X_TEST_CHECK(first.id == test_case->id);
            TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
            TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                                   &first, &bundle, &instruction) ==
                               TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
            TI_C54X_TEST_CHECK(instruction.id == test_case->id);
            TI_C54X_TEST_CHECK(instruction.operand_layout ==
                               TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD);
            TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                   instruction.operand_layout, UINT8_C(2),
                                   &view) == TI_C54X_OPERAND_LAYOUT_STATUS_OK);
            check_operand(&view, &words, UINT8_C(0), arf, UINT8_C(3));
            check_operand(&view, &words, UINT8_C(1), extension, UINT8_C(16));
            check_operand(&view, &words, UINT8_C(2), mode, UINT8_C(2));
        }
    }
}

int
main(void)
{
    test_all_modes_and_operand_shape();
    return 0;
}
