/*
 * Module contract
 *
 * Owns: executable proof of the ROLTC accumulator catalog row, including its
 * masked first-word identity, one-word width, source-operand layout,
 * unresolved repeatability, and accumulator/status-TC input declaration.
 * Does not own: opcode decoding, operand extraction, operation semantics,
 * CPU/state integration, timing, or generated lookup artifacts.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static void
test_roltc_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ROLTC_ACC);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_ROLTC_ACC);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xF492));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFEFF));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_TC);
}

static void
test_roltc_first_words(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF492)) ==
                       TI_C54X_OPCODE_ID_ROLTC_ACC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF592)) ==
                       TI_C54X_OPCODE_ID_ROLTC_ACC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF491)) ==
                       TI_C54X_OPCODE_ID_ROL_ACC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF490)) ==
                       TI_C54X_OPCODE_ID_ROR_ACC);
}

int
main(void)
{
    test_roltc_metadata();
    test_roltc_first_words();
    return 0;
}
