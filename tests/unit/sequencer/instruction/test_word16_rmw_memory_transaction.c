/*
 * Module contract
 *
 * Owns: executable proofs for the allocation-free WORD16 same-address RMW
 * root transaction. It checks cached addressing, two fresh bus IDs, transform
 * cardinality, resumable phases, rollback, and direct stale snapshots.
 * Does not own: opcode dispatch, address arithmetic, endpoint implementation,
 * timing, observer policy, or CPU lifecycle.
 */

#include "test_memory_instruction_fixture.h"

#include <stddef.h>
#include <string.h>

typedef enum rmw_endpoint_outcome {
    RMW_ENDPOINT_READY = 0,
    RMW_ENDPOINT_WAIT = 1,
    RMW_ENDPOINT_FAULT = 2
} rmw_endpoint_outcome;

typedef struct rmw_endpoint_script {
    rmw_endpoint_outcome read_outcome;
    rmw_endpoint_outcome write_outcome;
    uint16_t read_value_word;
    unsigned calls;
    unsigned read_calls;
    unsigned write_calls;
    unsigned cancel_calls;
    ti_c54x_bus_word16_request requests[4];
    ti_c54x_bus_word16_advance_phase phases[4];
    uint64_t ids[4];
    ti_c54x_memory_instruction_transaction *mutate_transaction;
    unsigned mutate_on_write;
} rmw_endpoint_script;

static void
rmw_endpoint_advance(void *context,
                     ti_c54x_bus_word16_advance_phase phase,
                     uint64_t transaction_id,
                     const ti_c54x_bus_word16_request *request,
                     ti_c54x_bus_word16_observation *observation)
{
    rmw_endpoint_script *script = (rmw_endpoint_script *)context;
    unsigned ordinal = script->calls;
    unsigned same_access_calls;
    rmw_endpoint_outcome outcome;

    TI_C54X_TEST_CHECK(ordinal < 4U);
    script->requests[ordinal] = *request;
    script->phases[ordinal] = phase;
    script->ids[ordinal] = transaction_id;
    ++script->calls;
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        same_access_calls = script->read_calls++;
        outcome = script->read_outcome;
    } else {
        same_access_calls = script->write_calls++;
        outcome = script->write_outcome;
    }
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = UINT16_C(0),
        .bus_status = TI_C54X_BUS_OK};
    if (outcome == RMW_ENDPOINT_FAULT) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
    } else if (outcome == RMW_ENDPOINT_WAIT && same_access_calls == 0U) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
    } else if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        observation->read_value_word = script->read_value_word;
    }
    if (observation->kind == TI_C54X_BUS_WORD16_OBSERVATION_READY &&
        script->mutate_transaction != NULL) {
        if ((request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
             script->mutate_on_write == 0U) ||
            (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
             script->mutate_on_write != 0U)) {
            if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
                script->mutate_transaction->candidate.effect.payload.memory
                    .value_word ^= UINT16_C(1);
            } else {
                script->mutate_transaction->candidate.address_resolution
                    .address_commit.value_after ^= UINT16_C(1);
            }
        }
    }
}

static ti_c54x_bus_word16_cancel_result
rmw_endpoint_cancel(void *context,
                    uint64_t transaction_id,
                    const ti_c54x_bus_word16_request *request)
{
    rmw_endpoint_script *script = (rmw_endpoint_script *)context;

    ++script->cancel_calls;
    TI_C54X_TEST_CHECK(script->calls > 0U);
    TI_C54X_TEST_CHECK(script->ids[script->calls - 1U] == transaction_id);
    TI_C54X_TEST_CHECK(script->requests[script->calls - 1U].access ==
                       request->access);
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint
rmw_endpoint_for(rmw_endpoint_script *script)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = script,
        .advance = rmw_endpoint_advance,
        .cancel = rmw_endpoint_cancel};
    return endpoint;
}

static ti_c54x_op_effect
rmw_memory_effect(ti_c54x_op_memory_rmw_operation operation,
                  uint16_t value_word)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
    effect.payload.memory.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD2;
    effect.payload.memory.address_spec.arf = UINT8_C(2);
    effect.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    effect.payload.memory.value_word = value_word;
    effect.payload.memory.rmw_operation = (uint8_t)operation;
    effect.payload.memory.complete = NULL;
    return effect;
}

