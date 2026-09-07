/*
 * Module contract
 *
 * Owns: commit-lane proof of resumable WAIT/READY publication and retirement.
 * Does not own: lifecycle rejection, CPU orchestration, or endpoint behavior.
 */

#include "test_memory_instruction_fixture.h"

#include <string.h>

static void test_read_wait_resume_completion_and_atomic_commit(void)
{
    const ti_c54x_op_effect effect =
        memory_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    endpoint_script script = {SCRIPT_WAIT_THEN_READ_READY, 0, 0, 0, 0,
                              {0}, UINT16_C(0xbeef)};
    ti_c54x_decoded_instruction instruction = decoded_nop();
    ti_c54x_op_input_snapshot input = input_snapshot();
    ti_c54x_memory_instruction_result result;
    uint64_t accumulator_value = 0;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_prepare(
                           &transaction, &instruction, &input, &effect, &state,
                           4, 2) == TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    instruction.id = TI_C54X_OPCODE_ID_UNKNOWN;
    instruction.bundle.program_words[0] = UINT16_C(0);
    input.accumulator_a_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);

    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK_U64(result.instructions_attempted, 1);
    TI_C54X_TEST_CHECK_U64(result.instructions_retired, 0);
    TI_C54X_TEST_CHECK(script.begin_calls == 1U);
    TI_C54X_TEST_CHECK(script.resume_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(result.instructions_attempted, 0);
    TI_C54X_TEST_CHECK_U64(result.instructions_retired, 1);
    TI_C54X_TEST_CHECK(script.begin_calls == 1U);
    TI_C54X_TEST_CHECK(script.resume_calls == 1U);
    TI_C54X_TEST_CHECK_U64(state.ar[2], UINT16_C(0x2111));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x21));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           &accumulator_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(accumulator_value, UINT16_C(0xbeef));
}

static void test_write_ready_publishes_before_infallible_arch_commit(void)
{
    const ti_c54x_op_effect effect =
        memory_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    endpoint_script script = {SCRIPT_WAIT_THEN_WRITE_READY, 0, 0, 0, 0,
                              {0}, 0};
    ti_c54x_memory_instruction_result result;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    result = advance(&transaction, &script, &state, &accumulators,
                     &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.begin_calls == 1U);
    TI_C54X_TEST_CHECK(script.resume_calls == 1U);
    TI_C54X_TEST_CHECK_U64(state.ar[2], UINT16_C(0x2111));
}

int main(void)
{
    test_read_wait_resume_completion_and_atomic_commit();
    test_write_ready_publishes_before_infallible_arch_commit();
    return 0;
}
