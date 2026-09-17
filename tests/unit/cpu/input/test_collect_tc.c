/* Module contract
 * Owns: the TC-aware input-collection proofs for CPU resource masks,
 * including canonical knownness and transactional rejection.
 * Does not own: collection implementation, status-register accessors,
 * opcode semantics, decode, CPU lifecycle, sequencing, timing, memory,
 * buses, or diagnostics.
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

    /* A TC-only request reads status without requiring accumulator state. */
    sources = sources_for(NULL, &status);
    missing_sources = sources_for(NULL, NULL);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_TC, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_tc == 1U);
    TI_C54X_TEST_CHECK(output.status_tc_known);
    TI_C54X_TEST_CHECK(output.accumulator_a_raw40 == 0U);
    TI_C54X_TEST_CHECK(output.accumulator_b_raw40 == 0U);
    TI_C54X_TEST_CHECK(!output.accumulator_a_known);
    TI_C54X_TEST_CHECK(!output.accumulator_b_known);
    TI_C54X_TEST_CHECK(output.status_c == 0U);
    TI_C54X_TEST_CHECK(!output.status_c_known);

    status.st0 = (uint16_t)(status.st0 & (uint16_t)~TI_C54X_ST0_TC_MASK);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_TC, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_tc == 0U);
    TI_C54X_TEST_CHECK(output.status_tc_known);

    /* Unknown raw TC is canonicalized to zero while knownness is preserved. */
    status.st0 = (uint16_t)(status.st0 | TI_C54X_ST0_TC_MASK);
    status.st0_known_mask = (uint16_t)(status.st0_known_mask &
                                       (uint16_t)~TI_C54X_ST0_TC_MASK);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_TC, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_tc == 0U);
    TI_C54X_TEST_CHECK(!output.status_tc_known);

    /* Composite collection reads both requested resources atomically. */
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0xABCDE12345)) == TI_C54X_ACCUMULATOR_OK);
    status.st0_known_mask = (uint16_t)(status.st0_known_mask |
                                       TI_C54X_ST0_TC_MASK);
    output = sentinel();
    sources = sources_for(&accumulators, &status);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_TC,
                           &sources, &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.accumulator_a_raw40 ==
                       UINT64_C(0xABCDE12345));
    TI_C54X_TEST_CHECK(output.accumulator_a_known);
    TI_C54X_TEST_CHECK(output.status_tc == 1U);
    TI_C54X_TEST_CHECK(output.status_tc_known);

    /* An accumulator-only request does not require status state. */
    output = sentinel();
    sources = sources_for(&accumulators, NULL);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_ACCUMULATORS, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.accumulator_a_known);
    TI_C54X_TEST_CHECK(output.status_c == 0U);
    TI_C54X_TEST_CHECK(!output.status_c_known);
    TI_C54X_TEST_CHECK(output.status_tc == 0U);
    TI_C54X_TEST_CHECK(!output.status_tc_known);

    sources = sources_for(NULL, &status);
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_ACCUMULATORS, &sources, &output);
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_STATUS_TC, NULL, &output);
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_STATUS_TC, &missing_sources,
                    &output);
    output = sentinel();
    expect_rejected((ti_c54x_input_resource_mask)0x2000U, &sources, &output);

    /* Raw carriers stay canonical even when their knownness says unknown. */
    accumulators.a_raw40 = TI_C54X_U40_MASK + UINT64_C(1);
    accumulators.known_mask = 0U;
    sources = sources_for(&accumulators, &status);
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_ACCUMULATORS, &sources, &output);

    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_NONE, NULL, &output) ==
                       TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_tc == 0U);
    TI_C54X_TEST_CHECK(!output.status_tc_known);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_TC, &sources, NULL) !=
                       TI_C54X_INPUT_COLLECT_OK);
    return 0;
}
