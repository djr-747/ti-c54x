/*
 * Module contract
 *
 * Owns: lifecycle-lane proof of preflight, identity, reset/cancel, and faults.
 * Does not own: commit arithmetic, CPU orchestration, or endpoint behavior.
 */

#include "test_memory_instruction_fixture.h"

#include <string.h>

static void test_absent_endpoint_faults_before_id_allocation(void)
{
    const ti_c54x_op_effect effect =
        memory_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    const ti_c54x_bus_word16_endpoint absent_endpoint = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_result result = {0};
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0x1234)};
    const ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &absent_endpoint, &state,
                           &accumulators, &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_NO_CALLBACK);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == UINT64_C(0));
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    result = (ti_c54x_memory_instruction_result){0};
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(script.begin_calls == 1U);
    TI_C54X_TEST_CHECK(script.id == UINT64_C(1));
}

static void test_preflight_rejects_forged_candidates_before_begin(void)
{
    const ti_c54x_op_effect effect =
        memory_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, 0};
    ti_c54x_arch_state before = state;
    const ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 4, 2) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.candidate.effect.kind = (ti_c54x_op_effect_kind)99;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, NULL) !=
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 4, 2) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.candidate.address_resolution.address_result.status =
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, NULL) !=
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 4, 2) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.candidate.pc_effect.public_pc_word_after = UINT32_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, NULL) !=
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, UINT64_MAX,
                               UINT64_MAX) !=
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction = (ti_c54x_memory_instruction_transaction){0};
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, 0};
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.last_transaction_id = UINT64_MAX;
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, NULL) !=
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == UINT64_MAX);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void test_phase_and_reducer_pair_mismatch_rejected(void)
{
    const ti_c54x_op_effect effect =
        memory_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, 0};
    ti_c54x_memory_instruction_result result = {0};
    const ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, 0};
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    transaction.phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    result = (ti_c54x_memory_instruction_result){0};
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.resume_calls == 0U);
}

static void test_reset_and_cancel_require_canonical_phase_reducer_pairs(void)
{
    const ti_c54x_op_effect effect =
        memory_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    endpoint_script wait_script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                                   {0}, UINT16_C(0x1234)};
    endpoint_script fault_script = {SCRIPT_FAULT, 0, 0, 0, 0, {0}, 0};
    const ti_c54x_bus_word16_endpoint wait_endpoint = endpoint_for(&wait_script);
    ti_c54x_memory_instruction_result result;
    uint64_t issued_id;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    transaction.last_transaction_id = UINT64_C(9);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == UINT64_C(9));
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == UINT64_C(9));

    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &wait_endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT);
    transaction.phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    transaction.bus_transaction.state =
        TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT;
    transaction.bus_transaction.transaction_id = UINT64_C(10);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &wait_endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance(&transaction, &wait_script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    issued_id = transaction.last_transaction_id;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &wait_endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == issued_id);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance(&transaction, &fault_script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    issued_id = transaction.last_transaction_id;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == issued_id);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance(&transaction, &wait_script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    result = advance(&transaction, &wait_script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    issued_id = transaction.last_transaction_id;
    transaction.phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == issued_id);
}

static void test_fault_cancel_and_malformed_observation_do_not_commit(void)
{
    const ti_c54x_op_effect effect =
        memory_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    endpoint_script fault = {SCRIPT_FAULT, 0, 0, 0, 0, {0}, 0};
    endpoint_script malformed = {SCRIPT_MALFORMED, 0, 0, 0, 0, {0}, 0};
    ti_c54x_memory_instruction_result result;
    const ti_c54x_bus_word16_endpoint fault_endpoint = endpoint_for(&fault);
    const ti_c54x_bus_word16_endpoint malformed_endpoint =
        endpoint_for(&malformed);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 2, 1) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = (ti_c54x_memory_instruction_result){0};
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &fault_endpoint, &state,
                           &accumulators, &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 2, 1) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = (ti_c54x_memory_instruction_result){0};
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &malformed_endpoint, &state,
                           &accumulators, &t_register, &result) !=
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.phase ==
                       TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &malformed_endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(malformed.cancel_calls == 1U);
    TI_C54X_TEST_CHECK(transaction.phase ==
                       TI_C54X_MEMORY_INSTRUCTION_PHASE_IDLE);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

int main(void)
{
    test_absent_endpoint_faults_before_id_allocation();
    test_preflight_rejects_forged_candidates_before_begin();
    test_phase_and_reducer_pair_mismatch_rejected();
    test_reset_and_cancel_require_canonical_phase_reducer_pairs();
    test_fault_cancel_and_malformed_observation_do_not_commit();
    return 0;
}
