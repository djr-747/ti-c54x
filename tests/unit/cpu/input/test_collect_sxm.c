/* Module contract
 * Owns: the STATUS_SXM input-resource contract proof, including canonical
 * knownness, selective collection, composite collection, and transactional
 * rejection of invalid requests or missing sources.
 * Does not own: collection implementation, status-register accessors, opcode
 * semantics, decode, CPU lifecycle, sequencing, timing, memory, buses, or
 * diagnostics.
 */

#include "../../../../src/cpu/input/collect.h"
#include "../../../../src/isa/ops/op_input.h"
#include "../../../../src/state/accumulator/accumulator.h"
#include "../../../../src/state/register_layout.h"
#include "../../../../src/state/status/registers.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_input_sources sources_for(
    const ti_c54x_accumulator_state *accumulators,
    const ti_c54x_status_register_state *status)
{
    ti_c54x_input_sources sources = {0};

    sources.accumulators = accumulators;
    sources.status = status;
    return sources;
}

static ti_c54x_op_input_snapshot sentinel(void)
{
    ti_c54x_op_input_snapshot output = {0};

    output.accumulator_a_raw40 = UINT64_C(0x123456789A);
    output.accumulator_b_raw40 = UINT64_C(0x0ABCDEFFF0);
    output.accumulator_a_known = true;
    output.accumulator_b_known = true;
    output.status_c = 1U;
    output.status_c_known = true;
    output.status_tc = 1U;
    output.status_tc_known = true;
    output.status_sxm = 1U;
    output.status_sxm_known = true;
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
    ti_c54x_accumulator_state accumulators;
    ti_c54x_status_register_state status;
    ti_c54x_input_sources sources;
    ti_c54x_input_sources missing_sources;
    ti_c54x_op_input_snapshot output;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&status) ==
                       TI_C54X_STATUS_REGISTER_OK);

    /* STATUS_SXM-only requests expose reset SXM=1 and preserve no other input. */
    sources = sources_for(NULL, &status);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_SXM, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_sxm == 1U);
    TI_C54X_TEST_CHECK(output.status_sxm_known);
    TI_C54X_TEST_CHECK(output.accumulator_a_raw40 == 0U);
    TI_C54X_TEST_CHECK(output.accumulator_b_raw40 == 0U);
    TI_C54X_TEST_CHECK(!output.accumulator_a_known);
    TI_C54X_TEST_CHECK(!output.accumulator_b_known);
    TI_C54X_TEST_CHECK(output.status_c == 0U);
    TI_C54X_TEST_CHECK(!output.status_c_known);
    TI_C54X_TEST_CHECK(output.status_tc == 0U);
    TI_C54X_TEST_CHECK(!output.status_tc_known);

    /* A known raw SXM=0 is copied as zero with knownness preserved. */
    status.st1 = (uint16_t)(status.st1 & (uint16_t)~TI_C54X_ST1_SXM_MASK);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_SXM, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_sxm == 0U);
    TI_C54X_TEST_CHECK(output.status_sxm_known);

    /* Unknown raw SXM is canonicalized to zero and reports unknown. */
    status.st1 = (uint16_t)(status.st1 | TI_C54X_ST1_SXM_MASK);
    status.st1_known_mask = (uint16_t)(status.st1_known_mask &
                                       (uint16_t)~TI_C54X_ST1_SXM_MASK);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_SXM, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_sxm == 0U);
    TI_C54X_TEST_CHECK(!output.status_sxm_known);

    /* Composite collection copies every requested resource in one snapshot. */
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0xABCDE12345)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0123456789)) == TI_C54X_ACCUMULATOR_OK);
    status.st1_known_mask = (uint16_t)(status.st1_known_mask |
                                       TI_C54X_ST1_SXM_MASK);
    sources = sources_for(&accumulators, &status);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           (ti_c54x_input_resource_mask)(
                               TI_C54X_INPUT_RESOURCES_ACCUMULATORS |
                               TI_C54X_INPUT_RESOURCES_STATUS_C |
                               TI_C54X_INPUT_RESOURCES_STATUS_TC |
                               TI_C54X_INPUT_RESOURCES_STATUS_SXM),
                           &sources, &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.accumulator_a_raw40 ==
                       UINT64_C(0xABCDE12345));
    TI_C54X_TEST_CHECK(output.accumulator_b_raw40 ==
                       UINT64_C(0x0123456789));
    TI_C54X_TEST_CHECK(output.accumulator_a_known);
    TI_C54X_TEST_CHECK(output.accumulator_b_known);
    TI_C54X_TEST_CHECK(output.status_c == 1U);
    TI_C54X_TEST_CHECK(output.status_c_known);
    TI_C54X_TEST_CHECK(output.status_tc == 1U);
    TI_C54X_TEST_CHECK(output.status_tc_known);
    TI_C54X_TEST_CHECK(output.status_sxm == 1U);
    TI_C54X_TEST_CHECK(output.status_sxm_known);

    /* Invalid masks and missing requested status reject without mutation. */
    output = sentinel();
    expect_rejected((ti_c54x_input_resource_mask)0x0100U, &sources, &output);
    missing_sources = sources_for(NULL, NULL);
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_STATUS_SXM,
                    &missing_sources, &output);
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_STATUS_SXM, NULL, &output);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_SXM, &sources,
                           NULL) != TI_C54X_INPUT_COLLECT_OK);
    return 0;
}
