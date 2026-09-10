/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proofs for the stack-pop WORD16
 * root transaction. It checks DATA READ issue at old SP, WAIT/READY
 * retirement with atomic SP-plus-TOS-target commit, FAULT rollback,
 * stale-SP rejection, and the SP=0xFFFF wrap pin. The future RET row
 * binds the real catalog identity in W66B; this seam test synthesizes
 * the one-word bundle directly and consumes no catalog identity.
 * Does not own: opcode dispatch, catalog, endpoint implementation, timing,
 * observer policy, or CPU lifecycle.
 */

#include "bus/transaction/word16_transaction.h"
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

#include <stddef.h>
#include <stdio.h>

typedef struct pop_endpoint_script {
    int fault;
    int wait_once;
    bool waited;
    uint16_t read_value;
    ti_c54x_bus_word16_request request;
    unsigned calls;
} pop_endpoint_script;

static void
pop_endpoint_advance(void *context,
                     ti_c54x_bus_word16_advance_phase phase,
                     uint64_t transaction_id,
                     const ti_c54x_bus_word16_request *request,
                     ti_c54x_bus_word16_observation *observation)
{
    pop_endpoint_script *script = (pop_endpoint_script *)context;

    ++script->calls;
    script->request = *request;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    if (script->fault) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
        return;
    }
    if (script->wait_once && !script->waited &&
        phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        script->waited = true;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        return;
    }
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
    observation->read_value_word = script->read_value;
}

static ti_c54x_bus_word16_cancel_result
pop_endpoint_cancel(void *context, uint64_t transaction_id,
                    const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint
pop_endpoint_for(pop_endpoint_script *script)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = script,
        .advance = pop_endpoint_advance,
        .cancel = pop_endpoint_cancel};
    return endpoint;
}

static ti_c54x_memory_instruction_result
pop_advance(ti_c54x_memory_instruction_transaction *transaction,
            pop_endpoint_script *script, ti_c54x_arch_state *state,
            ti_c54x_accumulator_state *accumulators,
            ti_c54x_t_register_state *t_register)
{
    const ti_c54x_bus_word16_endpoint endpoint = pop_endpoint_for(script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           transaction, &endpoint, state, accumulators,
                           t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    return result;
}

static ti_c54x_decoded_instruction
pop_instruction(void)
{
    const ti_c54x_decoded_instruction instruction = {
        /* No catalog identity exists in the seam slice; W66B binds RET. */
        .id = TI_C54X_OPCODE_ID_UNKNOWN,
        .operand_layout = TI_C54X_OPERAND_LAYOUT_NONE,
        .repeatability = TI_C54X_REPEATABILITY_INELIGIBLE,
        .input_resources = TI_C54X_INPUT_RESOURCES_STACK_POINTER,
        .bundle = {.program_words = {UINT16_C(0xFC00)},
                   .program_word_count = UINT8_C(1)}};
    return instruction;
}

static ti_c54x_op_effect
pop_effect(void)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16;
    effect.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    effect.payload.memory.value_word = UINT16_C(0);
    effect.payload.memory.rmw_operation =
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE;
    effect.payload.memory.complete = NULL;
    return effect;
}

