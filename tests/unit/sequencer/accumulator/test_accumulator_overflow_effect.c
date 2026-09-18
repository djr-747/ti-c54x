/* Module contract
 * Owns: executable proof for accumulator destination overflow status deltas,
 * including A/B routing, sticky WRITE_1, recompute-to-clear WRITE_0,
 * PRESERVE knownness, and atomic malformed-effect rejection.
 * Does not own: arithmetic policy, opcode decode, or CPU lifecycle.
 */

#include "../../../../src/isa/ops/op_effect.h"
#include "../../../../src/sequencer/accumulator/effect_apply.h"
#include "../../../../src/state/accumulator/accumulator.h"
#include "../../../../src/state/register_layout.h"
#include "../../../../src/state/status/registers.h"
#include "../../../support/test_harness.h"

#include <string.h>

#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(ti_c54x_op_effect) == 56U,
               "outer op effect layout must remain 56 bytes on LP64");
#elif UINTPTR_MAX == UINT32_MAX
_Static_assert(sizeof(ti_c54x_op_effect) == 44U,
               "outer op effect layout must remain 44 bytes on ILP32");
#endif

static void seed(ti_c54x_accumulator_state *accumulators,
                 ti_c54x_status_register_state *status)
{
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(status) ==
                       TI_C54X_STATUS_REGISTER_OK);
}

static ti_c54x_op_effect effect(uint8_t destination,
                                ti_c54x_op_destination_overflow_action action)
{
    ti_c54x_op_effect output = {0};
    output.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    output.payload.accumulator_write.destination = destination;
    output.payload.accumulator_write.value_raw40 = UINT64_C(0x12345);
    output.payload.accumulator_write.status_delta.destination_overflow_action =
        action;
    return output;
}

int main(void)
{
    ti_c54x_accumulator_state accumulators;
    ti_c54x_status_register_state status;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_op_effect write_a;
    ti_c54x_op_effect write_b;

    seed(&accumulators, &status);
    write_a = effect(TI_C54X_ACCUMULATOR_A,
                     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &write_a, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    TI_C54X_TEST_CHECK((status.st0 & TI_C54X_ST0_OVA_MASK) != 0U);
    TI_C54X_TEST_CHECK((status.st0_known_mask & TI_C54X_ST0_OVA_MASK) != 0U);
    TI_C54X_TEST_CHECK((status.st0 & TI_C54X_ST0_OVB_MASK) == 0U);

    write_b = effect(TI_C54X_ACCUMULATOR_B,
                     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &write_b, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    TI_C54X_TEST_CHECK((status.st0 & TI_C54X_ST0_OVB_MASK) != 0U);
    TI_C54X_TEST_CHECK((status.st0 & TI_C54X_ST0_OVA_MASK) != 0U);

    write_a = effect(TI_C54X_ACCUMULATOR_A,
                     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_0);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &write_a, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    TI_C54X_TEST_CHECK((status.st0 & TI_C54X_ST0_OVA_MASK) == 0U);
    TI_C54X_TEST_CHECK((status.st0_known_mask & TI_C54X_ST0_OVA_MASK) != 0U);
    TI_C54X_TEST_CHECK((status.st0 & TI_C54X_ST0_OVB_MASK) != 0U);

    status.st0_known_mask = (uint16_t)(status.st0_known_mask &
                                       (uint16_t)~TI_C54X_ST0_OVA_MASK);
    before_status = status;
    write_a = effect(TI_C54X_ACCUMULATOR_A,
                     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &write_a, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    TI_C54X_TEST_CHECK((status.st0_known_mask & TI_C54X_ST0_OVA_MASK) == 0U);
    status.st0 = (uint16_t)(status.st0 | TI_C54X_ST0_OVA_MASK);
    status.st0_known_mask = (uint16_t)(status.st0_known_mask |
                                       TI_C54X_ST0_OVA_MASK);
    before_status = status;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &write_a, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    TI_C54X_TEST_CHECK((status.st0 & TI_C54X_ST0_OVA_MASK) != 0U);
    TI_C54X_TEST_CHECK((status.st0_known_mask & TI_C54X_ST0_OVA_MASK) != 0U);
    before_accumulators = accumulators;
    before_status = status;
    write_a.payload.accumulator_write.status_delta.destination_overflow_action =
        (ti_c54x_op_destination_overflow_action)-1;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &write_a, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &status, sizeof(status)) == 0);
    write_a.payload.accumulator_write.status_delta.destination_overflow_action =
        TI_C54X_OP_DESTINATION_OVERFLOW_ACTION_COUNT;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &write_a, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &status, sizeof(status)) == 0);
    return 0;
}
