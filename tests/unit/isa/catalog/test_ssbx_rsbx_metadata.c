/*
 * Module contract
 *
 * Owns: executable proof of the SSBX/RSBX catalog rows, including their
 * masked one-word identities, status-bit layout, repeatability, resources,
 * and complete first-word population. It deliberately verifies the
 * mathematically implied 32 encodings per row (N plus SBIT), even though an
 * activation note accidentally says 16.
 * Does not own: opcode decoding, operand extraction, status mutation,
 * operation semantics, CPU/state integration, timing, or generated lookup.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static void check_row(ti_c54x_opcode_id id, uint16_t value)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(id);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == id);
    TI_C54X_TEST_CHECK(metadata->first_word_value == value);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFDF0));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_BIT);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
}

static void test_population_and_lookup(void)
{
    unsigned ssbx_count = 0U;
    unsigned rsbx_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool ssbx = (word & UINT16_C(0xFDF0)) == UINT16_C(0xF5B0);
        const bool rsbx = (word & UINT16_C(0xFDF0)) == UINT16_C(0xF4B0);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_SSBX_STATUS) == ssbx);
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_RSBX_STATUS) == rsbx);
        if (ssbx) {
            ++ssbx_count;
        }
        if (rsbx) {
            ++rsbx_count;
        }
    }

    /* Mask 0xFDF0 leaves N at bit 9 and SBIT at bits 3:0 variable. */
    TI_C54X_TEST_CHECK(ssbx_count == 32U);
    TI_C54X_TEST_CHECK(rsbx_count == 32U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF5B0)) ==
                       TI_C54X_OPCODE_ID_SSBX_STATUS);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF7BF)) ==
                       TI_C54X_OPCODE_ID_SSBX_STATUS);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF4B0)) ==
                       TI_C54X_OPCODE_ID_RSBX_STATUS);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF6BF)) ==
                       TI_C54X_OPCODE_ID_RSBX_STATUS);
}

int main(void)
{
    check_row(TI_C54X_OPCODE_ID_SSBX_STATUS, UINT16_C(0xF5B0));
    check_row(TI_C54X_OPCODE_ID_RSBX_STATUS, UINT16_C(0xF4B0));
    test_population_and_lookup();
    return 0;
}
