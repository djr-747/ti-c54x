/*
 * Module contract
 *
 * Owns: the root-composition contract for a DATA+PROGRAM WORD16
 * READ pair. The tests pin deterministic Smem-DATA-then-PROGRAM
 * issue order, monotonic child IDs, resumable WAIT/fault/cancel
 * behavior, stale observations, dependency revalidation, the
 * read-phase Smem T load, the single Smem AR update, and one atomic
 * completion.
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
#include <stdio.h>
#include <string.h>

typedef enum pair_script_mode {
    PAIR_SCRIPT_READY_BOTH = 0,
    PAIR_SCRIPT_WAIT_X = 1,
    PAIR_SCRIPT_WAIT_Y = 2,
    PAIR_SCRIPT_FAULT_X = 3,
    PAIR_SCRIPT_FAULT_Y = 4,
    PAIR_SCRIPT_MALFORMED_X = 5,
    PAIR_SCRIPT_MALFORMED_Y = 6,
    PAIR_SCRIPT_STALE_X = 7,
    PAIR_SCRIPT_STALE_Y = 8,
    PAIR_SCRIPT_MUTATE_LIVE_AR = 9,
    PAIR_SCRIPT_MUTATE_CANDIDATE = 10,
    PAIR_SCRIPT_MUTATE_SNAPSHOT_AND_LIVE_AR = 11,
    PAIR_SCRIPT_CANCEL_FAILED = 12
} pair_script_mode;

typedef struct pair_endpoint_script {
    pair_script_mode mode;
    ti_c54x_arch_state *state;
    ti_c54x_memory_instruction_transaction *transaction;
    uint32_t x_address;
    uint32_t y_address;
    uint16_t x_value;
    uint16_t y_value;
    unsigned callback_count;
    unsigned begin_count;
    unsigned resume_count;
    unsigned cancel_count;
    unsigned slot_calls[2];
    uint64_t slot_ids[2];
    ti_c54x_bus_word16_request slot_requests[2];
    uint64_t cancelled_id;
} pair_endpoint_script;

static unsigned pair_completion_calls;

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

static void check_pair_request(const ti_c54x_bus_word16_request *actual,
                               const ti_c54x_bus_word16_request *expected)
{
    TI_C54X_TEST_CHECK(actual->space == expected->space);
    TI_C54X_TEST_CHECK(actual->access == expected->access);
    TI_C54X_TEST_CHECK(actual->address_word == expected->address_word);
    TI_C54X_TEST_CHECK(actual->write_value_word == expected->write_value_word);
}

/* Slot 0 is the Smem DATA leg; slot 1 is the absolute PROGRAM leg.
 * The legs never alias: spaces differ. */
static int pair_request_slot(const pair_endpoint_script *script,
                             const ti_c54x_bus_word16_request *request)
{
    if (request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        request->write_value_word != UINT16_C(0)) {
        return -1;
    }
    if (request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
        request->address_word == script->x_address) {
        return 0;
    }
    if (request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM &&
        request->address_word == script->y_address) {
        return 1;
    }
    return -1;
}

