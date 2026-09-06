/*
 * Module contract
 *
 * Owns: executable proof of the ROL/ROR catalog rows, including their
 * masked first-word identities, one-word width, source-operand layout,
 * unresolved repeatability, and combined accumulator/status-C inputs.
 * Does not own: opcode decoding, operand extraction, operation semantics,
 * CPU/state integration, timing, or generated lookup artifacts.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

static void
check_rotate_metadata(ti_c54x_opcode_id id, uint16_t first_word_value)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(id);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == id);
    TI_C54X_TEST_CHECK(metadata->first_word_value == first_word_value);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFEFF));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C);
}

static void
test_status_c_resource_is_a_named_combination(void)
{
    TI_C54X_TEST_CHECK(TI_C54X_INPUT_RESOURCES_STATUS_C !=
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C ==
                       (TI_C54X_INPUT_RESOURCES_ACCUMULATORS |
                        TI_C54X_INPUT_RESOURCES_STATUS_C));
    TI_C54X_TEST_CHECK((TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C &
                        (ti_c54x_input_resource_mask)~
                            TI_C54X_INPUT_RESOURCES_KNOWN_MASK) ==
                       TI_C54X_INPUT_RESOURCES_NONE);
}

int
main(void)
{
    test_status_c_resource_is_a_named_combination();
    check_rotate_metadata(TI_C54X_OPCODE_ID_ROL_ACC, UINT16_C(0xF491));
    check_rotate_metadata(TI_C54X_OPCODE_ID_ROR_ACC, UINT16_C(0xF490));
    return 0;
}
