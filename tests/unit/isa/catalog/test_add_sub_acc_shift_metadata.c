/* Module contract
 * Owns: exhaustive catalog identity, operand-layout, resource, and lookup
 * proofs for the ADD/SUB accumulator source-destination-shift5 rows.
 * Does not own: arithmetic semantics, CPU mutation, or inventory policy.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

static void check_row(ti_c54x_opcode_id id, uint16_t base)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(id);
    uint8_t source;
    uint8_t destination;
    uint8_t shift;

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == id);
    TI_C54X_TEST_CHECK(metadata->first_word_value == base);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFCE0));
    TI_C54X_TEST_CHECK(metadata->program_word_count == 1U);
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM);

    for (source = 0U; source < 2U; ++source) {
        for (destination = 0U; destination < 2U; ++destination) {
            for (shift = 0U; shift < 32U; ++shift) {
                const uint16_t word = (uint16_t)(base |
                    ((uint16_t)source << 9U) |
                    ((uint16_t)destination << 8U) | shift);
                TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                                       word) == id);
            }
        }
    }
}

int main(void)
{
    check_row(TI_C54X_OPCODE_ID_ADD_ACC_SHIFT, UINT16_C(0xF400));
    check_row(TI_C54X_OPCODE_ID_SUB_ACC_SHIFT, UINT16_C(0xF420));
    return 0;
}
