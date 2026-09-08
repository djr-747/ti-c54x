/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proofs for the indexed WORD16
 * memory-instruction root transaction. It proves WAIT, FAULT, cancel, stale
 * ARx/AR0 rejection, and atomic READY publication. Does not own: opcode
 * dispatch, CPU lifecycle, address arithmetic, endpoint implementation,
 * timing, observers, or diagnostics.
 */

#include "test_memory_instruction_fixture.h"

#include "address/indirect_indexed.h"

#include <string.h>

static ti_c54x_arch_state
indexed_known_state(void)
{
    ti_c54x_arch_state state = known_arch_state();

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), UINT16_C(0x0010)) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_effect
indexed_memory_effect(ti_c54x_bus_word16_request_access access)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16;
    effect.payload.memory.address_spec.indexed_mode =
        TI_C54X_INDIRECT_INDEXED_MOD5;
    effect.payload.memory.address_spec.arf = UINT8_C(2);
    effect.payload.memory.access = access;
    effect.payload.memory.write_value_word =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ? UINT16_C(0x00A5)
                                                          : UINT16_C(0);
    effect.payload.memory.complete =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ? complete_read : NULL;
    return effect;
}

static ti_c54x_memory_instruction_status
prepare_indexed(ti_c54x_memory_instruction_transaction *transaction,
                const ti_c54x_op_effect *effect, ti_c54x_arch_state *state)
{
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = input_snapshot();

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, &instruction, &input, effect, state, UINT64_C(0),
        UINT64_C(0));
}

static void
check_wait_fault_and_cancel_preserve_state(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_op_effect effect = indexed_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_arch_state state = indexed_known_state();
    ti_c54x_arch_state before;
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xBEEF)};
    ti_c54x_memory_instruction_result result = {0};
    ti_c54x_bus_word16_endpoint endpoint;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_indexed(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    before = state;
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK_U64(state.ar[0], UINT16_C(0x0010));
    TI_C54X_TEST_CHECK_U64(state.ar[2], UINT16_C(0x2110));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x20));

    /* A fault after issue is equally non-publishing. */
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = indexed_known_state();
    before = state;
    script = (endpoint_script){SCRIPT_FAULT, 0, 0, 0, 0, {0}, 0};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_indexed(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
check_ready_rejects_stale_selected_ar_and_ar0(void)
{
    const ti_c54x_op_effect effect = indexed_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = indexed_known_state();
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xBEEF)};
    ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};
    ti_c54x_arch_state before;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_indexed(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), UINT16_C(0x0011)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = indexed_known_state();
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xBEEF)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_indexed(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2111)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
check_ready_atomically_commits_selected_ar_pc_and_effect(void)
{
    const ti_c54x_op_effect effect = indexed_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = indexed_known_state();
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xBEEF)};
    ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};
    uint64_t accumulator_value = UINT64_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_indexed(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(state.ar[0], UINT16_C(0x0010));
    TI_C54X_TEST_CHECK_U64(state.ar[2], UINT16_C(0x2100));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x21));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           &accumulator_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(accumulator_value, UINT16_C(0xBEEF));
}

int
main(void)
{
    /* Keep the shared fixture's legacy helpers link-visible under -Werror. */
    (void)memory_effect;
    (void)prepare;
    (void)advance;
    check_wait_fault_and_cancel_preserve_state();
    check_ready_rejects_stale_selected_ar_and_ar0();
    check_ready_atomically_commits_selected_ar_pc_and_effect();
    return 0;
}
