/*
 * Module contract
 *
 * Owns: executable proof that the ten neutral standard-indirect Smem catalog
 * rows classify every MOD0--MOD3 encoding, complete with scalar metadata, and
 * expose appended MOD through the declared operand layouts. It also proves
 * transactional rejection of forged IDs and layout/width mismatches.
 * Does not own: catalog generation, address resolution, memory effects, CPU/state
 * mutation, sequencing, timing, or opcode handlers.
 *
 * Test words are constructed from pinned catalog bases and fields. This is
 * decode/layout coverage; no handler is allowed to recover MOD from a raw
 * instruction word.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct standard_indirect_case {
    ti_c54x_opcode_id id;
    uint16_t base;
    uint16_t mask;
    ti_c54x_operand_layout_id layout;
    uint8_t program_word_count;
    ti_c54x_repeatability_class repeatability;
    ti_c54x_input_resource_mask input_resources;
    bool has_d_source;
} standard_indirect_case;

static const standard_indirect_case cases[] = {
    {TI_C54X_OPCODE_ID_LD_SMEM, UINT16_C(0x1080), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD, UINT8_C(1),
     TI_C54X_REPEATABILITY_UNRESOLVED, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
     true},
    {TI_C54X_OPCODE_ID_LD_SMEM_TS, UINT16_C(0x1480), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD, UINT8_C(1),
     TI_C54X_REPEATABILITY_UNRESOLVED,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, true},
    {TI_C54X_OPCODE_ID_STL_SMEM, UINT16_C(0x8080), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD, UINT8_C(1),
     TI_C54X_REPEATABILITY_UNRESOLVED,
     TI_C54X_INPUT_RESOURCES_ACCUMULATORS, true},
    {TI_C54X_OPCODE_ID_LDU_SMEM, UINT16_C(0x1280), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD, UINT8_C(1),
     TI_C54X_REPEATABILITY_UNRESOLVED, TI_C54X_INPUT_RESOURCES_NONE, true},
    {TI_C54X_OPCODE_ID_LD_T_SMEM, UINT16_C(0x3080), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD, UINT8_C(1),
     TI_C54X_REPEATABILITY_UNRESOLVED, TI_C54X_INPUT_RESOURCES_NONE, false},
    {TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM, UINT16_C(0x7680), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD, UINT8_C(2),
     TI_C54X_REPEATABILITY_UNRESOLVED, TI_C54X_INPUT_RESOURCES_NONE, false},
    {TI_C54X_OPCODE_ID_LD_DP_SMEM, UINT16_C(0x4680), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD, UINT8_C(1),
     TI_C54X_REPEATABILITY_INELIGIBLE, TI_C54X_INPUT_RESOURCES_NONE, false},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM, UINT16_C(0x3280), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD, UINT8_C(1),
     TI_C54X_REPEATABILITY_UNRESOLVED, TI_C54X_INPUT_RESOURCES_NONE, false},
    {TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16, UINT16_C(0x4480), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD, UINT8_C(1),
     TI_C54X_REPEATABILITY_UNRESOLVED, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
     true},
    {TI_C54X_OPCODE_ID_ST_T_SMEM, UINT16_C(0x8C80), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD, UINT8_C(1),
     TI_C54X_REPEATABILITY_UNRESOLVED,
     TI_C54X_INPUT_RESOURCES_T_REGISTER, false},
};

static void
check_operand(const ti_c54x_operand_layout_set_view *view,
              const ti_c54x_program_words_view *words, uint8_t index,
              uint64_t expected, uint8_t width)
{
    ti_c54x_operand_field_value value = {UINT64_C(0xDEADBEEF), UINT8_C(0xA5)};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           view, words, index, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(value.raw_value == expected);
    TI_C54X_TEST_CHECK(value.width_bits == width);
}

static void
check_case(const standard_indirect_case *test_case, uint8_t mode,
           uint8_t arf, uint8_t d_source)
{
    const uint16_t first_word = (uint16_t)(
        test_case->base | (uint16_t)(mode << 3U) | arf |
        (test_case->has_d_source ? (uint16_t)(d_source << 8U) : UINT16_C(0)));
    const uint16_t extension = UINT16_C(0xC53A);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, extension},
        .program_word_count = test_case->program_word_count};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK((uint16_t)(first_word & test_case->mask) ==
                       test_case->base);
    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == test_case->id);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count ==
                       test_case->program_word_count);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == test_case->id);
    TI_C54X_TEST_CHECK(instruction.operand_layout == test_case->layout);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       test_case->repeatability);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       test_case->input_resources);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count ==
                       test_case->program_word_count);
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == first_word);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout,
                           test_case->program_word_count, &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);

    if (test_case->layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD) {
        TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(3));
        check_operand(&view, &words, UINT8_C(0), d_source, UINT8_C(1));
        check_operand(&view, &words, UINT8_C(1), arf, UINT8_C(3));
        check_operand(&view, &words, UINT8_C(2), mode, UINT8_C(2));
    } else if (test_case->layout == TI_C54X_OPERAND_LAYOUT_ARF_MOD) {
        TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(2));
        check_operand(&view, &words, UINT8_C(0), arf, UINT8_C(3));
        check_operand(&view, &words, UINT8_C(1), mode, UINT8_C(2));
    } else {
        TI_C54X_TEST_CHECK(test_case->layout ==
                           TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD);
        TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(3));
        check_operand(&view, &words, UINT8_C(0), arf, UINT8_C(3));
        check_operand(&view, &words, UINT8_C(1), extension, UINT8_C(16));
        check_operand(&view, &words, UINT8_C(2), mode, UINT8_C(2));
    }
}

static void
test_all_neutral_rows_and_modes(void)
{
    static const uint8_t representative_arfs[] = {UINT8_C(0), UINT8_C(7)};
    static const uint8_t representative_d_sources[] = {UINT8_C(0), UINT8_C(1)};

    for (size_t case_index = 0U;
         case_index < sizeof(cases) / sizeof(cases[0]); ++case_index) {
        const standard_indirect_case *test_case = &cases[case_index];
        for (uint8_t mode = UINT8_C(0); mode < UINT8_C(4); ++mode) {
            for (size_t arf_index = 0U;
                 arf_index < sizeof(representative_arfs) /
                                  sizeof(representative_arfs[0]);
                 ++arf_index) {
                for (size_t d_index = 0U;
                     d_index < sizeof(representative_d_sources) /
                                   sizeof(representative_d_sources[0]);
                     ++d_index) {
                    check_case(test_case, mode, representative_arfs[arf_index],
                               representative_d_sources[d_index]);
                }
            }
        }
    }
}

static void
test_layout_width_mismatches_are_rejected(void)
{
    for (size_t case_index = 0U;
         case_index < sizeof(cases) / sizeof(cases[0]); ++case_index) {
        const standard_indirect_case *test_case = &cases[case_index];
        const uint8_t wrong_word_count =
            test_case->program_word_count == UINT8_C(1) ? UINT8_C(2)
                                                        : UINT8_C(1);
        ti_c54x_operand_layout_set_view view;
        unsigned char before[sizeof(view)];

        (void)memset(&view, 0xA5, sizeof(view));
        (void)memcpy(before, &view, sizeof(before));
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                               test_case->layout, wrong_word_count, &view) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_MISMATCH);
        TI_C54X_TEST_CHECK(memcmp(before, &view, sizeof(view)) == 0);
    }
}

static void
test_forged_id_completion_is_transactional(void)
{
    for (size_t case_index = 0U;
         case_index < sizeof(cases) / sizeof(cases[0]); ++case_index) {
        const standard_indirect_case *test_case = &cases[case_index];
        const size_t forged_index =
            (case_index + 1U) % (sizeof(cases) / sizeof(cases[0]));
        const ti_c54x_instruction_completion_status expected_status =
            test_case->program_word_count == cases[forged_index].program_word_count
                ? TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH
                : TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH;
        const uint16_t word = (uint16_t)(test_case->base | UINT16_C(7));
        const ti_c54x_decode_result valid =
            ti_c54x_decode_first_word(word);
        const ti_c54x_instruction_word_bundle bundle = {
            .program_words = {word, UINT16_C(0)},
            .program_word_count = test_case->program_word_count};
        ti_c54x_decode_result forged = valid;
        ti_c54x_decoded_instruction output;
        unsigned char before[sizeof(output)];

        TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(valid.id == test_case->id);
        forged.id = cases[forged_index].id;
        (void)memset(&output, 0xA5, sizeof(output));
        (void)memcpy(before, &output, sizeof(before));
        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &forged, &bundle, &output) == expected_status);
        TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
    }
}

int
main(void)
{
    test_all_neutral_rows_and_modes();
    test_layout_width_mismatches_are_rejected();
    test_forged_id_completion_is_transactional();
    return 0;
}