static ti_c54x_arch_state
pop_state(uint16_t sp_value, uint32_t pc_value)
{
    ti_c54x_arch_state state = {0};

    /* CMPT/ARP/CPL stay unknown: success proves the root stages the pop
     * without mode reads. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&state, sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    state.pc_word = pc_value;
    return state;
}

static ti_c54x_memory_instruction_status
prepare_pop(ti_c54x_memory_instruction_transaction *transaction,
            const ti_c54x_decoded_instruction *instruction,
            const ti_c54x_op_effect *effect, ti_c54x_arch_state *state)
{
    const ti_c54x_op_input_snapshot input = {0};

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, instruction, &input, effect, state, UINT64_C(0),
        UINT64_C(0));
}

static void
check_frozen_pop(const ti_c54x_memory_instruction_transaction *transaction,
                 uint16_t expected_address)
{
    TI_C54X_TEST_CHECK(
        transaction->candidate.address_resolution.request.space ==
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(
        transaction->candidate.address_resolution.request.access ==
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK_U64(
        transaction->candidate.address_resolution.request.address_word,
        expected_address);
    TI_C54X_TEST_CHECK(transaction->candidate.address_resolution
                           .address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_SP);
    TI_C54X_TEST_CHECK_U64(transaction->candidate.address_resolution
                               .address_commit.value_after,
                           (uint16_t)(expected_address + UINT16_C(1)));
}

static void
test_prepare_freezes_read(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction = pop_instruction();
    const ti_c54x_op_effect effect = pop_effect();
    ti_c54x_arch_state state = pop_state(UINT16_C(0x0300), UINT32_C(0x2112));

    TI_C54X_TEST_CHECK(prepare_pop(&transaction, &instruction, &effect,
                                   &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    /* The TOS READ issues at old SP. */
    check_frozen_pop(&transaction, UINT16_C(0x0300));
}

static void
test_full_wait_ready_cycle(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction = pop_instruction();
    const ti_c54x_op_effect effect = pop_effect();
    ti_c54x_arch_state state = pop_state(UINT16_C(0x0300), UINT32_C(0x2112));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pop_endpoint_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.wait_once = 1;
    script.read_value = UINT16_C(0x1000);
    TI_C54X_TEST_CHECK(prepare_pop(&transaction, &instruction, &effect,
                                   &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = pop_advance(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    /* WAIT publishes nothing: SP and PC hold while the bus is pending. */
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x0300));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x2112));
    result = pop_advance(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* Atomic commit: SP incremented, PC redirected to the TOS value. */
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x0301));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(script.request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK_U64(script.request.address_word, UINT16_C(0x0300));
}

static void
test_fault_rolls_back(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction = pop_instruction();
    const ti_c54x_op_effect effect = pop_effect();
    ti_c54x_arch_state state = pop_state(UINT16_C(0x0300), UINT32_C(0x2112));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pop_endpoint_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.fault = 1;
    TI_C54X_TEST_CHECK(prepare_pop(&transaction, &instruction, &effect,
                                   &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = pop_advance(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x0300));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x2112));
}

static void
test_stale_sp_rejected(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction = pop_instruction();
    const ti_c54x_op_effect effect = pop_effect();
    ti_c54x_arch_state state = pop_state(UINT16_C(0x0300), UINT32_C(0x2112));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pop_endpoint_script script = {0};
    const ti_c54x_bus_word16_endpoint endpoint = pop_endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    script.read_value = UINT16_C(0x1000);
    TI_C54X_TEST_CHECK(prepare_pop(&transaction, &instruction, &effect,
                                   &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&state, UINT16_C(0x0400)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
}

static void
test_wrap_sp_ffff(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction = pop_instruction();
    const ti_c54x_op_effect effect = pop_effect();
    ti_c54x_arch_state state = pop_state(UINT16_C(0xffff), UINT32_C(0x2112));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pop_endpoint_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.wait_once = 1;
    script.read_value = UINT16_C(0x00a5);
    TI_C54X_TEST_CHECK(prepare_pop(&transaction, &instruction, &effect,
                                   &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    check_frozen_pop(&transaction, UINT16_C(0xffff));
    result = pop_advance(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    result = pop_advance(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x0000));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x00a5));
    TI_C54X_TEST_CHECK_U64(script.request.address_word, UINT16_C(0xffff));
}

int
main(void)
{
    test_prepare_freezes_read();
    test_full_wait_ready_cycle();
    test_fault_rolls_back();
    test_stale_sp_rejected();
    test_wrap_sp_ffff();
    puts("stack_pop_root: ok");
    return 0;
}
