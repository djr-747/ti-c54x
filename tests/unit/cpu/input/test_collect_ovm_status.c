/* Module contract
 * Owns: executable proof for ST1.OVM input collection, including canonical
 * unknownness, resource composition, layout stability, and rejection atomicity.
 * Does not own: status accessors, opcode semantics, or CPU execution.
 */

#include "../../../../src/cpu/input/collect.h"
#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../src/isa/ops/dispatch/trusted_handler.h"
#include "../../../../src/isa/ops/op_input.h"
#include "../../../../src/state/register_layout.h"
#include "../../../../src/state/status/registers.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(ti_c54x_op_input_snapshot) == 80U,
               "op input layout must remain 80 bytes on LP64");
_Static_assert(sizeof(ti_c54x_opcode_metadata) == 24U,
               "metadata carrier must not grow on LP64");
_Static_assert(sizeof(ti_c54x_decoded_instruction) == 20U,
               "decoded carrier must not grow on LP64");
_Static_assert(sizeof(ti_c54x_trusted_execution_packet) == 104U,
               "trusted packet must not grow on LP64");
#elif UINTPTR_MAX == UINT32_MAX
_Static_assert(sizeof(ti_c54x_opcode_metadata) == 24U,
               "metadata carrier must remain 24 bytes on ILP32");
_Static_assert(sizeof(ti_c54x_decoded_instruction) == 20U,
               "decoded carrier must remain 20 bytes on ILP32");
_Static_assert(sizeof(ti_c54x_op_input_snapshot) == 76U,
               "op input carrier must remain 76 bytes on ILP32");
_Static_assert(sizeof(ti_c54x_trusted_execution_packet) == 76U,
               "trusted packet must remain 76 bytes on ILP32");
#endif

static ti_c54x_input_sources source_for(
    const ti_c54x_status_register_state *status)
{
    ti_c54x_input_sources sources = {0};
    sources.status = status;
    return sources;
}

int main(void)
{
    ti_c54x_status_register_state status;
    ti_c54x_accumulator_state accumulators;
    ti_c54x_input_sources sources;
    ti_c54x_op_input_snapshot output;


    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&status) ==
                       TI_C54X_STATUS_REGISTER_OK);
    sources = source_for(&status);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    sources.accumulators = &accumulators;
    output = (ti_c54x_op_input_snapshot){0};
    output.status_ovm = 1U;
    output.status_ovm_known = true;
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_OVM, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_ovm == 0U);
    TI_C54X_TEST_CHECK(output.status_ovm_known);

    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_ovm(&status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM,
                           &sources, &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(!output.accumulator_a_known);
    TI_C54X_TEST_CHECK(output.status_sxm_known);
    TI_C54X_TEST_CHECK(output.status_ovm_known);

    output = (ti_c54x_op_input_snapshot){0};
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_OVM, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_ovm == 1U && output.status_ovm_known);

    status.st1_known_mask = (uint16_t)(status.st1_known_mask &
                                       (uint16_t)~TI_C54X_ST1_OVM_MASK);
    output = (ti_c54x_op_input_snapshot){0};
    output.status_ovm = 1U;
    output.status_ovm_known = true;
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_OVM, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_ovm == 0U && !output.status_ovm_known);

    /* Bit 15 (0x8000) is the sixteenth and final resource
     * (STATUS_C16): the 16-bit mask is fully allocated, so the old
     * invalid-mask probe now collects reset-state C16 (0, known). */
    output.status_ovm = 1U;
    output.status_ovm_known = true;
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           (ti_c54x_input_resource_mask)0x8000U, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_c16 == 0U && output.status_c16_known);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_OVM, NULL, &output) ==
                       TI_C54X_INPUT_COLLECT_INVALID_SOURCE);
    return 0;
}
