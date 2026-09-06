/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT_ONLY proof of the root tagged effect transaction.
 * It verifies route-specific candidate staging, leaf validation boundaries,
 * atomic commit, and failure non-mutation.
 * Does not own: opcode semantics, decode, CPU lifecycle, PC effects, timing,
 * memory, repeat/interrupt policy, diagnostics, or public persistence ABI.
 */

#include "../../../src/isa/ops/op_effect.h"
#include "../../../src/sequencer/effect_transaction.h"
#include "../../../src/state/accumulator/accumulator.h"
#include "../../../src/state/status/registers.h"
#include "../../../src/state/temporary/t_register.h"
#include "../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_status_register_state reset_status(void)
{
    ti_c54x_status_register_state state;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    return state;
}

static ti_c54x_accumulator_state reset_accumulators(void)
{
    ti_c54x_accumulator_state state;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&state) ==
                       TI_C54X_ACCUMULATOR_OK);
    return state;
}

static ti_c54x_t_register_state known_t(uint16_t raw_value)
{
    ti_c54x_t_register_state state = {raw_value, true};

    return state;
}

static ti_c54x_op_effect status_effect(uint16_t value)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE;
    effect.payload.status_field_write.target =
        TI_C54X_OP_STATUS_FIELD_TARGET_DP;
    effect.payload.status_field_write.raw_value = value;
    return effect;
}

static ti_c54x_op_effect accumulator_effect(uint64_t value)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    effect.payload.accumulator_write.destination = TI_C54X_ACCUMULATOR_A;
    effect.payload.accumulator_write.value_raw40 = value;
    effect.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_WRITE_1;
    effect.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_WRITE_0;
    return effect;
}

static ti_c54x_op_effect t_register_effect(uint16_t value)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_T_REGISTER_WRITE;
    effect.payload.t_register_write.raw_value = value;
    return effect;
}

static void check_status_dp(const ti_c54x_status_register_state *state,
                            uint16_t expected)
{
    const ti_c54x_status_register_field_result field =
        ti_c54x_status_register_read_dp(state);

    TI_C54X_TEST_CHECK(field.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(field.known);
    TI_C54X_TEST_CHECK(field.raw_value == expected);
}

static void test_none_route_accepts_null_live_state(void)
{
    const ti_c54x_op_effect none = {0};
    ti_c54x_effect_transaction transaction;

    memset(&transaction, 0xa5, sizeof(transaction));
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_stage(
                           &none, NULL, NULL, NULL, &transaction) ==
                       TI_C54X_EFFECT_TRANSACTION_OK);
    TI_C54X_TEST_CHECK(transaction.target ==
                       TI_C54X_EFFECT_TRANSACTION_NONE);
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_commit(
                           &transaction, NULL, NULL, NULL) ==
                       TI_C54X_EFFECT_TRANSACTION_OK);
}

static void test_status_route_snapshots_and_commits_only_status(void)
{
    ti_c54x_status_register_state status = reset_status();
    const ti_c54x_accumulator_state before_accumulators = reset_accumulators();
    const ti_c54x_t_register_state before_t = known_t(UINT16_C(0x2468));
    ti_c54x_accumulator_state accumulators = before_accumulators;
    ti_c54x_t_register_state t_register = before_t;
    const ti_c54x_op_effect effect = status_effect(UINT16_C(0x01a5));
    ti_c54x_effect_transaction transaction;

    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_stage(
                           &effect, &status, NULL, NULL, &transaction) ==
                       TI_C54X_EFFECT_TRANSACTION_OK);
    TI_C54X_TEST_CHECK(transaction.target ==
                       TI_C54X_EFFECT_TRANSACTION_STATUS_FIELD);

    /* The candidate is independent of the live source after staging. */
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_dp(
                           &status, UINT16_C(0x0033)) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_commit(
                           &transaction, &status, &accumulators, &t_register) ==
                       TI_C54X_EFFECT_TRANSACTION_OK);
    check_status_dp(&status, UINT16_C(0x01a5));
    TI_C54X_TEST_CHECK(memcmp(&accumulators, &before_accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&t_register, &before_t, sizeof(t_register)) == 0);
}

