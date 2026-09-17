/*
 * Module contract
 *
 * Owns: executable proof of the LDR Smem,dst catalog row, including
 * its exact masked identity, D/Smem population, reused
 * single-indirect layout, SXM-only input resources and
 * repeatability metadata, and fixed-bit separation.
 * Does not own: opcode decoding, operand extraction, AR reads,
 * CPU/state mutation, sequencing, timing, generated lookup
 * production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t LDR_SMEM_VALUE = UINT16_C(0x1680);
static const uint16_t LDR_SMEM_SAMPLE_WORD = UINT16_C(0x1681);
static const uint16_t LDR_SMEM_MASK = UINT16_C(0xFEE0);

static bool
matches_ldr_smem(uint16_t word)
{
    return (word & LDR_SMEM_MASK) == LDR_SMEM_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LDR);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 259);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LDR);
    TI_C54X_TEST_CHECK(metadata->first_word_value == LDR_SMEM_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == LDR_SMEM_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LDR_SMEM_SAMPLE_WORD) == TI_C54X_OPCODE_ID_LDR);

    /* D(8) and Smem(4:0) are variable; bits 15:9 and 7:5 are fixed. */
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x011F));
}

static void
test_exact_population_and_disjointness(void)
{
    unsigned encoded_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool matches = matches_ldr_smem(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_LDR) == matches);
        if (matches) {
            ++encoded_count;
        }
    }

    TI_C54X_TEST_CHECK(encoded_count == 64U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LDR_SMEM_VALUE) == TI_C54X_OPCODE_ID_LDR);
}

int main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    return 0;
}
