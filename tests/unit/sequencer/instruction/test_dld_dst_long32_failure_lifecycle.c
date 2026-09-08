/*
 * Module contract
 *
 * Owns: RED contracts for root DLD/DST MOD13 LONG32 composition: ordered
 * child issues, copied values, WAIT/FAULT/cancel identity, stale observation
 * rejection, partial write visibility, and deferred architectural commit.
 * Does not own: opcode decode/catalog coverage, the private pair reducer,
 * endpoint implementation, timing, or diagnostics.
 */

#include "test_memory_instruction_fixture.h"

#include <string.h>

typedef enum endpoint_mode {
    ENDPOINT_READY = 0,
    ENDPOINT_WAIT_THEN_READY = 1,
    ENDPOINT_FAULT_FIRST = 2,
    ENDPOINT_FAULT_SECOND = 3,
    ENDPOINT_MALFORMED = 4,
    ENDPOINT_CANCEL_FAILED = 5,
    ENDPOINT_MALFORMED_SECOND = 6
} endpoint_mode;

typedef struct pair_endpoint {
    endpoint_mode mode;
    unsigned issue_count;
    unsigned callback_count;
    unsigned cancel_count;
    uint64_t ids[2];
    ti_c54x_bus_word16_request requests[2];
    uint16_t read_values[2];
} pair_endpoint;

static unsigned completion_calls;
static uint16_t completion_msw;
static uint16_t completion_lsw;
static ti_c54x_arch_state *mutate_after_first_ready_state;
static ti_c54x_memory_instruction_transaction *
    mutate_after_first_ready_candidate;
static ti_c54x_memory_instruction_transaction *
    mutate_after_first_ready_watermark;
static ti_c54x_arch_state *mutate_after_second_ready_state;
static ti_c54x_memory_instruction_transaction *
    mutate_after_second_ready_watermark;

static ti_c54x_op_status complete_long32(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t first_msw,
    uint16_t second_lsw,
    ti_c54x_op_effect *next_effect)
{
    TI_C54X_TEST_CHECK(instruction != NULL);
    TI_C54X_TEST_CHECK(input != NULL);
    TI_C54X_TEST_CHECK(next_effect != NULL);
    completion_msw = first_msw;
    completion_lsw = second_lsw;
    ++completion_calls;
    *next_effect = (ti_c54x_op_effect){0};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_effect long32_effect(
    ti_c54x_bus_word16_request_access access)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32;
    effect.payload.memory.address_spec.arf = UINT8_C(2);
    effect.payload.memory.address_spec.long_offset_word = UINT16_C(2);
    effect.payload.memory.access = access;
    effect.payload.memory.first_value_msw =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ? UINT16_C(0x1234)
                                                          : UINT16_C(0);
    effect.payload.memory.second_value_lsw =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ? UINT16_C(0x5678)
                                                          : UINT16_C(0);
    effect.payload.memory.complete_long32 =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ? complete_long32
                                                         : NULL;
    return effect;
}