static void test_accumulator_route_snapshots_and_commits_only_route_state(void)
{
    ti_c54x_accumulator_state accumulators = reset_accumulators();
    ti_c54x_status_register_state status = reset_status();
    const ti_c54x_accumulator_state before_accumulators = accumulators;
    const ti_c54x_status_register_state before_status = status;
    const ti_c54x_t_register_state before_t = known_t(UINT16_C(0x1357));
    ti_c54x_t_register_state t_register = before_t;
    const ti_c54x_op_effect effect = accumulator_effect(UINT64_C(0x12345));
    ti_c54x_effect_transaction transaction;

    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_stage(
                           &effect, &status, &accumulators, NULL,
                           &transaction) == TI_C54X_EFFECT_TRANSACTION_OK);
    TI_C54X_TEST_CHECK(transaction.target ==
                       TI_C54X_EFFECT_TRANSACTION_ACCUMULATOR);

    /* Staging owns both accumulator and status candidates, not live aliases. */
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x777)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&status, 0U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_commit(
                           &transaction, &status, &accumulators, &t_register) ==
                       TI_C54X_EFFECT_TRANSACTION_OK);

    {
        uint64_t value = 0U;
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                               &accumulators, TI_C54X_ACCUMULATOR_A,
                               &value) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK_U64(value, UINT64_C(0x12345));
    }
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_c(&status).raw_value ==
                       UINT16_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_tc(&status).raw_value ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_cpl(&status).raw_value ==
                       ti_c54x_status_register_read_cpl(&before_status).raw_value);
    TI_C54X_TEST_CHECK(memcmp(&t_register, &before_t, sizeof(t_register)) == 0);
    TI_C54X_TEST_CHECK(before_accumulators.b_raw40 ==
                       accumulators.b_raw40);
}

static void test_t_route_snapshots_and_commits_only_t(void)
{
    ti_c54x_status_register_state status = reset_status();
    ti_c54x_accumulator_state accumulators = reset_accumulators();
    const ti_c54x_status_register_state before_status = status;
    const ti_c54x_accumulator_state before_accumulators = accumulators;
    ti_c54x_t_register_state t_register = known_t(UINT16_C(0x1111));
    const ti_c54x_op_effect effect = t_register_effect(UINT16_C(0xbeef));
    ti_c54x_effect_transaction transaction;

    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_stage(
                           &effect, &status, NULL, &t_register, &transaction) ==
                       TI_C54X_EFFECT_TRANSACTION_OK);
    TI_C54X_TEST_CHECK(transaction.target ==
                       TI_C54X_EFFECT_TRANSACTION_T_REGISTER);
    t_register.raw_value = UINT16_C(0x2222);
    t_register.known = true;
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_commit(
                           &transaction, &status, &accumulators, &t_register) ==
                       TI_C54X_EFFECT_TRANSACTION_OK);
    TI_C54X_TEST_CHECK(t_register.raw_value == UINT16_C(0xbeef));
    TI_C54X_TEST_CHECK(t_register.known);
    TI_C54X_TEST_CHECK(memcmp(&status, &before_status, sizeof(status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&accumulators, &before_accumulators,
                              sizeof(accumulators)) == 0);
}

static void test_stage_failures_preserve_transaction_byte_for_byte(void)
{
    ti_c54x_status_register_state status = reset_status();
    ti_c54x_effect_transaction transaction;
    ti_c54x_effect_transaction before;
    ti_c54x_op_effect forged = {0};
    const ti_c54x_op_effect repeat = {
        TI_C54X_OP_EFFECT_REPEAT_START, {.repeat_start = {UINT16_C(2)}}};

    memset(&transaction, 0x5a, sizeof(transaction));
    before = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_stage(
                           NULL, &status, NULL, NULL, &transaction) ==
                       TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &before, sizeof(transaction)) == 0);

    forged.kind = (ti_c54x_op_effect_kind)UINT8_C(0x7f);
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_stage(
                           &forged, &status, NULL, NULL, &transaction) ==
                       TI_C54X_EFFECT_TRANSACTION_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &before, sizeof(transaction)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_stage(
                           &repeat, &status, NULL, NULL, &transaction) ==
                       TI_C54X_EFFECT_TRANSACTION_UNSUPPORTED);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &before, sizeof(transaction)) == 0);

    {
        const ti_c54x_op_effect status_write = status_effect(1U);
        TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_stage(
                               &status_write, NULL, NULL, NULL, &transaction) ==
                           TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(memcmp(&transaction, &before, sizeof(transaction)) ==
                           0);
    }
}

