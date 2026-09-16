/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT executable proof of the S1 MMR root
 * transaction hook: routed MMR READ/WRITE retire locally with
 * zero endpoint traffic and atomic publication, unknown MMR
 * reads and guard-range writes reject transactionally with
 * state untouched, and non-MMR traffic still reaches the
 * endpoint unchanged.
 * Does not own: register storage, address planning, endpoint
 * implementation, opcode dispatch, CPU lifecycle, or timing.
 */

#include "test_memory_instruction_fixture.h"

#include "state/accumulator/accumulator.h"
#include "state/temporary/t_register.h"

#include <string.h>

static ti_c54x_op_effect
mmr_effect(ti_c54x_bus_word16_request_access access, uint16_t value_word)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
    effect.payload.memory.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD0;
    effect.payload.memory.address_spec.arf = UINT8_C(2);
    effect.payload.memory.access = access;
    effect.payload.memory.value_word = value_word;
    effect.payload.memory.complete =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ? complete_read
                                                         : NULL;
    return effect;
}

static ti_c54x_arch_state
mmr_known_state(uint16_t ar2_value)
{
    ti_c54x_arch_state state = known_arch_state();

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), ar2_value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static void
test_routed_read_retires_with_no_endpoint_traffic(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_op_effect effect =
        mmr_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT16_C(0));
    ti_c54x_arch_state state = mmr_known_state(UINT16_C(0x000E));
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xBEEF)};
    ti_c54x_memory_instruction_result result;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_write(&t_register,
                                                UINT16_C(0x2468)) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, UINT64_C(0),
                               UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);
    TI_C54X_TEST_CHECK(script.resume_calls == 0U);
    /* The T value flows through the read completion into A. */
    TI_C54X_TEST_CHECK_U64(accumulators.a_raw40, UINT64_C(0x2468));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x21));
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
}

static void
test_routed_write_publishes_atomically(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_op_effect effect =
        mmr_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT16_C(0x0300));
    ti_c54x_arch_state state = mmr_known_state(UINT16_C(0x0013));
    ti_c54x_arch_state before;
    endpoint_script script = {SCRIPT_WAIT_THEN_WRITE_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0)};
    ti_c54x_memory_instruction_result result;
    uint16_t ar3 = UINT16_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, UINT64_C(0),
                               UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    before = state;
    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, UINT8_C(3),
                                                 &ar3) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(ar3, UINT16_C(0x0300));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x21));
    TI_C54X_TEST_CHECK(state.ar[2] == before.ar[2]);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
}

static void
test_unknown_read_rejects_transactionally(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_op_effect effect =
        mmr_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT16_C(0));
    ti_c54x_arch_state state = mmr_known_state(UINT16_C(0x0008));
    ti_c54x_arch_state before;
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xBEEF)};
    const ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    /* AL resets unknown: the routed read must reject. */
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, UINT64_C(0),
                               UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
test_guard_range_write_rejects_transactionally(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_op_effect effect =
        mmr_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT16_C(0x0100));
    ti_c54x_arch_state state = mmr_known_state(UINT16_C(0x000A));
    ti_c54x_arch_state before;
    endpoint_script script = {SCRIPT_WAIT_THEN_WRITE_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0)};
    const ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, UINT64_C(0),
                               UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.begin_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
test_plain_traffic_still_reaches_the_endpoint(void)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_op_effect effect = memory_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_arch_state state = known_arch_state();
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xBEEF)};
    const ti_c54x_bus_word16_endpoint endpoint = endpoint_for(&script);
    ti_c54x_memory_instruction_result result;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, UINT64_C(0),
                               UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(script.begin_calls == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
}

int
main(void)
{
    test_routed_read_retires_with_no_endpoint_traffic();
    test_routed_write_publishes_atomically();
    test_unknown_read_rejects_transactionally();
    test_guard_range_write_rejects_transactionally();
    test_plain_traffic_still_reaches_the_endpoint();
    return 0;
}