static void endpoint_advance(
    void *context, ti_c54x_bus_word16_advance_phase phase,
    uint64_t transaction_id, const ti_c54x_bus_word16_request *request,
    ti_c54x_bus_word16_observation *observation)
{
    pair_endpoint *endpoint = (pair_endpoint *)context;
    unsigned slot;

    ++endpoint->callback_count;
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        slot = endpoint->issue_count++;
        TI_C54X_TEST_CHECK(slot < 2U);
        endpoint->ids[slot] = transaction_id;
        endpoint->requests[slot] = *request;
    } else {
        TI_C54X_TEST_CHECK(endpoint->issue_count > 0U);
        slot = endpoint->issue_count - 1U;
        TI_C54X_TEST_CHECK(endpoint->ids[slot] == transaction_id);
        TI_C54X_TEST_CHECK(memcmp(&endpoint->requests[slot], request,
                                   sizeof(*request)) == 0);
    }

    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    if (endpoint->mode == ENDPOINT_MALFORMED) {
        observation->transaction_id = transaction_id + UINT64_C(1);
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        return;
    }
    if (endpoint->mode == ENDPOINT_MALFORMED_SECOND && slot == 1U) {
        observation->transaction_id = transaction_id + UINT64_C(1);
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        return;
    }
    if ((endpoint->mode == ENDPOINT_FAULT_FIRST && slot == 0U) ||
        (endpoint->mode == ENDPOINT_FAULT_SECOND && slot == 1U)) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
        return;
    }
    if ((endpoint->mode == ENDPOINT_WAIT_THEN_READY ||
         endpoint->mode == ENDPOINT_CANCEL_FAILED) &&
        phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        return;
    }
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        observation->read_value_word = endpoint->read_values[slot];
    }
    if (slot == 0U && phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN &&
        mutate_after_first_ready_state != NULL) {
        mutate_after_first_ready_state->ar[2] ^= UINT16_C(1);
        mutate_after_first_ready_state = NULL;
    }
    if (slot == 0U && phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN &&
        mutate_after_first_ready_candidate != NULL) {
        mutate_after_first_ready_candidate->candidate.effect.payload.memory
            .second_value_lsw ^= UINT16_C(1);
        mutate_after_first_ready_candidate = NULL;
    }
    if (slot == 0U && phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN &&
        mutate_after_first_ready_watermark != NULL) {
        mutate_after_first_ready_watermark->last_transaction_id +=
            UINT64_C(1);
        mutate_after_first_ready_watermark = NULL;
    }
    if (slot == 1U && phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN &&
        mutate_after_second_ready_state != NULL) {
        mutate_after_second_ready_state->ar[2] ^= UINT16_C(1);
        mutate_after_second_ready_state = NULL;
    }
    if (slot == 1U && phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN &&
        mutate_after_second_ready_watermark != NULL) {
        mutate_after_second_ready_watermark->last_transaction_id +=
            UINT64_C(1);
        mutate_after_second_ready_watermark = NULL;
    }
}

static ti_c54x_bus_word16_cancel_result endpoint_cancel(
    void *context, uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request)
{
    pair_endpoint *endpoint = (pair_endpoint *)context;
    const unsigned slot = endpoint->issue_count - 1U;

    ++endpoint->cancel_count;
    TI_C54X_TEST_CHECK(slot < 2U);
    TI_C54X_TEST_CHECK(endpoint->ids[slot] == transaction_id);
    TI_C54X_TEST_CHECK(memcmp(&endpoint->requests[slot], request,
                               sizeof(*request)) == 0);
    return endpoint->mode == ENDPOINT_CANCEL_FAILED
               ? TI_C54X_BUS_WORD16_CANCEL_RESULT_FAILED
               : TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint pair_endpoint_for(pair_endpoint *script)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = script, .advance = endpoint_advance, .cancel = endpoint_cancel};
    return endpoint;
}

static ti_c54x_memory_instruction_status prepare_long32(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_op_effect *effect, ti_c54x_arch_state *state)
{
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = input_snapshot();

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, &instruction, &input, effect, state, 0U, 0U);
}

static void reset_capture(void)
{
    completion_calls = 0U;
    completion_msw = 0U;
    completion_lsw = 0U;
    mutate_after_first_ready_state = NULL;
    mutate_after_first_ready_candidate = NULL;
    mutate_after_first_ready_watermark = NULL;
    mutate_after_second_ready_state = NULL;
    mutate_after_second_ready_watermark = NULL;
}

/* The shared fixture also exports WORD16-only helpers.  Keep this RED test
 * linked against its state/instruction snapshots without tripping the
 * repository's -Werror unused-static-function gate. */
