/*
 * Module contract
 *
 * Owns: executable W51B RED contracts for the dual-indirect WORD16 root
 * transaction.  The tests pin persistence of both address commits across
 * WAIT, immutable mode/selector/address validation, atomic distinct-AR
 * publication, Xmod precedence for aliases, non-publication on WAIT/FAULT,
 * and the frozen root carrier sizes.
 * Does not own: production sequencing, address arithmetic, opcode dispatch,
 * CPU lifecycle, endpoint storage, timing, or architectural policy.
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
#include <stdio.h>
#include <string.h>

typedef enum w51b_script_mode {
    W51B_READY = 0,
    W51B_WAIT_X = 1,
    W51B_WAIT_Y = 2,
    W51B_FAULT_X = 3,
    W51B_FAULT_Y = 4
} w51b_script_mode;

typedef struct w51b_script {
    w51b_script_mode mode;
    uint32_t x_address;
    uint32_t y_address;
    uint16_t x_value;
    uint16_t y_value;
    unsigned callback_count;
    unsigned slot_calls[2];
    uint64_t slot_ids[2];
    ti_c54x_bus_word16_request slot_requests[2];
} w51b_script;

static unsigned completion_calls;

static ti_c54x_arch_state known_state(void)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(7)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2110)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x3220)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(4), UINT16_C(0x4330)) ==
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

static ti_c54x_op_status complete_dual(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word, uint16_t y_word,
    ti_c54x_op_effect *effect)
{
    TI_C54X_TEST_CHECK(instruction != NULL);
    TI_C54X_TEST_CHECK(input != NULL);
    TI_C54X_TEST_CHECK(effect != NULL);
    ++completion_calls;
    *effect = (ti_c54x_op_effect){0};
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

static ti_c54x_op_effect dual_effect(uint8_t x_mode, uint8_t x_ar,
                                     uint8_t y_mode, uint8_t y_ar)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16;
    effect.payload.memory.address_spec.dual.x_mode = x_mode;
    effect.payload.memory.address_spec.dual.x_ar = x_ar;
    effect.payload.memory.address_spec.dual.y_mode = y_mode;
    effect.payload.memory.address_spec.dual.y_ar = y_ar;
    effect.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    effect.payload.memory.complete_dual = complete_dual;
    return effect;
}

static int slot_for(const w51b_script *script,
                    const ti_c54x_bus_word16_request *request)
{
    if (request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        request->write_value_word != UINT16_C(0)) {
        return -1;
    }
    if (script->x_address == script->y_address) {
        return script->slot_calls[0] == 0U ? 0 : 1;
    }
    if (request->address_word == script->x_address) {
        return 0;
    }
    if (request->address_word == script->y_address) {
        return 1;
    }
    return -1;
}

static void w51b_advance(void *context,
                         ti_c54x_bus_word16_advance_phase phase,
                         uint64_t transaction_id,
                         const ti_c54x_bus_word16_request *request,
                         ti_c54x_bus_word16_observation *observation)
{
    w51b_script *script = (w51b_script *)context;
    const int slot = slot_for(script, request);
    const bool first = slot >= 0 && script->slot_calls[slot] == 0U;

    TI_C54X_TEST_CHECK(slot >= 0);
    (void)phase;
    ++script->callback_count;
    if (first) {
        script->slot_ids[slot] = transaction_id;
        script->slot_requests[slot] = *request;
    } else {
        TI_C54X_TEST_CHECK(transaction_id == script->slot_ids[slot]);
        TI_C54X_TEST_CHECK(memcmp(request, &script->slot_requests[slot],
                                  sizeof(*request)) == 0);
    }
    ++script->slot_calls[slot];
    if (slot == 1) {
        TI_C54X_TEST_CHECK(script->slot_calls[0] != 0U);
        TI_C54X_TEST_CHECK(transaction_id > script->slot_ids[0]);
    }
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = slot == 0 ? script->x_value : script->y_value,
        .bus_status = TI_C54X_BUS_OK};
    if ((script->mode == W51B_WAIT_X && slot == 0 && first) ||
        (script->mode == W51B_WAIT_Y && slot == 1 && first)) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        observation->read_value_word = UINT16_C(0);
    } else if ((script->mode == W51B_FAULT_X && slot == 0 && first) ||
               (script->mode == W51B_FAULT_Y && slot == 1 && first)) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->read_value_word = UINT16_C(0);
        observation->bus_status = TI_C54X_BUS_FAULT;
    }
}

static ti_c54x_bus_word16_cancel_result
w51b_cancel(void *context, uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request)
{
    w51b_script *script = (w51b_script *)context;
    const int slot = slot_for(script, request);

    TI_C54X_TEST_CHECK(slot >= 0);
    TI_C54X_TEST_CHECK(script->slot_ids[slot] == transaction_id);
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint endpoint_for(w51b_script *script)
{
    return (ti_c54x_bus_word16_endpoint){
        .context = script, .advance = w51b_advance, .cancel = w51b_cancel};
}

static ti_c54x_memory_instruction_status
prepare_dual(ti_c54x_memory_instruction_transaction *transaction,
             const ti_c54x_op_effect *effect, ti_c54x_arch_state *state)
{
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = input_snapshot();

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, &instruction, &input, effect, state, UINT64_C(0),
        UINT64_C(0));
}

static ti_c54x_memory_instruction_result
advance_dual(ti_c54x_memory_instruction_transaction *transaction,
             w51b_script *script, ti_c54x_arch_state *state,
             ti_c54x_accumulator_state *accumulators,
             ti_c54x_t_register_state *t_register)
{
    ti_c54x_memory_instruction_result result = {0};
    const ti_c54x_bus_word16_endpoint endpoint = endpoint_for(script);

    (void)ti_c54x_memory_instruction_transaction_advance(
        transaction, &endpoint, state, accumulators, t_register, &result);
    return result;
}

static void init_runtime(ti_c54x_accumulator_state *accumulators,
                         ti_c54x_t_register_state *t_register)
{
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(t_register) ==
                       TI_C54X_T_REGISTER_OK);
    completion_calls = 0U;
}

static w51b_script script(w51b_script_mode mode)
{
    return (w51b_script){.mode = mode,
                          .x_address = UINT32_C(0x2110),
                          .y_address = UINT32_C(0x3220),
                          .x_value = UINT16_C(0x1111),
                          .y_value = UINT16_C(0x2222)};
}

static void test_sizes_and_distinct_ar_atomic_commit(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_op_effect effect = dual_effect(1, 2, 2, 3);
    ti_c54x_arch_state state = known_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    w51b_script io = script(W51B_READY);
    ti_c54x_memory_instruction_result result;
    uint64_t value = 0;

    TI_C54X_TEST_CHECK(sizeof(ti_c54x_memory_instruction_transaction) ==
                       (sizeof(void *) == 8U ? 360U : 328U));
    TI_C54X_TEST_CHECK(sizeof(ti_c54x_dual_memory_address_snapshot) == 52U);
    init_runtime(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare_dual(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance_dual(&transaction, &io, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(io.callback_count == 2U);
    TI_C54X_TEST_CHECK(completion_calls == 1U);
    TI_C54X_TEST_CHECK_U64(state.ar[2], UINT16_C(0x210f));
    TI_C54X_TEST_CHECK_U64(state.ar[3], UINT16_C(0x3221));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &accumulators, TI_C54X_ACCUMULATOR_A, &value) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT64_C(0x11112222));
}

static void test_wait_persists_both_commits_until_y_ready(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_op_effect effect = dual_effect(1, 2, 2, 3);
    ti_c54x_arch_state state = known_state();
    const ti_c54x_arch_state before = state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    w51b_script io = script(W51B_WAIT_Y);
    ti_c54x_memory_instruction_result result;
    uint64_t value = 0;

    init_runtime(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare_dual(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance_dual(&transaction, &io, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(completion_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &accumulators, TI_C54X_ACCUMULATOR_A, &value) ==
                       TI_C54X_ACCUMULATOR_UNKNOWN);
    result = advance_dual(&transaction, &io, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(state.ar[2], UINT16_C(0x210f));
    TI_C54X_TEST_CHECK_U64(state.ar[3], UINT16_C(0x3221));
}

static void test_same_ar_commits_once_with_xmod_precedence(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_op_effect effect = dual_effect(1, 2, 2, 2);
    ti_c54x_arch_state state = known_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    w51b_script io = script(W51B_READY);
    ti_c54x_memory_instruction_result result;

    io.y_address = io.x_address;
    io.y_value = io.x_value;
    init_runtime(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare_dual(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance_dual(&transaction, &io, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(io.callback_count == 2U);
    TI_C54X_TEST_CHECK_U64(state.ar[2], UINT16_C(0x210f));
}

static void test_wait_and_fault_never_publish_partial_updates(void)
{
    const w51b_script_mode modes[] = {W51B_FAULT_X, W51B_FAULT_Y};
    size_t i;

    for (i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        ti_c54x_memory_instruction_transaction transaction = {0};
        const ti_c54x_op_effect effect = dual_effect(1, 2, 2, 3);
        ti_c54x_arch_state state = known_state();
        const ti_c54x_arch_state before = state;
        ti_c54x_accumulator_state accumulators = {0};
        ti_c54x_t_register_state t_register = {0};
        w51b_script io = script(modes[i]);
        ti_c54x_memory_instruction_result result;

        init_runtime(&accumulators, &t_register);
        TI_C54X_TEST_CHECK(prepare_dual(&transaction, &effect, &state) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
        result = advance_dual(&transaction, &io, &state, &accumulators,
                              &t_register);
        TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
        TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
        TI_C54X_TEST_CHECK(completion_calls == 0U);
    }
}

static void test_mode_selector_address_and_live_state_drift_reject(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_op_effect effect = dual_effect(1, 2, 2, 3);
    ti_c54x_arch_state state = known_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    w51b_script io = script(W51B_WAIT_X);
    ti_c54x_memory_instruction_result result;

    init_runtime(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(prepare_dual(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = advance_dual(&transaction, &io, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    transaction.candidate.effect.payload.memory.address_spec.dual.x_mode =
        UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2111)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = advance_dual(&transaction, &io, &state, &accumulators,
                          &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(io.callback_count == 1U);
    TI_C54X_TEST_CHECK(completion_calls == 0U);
}

int main(void)
{
    test_sizes_and_distinct_ar_atomic_commit();
    test_wait_persists_both_commits_until_y_ready();
    test_same_ar_commits_once_with_xmod_precedence();
    test_wait_and_fault_never_publish_partial_updates();
    test_mode_selector_address_and_live_state_drift_reject();
    puts("dual_memory_instruction_w51b_contract: ok");
    return 0;
}