static void pair_script_advance(
    void *context,
    ti_c54x_bus_word16_advance_phase phase,
    uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request,
    ti_c54x_bus_word16_observation *observation)
{
    pair_endpoint_script *script = (pair_endpoint_script *)context;
    const int slot = pair_request_slot(script, request);
    bool first_for_slot;

    TI_C54X_TEST_CHECK(slot >= 0);
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        ++script->begin_count;
    } else {
        ++script->resume_count;
    }
    ++script->callback_count;
    first_for_slot = script->slot_calls[slot] == 0U;
    if (first_for_slot) {
        script->slot_ids[slot] = transaction_id;
        script->slot_requests[slot] = *request;
        ++script->slot_calls[slot];
        if (slot == 1) {
            TI_C54X_TEST_CHECK(script->slot_calls[0] != 0U);
            TI_C54X_TEST_CHECK(transaction_id > script->slot_ids[0]);
        }
    } else {
        TI_C54X_TEST_CHECK(transaction_id == script->slot_ids[slot]);
        check_pair_request(request, &script->slot_requests[slot]);
        ++script->slot_calls[slot];
    }

    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = slot == 0 ? script->x_value : script->y_value,
        .bus_status = TI_C54X_BUS_OK};

    if ((script->mode == PAIR_SCRIPT_WAIT_X ||
         script->mode == PAIR_SCRIPT_CANCEL_FAILED) &&
        slot == 0 &&
        first_for_slot) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        observation->read_value_word = UINT16_C(0);
        return;
    }
    if (script->mode == PAIR_SCRIPT_WAIT_Y && slot == 1 &&
        first_for_slot) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        observation->read_value_word = UINT16_C(0);
        return;
    }
    if (script->mode == PAIR_SCRIPT_FAULT_X && slot == 0 &&
        first_for_slot) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->read_value_word = UINT16_C(0);
        observation->bus_status = TI_C54X_BUS_FAULT;
        return;
    }
    if (script->mode == PAIR_SCRIPT_FAULT_Y && slot == 1 &&
        first_for_slot) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->read_value_word = UINT16_C(0);
        observation->bus_status = TI_C54X_BUS_FAULT;
        return;
    }
    if ((script->mode == PAIR_SCRIPT_MALFORMED_X && slot == 0 &&
         first_for_slot) ||
        (script->mode == PAIR_SCRIPT_MALFORMED_Y && slot == 1 &&
         first_for_slot)) {
        observation->transaction_id = transaction_id + UINT64_C(1);
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        return;
    }
    if ((script->mode == PAIR_SCRIPT_STALE_X && slot == 0 &&
         first_for_slot) ||
        (script->mode == PAIR_SCRIPT_STALE_Y && slot == 1 &&
         first_for_slot)) {
        observation->transaction_id = transaction_id - UINT64_C(1);
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        return;
    }

    if (script->mode == PAIR_SCRIPT_MUTATE_LIVE_AR && slot == 0 &&
        first_for_slot) {
        /* The callback's host-side mutation is deliberately visible; the
         * root must reject before issuing Y or publishing PC/effects. */
        TI_C54X_TEST_CHECK(script->state != NULL);
        script->state->ar[5] = UINT16_C(0x3fff);
    }
    if (script->mode == PAIR_SCRIPT_MUTATE_CANDIDATE && slot == 0 &&
        first_for_slot) {
        TI_C54X_TEST_CHECK(script->transaction != NULL);
        script->transaction->candidate.effect.payload.memory.address_spec
            .pair.smem_arf = UINT8_C(4);
    }
    if (script->mode == PAIR_SCRIPT_MUTATE_SNAPSHOT_AND_LIVE_AR && slot == 0 &&
        first_for_slot) {
        TI_C54X_TEST_CHECK(script->transaction != NULL);
        TI_C54X_TEST_CHECK(script->state != NULL);
        script->transaction->candidate.dual_address_snapshot.x_request
            .address_word = UINT32_C(0x3fff);
        script->state->ar[5] = UINT16_C(0x3fff);
    }
}

static ti_c54x_bus_word16_cancel_result pair_script_cancel(
    void *context,
    uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request)
{
    pair_endpoint_script *script = (pair_endpoint_script *)context;
    const int slot = pair_request_slot(script, request);

    TI_C54X_TEST_CHECK(slot >= 0);
    TI_C54X_TEST_CHECK(script->slot_calls[slot] != 0U);
    TI_C54X_TEST_CHECK(transaction_id == script->slot_ids[slot]);
    check_pair_request(request, &script->slot_requests[slot]);
    script->cancelled_id = transaction_id;
    ++script->cancel_count;
    return script->mode == PAIR_SCRIPT_CANCEL_FAILED
               ? TI_C54X_BUS_WORD16_CANCEL_RESULT_FAILED
               : TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint
pair_endpoint_for(pair_endpoint_script *script)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = script,
        .advance = pair_script_advance,
        .cancel = pair_script_cancel};
    return endpoint;
}

static ti_c54x_op_status complete_pair_effect(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t x_word,
    uint16_t y_word,
    ti_c54x_op_effect *effect)
{
    ++pair_completion_calls;
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

/* Indirect MOD2 Smem leg (*AR5+) with the verbatim PROGRAM leg and
 * read-phase Smem T load. */
static ti_c54x_op_effect pair_memory_effect_indirect(void)
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
    effect.payload.memory.complete_dual = complete_pair_effect;
    return effect;
}

/* Direct page-0 Smem leg with the verbatim PROGRAM leg. */
static ti_c54x_op_effect pair_memory_effect_direct(void)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16;
    effect.payload.memory.address_spec.pair.indirect = UINT8_C(0);
    effect.payload.memory.address_spec.pair.smem_dma7 = UINT8_C(0x34);
    effect.payload.memory.address_spec.pair.program_pmad = UINT16_C(0x0200);
    effect.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    effect.payload.memory.value_word = UINT16_C(0);
    effect.payload.memory.load_t_with_read = true;
    effect.payload.memory.complete_dual = complete_pair_effect;
    return effect;
}