static void touch_word16_fixture_helpers(void)
{
    (void)memory_effect;
    (void)prepare;
    (void)advance;
}
static void test_immediate_read_orders_children_and_commits_once(void)
{
    ti_c54x_op_effect effect =
        long32_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint script = {ENDPOINT_READY, 0U, 0U, 0U, {0}, {{0}},
                            {UINT16_C(0x1111), UINT16_C(0x2222)}};
    const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    reset_capture();
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.child_kind != 0);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.issue_count == 2U);
    TI_C54X_TEST_CHECK(script.ids[0] != 0U && script.ids[1] == script.ids[0] + 1U);
    TI_C54X_TEST_CHECK(result.transaction_id == script.ids[1]);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == script.ids[1]);
    TI_C54X_TEST_CHECK(script.requests[0].address_word == UINT32_C(0x2112));
    TI_C54X_TEST_CHECK(script.requests[1].address_word == UINT32_C(0x2113));
    TI_C54X_TEST_CHECK(completion_calls == 1U);
    TI_C54X_TEST_CHECK(completion_msw == UINT16_C(0x1111));
    TI_C54X_TEST_CHECK(completion_lsw == UINT16_C(0x2222));
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(state.ar[2] == UINT16_C(0x2112));
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x21));
    TI_C54X_TEST_CHECK(transaction.long32_bus_transaction.state ==
                       TI_C54X_LONG32_PAIR_STATE_COMPLETE);
    TI_C54X_TEST_CHECK(transaction.long32_bus_transaction.staged_bus_status ==
                       TI_C54X_BUS_OK);
}

static void test_write_copies_values_and_faults_after_first_ready(void)
{
    ti_c54x_op_effect effect =
        long32_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    const ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint script = {ENDPOINT_FAULT_SECOND, 0U, 0U, 0U, {0}, {{0}}, {0}};
    const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(script.issue_count == 2U);
    TI_C54X_TEST_CHECK(script.requests[0].write_value_word == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(script.requests[1].write_value_word == UINT16_C(0x5678));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(transaction.long32_bus_transaction.state ==
                       TI_C54X_LONG32_PAIR_STATE_FAULT);
    TI_C54X_TEST_CHECK(state.ar[2] == before.ar[2]);
    TI_C54X_TEST_CHECK(state.pc_word == before.pc_word);
}

static void test_wait_reuses_child_and_stale_or_malformed_never_reissues(void)
{
    ti_c54x_op_effect effect =
        long32_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint script = {ENDPOINT_WAIT_THEN_READY, 0U, 0U, 0U, {0}, {{0}},
                            {UINT16_C(0x1111), UINT16_C(0x2222)}};
    const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(result.transaction_id == script.ids[0]);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(script.issue_count == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(result.transaction_id == script.ids[1]);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(script.issue_count == 2U);
    TI_C54X_TEST_CHECK(script.callback_count == 3U);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(result.transaction_id == script.ids[1]);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(script.issue_count == 2U);
    TI_C54X_TEST_CHECK(script.callback_count == 4U);
}

static void test_cancel_lanes_and_failed_cancel_preserve_bytes(void)
{
    ti_c54x_op_effect effect =
        long32_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint script = {ENDPOINT_WAIT_THEN_READY, 0U, 0U, 0U, {0}, {{0}},
                            {UINT16_C(0x1111), UINT16_C(0x2222)}};
    const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};
    ti_c54x_memory_instruction_transaction before;

    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    before = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(transaction.phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_IDLE);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == before.last_transaction_id);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    before = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == before.last_transaction_id);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    script = (pair_endpoint){ENDPOINT_CANCEL_FAILED, 0U, 0U, 0U, {0}, {{0}},
                             {UINT16_C(0x1111), UINT16_C(0x2222)}};
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state,
                           &accumulators, &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    before = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &before, sizeof(transaction)) == 0);
}

