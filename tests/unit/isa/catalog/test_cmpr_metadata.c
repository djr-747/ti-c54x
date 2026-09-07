/*
 * Module contract
 *
 * Owns: executable proof of the CMPR CC,ARx catalog row, including its
 * masked one-word identity, operand layout, repeatability, copied auxiliary
 * register input resource, and complete first-word population. The mask
 * leaves two CC bits and three ARx bits variable, yielding 32 encodings.
 * Does not own: opcode decoding, operand extraction, compare semantics,
 * architectural state, sequencing, timing, or generated lookup production.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static void test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_CMPR_AR);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_CMPR_AR);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xF4A8));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFCF8));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_CONDITION_ARF);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
}

static void test_population_and_lookup(void)
{
    unsigned cmpr_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool cmpr = (word & UINT16_C(0xFCF8)) == UINT16_C(0xF4A8);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_CMPR_AR) == cmpr);
        if (cmpr) {
            ++cmpr_count;
        }
    }

    /* Mask 0xFCF8 leaves CC at bits 9:8 and ARx at bits 2:0. */
    TI_C54X_TEST_CHECK(cmpr_count == 32U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF4A8)) ==
                       TI_C54X_OPCODE_ID_CMPR_AR);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF4AF)) ==
                       TI_C54X_OPCODE_ID_CMPR_AR);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF7AF)) ==
                       TI_C54X_OPCODE_ID_CMPR_AR);
}

int main(void)
{
    test_metadata();
    test_population_and_lookup();
    return 0;
}
