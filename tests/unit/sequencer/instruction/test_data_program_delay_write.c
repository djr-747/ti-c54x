/*
 * Module contract
 *
 * Owns: the root-composition contract for the Smem delay
 * write-back (MACD) staged on a DATA+PROGRAM WORD16 READ pair. The
 * tests pin the third-ID DATA WRITE of the Smem word to Smem+1
 * (with 16-bit wraparound) after both reads complete and before
 * the architecture commit, WAIT resume without reissuing the
 * reads, FAULT and cancel publishing nothing, and the flag-off
 * control issuing no third request.
 * Does not own: production sequencing, address planning, endpoint
 * storage, opcode dispatch, timing, or architectural policy.
 * Endpoint scripts are test-owned and all state is passed through
 * their fixture context.
 */

#include "isa/decode/opcode_decode.h"
#include "isa/ops/memory/memory_effect.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "state/accumulator/accumulator.h"
#include "state/arch_state.h"
#include "state/temporary/t_register.h"
#include "../../../support/test_harness.h"

#include <ti_c54x/bus_endpoint.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum delay_script_mode {
    DELAY_SCRIPT_READY = 0,
    DELAY_SCRIPT_WAIT_DELAY = 1,
    DELAY_SCRIPT_FAULT_DELAY = 2
} delay_script_mode;

typedef struct delay_endpoint_script {
    delay_script_mode mode;
    uint32_t x_address;
    uint32_t y_address;
    uint16_t x_value;
    uint16_t y_value;
    unsigned callback_count;
    unsigned resume_count;
    unsigned cancel_count;
    uint64_t cancelled_id;
    bool delay_waited;
    ti_c54x_bus_word16_request requests[4];
    uint64_t ids[4];
} delay_endpoint_script;

static unsigned delay_completion_calls;

static ti_c54x_arch_state known_arch_state(void)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(5), UINT16_C(0x0100)) ==
                       TI_C54X_ARCH_STATE_OK);
    state.pc_word = UINT32_C(0x20);
    return state;
}

static ti_c54x_decoded_instruction decoded_nop(void)
{
    return (ti_c54x_decoded_instruction){
        .id = TI_C54X_OPCODE_ID_NOP,
        .operand_layout = TI_C54X_OPERAND_LAYOUT_NONE,
        .repeatability = TI_C54X_REPEATABILITY_UNRESOLVED,
        .input_resources = TI_C54X_INPUT_RESOURCES_NONE,
        .bundle = {.program_words = {UINT16_C(0xF495), UINT16_C(0)},
                   .program_word_count = UINT8_C(1)}};
}

static ti_c54x_op_input_snapshot input_snapshot(void)
{
    return (ti_c54x_op_input_snapshot){
        .accumulator_a_raw40 = UINT64_C(0x1122),
        .accumulator_b_raw40 = UINT64_C(0x3344),
        .accumulator_a_known = 1,
        .accumulator_b_known = 1,
        .status_c = 0,
        .status_c_known = 1,
        .status_tc = 0,
        .status_tc_known = 1,
        .status_sxm = 0,
        .status_sxm_known = 1};
}

/* Calls 0 and 1 are the Smem DATA and PROGRAM reads; call 2 on is
 * the delay DATA WRITE of the Smem word to Smem+1. */