static void init_runtime(ti_c54x_accumulator_state *accumulators,
                         ti_c54x_t_register_state *t_register)
{
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(t_register) ==
                       TI_C54X_T_REGISTER_OK);
    pair_completion_calls = 0U;
}

static ti_c54x_memory_instruction_status prepare_pair(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_op_effect *effect,
    ti_c54x_arch_state *state,
    uint64_t attempted,
    uint64_t retired)
{
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = input_snapshot();

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, &instruction, &input, effect, state, attempted, retired);
}

static ti_c54x_memory_instruction_result pair_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    pair_endpoint_script *script,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register)
{
    const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(script);
    ti_c54x_memory_instruction_result result = {0};

    (void)ti_c54x_memory_instruction_transaction_advance(
        transaction, &endpoint, state, accumulators, t_register, &result);
    return result;
}

static pair_endpoint_script pair_script(pair_script_mode mode,
                                        ti_c54x_arch_state *state,
                                        ti_c54x_memory_instruction_transaction
                                            *transaction)
{
    return (pair_endpoint_script){
        .mode = mode,
        .state = state,
        .transaction = transaction,
        .x_address = UINT32_C(0x0100),
        .y_address = UINT32_C(0x0200),
        .x_value = UINT16_C(0x1111),
        .y_value = UINT16_C(0x2222)};
}

static void test_ready_pair_order_and_atomic_completion(void)
{
    const ti_c54x_op_effect effect = pair_memory_effect_indirect();
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint_script script;
    ti_c54x_memory_instruction_result result;
    uint64_t accumulator_value = 0;

    init_runtime(&accumulators, &t_register);
    script = pair_script(PAIR_SCRIPT_READY_BOTH, &state, &transaction);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(result.instructions_attempted, 1);
    TI_C54X_TEST_CHECK_U64(result.instructions_retired, 1);
    TI_C54X_TEST_CHECK_U64(result.transaction_id, 2);
    TI_C54X_TEST_CHECK(script.callback_count == 2U);
    TI_C54X_TEST_CHECK(script.begin_count == 2U);
    TI_C54X_TEST_CHECK(script.resume_count == 0U);
    TI_C54X_TEST_CHECK(script.slot_ids[0] == UINT64_C(1));
    TI_C54X_TEST_CHECK(script.slot_ids[1] == UINT64_C(2));
    /* Smem DATA issues before the PROGRAM coefficient. */
    TI_C54X_TEST_CHECK(script.slot_requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.slot_requests[0].address_word ==
                       UINT32_C(0x0100));
    TI_C54X_TEST_CHECK(script.slot_requests[1].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM);
    TI_C54X_TEST_CHECK(script.slot_requests[1].address_word ==
                       UINT32_C(0x0200));
    TI_C54X_TEST_CHECK(pair_completion_calls == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           &accumulator_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(accumulator_value, UINT64_C(0x11112222));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x21));
    /* MOD2 post-increments the Smem AR. */
    TI_C54X_TEST_CHECK_U64(state.ar[5], UINT16_C(0x0101));
    /* Read-phase T load takes the Smem word. */
    TI_C54X_TEST_CHECK(t_register.raw_value == UINT16_C(0x1111));
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(pair_completion_calls == 1U);
    TI_C54X_TEST_CHECK(script.callback_count == 2U);
}

