/* Module contract
 * Owns: the minimal C+TC status-delta contract at an accumulator-write
 * effect boundary, including exhaustive action combinations and atomic
 * rejection checks.
 * Does not own: opcode semantics, decode, CPU lifecycle, or timing.
 */

#include "../../../../src/isa/ops/op_effect.h"
#include "../../../../src/sequencer/accumulator/effect_apply.h"
#include "../../../../src/state/accumulator/accumulator.h"
#include "../../../../src/state/status/registers.h"
#include "../../../support/test_harness.h"

#include <string.h>

static uint16_t status_bit_value(const ti_c54x_status_register_state *status,
                                 uint16_t mask)
{
    return (uint16_t)((status->st0 & mask) != 0U);
}

static bool status_bit_known(const ti_c54x_status_register_state *status,
                             uint16_t mask)
{
    return (status->st0_known_mask & mask) != 0U;
}

static void set_status_bit(ti_c54x_status_register_state *status,
                           uint16_t mask,
                           uint8_t value,
                           bool known)
{
    if (value != 0U) {
        status->st0 = (uint16_t)(status->st0 | mask);
    } else {
        status->st0 = (uint16_t)(status->st0 & (uint16_t)~mask);
    }
    if (known) {
        status->st0_known_mask = (uint16_t)(status->st0_known_mask | mask);
    } else {
        status->st0_known_mask =
            (uint16_t)(status->st0_known_mask & (uint16_t)~mask);
    }
}

static void apply_expected_action(ti_c54x_status_register_state *status,
                                  uint16_t mask,
                                  ti_c54x_op_status_bit_action action)
{
    if (action == TI_C54X_OP_STATUS_BIT_WRITE_0) {
        set_status_bit(status, mask, 0U, true);
    } else if (action == TI_C54X_OP_STATUS_BIT_WRITE_1) {
        set_status_bit(status, mask, 1U, true);
    }
}

static void seed_state(ti_c54x_accumulator_state *accumulators,
                       ti_c54x_status_register_state *status)
{
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(status) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           accumulators, TI_C54X_ACCUMULATOR_A, UINT64_C(4)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           accumulators, TI_C54X_ACCUMULATOR_B, UINT64_C(9)) ==
                       TI_C54X_ACCUMULATOR_OK);
    /* Deliberately contrast C=0 and TC=1 for PRESERVE assertions. */
    set_status_bit(status, TI_C54X_ST0_C_MASK, 0U, true);
    set_status_bit(status, TI_C54X_ST0_TC_MASK, 1U, true);
}

static void expect_action(const ti_c54x_op_status_bit_action action,
                          uint16_t original_value,
                          bool original_known,
                          uint16_t mask,
                          const ti_c54x_status_register_state *status)
{
    switch (action) {
    case TI_C54X_OP_STATUS_BIT_PRESERVE:
        TI_C54X_TEST_CHECK(status_bit_value(status, mask) == original_value);
        TI_C54X_TEST_CHECK(status_bit_known(status, mask) == original_known);
        break;
    case TI_C54X_OP_STATUS_BIT_WRITE_0:
        TI_C54X_TEST_CHECK(status_bit_value(status, mask) == 0U);
        TI_C54X_TEST_CHECK(status_bit_known(status, mask));
        break;
    case TI_C54X_OP_STATUS_BIT_WRITE_1:
        TI_C54X_TEST_CHECK(status_bit_value(status, mask) == 1U);
        TI_C54X_TEST_CHECK(status_bit_known(status, mask));
        break;
    default:
        TI_C54X_TEST_CHECK(false);
    }
}