static void delay_script_advance(
    void *context,
    ti_c54x_bus_word16_advance_phase phase,
    uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request,
    ti_c54x_bus_word16_observation *observation)
{
    delay_endpoint_script *script = (delay_endpoint_script *)context;
    const unsigned index = script->callback_count;
    const uint32_t delay_address =
        (uint32_t)(((uint32_t)script->x_address + UINT32_C(1)) &
                   UINT32_C(0xffff));

    TI_C54X_TEST_CHECK(index < 4U);
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        TI_C54X_TEST_CHECK(index == 0U || index == 1U || index == 2U);
        TI_C54X_TEST_CHECK(transaction_id == (uint64_t)(index + 1U));
    } else {
        ++script->resume_count;
        TI_C54X_TEST_CHECK(index == 3U);
        TI_C54X_TEST_CHECK(transaction_id == UINT64_C(3));
    }
    script->requests[index] = *request;
    script->ids[index] = transaction_id;
    ++script->callback_count;

    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = UINT16_C(0),
        .bus_status = TI_C54X_BUS_OK};

    if (index == 0U) {
        TI_C54X_TEST_CHECK(request->access ==
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
        TI_C54X_TEST_CHECK(request->space ==
                           TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
        TI_C54X_TEST_CHECK(request->address_word == script->x_address);
        observation->read_value_word = script->x_value;
        return;
    }
    if (index == 1U) {
        TI_C54X_TEST_CHECK(request->access ==
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
        TI_C54X_TEST_CHECK(request->space ==
                           TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM);
        TI_C54X_TEST_CHECK(request->address_word == script->y_address);
        observation->read_value_word = script->y_value;
        return;
    }
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->address_word == delay_address);
    TI_C54X_TEST_CHECK(request->write_value_word == script->x_value);
    if (script->mode == DELAY_SCRIPT_WAIT_DELAY && !script->delay_waited) {
        script->delay_waited = true;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        return;
    }
    if (script->mode == DELAY_SCRIPT_FAULT_DELAY) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
        return;
    }
}

static ti_c54x_bus_word16_cancel_result delay_script_cancel(
    void *context,
    uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request)
{
    delay_endpoint_script *script = (delay_endpoint_script *)context;

    (void)request;
    TI_C54X_TEST_CHECK(transaction_id == UINT64_C(3));
    script->cancelled_id = transaction_id;
    ++script->cancel_count;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint delay_endpoint_for(
    delay_endpoint_script *script)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = script,
        .advance = delay_script_advance,
        .cancel = delay_script_cancel};
    return endpoint;
}

static ti_c54x_op_status complete_delay_effect(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t x_word,
    uint16_t y_word,
    ti_c54x_op_effect *effect)
{
    ++delay_completion_calls;
    TI_C54X_TEST_CHECK(instruction != NULL);
    TI_C54X_TEST_CHECK(input != NULL);
    TI_C54X_TEST_CHECK(effect != NULL);
    effect->kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    effect->payload.accumulator_write.destination = TI_C54X_ACCUMULATOR_A;
    effect->payload.accumulator_write.value_raw40 =
        ((uint64_t)x_word << 16) | (uint64_t)y_word;
    effect->payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    effect->payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    effect->payload.accumulator_write.status_delta
        .destination_overflow_action = TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    return TI_C54X_OP_STATUS_OK;
}

/* Indirect MOD2 Smem leg (*AR5+) with the verbatim PROGRAM leg,
 * read-phase Smem T load, and the Smem delay write-back. */
static ti_c54x_op_effect delay_memory_effect(bool write_smem_delay)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16;
    effect.payload.memory.address_spec.pair.indirect = UINT8_C(1);
    effect.payload.memory.address_spec.pair.smem_mode = UINT8_C(2);
    effect.payload.memory.address_spec.pair.smem_arf = UINT8_C(5);
    effect.payload.memory.address_spec.pair.program_pmad = UINT16_C(0x0200);
    effect.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    effect.payload.memory.value_word = UINT16_C(0);
    effect.payload.memory.load_t_with_read = true;
    effect.payload.memory.write_smem_delay = write_smem_delay;
    effect.payload.memory.complete_dual = complete_delay_effect;
    return effect;
}

/* MOD0 Smem leg (*AR5, no update) with the verbatim PROGRAM leg
 * and the Smem delay write-back. */
static ti_c54x_op_effect delay_memory_effect_mod0(void)
{
    ti_c54x_op_effect effect = delay_memory_effect(true);

    effect.payload.memory.address_spec.pair.smem_mode = UINT8_C(0);
    return effect;
}

