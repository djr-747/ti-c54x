/*
 * Module contract
 *
 * Owns: exhaustive proof of the three immediate status-field opcode masks,
 * catalog identities, widths, and raw operand extraction. It does not own
 * Does not own: opcode effects, CPU/state mutation, timing, memory, buses, or
 * diagnostics.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct ld_case {
    ti_c54x_opcode_id id;
    ti_c54x_operand_layout_id layout;
    uint16_t value;
    uint16_t mask;
    uint16_t operand_limit;
    uint8_t width;
    ti_c54x_repeatability_class repeatability;
} ld_case;

static const ld_case cases[] = {
    {TI_C54X_OPCODE_ID_LD_IMMEDIATE_DP,
     TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_DP,
     UINT16_C(0xEA00), UINT16_C(0xFE00), UINT16_C(0x0200), UINT8_C(9),
     TI_C54X_REPEATABILITY_INELIGIBLE},
    {TI_C54X_OPCODE_ID_LD_IMMEDIATE_ASM,
     TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ASM,
     UINT16_C(0xED00), UINT16_C(0xFFE0), UINT16_C(0x0020), UINT8_C(5),
     TI_C54X_REPEATABILITY_UNRESOLVED},
    {TI_C54X_OPCODE_ID_LD_IMMEDIATE_ARP,
     TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ARP,
     UINT16_C(0xF4A0), UINT16_C(0xFFF8), UINT16_C(0x0008), UINT8_C(3),
     TI_C54X_REPEATABILITY_INELIGIBLE},
};

static void
check_case(const ld_case *test_case)
{
    uint32_t raw;
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(test_case->id);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->first_word_value == test_case->value);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == test_case->mask);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout == test_case->layout);
    TI_C54X_TEST_CHECK(metadata->repeatability == test_case->repeatability);

    for (raw = 0U; raw < (uint32_t)test_case->operand_limit; ++raw) {
        const uint16_t word = (uint16_t)(test_case->value | raw);
        const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
        ti_c54x_operand_layout_set_view view;
        ti_c54x_operand_field_value operand = {0U, 0U};
        const ti_c54x_program_words_view words = {&word, UINT8_C(1)};

        TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(first.id == test_case->id);
        TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                               test_case->layout, UINT8_C(1), &view) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, UINT8_C(0), &operand) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK_U64(operand.raw_value, raw);
        TI_C54X_TEST_CHECK(operand.width_bits == test_case->width);
    }
}

static void
test_masks_do_not_accept_near_misses(void)
{
    const uint16_t near_misses[] = {
        UINT16_C(0xE800), /* unregistered short accumulator load */
        UINT16_C(0xED20), /* ASM fixed bit 5 differs */
        UINT16_C(0xF480), /* ARP fixed bit 5 differs */
        UINT16_C(0xE900), /* DP fixed bit differs */
    };
    size_t index;

    for (index = 0U; index < sizeof(near_misses) / sizeof(near_misses[0]);
         ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(near_misses[index]).status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
    }
}

static void
test_layout_failures_are_transactional(void)
{
    const ti_c54x_operand_layout_id ids[] = {
        TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_DP,
        TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ASM,
        TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ARP,
    };
    size_t index;

    for (index = 0U; index < sizeof(ids) / sizeof(ids[0]); ++index) {
        ti_c54x_operand_layout_set_view view;
        unsigned char before[sizeof(view)];
        (void)memset(&view, 0xA5, sizeof(view));
        (void)memcpy(before, &view, sizeof(before));
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                               ids[index], UINT8_C(2), &view) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_MISMATCH);
        TI_C54X_TEST_CHECK(memcmp(before, &view, sizeof(view)) == 0);
    }
}

int
main(void)
{
    size_t index;
    for (index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        check_case(&cases[index]);
    }
    test_masks_do_not_accept_near_misses();
    test_layout_failures_are_transactional();
    puts("ld_status_fields_decode: ok");
    return 0;
}