static void test_commit_prevalidates_and_never_partially_mutates(void)
{
    ti_c54x_status_register_state status = reset_status();
    ti_c54x_accumulator_state accumulators = reset_accumulators();
    ti_c54x_t_register_state t_register = known_t(UINT16_C(0x4444));
    ti_c54x_effect_transaction transaction;
    ti_c54x_effect_transaction before_transaction;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_t_register_state before_t;
    const ti_c54x_op_effect status_write = status_effect(UINT16_C(0x0042));
    const ti_c54x_op_effect accumulator_write = accumulator_effect(0x88U);
    const ti_c54x_op_effect t_write = t_register_effect(UINT16_C(0xaaaa));

    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_stage(
                           &status_write, &status, NULL, NULL, &transaction) ==
                       TI_C54X_EFFECT_TRANSACTION_OK);
    before_transaction = transaction;
    before_status = status;
    before_accumulators = accumulators;
    before_t = t_register;
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_commit(
                           &transaction, NULL, &accumulators, &t_register) ==
                       TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &before_transaction,
                              sizeof(transaction)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&status, &before_status, sizeof(status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&accumulators, &before_accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&t_register, &before_t, sizeof(t_register)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_stage(
                           &accumulator_write, &status, &accumulators, NULL,
                           &transaction) == TI_C54X_EFFECT_TRANSACTION_OK);
    before_transaction = transaction;
    before_status = status;
    before_accumulators = accumulators;
    before_t = t_register;
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_commit(
                           &transaction, &status, NULL, &t_register) ==
                       TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &before_transaction,
                              sizeof(transaction)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&status, &before_status, sizeof(status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&accumulators, &before_accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&t_register, &before_t, sizeof(t_register)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_stage(
                           &t_write, &status, NULL, &t_register, &transaction) ==
                       TI_C54X_EFFECT_TRANSACTION_OK);
    before_transaction = transaction;
    before_status = status;
    before_accumulators = accumulators;
    before_t = t_register;
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_commit(
                           &transaction, &status, &accumulators, NULL) ==
                       TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &before_transaction,
                              sizeof(transaction)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&status, &before_status, sizeof(status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&accumulators, &before_accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&t_register, &before_t, sizeof(t_register)) == 0);

    transaction.target = (ti_c54x_effect_transaction_target)UINT8_C(0x7f);
    before_transaction = transaction;
    before_status = status;
    before_accumulators = accumulators;
    before_t = t_register;
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_commit(
                           &transaction, &status, &accumulators, &t_register) ==
                       TI_C54X_EFFECT_TRANSACTION_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &before_transaction,
                              sizeof(transaction)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&status, &before_status, sizeof(status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&accumulators, &before_accumulators,
                              sizeof(accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&t_register, &before_t, sizeof(t_register)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_effect_transaction_commit(
                           NULL, &status, &accumulators, &t_register) ==
                       TI_C54X_EFFECT_TRANSACTION_INVALID_ARGUMENT);
}

int main(void)
{
    test_none_route_accepts_null_live_state();
    test_status_route_snapshots_and_commits_only_status();
    test_accumulator_route_snapshots_and_commits_only_route_state();
    test_t_route_snapshots_and_commits_only_t();
    test_stage_failures_preserve_transaction_byte_for_byte();
    test_commit_prevalidates_and_never_partially_mutates();
    return 0;
}