static void init_runtime(ti_c54x_accumulator_state *accumulators,
                         ti_c54x_t_register_state *t_register)
{
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(t_register) ==
                       TI_C54X_T_REGISTER_OK);
    delay_completion_calls = 0U;
}

static delay_endpoint_script delay_script(delay_script_mode mode)
{
    return (delay_endpoint_script){
        .mode = mode,
        .x_address = UINT32_C(0x0100),
        .y_address = UINT32_C(0x0200),
        .x_value = UINT16_C(0x1111),
        .y_value = UINT16_C(0x2222)};
}

static ti_c54x_memory_instruction_status prepare_delay(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_op_effect *effect,
    ti_c54x_arch_state *state)
{
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = input_snapshot();

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, &instruction, &input, effect, state, 0, 0);
}

static ti_c54x_memory_instruction_result delay_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    delay_endpoint_script *script,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register)
{
    const ti_c54x_bus_word16_endpoint endpoint = delay_endpoint_for(script);
    ti_c54x_memory_instruction_result result = {0};

    (void)ti_c54x_memory_instruction_transaction_advance(
        transaction, &endpoint, state, accumulators, t_register, &result);
    return result;
}

static void check_delay_retired(delay_endpoint_script *script,
                                ti_c54x_arch_state *state,
                                ti_c54x_accumulator_state *accumulators,
                                ti_c54x_t_register_state *t_register)
{
    uint64_t accumulator_value = 0;

    TI_C54X_TEST_CHECK(delay_completion_calls == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           accumulators, TI_C54X_ACCUMULATOR_A,
                           &accumulator_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(accumulator_value, UINT64_C(0x11112222));
    TI_C54X_TEST_CHECK_U64(state->pc_word, UINT32_C(0x21));
    TI_C54X_TEST_CHECK_U64(state->ar[5], UINT16_C(0x0101));
    TI_C54X_TEST_CHECK(t_register->raw_value == UINT16_C(0x1111));
    (void)script;
}

static void test_ready_delay_write_order_value_and_atomic_commit(void)
{
    const ti_c54x_op_effect effect = delay_memory_effect(true);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delay_endpoint_script script = delay_script(DELAY_SCRIPT_READY);
    ti_c54x_memory_instruction_result result;

    init_runtime(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delay_advance(&transaction, &script, &state, &accumulators,
                           &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(result.instructions_attempted, 1);
    TI_C54X_TEST_CHECK_U64(result.instructions_retired, 1);
    TI_C54X_TEST_CHECK_U64(result.transaction_id, 3);
    TI_C54X_TEST_CHECK(script.callback_count == 3U);
    TI_C54X_TEST_CHECK(script.resume_count == 0U);
    TI_C54X_TEST_CHECK(script.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(script.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.requests[1].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM);
    /* The third ID is the delay DATA WRITE of the Smem word. */
    TI_C54X_TEST_CHECK(script.requests[2].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(script.requests[2].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.requests[2].address_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(script.requests[2].write_value_word ==
                       UINT16_C(0x1111));
    check_delay_retired(&script, &state, &accumulators, &t_register);
}

static void test_delay_write_wait_resumes_without_reissuing_reads(void)
{
    const ti_c54x_op_effect effect = delay_memory_effect(true);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delay_endpoint_script script = delay_script(DELAY_SCRIPT_WAIT_DELAY);
    ti_c54x_memory_instruction_result result;

    init_runtime(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delay_advance(&transaction, &script, &state, &accumulators,
                           &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(script.callback_count == 3U);
    /* The pair holds staged values; nothing publishes yet: no
     * commit, no PC advance, no Smem AR update. */
    TI_C54X_TEST_CHECK(delay_completion_calls == 0U);
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x20));
    TI_C54X_TEST_CHECK_U64(state.ar[5], UINT16_C(0x0100));
    result = delay_advance(&transaction, &script, &state, &accumulators,
                           &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(result.instructions_retired, 1);
    TI_C54X_TEST_CHECK_U64(result.transaction_id, 3);
    /* Resume re-serves only the delay WRITE under the third ID. */
    TI_C54X_TEST_CHECK(script.callback_count == 4U);
    TI_C54X_TEST_CHECK(script.resume_count == 1U);
    TI_C54X_TEST_CHECK(script.ids[3] == UINT64_C(3));
    TI_C54X_TEST_CHECK(script.requests[3].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(script.requests[3].address_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(script.requests[3].write_value_word ==
                       UINT16_C(0x1111));
    check_delay_retired(&script, &state, &accumulators, &t_register);
}

static void test_delay_write_fault_publishes_nothing(void)
{
    const ti_c54x_op_effect effect = delay_memory_effect(true);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delay_endpoint_script script = delay_script(DELAY_SCRIPT_FAULT_DELAY);
    ti_c54x_memory_instruction_result result;

    init_runtime(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delay_advance(&transaction, &script, &state, &accumulators,
                           &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK_U64(result.instructions_retired, 0);
    TI_C54X_TEST_CHECK(script.callback_count == 3U);
    TI_C54X_TEST_CHECK(delay_completion_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void test_flag_off_issues_no_third_request(void)
{
    const ti_c54x_op_effect effect = delay_memory_effect(false);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delay_endpoint_script script = delay_script(DELAY_SCRIPT_READY);
    ti_c54x_memory_instruction_result result;

    init_runtime(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delay_advance(&transaction, &script, &state, &accumulators,
                           &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(result.transaction_id, 2);
    TI_C54X_TEST_CHECK(script.callback_count == 2U);
    check_delay_retired(&script, &state, &accumulators, &t_register);
}

static void test_delay_write_wraps_smem_plus_one(void)
{
    const ti_c54x_op_effect effect = delay_memory_effect_mod0();
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delay_endpoint_script script = delay_script(DELAY_SCRIPT_READY);
    ti_c54x_memory_instruction_result result;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(5), UINT16_C(0xffff)) ==
                       TI_C54X_ARCH_STATE_OK);
    script.x_address = UINT32_C(0xffff);
    init_runtime(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delay_advance(&transaction, &script, &state, &accumulators,
                           &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(result.transaction_id, 3);
    TI_C54X_TEST_CHECK(script.callback_count == 3U);
    TI_C54X_TEST_CHECK(script.requests[2].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(script.requests[2].address_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(script.requests[2].write_value_word ==
                       UINT16_C(0x1111));
    TI_C54X_TEST_CHECK(delay_completion_calls == 1U);
}

static void test_delay_write_cancel_publishes_nothing(void)
{
    const ti_c54x_op_effect effect = delay_memory_effect(true);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delay_endpoint_script script = delay_script(DELAY_SCRIPT_WAIT_DELAY);
    ti_c54x_memory_instruction_result result;

    init_runtime(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delay_advance(&transaction, &script, &state, &accumulators,
                           &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    {
        const ti_c54x_bus_word16_endpoint endpoint =
            delay_endpoint_for(&script);
        TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                               &transaction, &endpoint) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    }
    TI_C54X_TEST_CHECK(script.cancel_count == 1U);
    TI_C54X_TEST_CHECK(script.cancelled_id == UINT64_C(3));
    TI_C54X_TEST_CHECK(delay_completion_calls == 0U);
}

int
main(void)
{
    test_ready_delay_write_order_value_and_atomic_commit();
    test_delay_write_wait_resumes_without_reissuing_reads();
    test_delay_write_fault_publishes_nothing();
    test_flag_off_issues_no_third_request();
    test_delay_write_wraps_smem_plus_one();
    test_delay_write_cancel_publishes_nothing();
    return 0;
}
