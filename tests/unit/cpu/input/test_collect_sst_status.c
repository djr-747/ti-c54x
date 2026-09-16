/* Module contract
 * Owns: executable proof for PMST.SST input collection, including canonical
 * unknownness, independent SXM/SST selection, and transactional rejection.
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

_Static_assert(sizeof(ti_c54x_op_input_snapshot) == 64U,
               "op input snapshot must retain its fixed 64-byte layout");

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
    output.status_tc = 1U;
    output.status_tc_known = true;
    output.status_sxm = 1U;
    output.status_sxm_known = true;
    output.status_sst = 1U;
    output.status_sst_known = true;
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

    /* NONE remains source-independent and canonicalizes all SST fields. */
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_NONE, NULL, &output) ==
                       TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_sst == 0U);
    TI_C54X_TEST_CHECK(!output.status_sst_known);
    TI_C54X_TEST_CHECK(output.status_sxm == 0U);
    TI_C54X_TEST_CHECK(!output.status_sxm_known);

    /* Common reset deliberately leaves PMST.SST unknown. */
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_SST, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_sst == 0U);
    TI_C54X_TEST_CHECK(!output.status_sst_known);
    TI_C54X_TEST_CHECK(output.status_sxm == 0U);
    TI_C54X_TEST_CHECK(!output.status_sxm_known);

    /* A typed write makes SST known without changing SXM knownness. */
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_sst(&status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_SST, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_sst == 1U);
    TI_C54X_TEST_CHECK(output.status_sst_known);
    TI_C54X_TEST_CHECK(output.status_sxm == 0U);
    TI_C54X_TEST_CHECK(!output.status_sxm_known);

    /* SXM and SST can be requested together and carry independent knownness. */
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           (ti_c54x_input_resource_mask)(
                               TI_C54X_INPUT_RESOURCES_STATUS_SXM |
                               TI_C54X_INPUT_RESOURCES_STATUS_SST),
                           &sources, &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_sxm == 1U);
    TI_C54X_TEST_CHECK(output.status_sxm_known);
    TI_C54X_TEST_CHECK(output.status_sst == 1U);
    TI_C54X_TEST_CHECK(output.status_sst_known);

    status.st1_known_mask = (uint16_t)(status.st1_known_mask &
                                       (uint16_t)~TI_C54X_ST1_SXM_MASK);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           (ti_c54x_input_resource_mask)(
                               TI_C54X_INPUT_RESOURCES_STATUS_SXM |
                               TI_C54X_INPUT_RESOURCES_STATUS_SST),
                           &sources, &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_sxm == 0U);
    TI_C54X_TEST_CHECK(!output.status_sxm_known);
    TI_C54X_TEST_CHECK(output.status_sst == 1U);
    TI_C54X_TEST_CHECK(output.status_sst_known);

    /* Missing SST sources, invalid masks, and NULL outputs are nonmutating. */
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_STATUS_SST, NULL, &output);
    output = sentinel();
    expect_rejected((ti_c54x_input_resource_mask)0x1000U, &sources, &output);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_SST, &sources,
                           NULL) != TI_C54X_INPUT_COLLECT_OK);
    return 0;
}
