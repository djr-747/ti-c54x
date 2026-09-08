/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proofs for the reverse-carry
 * WORD16 memory-instruction transaction. It proves typed MOD4/MOD7 READ and
 * WRITE WAIT/READY publication, FAULT and cancel rollback, exact selected
 * AR/AR0 snapshot rejection, malformed candidates, and no-refetch behavior.
 * Does not own: opcode identity, address arithmetic, endpoint implementation,
 * CPU lifecycle, BK semantics, or test-harness implementation.
 */

#include "test_memory_instruction_fixture.h"

#include "address/indirect_reverse_carry.h"
#include "state/block_size_register.h"

#include <string.h>

static ti_c54x_arch_state
reverse_carry_known_state(uint8_t ar_index,
                          uint16_t ar_value,
                          uint16_t ar0_value)
{
    ti_c54x_arch_state state = known_arch_state();

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), ar0_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, ar_index, ar_value) ==
                       TI_C54X_ARCH_STATE_OK);
    /* Reverse-carry is independent of BK; keep a known nonzero sentinel to
     * prove the transaction never infers circular behavior from it. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(
                           &state, UINT16_C(0x4a4a)) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_effect
reverse_carry_memory_effect(ti_c54x_bus_word16_request_access access,
                            ti_c54x_indirect_reverse_carry_mode mode,
                            uint8_t ar_index)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16;
    effect.payload.memory.address_spec.reverse_carry_mode = mode;
    effect.payload.memory.address_spec.arf = ar_index;
    effect.payload.memory.access = access;
    effect.payload.memory.value_word =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ? UINT16_C(0x00a5)
                                                          : UINT16_C(0);
    effect.payload.memory.complete =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ? complete_read : NULL;
    return effect;
}

static ti_c54x_memory_instruction_status
prepare_reverse_carry(ti_c54x_memory_instruction_transaction *transaction,
                      const ti_c54x_op_effect *effect,
                      ti_c54x_arch_state *state)
{
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = input_snapshot();

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, &instruction, &input, effect, state, UINT64_C(0),
        UINT64_C(0));
}

static void
check_wait_ready_read_and_write(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = reverse_carry_known_state(
        UINT8_C(2), UINT16_C(0x68), UINT16_C(0x08));
    ti_c54x_op_effect effect = reverse_carry_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD4, UINT8_C(2));
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xbeef)};
    ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};
    uint64_t accumulator_value = UINT64_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_reverse_carry(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(script.begin_calls == 1U);
    TI_C54X_TEST_CHECK(script.resume_calls == 0U);
    TI_C54X_TEST_CHECK(state.ar[2] == UINT16_C(0x68));
    TI_C54X_TEST_CHECK(state.ar[0] == UINT16_C(0x08));
    TI_C54X_TEST_CHECK(state.bk == UINT16_C(0x4a4a));
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x20));

    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.begin_calls == 1U);
    TI_C54X_TEST_CHECK(script.resume_calls == 1U);
    TI_C54X_TEST_CHECK(state.ar[2] == UINT16_C(0x60));
    TI_C54X_TEST_CHECK(state.ar[0] == UINT16_C(0x08));
    TI_C54X_TEST_CHECK(state.bk == UINT16_C(0x4a4a));
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x21));
    TI_C54X_TEST_CHECK(script.request.address_word == UINT16_C(0x68));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           &accumulator_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(accumulator_value, UINT16_C(0xbeef));

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = reverse_carry_known_state(UINT8_C(2), UINT16_C(0x60),
                                      UINT16_C(0x08));
    effect = reverse_carry_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, UINT8_C(2));
    script = (endpoint_script){SCRIPT_WAIT_THEN_WRITE_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_reverse_carry(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(state.ar[2] == UINT16_C(0x60));
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x20));
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(state.ar[2] == UINT16_C(0x68));
    TI_C54X_TEST_CHECK(state.ar[0] == UINT16_C(0x08));
    TI_C54X_TEST_CHECK(state.bk == UINT16_C(0x4a4a));
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x21));
    TI_C54X_TEST_CHECK(script.request.address_word == UINT16_C(0x60));
    TI_C54X_TEST_CHECK(script.request.write_value_word == UINT16_C(0x00a5));
}

static void
check_fault_and_cancel_rollback(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_op_effect effect = reverse_carry_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, UINT8_C(2));
    ti_c54x_arch_state state = reverse_carry_known_state(
        UINT8_C(2), UINT16_C(0x60), UINT16_C(0x08));
    ti_c54x_arch_state before;
    endpoint_script script = {SCRIPT_FAULT, 0, 0, 0, 0, {0}, 0};
    ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_reverse_carry(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = reverse_carry_known_state(UINT8_C(2), UINT16_C(0x60),
                                      UINT16_C(0x08));
    before = state;
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0xbeef)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_reverse_carry(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(script.cancel_calls == 1U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
check_stale_selected_ar_and_ar0_reject_before_resume(void)
{
    const ti_c54x_op_effect effect = reverse_carry_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, UINT8_C(2));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = reverse_carry_known_state(
        UINT8_C(2), UINT16_C(0x60), UINT16_C(0x08));
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xbeef)};
    ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};
    ti_c54x_arch_state before;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_reverse_carry(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x68)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.resume_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = reverse_carry_known_state(UINT8_C(2), UINT16_C(0x60),
                                      UINT16_C(0x08));
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0xbeef)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_reverse_carry(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), UINT16_C(0x10)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.resume_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
check_malformed_candidate_and_snapshot_reject_before_begin(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = reverse_carry_known_state(
        UINT8_C(2), UINT16_C(0x60), UINT16_C(0x08));
    const ti_c54x_op_effect effect = reverse_carry_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, UINT8_C(2));
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xbeef)};
    ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};
    ti_c54x_arch_state before;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_reverse_carry(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.candidate.address_resolution.address_result.effect.source
        .value.ar_reverse_carry.modification = UINT8_C(4);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0xbeef)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_reverse_carry(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    before = state;
    transaction.candidate.address_resolution.address_commit.value_after =
        UINT16_C(0x67);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0xbeef)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_reverse_carry(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.candidate.address_resolution.address_result.effect.source
        .value.ar_reverse_carry.ar0_value_before = UINT16_C(0x10);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0xbeef)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_reverse_carry(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.candidate.effect.payload.memory.address_spec.arf =
        UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0xbeef)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_reverse_carry(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.candidate.address_resolution.request.address_word =
        UINT16_C(0x61);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);
}

static void
check_selected_ar1_through_ar7_are_supported(void)
{
    uint8_t ar_index;

    for (ar_index = UINT8_C(1); ar_index <= UINT8_C(7); ++ar_index) {
        ti_c54x_accumulator_state accumulators = {0};
        ti_c54x_t_register_state t_register = {0};
        ti_c54x_memory_instruction_transaction transaction = {0};
        ti_c54x_arch_state state = reverse_carry_known_state(
            ar_index, UINT16_C(0x60), UINT16_C(0x08));
        const ti_c54x_op_effect effect = reverse_carry_memory_effect(
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, ar_index);
        endpoint_script script = {SCRIPT_WAIT_THEN_WRITE_READY, 0, 0, 0,
                                  0, {0}, UINT16_C(0)};
        ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
        ti_c54x_memory_instruction_result result = {0};

        TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                           TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                           TI_C54X_T_REGISTER_OK);
        TI_C54X_TEST_CHECK(prepare_reverse_carry(&transaction, &effect,
                                                 &state) ==
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
        TI_C54X_TEST_CHECK(state.ar[ar_index] == UINT16_C(0x68));
        TI_C54X_TEST_CHECK(state.ar[0] == UINT16_C(0x08));
    }
}

int
main(void)
{
    /* Keep the generic transaction helpers link-visible under -Werror. */
    (void)memory_effect;
    (void)prepare;
    (void)advance;
    check_wait_ready_read_and_write();
    check_fault_and_cancel_rollback();
    check_stale_selected_ar_and_ar0_reject_before_resume();
    check_malformed_candidate_and_snapshot_reject_before_begin();
    check_selected_ar1_through_ar7_are_supported();
    return 0;
}