static void test_direct_leg_retires_with_no_ar_update(void)
{
    const ti_c54x_op_effect effect = pair_memory_effect_direct();
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint_script script;
    ti_c54x_memory_instruction_result result;
    uint64_t accumulator_value = 0;

    init_runtime(&accumulators, &t_register);
    script = pair_script(PAIR_SCRIPT_READY_BOTH, &state, &transaction);
    script.x_address = UINT32_C(0x0034);
    script.x_value = UINT16_C(0x3333);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.callback_count == 2U);
    TI_C54X_TEST_CHECK(pair_completion_calls == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           &accumulator_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(accumulator_value, UINT64_C(0x33332222));
    TI_C54X_TEST_CHECK_U64(state.ar[5], UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(t_register.raw_value == UINT16_C(0x3333));
}

static void test_wait_x_and_wait_y_never_reissue_or_publish_early(void)
{
    const ti_c54x_op_effect effect = pair_memory_effect_indirect();
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint_script script;
    ti_c54x_memory_instruction_result result;
    uint64_t accumulator_value = 0;

    init_runtime(&accumulators, &t_register);
    script = pair_script(PAIR_SCRIPT_WAIT_X, &state, &transaction);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state, 4, 2) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK_U64(result.instructions_attempted, 1);
    TI_C54X_TEST_CHECK_U64(result.instructions_retired, 0);
    TI_C54X_TEST_CHECK(script.callback_count == 1U);
    TI_C54X_TEST_CHECK(pair_completion_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &accumulators, TI_C54X_ACCUMULATOR_A,
                           &accumulator_value) == TI_C54X_ACCUMULATOR_UNKNOWN);
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.callback_count == 3U);
    TI_C54X_TEST_CHECK(script.slot_calls[0] == 2U);
    TI_C54X_TEST_CHECK(script.slot_calls[1] == 1U);
    TI_C54X_TEST_CHECK(script.slot_ids[1] > script.slot_ids[0]);
    TI_C54X_TEST_CHECK(pair_completion_calls == 1U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = known_arch_state();
    before = state;
    init_runtime(&accumulators, &t_register);
    script = pair_script(PAIR_SCRIPT_WAIT_Y, &state, &transaction);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(script.callback_count == 2U);
    TI_C54X_TEST_CHECK(script.slot_calls[0] == 1U);
    TI_C54X_TEST_CHECK(script.slot_calls[1] == 1U);
    TI_C54X_TEST_CHECK(pair_completion_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.callback_count == 3U);
    TI_C54X_TEST_CHECK(script.slot_calls[1] == 2U);
    TI_C54X_TEST_CHECK(pair_completion_calls == 1U);
}

static void test_faults_and_cancellation_publish_nothing(void)
{
    const ti_c54x_op_effect effect = pair_memory_effect_indirect();
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint_script script;
    ti_c54x_memory_instruction_result result;

    init_runtime(&accumulators, &t_register);
    script = pair_script(PAIR_SCRIPT_FAULT_X, &state, &transaction);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state, 2, 1) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(script.callback_count == 1U);
    TI_C54X_TEST_CHECK(script.slot_calls[1] == 0U);
    TI_C54X_TEST_CHECK(pair_completion_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = known_arch_state();
    before = state;
    init_runtime(&accumulators, &t_register);
    script = pair_script(PAIR_SCRIPT_FAULT_Y, &state, &transaction);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state, 2, 1) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(script.callback_count == 2U);
    TI_C54X_TEST_CHECK(script.slot_calls[0] == 1U);
    TI_C54X_TEST_CHECK(script.slot_calls[1] == 1U);
    TI_C54X_TEST_CHECK(pair_completion_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = known_arch_state();
    init_runtime(&accumulators, &t_register);
    script = pair_script(PAIR_SCRIPT_WAIT_X, &state, &transaction);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    {
        const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
        const ti_c54x_bus_word16_request request_before =
            transaction.data_program_bus_transaction.word16_transaction.request;
        const uint64_t id_before = transaction.data_program_bus_transaction
                                       .word16_transaction.transaction_id;

        transaction.data_program_bus_transaction.word16_transaction.request
            .address_word = UINT32_C(0x7777);
        TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                               &transaction, &endpoint) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(script.cancel_count == 0U);
        transaction.data_program_bus_transaction.word16_transaction.request =
            request_before;
        transaction.data_program_bus_transaction.word16_transaction
            .transaction_id = id_before + UINT64_C(4);
        TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                               &transaction, &endpoint) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(script.cancel_count == 0U);
        transaction.data_program_bus_transaction.word16_transaction
            .transaction_id = id_before;
        TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                               &transaction, &endpoint) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    }
    TI_C54X_TEST_CHECK(script.cancel_count == 1U);
    TI_C54X_TEST_CHECK(script.cancelled_id == script.slot_ids[0]);
    TI_C54X_TEST_CHECK(pair_completion_calls == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = known_arch_state();
    init_runtime(&accumulators, &t_register);
    script = pair_script(PAIR_SCRIPT_WAIT_Y, &state, &transaction);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    {
        const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
        TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                               &transaction, &endpoint) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    }
    TI_C54X_TEST_CHECK(script.cancel_count == 1U);
    TI_C54X_TEST_CHECK(script.cancelled_id == script.slot_ids[1]);
    TI_C54X_TEST_CHECK(pair_completion_calls == 0U);
}