static void test_malformed_stale_and_candidate_rejection_is_transactional(void)
{
    ti_c54x_op_effect effect =
        long32_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before_state = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint script = {ENDPOINT_MALFORMED, 0U, 0U, 0U, {0}, {{0}}, {0}};
    const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION);
    TI_C54X_TEST_CHECK(script.issue_count == 1U);
    TI_C54X_TEST_CHECK(transaction.phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(memcmp(&state, &before_state, sizeof(state)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(transaction.phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_IDLE);
    TI_C54X_TEST_CHECK(script.cancel_count == 1U);

    script = (pair_endpoint){ENDPOINT_READY, 0U, 0U, 0U, {0}, {{0}}, {0}};
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.child_kind = 99;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state,
                           &accumulators, &t_register, &result) !=
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(script.issue_count == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.candidate.address_resolution.request.address_word ^= 1U;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state,
                           &accumulators, &t_register, &result) !=
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(script.issue_count == 0U);
}

static void test_ready_dependency_edit_blocks_second_issue(void)
{
    ti_c54x_op_effect effect =
        long32_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint script = {ENDPOINT_READY, 0U, 0U, 0U, {0}, {{0}},
                            {UINT16_C(0x1111), UINT16_C(0x2222)}};
    const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    reset_capture();
    mutate_after_first_ready_state = &state;
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.issue_count == 1U);
    TI_C54X_TEST_CHECK(completion_calls == 0U);
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x20));
}

static void run_staged_mutation_case(bool after_second, bool watermark_edit)
{
    ti_c54x_op_effect effect =
        long32_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint script = {ENDPOINT_READY, 0U, 0U, 0U, {0}, {{0}},
                            {UINT16_C(0x1111), UINT16_C(0x2222)}};
    const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};
    const ti_c54x_arch_state before_state = state;
    uint64_t frozen_watermark;
    reset_capture();
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    if (after_second) {
        if (watermark_edit) {
            mutate_after_second_ready_watermark = &transaction;
        } else {
            mutate_after_second_ready_state = &state;
        }
    } else if (watermark_edit) {
        mutate_after_first_ready_watermark = &transaction;
    } else {
        mutate_after_first_ready_candidate = &transaction;
    }
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    frozen_watermark = script.ids[after_second ? 1U : 0U] +
                       (after_second ? UINT64_C(0) : UINT64_C(1));
    TI_C54X_TEST_CHECK(script.issue_count == (after_second ? 2U : 1U));
    TI_C54X_TEST_CHECK(completion_calls == 0U);
    TI_C54X_TEST_CHECK(state.pc_word == before_state.pc_word);
    TI_C54X_TEST_CHECK(
        after_second && !watermark_edit
            ? state.ar[2] == (uint16_t)(before_state.ar[2] ^ UINT16_C(1))
            : memcmp(&state, &before_state, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(transaction.phase ==
                       TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT);
    TI_C54X_TEST_CHECK(transaction.long32_bus_transaction.state ==
                       TI_C54X_LONG32_PAIR_STATE_IDLE);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) == TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == frozen_watermark);
}
static void test_staged_mutations_abandon_and_reset(void)
{
    run_staged_mutation_case(false, false);
    run_staged_mutation_case(false, true);
    run_staged_mutation_case(true, false);
    run_staged_mutation_case(true, true);
}
static void test_second_word_malformed_is_retryable_without_reissue(void)
{
    ti_c54x_op_effect effect =
        long32_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint script = {ENDPOINT_MALFORMED_SECOND, 0U, 0U, 0U, {0},
                            {{0}}, {UINT16_C(0x1111), UINT16_C(0x2222)}};
    const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    reset_capture();
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION);
    TI_C54X_TEST_CHECK(transaction.phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING);
    TI_C54X_TEST_CHECK(script.issue_count == 2U);
    TI_C54X_TEST_CHECK(result.transaction_id == script.ids[1]);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == script.ids[1]);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));

    script.mode = ENDPOINT_READY;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.issue_count == 2U);
    TI_C54X_TEST_CHECK(script.callback_count == 3U);
    TI_C54X_TEST_CHECK(completion_calls == 1U);
}

