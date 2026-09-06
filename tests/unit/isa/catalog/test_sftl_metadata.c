/*
 * Module contract
 *
 * Owns: executable proof of the planned SFTL accumulator catalog row,
 * including its masked identity, one-word width, generic source/destination
 * plus signed-five-bit-shift layout, unresolved repeatability, and
 * accumulator input declaration.
 * Does not own: opcode decoding, operand extraction, shift semantics, CPU or
 * accumulator mutation, sequencing, timing, or generated lookup artifacts.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

int
main(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_SFTL_ACC);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_SFTL_ACC);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xF0E0));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFCE0));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    return 0;
}