static void test_malformed_and_stale_observations_are_not_publication(void)
{
    const ti_c54x_op_effect effect = pair_memory_effect_indirect();
    const pair_script_mode modes[] = {PAIR_SCRIPT_MALFORMED_X,
                                      PAIR_SCRIPT_MALFORMED_Y,
                                      PAIR_SCRIPT_STALE_X,
                                      PAIR_SCRIPT_STALE_Y};
    size_t i;

    for (i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        ti_c54x_memory_instruction_transaction transaction = {0};
        ti_c54x_arch_state state = known_arch_state();
        ti_c54x_arch_state before = state;
        ti_c54x_accumulator_state accumulators = {0};
        ti_c54x_t_register_state t_register = {0};
        pair_endpoint_script script;
        ti_c54x_memory_instruction_result result;

        init_runtime(&accumulators, &t_register);
        script = pair_script(modes[i], &state, &transaction);
        TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state, 0, 0) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
        result = pair_advance(&transaction, &script, &state, &accumulators,
                              &t_register);
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION);
        TI_C54X_TEST_CHECK(pair_completion_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
        {
            const ti_c54x_bus_word16_endpoint endpoint =
                pair_endpoint_for(&script);
            TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                                   &transaction, &endpoint) ==
                               TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
        }
    }
}

static void test_cancel_failure_and_id_exhaustion_are_transactional(void)
{
    const ti_c54x_op_effect effect = pair_memory_effect_indirect();
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    const ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint_script script;
    ti_c54x_memory_instruction_result result;
    ti_c54x_bus_word16_endpoint endpoint;

    init_runtime(&accumulators, &t_register);
    script = pair_script(PAIR_SCRIPT_CANCEL_FAILED, &state, &transaction);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    endpoint = pair_endpoint_for(&script);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED);
    TI_C54X_TEST_CHECK(transaction.phase ==
                       TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    script.mode = PAIR_SCRIPT_WAIT_X;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    transaction.last_transaction_id = UINT64_MAX - UINT64_C(1);
    state = known_arch_state();
    script = pair_script(PAIR_SCRIPT_READY_BOTH, &state, &transaction);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state, 0, 0) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = pair_advance(&transaction, &script, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED);
    TI_C54X_TEST_CHECK(script.callback_count == 0U);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id ==
                       UINT64_MAX - UINT64_C(1));
    TI_C54X_TEST_CHECK(transaction.phase ==
                       TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED);
}

static void test_live_dependency_mutations_reject_before_y(void)
{
    const pair_script_mode modes[] = {PAIR_SCRIPT_MUTATE_LIVE_AR,
                                      PAIR_SCRIPT_MUTATE_CANDIDATE,
                                      PAIR_SCRIPT_MUTATE_SNAPSHOT_AND_LIVE_AR};
    size_t i;

    for (i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        const ti_c54x_op_effect effect = pair_memory_effect_indirect();
        ti_c54x_memory_instruction_transaction transaction = {0};
        ti_c54x_arch_state state = known_arch_state();
        const uint32_t pc_before = state.pc_word;
        ti_c54x_accumulator_state accumulators = {0};
        ti_c54x_t_register_state t_register = {0};
        pair_endpoint_script script;
        ti_c54x_memory_instruction_result result;

        init_runtime(&accumulators, &t_register);
        script = pair_script(modes[i], &state, &transaction);
        TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state, 0, 0) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
        result = pair_advance(&transaction, &script, &state, &accumulators,
                              &t_register);
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
        TI_C54X_TEST_CHECK(script.callback_count == 1U);
        TI_C54X_TEST_CHECK(script.slot_calls[1] == 0U);
        TI_C54X_TEST_CHECK(pair_completion_calls == 0U);
        TI_C54X_TEST_CHECK(state.pc_word == pc_before);
    }
}

int main(void)
{
    test_ready_pair_order_and_atomic_completion();
    test_direct_leg_retires_with_no_ar_update();
    test_wait_x_and_wait_y_never_reissue_or_publish_early();
    test_faults_and_cancellation_publish_nothing();
    test_malformed_and_stale_observations_are_not_publication();
    test_cancel_failure_and_id_exhaustion_are_transactional();
    test_live_dependency_mutations_reject_before_y();
    puts("data_program_memory_instruction_transaction: ok");
    return 0;
}