static ti_c54x_memory_instruction_status
prepare_rmw(ti_c54x_memory_instruction_transaction *transaction,
            const ti_c54x_op_effect *effect,
            ti_c54x_arch_state *state)
{
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = input_snapshot();

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, &instruction, &input, effect, state, UINT64_C(0),
        UINT64_C(0));
}

static ti_c54x_memory_instruction_result
advance_rmw(ti_c54x_memory_instruction_transaction *transaction,
            rmw_endpoint_script *script,
            ti_c54x_arch_state *state,
            ti_c54x_accumulator_state *accumulators,
            ti_c54x_t_register_state *t_register)
{
    const ti_c54x_bus_word16_endpoint endpoint = rmw_endpoint_for(script);
    ti_c54x_memory_instruction_result result = {0};
    const ti_c54x_memory_instruction_status status =
        ti_c54x_memory_instruction_transaction_advance(
            transaction, &endpoint, state, accumulators, t_register, &result);

    TI_C54X_TEST_CHECK(status == TI_C54X_MEMORY_INSTRUCTION_STATUS_OK ||
                       status ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE ||
                       status == TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED);
    return result;
}

static void
check_immediate_ready_uses_one_cached_address_and_two_ids(void)
{
    const ti_c54x_op_memory_rmw_operation operations[] = {
        TI_C54X_OP_MEMORY_RMW_OPERATION_AND,
        TI_C54X_OP_MEMORY_RMW_OPERATION_OR,
        TI_C54X_OP_MEMORY_RMW_OPERATION_XOR};
    const uint16_t expected_values[] = {UINT16_C(0x0000), UINT16_C(0xffff),
                                        UINT16_C(0xffff)};

    for (size_t index = 0; index < sizeof(operations) / sizeof(operations[0]);
         ++index) {
        ti_c54x_op_effect effect = rmw_memory_effect(
            operations[index], UINT16_C(0x0f0f));
        ti_c54x_memory_instruction_transaction transaction = {0};
        ti_c54x_arch_state state = known_arch_state();
        ti_c54x_arch_state before = state;
        ti_c54x_accumulator_state accumulators = {0};
        ti_c54x_t_register_state t_register = {0};
        rmw_endpoint_script script = {RMW_ENDPOINT_READY, RMW_ENDPOINT_READY,
                                       UINT16_C(0xf0f0), 0, 0, 0, 0, {{0}},
                                       {0}, {0}, NULL, 0U};
        ti_c54x_memory_instruction_result result;

        TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                           TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                           TI_C54X_T_REGISTER_OK);
        TI_C54X_TEST_CHECK(prepare_rmw(&transaction, &effect, &state) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
        result = advance_rmw(&transaction, &script, &state, &accumulators,
                             &t_register);
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
        TI_C54X_TEST_CHECK(script.calls == 2U);
        TI_C54X_TEST_CHECK(script.requests[0].access ==
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
        TI_C54X_TEST_CHECK(script.requests[1].access ==
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
        TI_C54X_TEST_CHECK(script.requests[0].address_word ==
                           script.requests[1].address_word);
        TI_C54X_TEST_CHECK(script.requests[1].write_value_word ==
                           expected_values[index]);
        TI_C54X_TEST_CHECK(script.ids[1] == script.ids[0] + UINT64_C(1));
        TI_C54X_TEST_CHECK(script.phases[0] ==
                           TI_C54X_BUS_WORD16_ADVANCE_BEGIN);
        TI_C54X_TEST_CHECK(script.phases[1] ==
                           TI_C54X_BUS_WORD16_ADVANCE_BEGIN);
        TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) != 0);
    }
}

