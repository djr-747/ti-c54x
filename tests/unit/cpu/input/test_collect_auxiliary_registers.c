/* Module contract
 * Owns: the generic AUXILIARY_REGISTERS input-resource proofs, including
 * copied AR values, explicit knownness, selective collection, and
 * transactional rejection.
 * Does not own: collection implementation, architectural-state storage,
 * opcode semantics, decode, CPU lifecycle, sequencing, timing, memory, buses,
 * or diagnostics.
 */

#include "../../../../src/cpu/input/collect.h"
#include "../../../../src/isa/ops/op_input.h"
#include "../../../../src/state/accumulator/accumulator.h"
#include "../../../../src/state/arch_state.h"
#include "../../../../src/state/register_layout.h"
#include "../../../../src/state/status/registers.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_input_sources sources_for(
    const ti_c54x_accumulator_state *accumulators,
    const ti_c54x_status_register_state *status,
    const ti_c54x_arch_state *arch_state)
{
    return (ti_c54x_input_sources){
        .accumulators = accumulators,
        .status = status,
        .arch_state = arch_state,
    };
}

static ti_c54x_op_input_snapshot sentinel(void)
{
    ti_c54x_op_input_snapshot output = {0};
    uint8_t index;

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
    output.auxiliary_registers_known_mask = UINT8_C(0xFF);
    for (index = 0U; index < UINT8_C(8); ++index) {
        output.auxiliary_registers_raw16[index] =
            (uint16_t)(UINT16_C(0xA000) + index);
    }
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

static void expect_auxiliary_zero_and_unknown(
    const ti_c54x_op_input_snapshot *output)
{
    uint8_t index;

    TI_C54X_TEST_CHECK(output->auxiliary_registers_known_mask == 0U);
    for (index = 0U; index < UINT8_C(8); ++index) {
        TI_C54X_TEST_CHECK(output->auxiliary_registers_raw16[index] == 0U);
    }
}

int main(void)
{
    ti_c54x_accumulator_state accumulators;
    ti_c54x_arch_state arch_state;
    ti_c54x_status_register_state status;
    ti_c54x_input_sources sources;
    ti_c54x_op_input_snapshot output;
    uint8_t index;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&status) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&arch_state) ==
                       TI_C54X_ARCH_STATE_OK);

    /* Every known AR is copied exactly and all eight knownness bits are set. */
    for (index = 0U; index < UINT8_C(8); ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                               &arch_state, index,
                               (uint16_t)(UINT16_C(0x1000) + index)) ==
                           TI_C54X_ARCH_STATE_OK);
    }
    sources = sources_for(NULL, NULL, &arch_state);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS,
                           &sources, &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.auxiliary_registers_known_mask ==
                       UINT8_C(0xFF));
    for (index = 0U; index < UINT8_C(8); ++index) {
        TI_C54X_TEST_CHECK(output.auxiliary_registers_raw16[index] ==
                           (uint16_t)(UINT16_C(0x1000) + index));
    }
    TI_C54X_TEST_CHECK(!output.accumulator_a_known);
    TI_C54X_TEST_CHECK(!output.accumulator_b_known);
    TI_C54X_TEST_CHECK(!output.status_c_known);
    TI_C54X_TEST_CHECK(!output.status_tc_known);
    TI_C54X_TEST_CHECK(!output.status_sxm_known);

    /* Reset leaves AR0-AR7 physically zero but architecturally unknown. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&arch_state) ==
                       TI_C54X_ARCH_STATE_OK);
    output = sentinel();
    sources = sources_for(NULL, NULL, &arch_state);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS,
                           &sources, &output) ==
                       TI_C54X_INPUT_COLLECT_OK);
    expect_auxiliary_zero_and_unknown(&output);

    /* Unknown raw storage is canonicalized while known ARs remain exact. */
    arch_state.ar[UINT8_C(3)] = UINT16_C(0xD00D);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &arch_state, UINT8_C(1), UINT16_C(0xBEEF)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &arch_state, UINT8_C(6), UINT16_C(0x1234)) ==
                       TI_C54X_ARCH_STATE_OK);
    output = sentinel();
    sources = sources_for(NULL, NULL, &arch_state);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS,
                           &sources, &output) ==
                       TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.auxiliary_registers_known_mask ==
                       (uint8_t)(UINT8_C(1) << 1 | UINT8_C(1) << 6));
    for (index = 0U; index < UINT8_C(8); ++index) {
        const uint16_t expected =
            index == UINT8_C(1) ? UINT16_C(0xBEEF) :
            index == UINT8_C(6) ? UINT16_C(0x1234) : 0U;
        TI_C54X_TEST_CHECK(output.auxiliary_registers_raw16[index] == expected);
    }

    /* AUXILIARY_REGISTERS composes with the existing resources. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&arch_state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &arch_state, UINT8_C(0), UINT16_C(0xCAFE)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0xABCDE12345)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0123456789)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(&status, 0U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    sources = sources_for(&accumulators, &status, &arch_state);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           (ti_c54x_input_resource_mask)(
                               TI_C54X_INPUT_RESOURCES_ACCUMULATORS |
                               TI_C54X_INPUT_RESOURCES_STATUS_C |
                               TI_C54X_INPUT_RESOURCES_STATUS_TC |
                               TI_C54X_INPUT_RESOURCES_STATUS_SXM |
                               TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS),
                           &sources, &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.accumulator_a_raw40 ==
                       UINT64_C(0xABCDE12345));
    TI_C54X_TEST_CHECK(output.accumulator_b_raw40 == UINT64_C(0x0123456789));
    TI_C54X_TEST_CHECK(output.accumulator_a_known);
    TI_C54X_TEST_CHECK(output.accumulator_b_known);
    TI_C54X_TEST_CHECK(output.status_c == 1U);
    TI_C54X_TEST_CHECK(output.status_c_known);
    TI_C54X_TEST_CHECK(output.status_tc == 0U);
    TI_C54X_TEST_CHECK(output.status_tc_known);
    TI_C54X_TEST_CHECK(output.status_sxm == 1U);
    TI_C54X_TEST_CHECK(output.status_sxm_known);
    TI_C54X_TEST_CHECK(output.auxiliary_registers_raw16[0] == UINT16_C(0xCAFE));
    TI_C54X_TEST_CHECK(output.auxiliary_registers_known_mask == UINT8_C(1));

    /* Missing AUX source, invalid masks, and null output reject transactionally. */
    output = sentinel();
    sources = sources_for(&accumulators, &status, NULL);
    expect_rejected((ti_c54x_input_resource_mask)(
                        TI_C54X_INPUT_RESOURCES_ACCUMULATORS |
                        TI_C54X_INPUT_RESOURCES_STATUS_C |
                        TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS),
                    &sources, &output);
    output = sentinel();
    sources = sources_for(&accumulators, &status, &arch_state);
    expect_rejected((ti_c54x_input_resource_mask)UINT16_C(0x8000), &sources,
                    &output);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS,
                           &sources, NULL) != TI_C54X_INPUT_COLLECT_OK);

    /* A missing peer source cannot publish a partially collected snapshot. */
    output = sentinel();
    sources = sources_for(NULL, NULL, &arch_state);
    expect_rejected((ti_c54x_input_resource_mask)(
                        TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS |
                        TI_C54X_INPUT_RESOURCES_STATUS_C),
                    &sources, &output);
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS, NULL,
                    &output);
    return 0;
}
