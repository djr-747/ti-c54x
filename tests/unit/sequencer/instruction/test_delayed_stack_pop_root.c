/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proofs for the delayed
 * stack-pop WORD16 root transaction. It checks DATA READ issue at
 * old SP, WAIT/READY retirement with atomic SP commit plus
 * sequential-PC advance and TOS-as-deferred-target reporting,
 * FAULT rollback with a disarmed report, stale-SP rejection, the
 * SP=0xFFFF wrap pin, forged-spec rejection, and that immediate
 * pops retire disarmed. The future RETD row binds the real catalog
 * identity in W67B; this seam test synthesizes the bundle directly
 * and consumes no catalog identity.
 * Does not own: opcode dispatch, catalog, endpoint implementation,
 * timing, observer policy, or CPU lifecycle.
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

typedef struct delayed_pop_script {
    int fault;
    int wait_once;
    bool waited;
    uint16_t read_value;
    ti_c54x_bus_word16_request request;
    unsigned calls;
} delayed_pop_script;

static void
delayed_pop_advance(void *context,
                    ti_c54x_bus_word16_advance_phase phase,
                    uint64_t transaction_id,
                    const ti_c54x_bus_word16_request *request,
                    ti_c54x_bus_word16_observation *observation)
{
    delayed_pop_script *script = (delayed_pop_script *)context;

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
delayed_pop_cancel(void *context, uint64_t transaction_id,
                   const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint
delayed_pop_endpoint_for(delayed_pop_script *script)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = script,
        .advance = delayed_pop_advance,
        .cancel = delayed_pop_cancel};
    return endpoint;
}

static ti_c54x_memory_instruction_result
delayed_pop_go(ti_c54x_memory_instruction_transaction *transaction,
               delayed_pop_script *script, ti_c54x_arch_state *state,
               ti_c54x_accumulator_state *accumulators,
               ti_c54x_t_register_state *t_register)
{
    const ti_c54x_bus_word16_endpoint endpoint =
        delayed_pop_endpoint_for(script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           transaction, &endpoint, state, accumulators,
                           t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    return result;
}

/* Placeholder first word for the future W67B row; the seam consumes
 * no catalog identity (UNKNOWN id). */
static const uint16_t DELAYED_POP_WORD0 = UINT16_C(0xFE00);

static ti_c54x_decoded_instruction
delayed_pop_instruction(void)
{
    const ti_c54x_decoded_instruction instruction = {
        .id = TI_C54X_OPCODE_ID_UNKNOWN,
        .operand_layout = TI_C54X_OPERAND_LAYOUT_NONE,
        .repeatability = TI_C54X_REPEATABILITY_INELIGIBLE,
        .input_resources = TI_C54X_INPUT_RESOURCES_STACK_POINTER,
        .bundle = {.program_words = {DELAYED_POP_WORD0},
                   .program_word_count = UINT8_C(1)}};
    return instruction;
}

static ti_c54x_op_effect
delayed_pop_effect(void)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16;
    effect.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    effect.payload.memory.value_word = UINT16_C(0);
    effect.payload.memory.rmw_operation =
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE;
    effect.payload.memory.complete = NULL;
    return effect;
}

static ti_c54x_arch_state
delayed_pop_state(uint16_t sp_value, uint32_t pc_value)
{
    ti_c54x_arch_state state = {0};

    /* CMPT/ARP/CPL stay unknown: success proves the root stages the
     * delayed pop without mode reads. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&state, sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    state.pc_word = pc_value;
    return state;
}

static ti_c54x_memory_instruction_status
prepare_delayed_pop(ti_c54x_memory_instruction_transaction *transaction,
                    const ti_c54x_decoded_instruction *instruction,
                    const ti_c54x_op_effect *effect,
                    ti_c54x_arch_state *state)
{
    const ti_c54x_op_input_snapshot input = {0};

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, instruction, &input, effect, state, UINT64_C(0),
        UINT64_C(0));
}

static void
test_prepare_freezes_read(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction =
        delayed_pop_instruction();
    const ti_c54x_op_effect effect = delayed_pop_effect();
    ti_c54x_arch_state state =
        delayed_pop_state(UINT16_C(0x0300), UINT32_C(0x2112));

    TI_C54X_TEST_CHECK(prepare_delayed_pop(&transaction, &instruction,
                                           &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    /* The TOS READ issues at old SP, exactly like an immediate pop. */
    TI_C54X_TEST_CHECK(
        transaction.candidate.address_resolution.request.space ==
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(
        transaction.candidate.address_resolution.request.access ==
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK_U64(
        transaction.candidate.address_resolution.request.address_word,
        UINT16_C(0x0300));
    TI_C54X_TEST_CHECK(transaction.candidate.address_resolution
                           .address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_SP);
    TI_C54X_TEST_CHECK_U64(transaction.candidate.address_resolution
                               .address_commit.value_after,
                           UINT16_C(0x0301));
}

static void
test_wait_ready_defers_pc(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction =
        delayed_pop_instruction();
    const ti_c54x_op_effect effect = delayed_pop_effect();
    ti_c54x_arch_state state =
        delayed_pop_state(UINT16_C(0x0300), UINT32_C(0x2112));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delayed_pop_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.wait_once = 1;
    script.read_value = UINT16_C(0x1000);
    TI_C54X_TEST_CHECK(prepare_delayed_pop(&transaction, &instruction,
                                           &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delayed_pop_go(&transaction, &script, &state, &accumulators,
                            &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x0300));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x2112));
    TI_C54X_TEST_CHECK(!result.deferred_branch_armed);
    result = delayed_pop_go(&transaction, &script, &state, &accumulators,
                            &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* Atomic stack commit with sequential PC: SP incremented, PC
     * advanced to the one-word successor, TOS reported deferred. */
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x0301));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x2113));
    TI_C54X_TEST_CHECK(result.deferred_branch_armed);
    TI_C54X_TEST_CHECK_U64(result.deferred_branch_pc_word,
                           UINT16_C(0x1000));
    TI_C54X_TEST_CHECK_U64(script.request.address_word, UINT16_C(0x0300));
}