static void test_reset_preserves_two_id_watermark_for_fresh_pair(void)
{
    ti_c54x_op_effect effect =
        long32_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint first = {ENDPOINT_READY, 0U, 0U, 0U, {0}, {{0}},
                           {UINT16_C(0x1111), UINT16_C(0x2222)}};
    pair_endpoint second = {ENDPOINT_READY, 0U, 0U, 0U, {0}, {{0}},
                            {UINT16_C(0x3333), UINT16_C(0x4444)}};
    const ti_c54x_bus_word16_endpoint first_endpoint = pair_endpoint_for(&first);
    const ti_c54x_bus_word16_endpoint second_endpoint = pair_endpoint_for(&second);
    ti_c54x_memory_instruction_result result = {0};
    uint64_t watermark;

    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &first_endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    watermark = transaction.last_transaction_id;
    TI_C54X_TEST_CHECK(watermark == first.ids[1]);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(&transaction) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &second_endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(second.ids[0] == watermark + UINT64_C(1));
    TI_C54X_TEST_CHECK(second.ids[1] == watermark + UINT64_C(2));
}

static void test_dependency_edits_and_id_exhaustion_happen_before_issue(void)
{
    ti_c54x_op_effect effect =
        long32_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint script = {ENDPOINT_READY, 0U, 0U, 0U, {0}, {{0}}, {0}};
    const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.last_transaction_id = UINT64_MAX;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state,
                           &accumulators, &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED);
    TI_C54X_TEST_CHECK(script.issue_count == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    script = (pair_endpoint){ENDPOINT_READY, 0U, 0U, 0U, {0}, {{0}}, {0}};
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.last_transaction_id = UINT64_MAX - UINT64_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state,
                           &accumulators, &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED);
    TI_C54X_TEST_CHECK(script.issue_count == 0U);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    script = (pair_endpoint){ENDPOINT_READY, 0U, 0U, 0U, {0}, {{0}}, {0}};
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.last_transaction_id = UINT64_MAX - UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state,
                           &accumulators, &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.issue_count == 2U);
    TI_C54X_TEST_CHECK(script.ids[0] == UINT64_MAX - UINT64_C(1));
    TI_C54X_TEST_CHECK(script.ids[1] == UINT64_MAX);
    TI_C54X_TEST_CHECK(result.transaction_id == UINT64_MAX);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == UINT64_MAX);

    transaction = (ti_c54x_memory_instruction_transaction){0};
    script = (pair_endpoint){ENDPOINT_READY, 0U, 0U, 0U, {0}, {{0}}, {0}};
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    state.ar[2] ^= 1U;
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state,
                           &accumulators, &t_register, &result) !=
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(script.issue_count == 0U);
}

static void test_reset_prepared_and_terminal(void)
{
    ti_c54x_op_effect effect =
        long32_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    pair_endpoint script = {ENDPOINT_FAULT_FIRST, 0U, 0U, 0U, {0}, {{0}}, {0}};
    const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_IDLE);
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) == TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
}

static void test_first_fault_never_issues_second(void)
{
    ti_c54x_op_effect effect =
        long32_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_endpoint script = {ENDPOINT_FAULT_FIRST, 0U, 0U, 0U, {0}, {{0}}, {0}};
    const ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    reset_capture();
    TI_C54X_TEST_CHECK(prepare_long32(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(script.issue_count == 1U);
    TI_C54X_TEST_CHECK(completion_calls == 0U);
}

int main(void)
{
    touch_word16_fixture_helpers();
    test_immediate_read_orders_children_and_commits_once();
    test_write_copies_values_and_faults_after_first_ready();
    test_wait_reuses_child_and_stale_or_malformed_never_reissues();
    test_cancel_lanes_and_failed_cancel_preserve_bytes();
    test_malformed_stale_and_candidate_rejection_is_transactional();
    test_ready_dependency_edit_blocks_second_issue();
    test_staged_mutations_abandon_and_reset();
    test_second_word_malformed_is_retryable_without_reissue();
    test_reset_preserves_two_id_watermark_for_fresh_pair();
    test_dependency_edits_and_id_exhaustion_happen_before_issue();
    test_reset_prepared_and_terminal();
    test_first_fault_never_issues_second();
    return 0;
}
