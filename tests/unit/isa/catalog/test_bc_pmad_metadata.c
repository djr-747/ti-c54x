/*
 * Module contract
 *
 * Owns: executable proof of the BC pmad,cond catalog row (Table 2-12
 * syntax 1), including its exact masked identity, 256-encoding
 * population over the condition-code byte, dedicated
 * branch-condition layout, accumulator/C/TC input resources and
 * INELIGIBLE repeatability metadata, and fixed-bit separation.
 * Does not own: opcode decoding, condition evaluation, operand
 * extraction, PC writes, CPU/state mutation, sequencing, timing,
 * generated lookup production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t BC_PMAD_VALUE = UINT16_C(0xF800);
static const uint16_t BC_PMAD_MASK = UINT16_C(0xFF00);
/* Bit 9 set selects the BCD sibling row bound in W69. */
static const uint16_t BC_DELAYED_WORD = UINT16_C(0xFA00);

static bool
matches_bc_pmad(uint16_t word)
{
    return (word & BC_PMAD_MASK) == BC_PMAD_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_BC_PMAD);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 259);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_BC_PMAD);
    TI_C54X_TEST_CHECK(metadata->first_word_value == BC_PMAD_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == BC_PMAD_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           BC_PMAD_VALUE) == TI_C54X_OPCODE_ID_BC_PMAD);

    /* Only the 8-bit condition code varies; Z stays fixed for BC. */
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x00FF));
}

static void
test_exact_population_and_disjointness(void)
{
    unsigned encoded_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool matches = matches_bc_pmad(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_BC_PMAD) == matches);
        if (matches) {
            ++encoded_count;
        }
    }

    TI_C54X_TEST_CHECK(encoded_count == 256U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           BC_DELAYED_WORD) != TI_C54X_OPCODE_ID_BC_PMAD);
}

int main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    return 0;
}
