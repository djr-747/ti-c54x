/* Module contract
 * Owns: exhaustive metadata and first-word lookup proofs for the bounded
 * syntax-6 ADD_DUAL_MOD0 and SUB_DUAL_MOD0 catalog rows.
 * Does not own: dual addressing, arithmetic semantics, CPU mutation, or
 * inventory policy.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

static void check_row(ti_c54x_opcode_id id, uint16_t base)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(id);
    uint8_t destination;
    uint8_t xar;
    uint8_t yar;

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == id);
    TI_C54X_TEST_CHECK(metadata->first_word_value == base);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFECC));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_DUAL_DESTINATION_XAR_YAR);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_C_SXM_OVM);

    for (destination = 0U; destination < 2U; ++destination) {
        for (xar = 0U; xar < 4U; ++xar) {
            for (yar = 0U; yar < 4U; ++yar) {
                const uint16_t word = (uint16_t)(base |
                    ((uint16_t)destination << 8U) |
                    ((uint16_t)xar << 4U) | yar);
                TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                                       word) == id);
            }
        }
    }
}

int main(void)
{
    check_row(TI_C54X_OPCODE_ID_ADD_DUAL_MOD0, UINT16_C(0xA000));
    check_row(TI_C54X_OPCODE_ID_SUB_DUAL_MOD0, UINT16_C(0xA200));
    /* Non-MOD0/MOD0 combinations live in sibling rows; W52C activates the
     * MOD3-involving rows. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xA0CC)) ==
                       TI_C54X_OPCODE_ID_ADD_DUAL_M3_M3);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xA2CC)) ==
                       TI_C54X_OPCODE_ID_SUB_DUAL_M3_M3);
    return 0;
}
