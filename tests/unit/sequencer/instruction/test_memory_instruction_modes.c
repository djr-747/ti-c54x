/*
 * Module contract
 *
 * Owns: W42A root-transaction proofs for standard single-indirect MOD0--MOD3
 * lifecycle and publication semantics. Does not own: address planning,
 * opcode dispatch, endpoint implementation, or CPU orchestration.
 */

#include "test_memory_instruction_fixture.h"

#include <string.h>

static ti_c54x_op_effect memory_effect_mode(
    ti_c54x_bus_word16_request_access access,
    ti_c54x_indirect_single_mode mode)
{
    ti_c54x_op_effect effect = memory_effect(access);

    effect.payload.memory.address_spec.mode = mode;
    return effect;
}

static void reset_effect_state(ti_c54x_accumulator_state *accumulators,
                               ti_c54x_t_register_state *t_register)
{
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(t_register) ==
                       TI_C54X_T_REGISTER_OK);
}

static void test_mod0_ready_retires_without_ar_update(void)
{
    const ti_c54x_op_effect effect = memory_effect_mode(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, TI_C54X_INDIRECT_SINGLE_MOD0);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xbeef)};
    ti_c54x_memory_instruction_result result;
    uint64_t accumulator_value = 0;

    reset_effect_state(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(state.ar[2], UINT16_C(0x2110));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x21));
    TI_C54X_TEST_CHECK_U64(script.request.address_word, UINT16_C(0x2110));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           &accumulator_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(accumulator_value, UINT16_C(0xbeef));
}

static void test_mod1_wait_then_ready_commits_postdecrement(void)
{
    const ti_c54x_op_effect effect = memory_effect_mode(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, TI_C54X_INDIRECT_SINGLE_MOD1);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0x1234)};
    ti_c54x_memory_instruction_result result;

    reset_effect_state(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK_U64(script.request.address_word, UINT16_C(0x2110));
    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(state.ar[2], UINT16_C(0x210f));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x21));
}

static void test_mod3_write_wait_requests_preincrement_without_exposing_it(void)
{
    const ti_c54x_op_effect effect = memory_effect_mode(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, TI_C54X_INDIRECT_SINGLE_MOD3);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    endpoint_script script = {SCRIPT_WAIT_THEN_WRITE_READY, 0, 0, 0, 0,
                              {0}, 0};
    ti_c54x_memory_instruction_result result;

    reset_effect_state(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK_U64(script.request.address_word, UINT16_C(0x2111));
    TI_C54X_TEST_CHECK_U64(state.ar[2], UINT16_C(0x2110));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x20));
    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(state.ar[2], UINT16_C(0x2111));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x21));
}

static void test_fault_and_cancel_preserve_update_mode_architecture(void)
{
    const ti_c54x_op_effect effect = memory_effect_mode(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, TI_C54X_INDIRECT_SINGLE_MOD1);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    endpoint_script fault = {SCRIPT_FAULT, 0, 0, 0, 0, {0}, 0};
    endpoint_script wait = {SCRIPT_WAIT_THEN_WRITE_READY, 0, 0, 0, 0,
                            {0}, 0};
    ti_c54x_memory_instruction_result result;

    reset_effect_state(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance(&transaction, &fault, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = known_arch_state();
    before = state;
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance(&transaction, &wait, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &(ti_c54x_bus_word16_endpoint){
                               .context = &wait,
                               .advance = script_advance,
                               .cancel = script_cancel}) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void test_mod3_read_rejects_before_endpoint_issue(void)
{
    const ti_c54x_op_effect effect = memory_effect_mode(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, TI_C54X_INDIRECT_SINGLE_MOD3);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, 0};

    reset_effect_state(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);
}

int main(void)
{
    test_mod0_ready_retires_without_ar_update();
    test_mod1_wait_then_ready_commits_postdecrement();
    test_mod3_write_wait_requests_preincrement_without_exposing_it();
    test_fault_and_cancel_preserve_update_mode_architecture();
    test_mod3_read_rejects_before_endpoint_issue();
    return 0;
}
