#ifndef TI_C54X_TEST_MEMORY_INSTRUCTION_FIXTURE_H
#define TI_C54X_TEST_MEMORY_INSTRUCTION_FIXTURE_H

/*
 * Module contract
 *
 * Owns: shared private endpoint scripts and value seeds for root transaction
 * tests. Does not own: production behavior, CPU lifecycle, or architecture.
 */

#include "isa/decode/opcode_decode.h"
#include "isa/ops/memory/memory_effect.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "state/accumulator/accumulator.h"
#include "state/arch_state.h"
#include "state/temporary/t_register.h"
#include "../../../support/test_harness.h"

#include <ti_c54x/bus_endpoint.h>

#include <stdint.h>

typedef enum script_mode {
    SCRIPT_WAIT_THEN_READ_READY = 0,
    SCRIPT_WAIT_THEN_WRITE_READY = 1,
    SCRIPT_FAULT = 2,
    SCRIPT_MALFORMED = 3
} script_mode;

typedef struct endpoint_script {
    script_mode mode;
    unsigned begin_calls;
    unsigned resume_calls;
    unsigned cancel_calls;
    uint64_t id;
    ti_c54x_bus_word16_request request;
    uint16_t read_value_word;
} endpoint_script;

static void check_request_equal(const ti_c54x_bus_word16_request *actual,
                                const ti_c54x_bus_word16_request *expected)
{
    TI_C54X_TEST_CHECK(actual->space == expected->space);
    TI_C54X_TEST_CHECK(actual->access == expected->access);
    TI_C54X_TEST_CHECK(actual->address_word == expected->address_word);
    TI_C54X_TEST_CHECK(actual->write_value_word == expected->write_value_word);
}

static void script_advance(void *context,
                           ti_c54x_bus_word16_advance_phase phase,
                           uint64_t transaction_id,
                           const ti_c54x_bus_word16_request *request,
                           ti_c54x_bus_word16_observation *observation)
{
    endpoint_script *script = (endpoint_script *)context;

    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        ++script->begin_calls;
    } else {
        ++script->resume_calls;
    }
    if (script->id == UINT64_C(0)) {
        script->id = transaction_id;
        script->request = *request;
    } else {
        TI_C54X_TEST_CHECK(script->id == transaction_id);
        check_request_equal(&script->request, request);
    }
    *observation = (ti_c54x_bus_word16_observation){0};
    if (script->mode == SCRIPT_MALFORMED) {
        observation->transaction_id = transaction_id + UINT64_C(1);
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        return;
    }
    if (script->mode == SCRIPT_FAULT) {
        observation->transaction_id = transaction_id;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
        return;
    }
    observation->transaction_id = transaction_id;
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        return;
    }
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        observation->read_value_word = script->read_value_word;
    }
}

static ti_c54x_bus_word16_cancel_result
script_cancel(void *context, uint64_t transaction_id,
              const ti_c54x_bus_word16_request *request)
{
    endpoint_script *script = (endpoint_script *)context;

    ++script->cancel_calls;
    TI_C54X_TEST_CHECK(script->id == transaction_id);
    check_request_equal(&script->request, request);
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint endpoint_for(endpoint_script *script)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = script, .advance = script_advance, .cancel = script_cancel};
    return endpoint;
}

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
                           &state, UINT8_C(2), UINT16_C(0x2110)) ==
                       TI_C54X_ARCH_STATE_OK);
    state.pc_word = UINT32_C(0x20);
    return state;
}

static ti_c54x_decoded_instruction decoded_nop(void)
{
    const ti_c54x_decoded_instruction instruction = {
        .id = TI_C54X_OPCODE_ID_NOP,
        .operand_layout = TI_C54X_OPERAND_LAYOUT_NONE,
        .repeatability = TI_C54X_REPEATABILITY_UNRESOLVED,
        .input_resources = TI_C54X_INPUT_RESOURCES_NONE,
        .bundle = {.program_words = {UINT16_C(0xF495), UINT16_C(0)},
                   .program_word_count = UINT8_C(1)}};
    return instruction;
}

static ti_c54x_op_status
complete_read(const ti_c54x_decoded_instruction *instruction,
              const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,
              ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    TI_C54X_TEST_CHECK(instruction != NULL);
    TI_C54X_TEST_CHECK(input != NULL);
    TI_C54X_TEST_CHECK(effect != NULL);
    TI_C54X_TEST_CHECK(instruction->id == TI_C54X_OPCODE_ID_NOP);
    TI_C54X_TEST_CHECK(instruction->bundle.program_words[0] ==
                       UINT16_C(0xF495));
    TI_C54X_TEST_CHECK(instruction->bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(input->accumulator_a_raw40 == UINT64_C(0x1122));
    TI_C54X_TEST_CHECK(input->accumulator_b_raw40 == UINT64_C(0x3344));
    TI_C54X_TEST_CHECK(input->status_sxm == UINT8_C(0));
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = TI_C54X_ACCUMULATOR_A;
    staged.payload.accumulator_write.value_raw40 = read_value_word;
    staged.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_effect memory_effect(ti_c54x_bus_word16_request_access access)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
    effect.payload.memory.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD2;
    effect.payload.memory.address_spec.arf = UINT8_C(2);
    effect.payload.memory.access = access;
    effect.payload.memory.write_value_word =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ? UINT16_C(0x00a5)
                                                          : UINT16_C(0);
    effect.payload.memory.complete =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ? complete_read : NULL;
    return effect;
}

static ti_c54x_op_input_snapshot input_snapshot(void)
{
    const ti_c54x_op_input_snapshot input = {
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
    return input;
}

static ti_c54x_memory_instruction_status
prepare(ti_c54x_memory_instruction_transaction *transaction,
        const ti_c54x_op_effect *effect, ti_c54x_arch_state *state,
        uint64_t attempted, uint64_t retired)
{
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = input_snapshot();

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, &instruction, &input, effect, state, attempted, retired);
}

static ti_c54x_memory_instruction_result
advance(ti_c54x_memory_instruction_transaction *transaction,
        endpoint_script *script, ti_c54x_arch_state *state,
        ti_c54x_accumulator_state *accumulators,
        ti_c54x_t_register_state *t_register)
{
    const ti_c54x_bus_word16_endpoint endpoint = endpoint_for(script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           transaction, &endpoint, state, accumulators,
                           t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    return result;
}

#endif
