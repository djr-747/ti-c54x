/* Module contract
 * Owns: structural validation and atomic accumulator-write effect tests.
 * Does not own: opcode semantics, CPU lifecycle, or timing.
 */

#include "../../../../src/isa/ops/op_effect.h"
#include "../../../../src/sequencer/accumulator/effect_apply.h"
#include "../../../../src/state/accumulator/accumulator.h"
#include "../../../../src/state/status/registers.h"
#include "../../../support/test_harness.h"

#include <string.h>

static void check_c(const ti_c54x_status_register_state *status,
                    uint8_t expected)
{
    const ti_c54x_status_register_field_result c =
        ti_c54x_status_register_read_c(status);
    TI_C54X_TEST_CHECK(c.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(c.raw_value == expected);
}

int main(void)
{
    ti_c54x_accumulator_state accumulators;
    ti_c54x_status_register_state status;
    ti_c54x_op_effect effect = {0};
    unsigned char before_a[sizeof(accumulators)];
    unsigned char before_s[sizeof(status)];

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&status) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &accumulators, TI_C54X_ACCUMULATOR_A, 4U) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &accumulators, TI_C54X_ACCUMULATOR_B, 9U) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_cpl(&status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);

    effect.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    effect.payload.accumulator_write.destination = TI_C54X_ACCUMULATOR_A;
    effect.payload.accumulator_write.value_raw40 = 9U;
    effect.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_WRITE_1;
    effect.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    {
        uint64_t value = 0U;
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                               &accumulators, TI_C54X_ACCUMULATOR_A,
                               &value) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK_U64(value, 9U);
        check_c(&status, 1U);
    }

    effect.payload.accumulator_write.destination = TI_C54X_ACCUMULATOR_B;
    effect.payload.accumulator_write.value_raw40 = 4U;
    effect.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_WRITE_0;
    effect.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    {
        uint64_t value = 0U;
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                               &accumulators, TI_C54X_ACCUMULATOR_B,
                               &value) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK_U64(value, 4U);
        check_c(&status, 0U);
        TI_C54X_TEST_CHECK(ti_c54x_status_register_read_cpl(&status).raw_value ==
                           1U);
    }

    /* CMPL-style writes preserve C, including the status knownness mask. */
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    {
        ti_c54x_status_register_state before_preserve = status;
        uint64_t value = 0U;
        effect.payload.accumulator_write.destination = TI_C54X_ACCUMULATOR_A;
        effect.payload.accumulator_write.value_raw40 = 12U;
        effect.payload.accumulator_write.status_delta.c_action =
            TI_C54X_OP_STATUS_BIT_PRESERVE;
        effect.payload.accumulator_write.status_delta.tc_action =
            TI_C54X_OP_STATUS_BIT_PRESERVE;
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                               &effect, &accumulators, &status) ==
                           TI_C54X_ACCUMULATOR_EFFECT_OK);
        TI_C54X_TEST_CHECK(memcmp(&before_preserve, &status,
                                  sizeof(status)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                               &accumulators, TI_C54X_ACCUMULATOR_A,
                               &value) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK_U64(value, 12U);
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                               &accumulators, TI_C54X_ACCUMULATOR_B,
                               &value) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK_U64(value, 4U);
    }

    /* Preservation also retains an unknown C field byte-for-byte. */
    {
        ti_c54x_status_register_state before_preserve;
        uint64_t value = 0U;
        status.st0_known_mask &= (uint16_t)~TI_C54X_ST0_C_MASK;
        before_preserve = status;
        effect.payload.accumulator_write.destination = TI_C54X_ACCUMULATOR_B;
        effect.payload.accumulator_write.value_raw40 = 13U;
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                               &effect, &accumulators, &status) ==
                           TI_C54X_ACCUMULATOR_EFFECT_OK);
        TI_C54X_TEST_CHECK(memcmp(&before_preserve, &status,
                                  sizeof(status)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                               &accumulators, TI_C54X_ACCUMULATOR_A,
                               &value) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK_U64(value, 12U);
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                               &accumulators, TI_C54X_ACCUMULATOR_B,
                               &value) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK_U64(value, 13U);
    }

    memcpy(before_a, &accumulators, sizeof(before_a));
    memcpy(before_s, &status, sizeof(before_s));
    effect.payload.accumulator_write.destination =
        TI_C54X_ACCUMULATOR_INVALID;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(before_a, &accumulators, sizeof(before_a)) == 0);
    TI_C54X_TEST_CHECK(memcmp(before_s, &status, sizeof(before_s)) == 0);

    effect.kind = TI_C54X_OP_EFFECT_NONE;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    effect.kind = TI_C54X_OP_EFFECT_REPEAT_START;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_UNSUPPORTED);

    effect.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    effect.payload.accumulator_write.destination = TI_C54X_ACCUMULATOR_A;
    effect.payload.accumulator_write.value_raw40 = 12U;
    effect.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_ACTION_COUNT;
    effect.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    memcpy(before_a, &accumulators, sizeof(before_a));
    memcpy(before_s, &status, sizeof(before_s));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(before_a, &accumulators, sizeof(before_a)) == 0);
    TI_C54X_TEST_CHECK(memcmp(before_s, &status, sizeof(before_s)) == 0);
    effect.payload.accumulator_write.status_delta.c_action =
        (ti_c54x_op_status_bit_action)99U;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    effect.payload.accumulator_write.status_delta.c_action =
        (ti_c54x_op_status_bit_action)-1;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    effect.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    effect.payload.accumulator_write.status_delta.tc_action =
        (ti_c54x_op_status_bit_action)-1;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    effect.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_WRITE_0;
    effect.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    effect.payload.accumulator_write.value_raw40 =
        UINT64_C(0x10000000000);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(before_a, &accumulators, sizeof(before_a)) == 0);
    TI_C54X_TEST_CHECK(memcmp(before_s, &status, sizeof(before_s)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           NULL, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, NULL, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, NULL) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT);
    return 0;
}