static void
check_wait_on_read_then_write_applies_transform_once(void)
{
    const ti_c54x_op_effect effect = rmw_memory_effect(
        TI_C54X_OP_MEMORY_RMW_OPERATION_XOR, UINT16_C(0xffff));
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    rmw_endpoint_script script = {RMW_ENDPOINT_WAIT, RMW_ENDPOINT_READY,
                                   UINT16_C(0x1234), 0, 0, 0, 0, {{0}}, {0},
                                   {0}, NULL, 0U};
    ti_c54x_memory_instruction_result result;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_rmw(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance_rmw(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(script.calls == 1U);
    TI_C54X_TEST_CHECK(script.phases[0] ==
                       TI_C54X_BUS_WORD16_ADVANCE_BEGIN);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    result = advance_rmw(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.calls == 3U);
    TI_C54X_TEST_CHECK(script.read_calls == 2U);
    TI_C54X_TEST_CHECK(script.write_calls == 1U);
    TI_C54X_TEST_CHECK(script.phases[1] ==
                       TI_C54X_BUS_WORD16_ADVANCE_RESUME);
    TI_C54X_TEST_CHECK(script.phases[2] ==
                       TI_C54X_BUS_WORD16_ADVANCE_BEGIN);
    TI_C54X_TEST_CHECK(script.ids[2] == script.ids[0] + UINT64_C(1));
    TI_C54X_TEST_CHECK(script.requests[1].address_word ==
                       script.requests[2].address_word);
    TI_C54X_TEST_CHECK(script.requests[2].write_value_word == UINT16_C(0xedcb));
}

static void
check_write_wait_cancel_and_fault_preserve_state(void)
{
    const ti_c54x_op_effect effect = rmw_memory_effect(
        TI_C54X_OP_MEMORY_RMW_OPERATION_OR, UINT16_C(0x00f0));
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    rmw_endpoint_script script = {RMW_ENDPOINT_READY, RMW_ENDPOINT_WAIT,
                                   UINT16_C(0x000f), 0, 0, 0, 0, {{0}}, {0},
                                   {0}, NULL, 0U};
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_memory_instruction_result result;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_rmw(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance_rmw(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(script.calls == 2U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    result = advance_rmw(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.calls == 3U);
    TI_C54X_TEST_CHECK(script.phases[2] ==
                       TI_C54X_BUS_WORD16_ADVANCE_RESUME);
    TI_C54X_TEST_CHECK(script.ids[2] == script.ids[1]);
    TI_C54X_TEST_CHECK_U64(result.instructions_attempted, UINT64_C(0));
    TI_C54X_TEST_CHECK_U64(result.instructions_retired, UINT64_C(1));
    TI_C54X_TEST_CHECK_U64(state.ar[2], UINT16_C(0x2111));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x21));

    /* A separate read-pending cancellation must also leave the candidate
     * unpublished and must cancel only the in-flight READ. */
    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = known_arch_state();
    before = state;
    script = (rmw_endpoint_script){RMW_ENDPOINT_WAIT, RMW_ENDPOINT_READY,
                                   UINT16_C(0x000f), 0, 0, 0, 0, {{0}}, {0},
                                   {0}, NULL, 0U};
    TI_C54X_TEST_CHECK(prepare_rmw(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance_rmw(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    endpoint = rmw_endpoint_for(&script);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(script.cancel_calls == 1U);
    TI_C54X_TEST_CHECK(script.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    /* A WRITE fault after a successful READ must not publish AR or PC. */
    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = known_arch_state();
    before = state;
    script = (rmw_endpoint_script){RMW_ENDPOINT_READY, RMW_ENDPOINT_FAULT,
                                   UINT16_C(0x000f), 0, 0, 0, 0, {{0}}, {0},
                                   {0}, NULL, 0U};
    TI_C54X_TEST_CHECK(prepare_rmw(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance_rmw(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(script.calls == 2U);
    TI_C54X_TEST_CHECK(script.requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    /* Keep the write-pending cancellation assertion in this same lane. */
    transaction = (ti_c54x_memory_instruction_transaction){0};
    state = known_arch_state();
    before = state;
    script = (rmw_endpoint_script){RMW_ENDPOINT_READY, RMW_ENDPOINT_WAIT,
                                   UINT16_C(0x000f), 0, 0, 0, 0, {{0}}, {0},
                                   {0}, NULL, 0U};
    TI_C54X_TEST_CHECK(prepare_rmw(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance_rmw(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(script.calls == 2U);
    endpoint = rmw_endpoint_for(&script);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(script.cancel_calls == 1U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
check_ready_callback_mutation_is_rejected_before_write(void)
{
    const ti_c54x_op_effect effect = rmw_memory_effect(
        TI_C54X_OP_MEMORY_RMW_OPERATION_XOR, UINT16_C(0xffff));
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    rmw_endpoint_script script = {RMW_ENDPOINT_READY, RMW_ENDPOINT_READY,
                                   UINT16_C(0x1234), 0, 0, 0, 0, {{0}}, {0},
                                   {0}, NULL, 0U};
    ti_c54x_memory_instruction_result result;

    TI_C54X_TEST_CHECK(prepare_rmw(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    script.mutate_transaction = &transaction;
    result = advance_rmw(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.read_calls == 1U);
    TI_C54X_TEST_CHECK(script.write_calls == 0U);
    TI_C54X_TEST_CHECK(transaction.phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
check_write_ready_callback_mutation_is_rejected_before_commit(void)
{
    const ti_c54x_op_effect effect = rmw_memory_effect(
        TI_C54X_OP_MEMORY_RMW_OPERATION_XOR, UINT16_C(0xffff));
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    rmw_endpoint_script script = {RMW_ENDPOINT_READY, RMW_ENDPOINT_READY,
                                   UINT16_C(0x1234), 0, 0, 0, 0, {{0}}, {0},
                                   {0}, NULL, 1U};
    ti_c54x_memory_instruction_result result;

    TI_C54X_TEST_CHECK(prepare_rmw(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    script.mutate_transaction = &transaction;
    result = advance_rmw(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.read_calls == 1U);
    TI_C54X_TEST_CHECK(script.write_calls == 1U);
    TI_C54X_TEST_CHECK(transaction.phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
check_pending_candidate_forgery_rejected(void)
{
    typedef enum candidate_forgery {
        FORGE_EFFECT_KIND,
        FORGE_RMW_OPERATION,
        FORGE_RMW_VALUE,
        FORGE_EFFECT_CALLBACK,
        FORGE_REQUEST_ADDRESS,
        FORGE_PC_EFFECT,
        FORGE_INSTRUCTION_WORD_COUNT,
        FORGE_ADDRESS_UPDATE,
        FORGE_MOD1_UPDATE,
        FORGE_MOD0_UPDATE_AR_INDEX,
        FORGE_MOD0_UPDATE_VALUE_BEFORE,
        FORGE_MOD0_UPDATE_VALUE_AFTER,
        FORGE_ADDRESS_COMMIT
    } candidate_forgery;
    const candidate_forgery forgeries[] = {
        FORGE_EFFECT_KIND, FORGE_RMW_OPERATION, FORGE_RMW_VALUE,
        FORGE_EFFECT_CALLBACK, FORGE_REQUEST_ADDRESS, FORGE_PC_EFFECT,
        FORGE_INSTRUCTION_WORD_COUNT, FORGE_ADDRESS_UPDATE,
        FORGE_MOD1_UPDATE,
        FORGE_MOD0_UPDATE_AR_INDEX, FORGE_MOD0_UPDATE_VALUE_BEFORE,
        FORGE_MOD0_UPDATE_VALUE_AFTER,
        FORGE_ADDRESS_COMMIT};

    for (size_t index = 0; index < sizeof(forgeries) / sizeof(forgeries[0]);
         ++index) {
        ti_c54x_op_effect effect = rmw_memory_effect(
            TI_C54X_OP_MEMORY_RMW_OPERATION_XOR, UINT16_C(0xffff));
        ti_c54x_memory_instruction_transaction transaction = {0};
        ti_c54x_arch_state state = known_arch_state();
        ti_c54x_accumulator_state accumulators = {0};
        ti_c54x_t_register_state t_register = {0};
        rmw_endpoint_script script = {RMW_ENDPOINT_WAIT, RMW_ENDPOINT_READY,
                                       UINT16_C(0x1234), 0, 0, 0, 0, {{0}},
                                       {0}, {0}, NULL, 0U};
        ti_c54x_memory_instruction_result result;

        if (forgeries[index] == FORGE_MOD1_UPDATE) {
            effect.payload.memory.address_spec.mode =
                TI_C54X_INDIRECT_SINGLE_MOD1;
        } else if (forgeries[index] == FORGE_MOD0_UPDATE_AR_INDEX ||
                   forgeries[index] == FORGE_MOD0_UPDATE_VALUE_BEFORE ||
                   forgeries[index] == FORGE_MOD0_UPDATE_VALUE_AFTER) {
            effect.payload.memory.address_spec.mode =
                TI_C54X_INDIRECT_SINGLE_MOD0;
        }
        TI_C54X_TEST_CHECK(prepare_rmw(&transaction, &effect, &state) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
        result = advance_rmw(&transaction, &script, &state, &accumulators,
                             &t_register);
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
        switch (forgeries[index]) {
        case FORGE_EFFECT_KIND:
            transaction.candidate.effect.kind =
                TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
            break;
        case FORGE_RMW_OPERATION:
            transaction.candidate.effect.payload.memory.rmw_operation =
                TI_C54X_OP_MEMORY_RMW_OPERATION_NONE;
            break;
        case FORGE_RMW_VALUE:
            transaction.candidate.effect.payload.memory.value_word ^= 1U;
            break;
        case FORGE_EFFECT_CALLBACK:
            transaction.candidate.effect.payload.memory.complete = complete_read;
            break;
        case FORGE_REQUEST_ADDRESS:
            transaction.candidate.address_resolution.request.address_word ^= 1U;
            break;
        case FORGE_PC_EFFECT:
            transaction.candidate.pc_effect.public_pc_word_after ^= 1U;
            break;
        case FORGE_INSTRUCTION_WORD_COUNT:
            transaction.candidate.instruction.bundle.program_word_count =
                UINT8_C(2);
            break;
        case FORGE_ADDRESS_UPDATE:
            transaction.candidate.address_resolution.address_result.effect
                .update.value_after ^= 1U;
            break;
        case FORGE_MOD1_UPDATE:
            transaction.candidate.address_resolution.address_result.effect
                .update.value_after ^= 1U;
            break;
        case FORGE_MOD0_UPDATE_AR_INDEX:
            transaction.candidate.address_resolution.address_result.effect
                .update.ar_index = UINT8_C(1);
            break;
        case FORGE_MOD0_UPDATE_VALUE_BEFORE:
            transaction.candidate.address_resolution.address_result.effect
                .update.value_before = UINT16_C(1);
            break;
        case FORGE_MOD0_UPDATE_VALUE_AFTER:
            transaction.candidate.address_resolution.address_result.effect
                .update.value_after = UINT16_C(1);
            break;
        case FORGE_ADDRESS_COMMIT:
            transaction.candidate.address_resolution.address_commit.value_after ^=
                1U;
            break;
        }
        result = advance_rmw(&transaction, &script, &state, &accumulators,
                             &t_register);
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
        TI_C54X_TEST_CHECK(script.calls == 1U);
        TI_C54X_TEST_CHECK(transaction.phase ==
                           TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING);
    }
}

static void
check_second_id_exhaustion_fault_resets(void)
{
    const ti_c54x_op_effect effect = rmw_memory_effect(
        TI_C54X_OP_MEMORY_RMW_OPERATION_AND, UINT16_C(0xffff));
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    rmw_endpoint_script script = {RMW_ENDPOINT_READY, RMW_ENDPOINT_READY,
                                   UINT16_C(0xffff), 0, 0, 0, 0, {{0}}, {0},
                                   {0}, NULL, 0U};
    ti_c54x_memory_instruction_result result;
    ti_c54x_arch_state before = state;

    TI_C54X_TEST_CHECK(prepare_rmw(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction.last_transaction_id = UINT64_MAX - UINT64_C(1);
    result = advance_rmw(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED);
    TI_C54X_TEST_CHECK(script.read_calls == 1U);
    TI_C54X_TEST_CHECK(script.write_calls == 0U);
    TI_C54X_TEST_CHECK(transaction.phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_reset(
                           &transaction) == TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_IDLE);
    TI_C54X_TEST_CHECK(transaction.last_transaction_id == UINT64_MAX);
}

static void
check_direct_stale_snapshot_rejects_before_second_issue(void)
{
    const ti_c54x_op_effect effect = rmw_memory_effect(
        TI_C54X_OP_MEMORY_RMW_OPERATION_AND, UINT16_C(0xffff));
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    rmw_endpoint_script script = {RMW_ENDPOINT_WAIT, RMW_ENDPOINT_READY,
                                   UINT16_C(0xffff), 0, 0, 0, 0, {{0}}, {0},
                                   {0}, NULL, 0U};
    ti_c54x_memory_instruction_result result;
    ti_c54x_arch_state before;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(prepare_rmw(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance_rmw(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    result = advance_rmw(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.calls == 1U);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

int
main(void)
{
    /* Keep the shared fixture's ordinary-memory helpers available to sibling
     * RED lanes without making this focused translation unit depend on them. */
    (void)memory_effect;
    (void)prepare;
    (void)advance;
    check_immediate_ready_uses_one_cached_address_and_two_ids();
    check_wait_on_read_then_write_applies_transform_once();
    check_write_wait_cancel_and_fault_preserve_state();
    check_ready_callback_mutation_is_rejected_before_write();
    check_write_ready_callback_mutation_is_rejected_before_commit();
    check_pending_candidate_forgery_rejected();
    check_second_id_exhaustion_fault_resets();
    check_direct_stale_snapshot_rejects_before_second_issue();
    return 0;
}
