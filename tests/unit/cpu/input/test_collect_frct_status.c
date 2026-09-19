/* Module contract
 *
 * Owns: executable proof for ST1.FRCT input collection, including canonical
 * unknownness, independent FRCT/OVM selection, and transactional rejection.
 * Does not own: collection implementation, status-register accessors, opcode
 * semantics, decode, CPU lifecycle, sequencing, timing, memory, buses, or
 * diagnostics.
 */

#include "../../../../src/cpu/input/collect.h"
#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/ops/op_input.h"
#include "../../../../src/state/register_layout.h"
#include "../../../../src/state/status/registers.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

_Static_assert(sizeof(ti_c54x_op_input_snapshot) == 80U,
               "op input snapshot must retain its fixed 80-byte layout");

#define FRCT_BIT_INDEX UINT8_C(6)

static ti_c54x_input_sources sources_for(
    const ti_c54x_status_register_state *status)
{
    ti_c54x_input_sources sources = {0};

    sources.status = status;
    return sources;
}

static ti_c54x_op_input_snapshot sentinel(void)
{
    ti_c54x_op_input_snapshot output = {0};

    output.status_c = 1U;
    output.status_c_known = true;
    output.status_ovm = 1U;
    output.status_ovm_known = true;
    output.status_frct = 1U;
    output.status_frct_known = true;
    return output;
}

static void expect_rejected(
    ti_c54x_input_resource_mask resources,
    const ti_c54x_input_sources *sources,
    ti_c54x_op_input_snapshot *output)
{
    const ti_c54x_op_input_snapshot before = *output;

    TI_C54X_TEST_CHECK(ti_c54x_input_collect(resources, sources, output) !=
                       TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(memcmp(&before, output, sizeof(before)) == 0);
}

int main(void)
{
    ti_c54x_status_register_state status;
    ti_c54x_input_sources sources;
    ti_c54x_op_input_snapshot output;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&status) ==
                       TI_C54X_STATUS_REGISTER_OK);
    sources = sources_for(&status);

    /* NONE remains source-independent and canonicalizes FRCT fields. */
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_NONE, NULL, &output) ==
                       TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_frct == 0U);
    TI_C54X_TEST_CHECK(!output.status_frct_known);
    TI_C54X_TEST_CHECK(output.status_ovm == 0U);
    TI_C54X_TEST_CHECK(!output.status_ovm_known);

    /* Common reset leaves ST1.FRCT at 0 and known. */
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_FRCT, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_frct == 0U);
    TI_C54X_TEST_CHECK(output.status_frct_known);
    TI_C54X_TEST_CHECK(output.status_ovm == 0U);
    TI_C54X_TEST_CHECK(!output.status_ovm_known);

    /* Clearing the FRCT known bit makes collection unknown again. */
    status.st1_known_mask = (uint16_t)(status.st1_known_mask &
                                       (uint16_t)~TI_C54X_ST1_FRCT_MASK);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_FRCT, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_frct == 0U);
    TI_C54X_TEST_CHECK(!output.status_frct_known);

    /* A bit write makes FRCT known without changing OVM knownness. */
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_bit(
                           &status,
                           TI_C54X_STATUS_REGISTER_TARGET_ST1,
                           FRCT_BIT_INDEX, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_FRCT, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_frct == 1U);
    TI_C54X_TEST_CHECK(output.status_frct_known);
    TI_C54X_TEST_CHECK(output.status_ovm == 0U);
    TI_C54X_TEST_CHECK(!output.status_ovm_known);

    /* OVM (0/known from common reset) and FRCT can be requested together
     * and carry independent knownness. */
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           (ti_c54x_input_resource_mask)(
                               TI_C54X_INPUT_RESOURCES_STATUS_OVM |
                               TI_C54X_INPUT_RESOURCES_STATUS_FRCT),
                           &sources, &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_ovm == 0U);
    TI_C54X_TEST_CHECK(output.status_ovm_known);
    TI_C54X_TEST_CHECK(output.status_frct == 1U);
    TI_C54X_TEST_CHECK(output.status_frct_known);

    status.st1_known_mask = (uint16_t)(status.st1_known_mask &
                                       (uint16_t)~TI_C54X_ST1_OVM_MASK);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           (ti_c54x_input_resource_mask)(
                               TI_C54X_INPUT_RESOURCES_STATUS_OVM |
                               TI_C54X_INPUT_RESOURCES_STATUS_FRCT),
                           &sources, &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_ovm == 0U);
    TI_C54X_TEST_CHECK(!output.status_ovm_known);
    TI_C54X_TEST_CHECK(output.status_frct == 1U);
    TI_C54X_TEST_CHECK(output.status_frct_known);

    /* Missing FRCT sources and NULL outputs are nonmutating. Bit 15
     * (0x8000) is the sixteenth and final resource (STATUS_C16):
     * the 16-bit mask is fully allocated, so the old invalid-mask
     * probe now collects reset-state C16 (0, known). */
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_STATUS_FRCT, NULL, &output);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_FRCT, &sources,
                           NULL) != TI_C54X_INPUT_COLLECT_OK);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           (ti_c54x_input_resource_mask)0x8000U, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_c16 == 0U && output.status_c16_known);

    return 0;
}
