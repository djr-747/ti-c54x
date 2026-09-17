/* Module contract
 * Owns: the private generic input-collection contract proof for CPU resource
 * masks, including canonical output and transactional rejection.
 * Does not own: collection implementation, opcode semantics, decode, CPU
 * lifecycle, sequencing, timing, memory, buses, or diagnostics.
 */

#include "../../../../src/cpu/input/collect.h"
#include "../../../../src/isa/catalog/opcode_catalog.h"
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
    ti_c54x_op_input_snapshot output;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&status) ==
                       TI_C54X_STATUS_REGISTER_OK);
    sources = sources_for(NULL, NULL);

    /* No resources reads no backing state and returns canonical zero/false. */
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_NONE, &sources, &output) ==
                       TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.accumulator_a_raw40 == 0U);
    TI_C54X_TEST_CHECK(output.accumulator_b_raw40 == 0U);
    TI_C54X_TEST_CHECK(!output.accumulator_a_known);
    TI_C54X_TEST_CHECK(!output.accumulator_b_known);
    TI_C54X_TEST_CHECK(output.status_c == 0U);
    TI_C54X_TEST_CHECK(!output.status_c_known);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0xABCDE12345)) == TI_C54X_ACCUMULATOR_OK);
    sources = sources_for(&accumulators, NULL);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_ACCUMULATORS, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.accumulator_a_raw40 == UINT64_C(0xABCDE12345));
    TI_C54X_TEST_CHECK(output.accumulator_a_known);
    TI_C54X_TEST_CHECK(output.accumulator_b_raw40 == 0U);
    TI_C54X_TEST_CHECK(!output.accumulator_b_known);
    TI_C54X_TEST_CHECK(output.status_c == 0U);
    TI_C54X_TEST_CHECK(!output.status_c_known);

    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    sources = sources_for(NULL, &status);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_C, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.accumulator_a_raw40 == 0U);
    TI_C54X_TEST_CHECK(output.accumulator_b_raw40 == 0U);
    TI_C54X_TEST_CHECK(!output.accumulator_a_known);
    TI_C54X_TEST_CHECK(!output.accumulator_b_known);
    TI_C54X_TEST_CHECK(output.status_c == 1U);
    TI_C54X_TEST_CHECK(output.status_c_known);

    sources = sources_for(&accumulators, &status);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           (ti_c54x_input_resource_mask)(
                               TI_C54X_INPUT_RESOURCES_ACCUMULATORS |
                               TI_C54X_INPUT_RESOURCES_STATUS_C),
                           &sources, &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.accumulator_a_known);
    TI_C54X_TEST_CHECK(output.accumulator_a_raw40 ==
                       UINT64_C(0xABCDE12345));
    TI_C54X_TEST_CHECK(output.status_c == 1U);
    TI_C54X_TEST_CHECK(output.status_c_known);

    /* Requested unknown A/B and C values remain explicit knownness data. */
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&status) ==
                       TI_C54X_STATUS_REGISTER_OK);
    status.st0_known_mask = (uint16_t)(status.st0_known_mask &
                                       (uint16_t)~TI_C54X_ST0_C_MASK);
    status.st0 = (uint16_t)(status.st0 | TI_C54X_ST0_C_MASK);
    sources = sources_for(&accumulators, &status);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           (ti_c54x_input_resource_mask)(
                               TI_C54X_INPUT_RESOURCES_ACCUMULATORS |
                               TI_C54X_INPUT_RESOURCES_STATUS_C),
                           &sources, &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.accumulator_a_raw40 == 0U);
    TI_C54X_TEST_CHECK(output.accumulator_b_raw40 == 0U);
    TI_C54X_TEST_CHECK(!output.accumulator_a_known);
    TI_C54X_TEST_CHECK(!output.accumulator_b_known);
    TI_C54X_TEST_CHECK(output.status_c == 0U);
    TI_C54X_TEST_CHECK(!output.status_c_known);

    sources = sources_for(NULL, NULL);
    output = sentinel();
    expect_rejected((ti_c54x_input_resource_mask)0x2000U, &sources, &output);
    output = sentinel();
    expect_rejected((ti_c54x_input_resource_mask)0x2000U, NULL, &output);

    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_ACCUMULATORS, &sources, &output);
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_STATUS_C, &sources, &output);
    output = sentinel();
    expect_rejected((ti_c54x_input_resource_mask)(
                        TI_C54X_INPUT_RESOURCES_ACCUMULATORS |
                        TI_C54X_INPUT_RESOURCES_STATUS_C),
                    &sources, &output);

    accumulators.a_raw40 = TI_C54X_U40_MASK + UINT64_C(1);
    accumulators.known_mask = UINT8_C(1);
    sources = sources_for(&accumulators, NULL);
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_ACCUMULATORS, &sources, &output);

    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_NONE, NULL, &output) ==
                       TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_NONE, &sources, NULL) !=
                       TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_ACCUMULATORS, NULL,
                           &output) != TI_C54X_INPUT_COLLECT_OK);
    return 0;
}