static void
test_fault_rolls_back_disarmed(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction =
        delayed_pop_instruction();
    const ti_c54x_op_effect effect = delayed_pop_effect();
    ti_c54x_arch_state state =
        delayed_pop_state(UINT16_C(0x0300), UINT32_C(0x2112));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delayed_pop_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.fault = 1;
    TI_C54X_TEST_CHECK(prepare_delayed_pop(&transaction, &instruction,
                                           &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delayed_pop_go(&transaction, &script, &state, &accumulators,
                            &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x0300));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x2112));
    TI_C54X_TEST_CHECK(!result.deferred_branch_armed);
}

static void
test_stale_sp_rejected(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction =
        delayed_pop_instruction();
    const ti_c54x_op_effect effect = delayed_pop_effect();
    ti_c54x_arch_state state =
        delayed_pop_state(UINT16_C(0x0300), UINT32_C(0x2112));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delayed_pop_script script = {0};
    const ti_c54x_bus_word16_endpoint endpoint =
        delayed_pop_endpoint_for(&script);
    ti_c54x_memory_instruction_result result = {0};

    script.read_value = UINT16_C(0x1000);
    TI_C54X_TEST_CHECK(prepare_delayed_pop(&transaction, &instruction,
                                           &effect, &state) ==
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
    const ti_c54x_decoded_instruction instruction =
        delayed_pop_instruction();
    const ti_c54x_op_effect effect = delayed_pop_effect();
    ti_c54x_arch_state state =
        delayed_pop_state(UINT16_C(0xffff), UINT32_C(0x2112));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delayed_pop_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.read_value = UINT16_C(0x00a5);
    TI_C54X_TEST_CHECK(prepare_delayed_pop(&transaction, &instruction,
                                           &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delayed_pop_go(&transaction, &script, &state, &accumulators,
                            &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x0000));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x2113));
    TI_C54X_TEST_CHECK(result.deferred_branch_armed);
    TI_C54X_TEST_CHECK_U64(result.deferred_branch_pc_word,
                           UINT16_C(0x00a5));
}

static void
test_forged_spec_rejected(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction =
        delayed_pop_instruction();
    ti_c54x_op_effect effect = delayed_pop_effect();
    ti_c54x_arch_state state =
        delayed_pop_state(UINT16_C(0x0300), UINT32_C(0x2112));

    /* Delayed pops share the all-zero spec contract: any AR
     * selector is forged. */
    effect.payload.memory.address_spec.arf = UINT8_C(1);
    TI_C54X_TEST_CHECK(prepare_delayed_pop(&transaction, &instruction,
                                           &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
}

static void
test_immediate_pop_retires_disarmed(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction =
        delayed_pop_instruction();
    ti_c54x_op_effect effect = delayed_pop_effect();
    ti_c54x_arch_state state =
        delayed_pop_state(UINT16_C(0x0300), UINT32_C(0x2112));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delayed_pop_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    /* The immediate kind keeps its redirecting commit and never arms
     * the deferred report. */
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16;
    script.read_value = UINT16_C(0x1000);
    TI_C54X_TEST_CHECK(prepare_delayed_pop(&transaction, &instruction,
                                           &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delayed_pop_go(&transaction, &script, &state, &accumulators,
                            &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(!result.deferred_branch_armed);
    TI_C54X_TEST_CHECK_U64(result.deferred_branch_pc_word, UINT16_C(0));
}

int
main(void)
{
    test_prepare_freezes_read();
    test_wait_ready_defers_pc();
    test_fault_rolls_back_disarmed();
    test_stale_sp_rejected();
    test_wrap_sp_ffff();
    test_forged_spec_rejected();
    test_immediate_pop_retires_disarmed();
    puts("delayed_stack_pop_root: ok");
    return 0;
}
