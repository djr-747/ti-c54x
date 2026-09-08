/*
 * Module contract
 *
 * Owns: executable proof that all twenty indexed catalog rows decode through
 * the appended MOD4 layouts, preserve D/ARF/extension operand indices, and
 * carry typed MOD5/MOD6 values. Does not own: catalog generation, handlers,
 * address arithmetic, memory endpoints, CPU sequencing, or inventory policy.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

typedef struct indexed_case {
    ti_c54x_opcode_id id;
    uint16_t base;
    uint8_t mode;
    uint8_t words;
    ti_c54x_operand_layout_id layout;
    uint8_t d_index;
    uint8_t arf_index;
    uint8_t mode_index;
    uint8_t extension_index;
} indexed_case;

#define CASE_PAIR(name, base5, base6, words_, layout_, d_, arf_, mode_, ext_) \
    {TI_C54X_OPCODE_ID_##name##_MOD5, UINT16_C(base5), 5U, words_, layout_, d_, arf_, mode_, ext_}, \
    {TI_C54X_OPCODE_ID_##name##_MOD6, UINT16_C(base6), 6U, words_, layout_, d_, arf_, mode_, ext_}

static const indexed_case cases[] = {
    CASE_PAIR(LD_SMEM, 0x10A8, 0x10B0, 1U,
              TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, 0U, 1U, 2U, 0U),
    CASE_PAIR(LD_SMEM_TS, 0x14A8, 0x14B0, 1U,
              TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, 0U, 1U, 2U, 0U),
    CASE_PAIR(STL_SMEM, 0x80A8, 0x80B0, 1U,
              TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, 0U, 1U, 2U, 0U),
    CASE_PAIR(LDU_SMEM, 0x12A8, 0x12B0, 1U,
              TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, 0U, 1U, 2U, 0U),
    CASE_PAIR(LD_T_SMEM, 0x30A8, 0x30B0, 1U,
              TI_C54X_OPERAND_LAYOUT_ARF_MOD4, 0U, 0U, 1U, 0U),
    CASE_PAIR(ST_IMMEDIATE_SMEM, 0x76A8, 0x76B0, 2U,
              TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4, 0U, 0U, 2U, 1U),
    CASE_PAIR(LD_DP_SMEM, 0x46A8, 0x46B0, 1U,
              TI_C54X_OPERAND_LAYOUT_ARF_MOD4, 0U, 0U, 1U, 0U),
    CASE_PAIR(LD_ASM_SMEM, 0x32A8, 0x32B0, 1U,
              TI_C54X_OPERAND_LAYOUT_ARF_MOD4, 0U, 0U, 1U, 0U),
    CASE_PAIR(LD_SMEM_SHIFT16, 0x44A8, 0x44B0, 1U,
              TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, 0U, 1U, 2U, 0U),
    CASE_PAIR(ST_T_SMEM, 0x8CA8, 0x8CB0, 1U,
              TI_C54X_OPERAND_LAYOUT_ARF_MOD4, 0U, 0U, 1U, 0U),
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
test_all_indexed_rows_and_modes(void)
{
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        const indexed_case *test_case = &cases[index];
        const uint8_t mode = test_case->mode;
        const uint8_t arf = UINT8_C(5);
        const uint8_t d = UINT8_C(1);
        const uint16_t extension = UINT16_C(0xC53A);
        uint16_t first_word = (uint16_t)(test_case->base | arf);
        if (test_case->layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4) {
            first_word = (uint16_t)(first_word | (uint16_t)(d << 8U));
        }
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(first_word);
        const ti_c54x_instruction_word_bundle bundle = {
            .program_words = {first_word, extension},
            .program_word_count = test_case->words};
        const ti_c54x_program_words_view words = {
            bundle.program_words, bundle.program_word_count};
        ti_c54x_decoded_instruction instruction = {0};
        ti_c54x_operand_layout_set_view view;

        TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(first.id == test_case->id);
        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &first, &bundle, &instruction) ==
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(instruction.operand_layout == test_case->layout);
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                               test_case->layout, test_case->words, &view) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        check_operand(&view, &words, test_case->arf_index, arf, 3U);
        check_operand(&view, &words, test_case->mode_index, mode, 4U);
        if (test_case->layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4) {
            check_operand(&view, &words, test_case->d_index, d, 1U);
        }
        if (test_case->extension_index != 0U) {
            check_operand(&view, &words, test_case->extension_index,
                          extension, 16U);
        }
    }
}

int
main(void)
{
    test_all_indexed_rows_and_modes();
    return 0;
}