static void check_all_action_combinations(void)
{
    const ti_c54x_op_status_bit_action actions[] = {
        TI_C54X_OP_STATUS_BIT_PRESERVE,
        TI_C54X_OP_STATUS_BIT_WRITE_0,
        TI_C54X_OP_STATUS_BIT_WRITE_1,
    };
    size_t c_index;
    size_t tc_index;

    for (c_index = 0U; c_index < sizeof(actions) / sizeof(actions[0]);
         ++c_index) {
        for (tc_index = 0U; tc_index < sizeof(actions) / sizeof(actions[0]);
             ++tc_index) {
            ti_c54x_accumulator_state accumulators;
            ti_c54x_status_register_state status;
            ti_c54x_status_register_state expected_status;
            ti_c54x_op_effect effect = {0};
            uint64_t value = 0U;

            seed_state(&accumulators, &status);
            effect.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
            effect.payload.accumulator_write.destination =
                TI_C54X_ACCUMULATOR_A;
            effect.payload.accumulator_write.value_raw40 = UINT64_C(0x12345);
            effect.payload.accumulator_write.status_delta.c_action =
                actions[c_index];
            effect.payload.accumulator_write.status_delta.tc_action =
                actions[tc_index];
            expected_status = status;
            apply_expected_action(&expected_status, TI_C54X_ST0_C_MASK,
                                  actions[c_index]);
            apply_expected_action(&expected_status, TI_C54X_ST0_TC_MASK,
                                  actions[tc_index]);

            TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                                   &effect, &accumulators, &status) ==
                               TI_C54X_ACCUMULATOR_EFFECT_OK);
            TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                                   &accumulators, TI_C54X_ACCUMULATOR_A,
                                   &value) == TI_C54X_ACCUMULATOR_OK);
            TI_C54X_TEST_CHECK_U64(value, UINT64_C(0x12345));
            expect_action(actions[c_index], 0U, true, TI_C54X_ST0_C_MASK,
                          &status);
            expect_action(actions[tc_index], 1U, true, TI_C54X_ST0_TC_MASK,
                          &status);
            TI_C54X_TEST_CHECK(memcmp(&expected_status, &status,
                                      sizeof(status)) == 0);
        }
    }
}

static void check_preserve_keeps_unknown_bytewise(void)
{
    ti_c54x_accumulator_state accumulators;
    ti_c54x_status_register_state status;
    ti_c54x_status_register_state before_status;
    ti_c54x_op_effect effect = {0};

    seed_state(&accumulators, &status);
    set_status_bit(&status, TI_C54X_ST0_C_MASK, 1U, false);
    set_status_bit(&status, TI_C54X_ST0_TC_MASK, 0U, false);
    before_status = status;
    effect.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    effect.payload.accumulator_write.destination = TI_C54X_ACCUMULATOR_B;
    effect.payload.accumulator_write.value_raw40 = UINT64_C(0x23456);
    effect.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    effect.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &status, sizeof(status)) == 0);
}

static void check_invalid_effects_are_atomic(void)
{
    ti_c54x_accumulator_state accumulators;
    ti_c54x_status_register_state status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_status_register_state before_status;
    ti_c54x_op_effect effect = {0};

    seed_state(&accumulators, &status);
    effect.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    effect.payload.accumulator_write.destination = TI_C54X_ACCUMULATOR_A;
    effect.payload.accumulator_write.value_raw40 = UINT64_C(0x34567);
    effect.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_WRITE_1;
    effect.payload.accumulator_write.status_delta.tc_action =
        (ti_c54x_op_status_bit_action)-1;
    before_accumulators = accumulators;
    before_status = status;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &status, sizeof(status)) == 0);

    effect.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_ACTION_COUNT;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &status, sizeof(status)) == 0);

    effect.payload.accumulator_write.status_delta.c_action =
        (ti_c54x_op_status_bit_action)-1;
    effect.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &status, sizeof(status)) == 0);

    effect.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_ACTION_COUNT;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &status, sizeof(status)) == 0);

    /* A rejected destination must not partially apply the accumulator or C. */
    effect.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_WRITE_1;
    effect.payload.accumulator_write.destination =
        TI_C54X_ACCUMULATOR_INVALID;
    effect.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_WRITE_0;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &status, sizeof(status)) == 0);

    effect.payload.accumulator_write.destination = TI_C54X_ACCUMULATOR_A;
    effect.payload.accumulator_write.value_raw40 = UINT64_C(0x10000000000);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &status, sizeof(status)) == 0);
}

static void check_unsupported_and_null(void)
{
    ti_c54x_accumulator_state accumulators;
    ti_c54x_status_register_state status;
    ti_c54x_op_effect effect = {0};

    seed_state(&accumulators, &status);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           NULL, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, NULL, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, NULL) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT);
    effect.kind = TI_C54X_OP_EFFECT_NONE;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    effect.kind = TI_C54X_OP_EFFECT_REPEAT_START;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_UNSUPPORTED);
}

int main(void)
{
    check_all_action_combinations();
    check_preserve_keeps_unknown_bytewise();
    check_invalid_effects_are_atomic();
    check_unsupported_and_null();
    return 0;
}
