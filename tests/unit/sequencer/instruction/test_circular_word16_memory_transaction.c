/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proofs for the circular WORD16
 * memory-instruction transaction. It proves BK=0 no-update retirement,
 * positive-BK AR publication, WAIT/READY, FAULT, cancel, stale dependency
 * rejection, and malformed candidate rejection. Does not own: opcode
 * identity, address arithmetic, endpoint implementation, or CPU lifecycle.
 */

#include "test_memory_instruction_fixture.h"

#include "address/indirect_circular.h"
#include "state/block_size_register.h"

#include <string.h>

static ti_c54x_arch_state
circular_known_state(uint16_t ar_value, uint16_t bk_value)
{
    ti_c54x_arch_state state = known_arch_state();

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), ar_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, bk_value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_effect
circular_memory_effect(ti_c54x_bus_word16_request_access access,
                       ti_c54x_indirect_circular_mode mode)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16;
    effect.payload.memory.address_spec.circular_mode = mode;
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
prepare_circular(ti_c54x_memory_instruction_transaction *transaction,
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
    ti_c54x_arch_state state = circular_known_state(UINT16_C(0x2102),
                                                    UINT16_C(5));
    ti_c54x_op_effect effect = circular_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        TI_C54X_INDIRECT_CIRCULAR_MOD10);
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xBEEF)};
    ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};
    uint64_t accumulator_value = UINT64_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_circular(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(script.begin_calls == 1U);
    TI_C54X_TEST_CHECK(state.ar[2] == UINT16_C(0x2102));
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x20));
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.resume_calls == 1U);
    TI_C54X_TEST_CHECK(state.ar[2] == UINT16_C(0x2103));
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x21));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           &accumulator_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(accumulator_value, UINT16_C(0xBEEF));
    TI_C54X_TEST_CHECK(script.request.address_word == UINT16_C(0x2102));

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = circular_known_state(UINT16_C(0x2102), UINT16_C(5));
    effect = circular_memory_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                                    TI_C54X_INDIRECT_CIRCULAR_MOD8);
    script = (endpoint_script){SCRIPT_WAIT_THEN_WRITE_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_circular(&transaction, &effect, &state) ==
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
    TI_C54X_TEST_CHECK(state.ar[2] == UINT16_C(0x2101));
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x21));
    TI_C54X_TEST_CHECK(script.request.address_word == UINT16_C(0x2102));
    TI_C54X_TEST_CHECK(script.request.write_value_word == UINT16_C(0x00A5));
}

static void
check_bk_zero_retires_without_ar_commit(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = circular_known_state(UINT16_C(0x2102),
                                                    UINT16_C(0));
    const ti_c54x_op_effect effect = circular_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        TI_C54X_INDIRECT_CIRCULAR_MOD8);
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xBEEF)};
    ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_circular(&transaction, &effect, &state) ==
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
    TI_C54X_TEST_CHECK(state.ar[2] == UINT16_C(0x2102));
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x21));
}

static void
check_fault_and_cancel_rollback(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = circular_known_state(UINT16_C(0x2102),
                                                    UINT16_C(5));
    const ti_c54x_op_effect effect = circular_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        TI_C54X_INDIRECT_CIRCULAR_MOD10);
    ti_c54x_arch_state before;
    endpoint_script script = {SCRIPT_FAULT, 0, 0, 0, 0, {0}, 0};
    ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_circular(&transaction, &effect, &state) ==
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
    state = circular_known_state(UINT16_C(0x2102), UINT16_C(5));
    before = state;
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0xBEEF)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_circular(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(script.cancel_calls == 1U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
check_stale_ar_and_bk_reject_before_endpoint(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = circular_known_state(UINT16_C(0x2102),
                                                    UINT16_C(5));
    const ti_c54x_op_effect effect = circular_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        TI_C54X_INDIRECT_CIRCULAR_MOD10);
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xBEEF)};
    ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};
    ti_c54x_arch_state before;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_circular(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2103)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.resume_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = circular_known_state(UINT16_C(0x2102), UINT16_C(5));
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0xBEEF)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_circular(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, UINT16_C(6)) ==
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
check_malformed_candidate_rejected(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = circular_known_state(UINT16_C(0x2102),
                                                    UINT16_C(5));
    const ti_c54x_op_effect effect = circular_memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        TI_C54X_INDIRECT_CIRCULAR_MOD10);
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xBEEF)};
    ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_circular(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.candidate.address_resolution.address_result.effect.source
        .value.ar_circular.bk_value_before = UINT16_C(6);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = circular_known_state(UINT16_C(0x2102), UINT16_C(5));
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0xBEEF)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_circular(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.candidate.effect.payload.memory.address_spec.circular_mode =
        TI_C54X_INDIRECT_CIRCULAR_MOD8;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = circular_known_state(UINT16_C(0x2102), UINT16_C(5));
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0xBEEF)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_circular(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.candidate.effect.payload.memory.address_spec.arf =
        UINT8_C(3);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = circular_known_state(UINT16_C(0x2102), UINT16_C(5));
    script = (endpoint_script){SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                               {0}, UINT16_C(0xBEEF)};
    endpoint = endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_circular(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.candidate.address_resolution.address_result.effect.source
        .value.ar_circular.modification =
        (uint8_t)TI_C54X_INDIRECT_CIRCULAR_MOD8;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);
}

int
main(void)
{
    /* Keep shared legacy helpers link-visible under -Werror. */
    (void)memory_effect;
    (void)prepare;
    (void)advance;
    check_wait_ready_read_and_write();
    check_bk_zero_retires_without_ar_commit();
    check_fault_and_cancel_rollback();
    check_stale_ar_and_bk_reject_before_endpoint();
    check_malformed_candidate_rejected();
    return 0;
}
