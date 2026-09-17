/*
 * Module contract
 *
 * Owns: executable proof of the MOD2 LD Smem,DP and LD Smem,ASM
 * catalog rows, including their exact masked identity, ARF layout,
 * repeatability/resource metadata, eight-word populations, and fixed-bit
 * separation. Does not own: opcode decoding, operand extraction, address
 * resolution, memory effects, CPU/state mutation, sequencing, timing,
 * generated lookup production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct ld_status_row {
    ti_c54x_opcode_id id;
    uint16_t value;
    ti_c54x_repeatability_class repeatability;
} ld_status_row;

static const ld_status_row rows[] = {
    {TI_C54X_OPCODE_ID_LD_DP_SMEM, UINT16_C(0x4680),
     TI_C54X_REPEATABILITY_INELIGIBLE},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM, UINT16_C(0x3280),
     TI_C54X_REPEATABILITY_UNRESOLVED},
};

static const uint16_t LD_STATUS_MASK = UINT16_C(0xFFE0);
static const uint16_t LD_STATUS_LAX_MASK = UINT16_C(0xFEE0);

static bool
matches(const ld_status_row *row, uint16_t word)
{
    return (word & LD_STATUS_MASK) == row->value;
}

static void
test_metadata_rows(void)
{
    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 199);

    for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        const ld_status_row *row = &rows[index];
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(row->id);

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == row->id);
        TI_C54X_TEST_CHECK(metadata->first_word_value == row->value);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == LD_STATUS_MASK);
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
        TI_C54X_TEST_CHECK(metadata->operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_ARF_MOD);
        TI_C54X_TEST_CHECK(metadata->operand_layout !=
                           TI_C54X_OPERAND_LAYOUT_LD_IMMEDIATE_ARP);
        TI_C54X_TEST_CHECK(metadata->repeatability == row->repeatability);
        TI_C54X_TEST_CHECK(metadata->input_resources ==
                           TI_C54X_INPUT_RESOURCES_NONE);

        /* Only ARF bits 2:0 vary; bit 8 is fixed zero. */
        TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                           UINT16_C(0x001F));
    }
}

static void
test_patterns_are_disjoint(void)
{
    const ld_status_row *dp = &rows[0];
    const ld_status_row *asm_form = &rows[1];

    TI_C54X_TEST_CHECK((dp->value & LD_STATUS_MASK) !=
                       (asm_form->value & LD_STATUS_MASK));
    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        TI_C54X_TEST_CHECK(!(matches(dp, word) && matches(asm_form, word)));
    }
}

static void
test_exact_eight_word_population(void)
{
    for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        const ld_status_row *row = &rows[index];
        unsigned encoded_count = 0U;

        for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
            const uint16_t word = (uint16_t)raw;
            const ti_c54x_opcode_id id =
                ti_c54x_opcode_catalog_lookup_first_word(word);

            if (matches(row, word)) {
                ++encoded_count;
                TI_C54X_TEST_CHECK(id == row->id);
            }
            TI_C54X_TEST_CHECK((id == row->id) == matches(row, word));
        }

        TI_C54X_TEST_CHECK(encoded_count == 32U);
        for (uint16_t arf = 0U; arf < UINT16_C(8); ++arf) {
            TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                                   (uint16_t)(row->value | arf)) == row->id);
        }
    }
}

static void
test_fixed_bit_near_misses_and_no_arp_alias(void)
{
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x0020), UINT16_C(0x0040), UINT16_C(0x0080),
        UINT16_C(0x0100), UINT16_C(0x0200), UINT16_C(0x0400),
        UINT16_C(0x0800), UINT16_C(0x1000), UINT16_C(0x2000),
        UINT16_C(0x4000), UINT16_C(0x8000)};

    for (size_t row_index = 0U;
         row_index < sizeof(rows) / sizeof(rows[0]); ++row_index) {
        const ld_status_row *row = &rows[row_index];

        for (size_t mask_index = 0U;
             mask_index < sizeof(fixed_bit_masks) /
                               sizeof(fixed_bit_masks[0]);
             ++mask_index) {
            const uint16_t word =
                (uint16_t)(row->value ^ fixed_bit_masks[mask_index]);
            const ti_c54x_opcode_id id =
                ti_c54x_opcode_catalog_lookup_first_word(word);

            TI_C54X_TEST_CHECK(id != row->id);
            if (fixed_bit_masks[mask_index] == UINT16_C(0x0100)) {
                /* A lax FE mask would overaccept this fixed-bit near miss. */
                TI_C54X_TEST_CHECK((word & LD_STATUS_LAX_MASK) == row->value);
                TI_C54X_TEST_CHECK(id == TI_C54X_OPCODE_ID_UNKNOWN);
            }
        }
    }

    /* ARP is an immediate-only form; neither status row claims its layout. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF4A0)) ==
                       TI_C54X_OPCODE_ID_LD_IMMEDIATE_ARP);
}

int
main(void)
{
    test_metadata_rows();
    test_patterns_are_disjoint();
    test_exact_eight_word_population();
    test_fixed_bit_near_misses_and_no_arp_alias();
    return 0;
}
