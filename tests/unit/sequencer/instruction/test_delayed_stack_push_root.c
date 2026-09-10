/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proofs for the delayed
 * stack-push WORD16 root transaction. It checks the PC+4
 * return-address staging at prepare, WAIT/READY retirement with
 * atomic SP commit plus sequential-PC advance and pmad-as-deferred
 * reporting, top-of-memory return rejection, forged-spec rejection,
 * and that immediate pushes retire disarmed. The future CALLD row
 * binds the real catalog identity in W67C; this seam test
 * synthesizes the bundle directly and consumes no catalog identity.
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

typedef struct delayed_push_script {
    int fault;
    int wait_once;
    bool waited;
    ti_c54x_bus_word16_request request;
    unsigned calls;
} delayed_push_script;

static void
delayed_push_advance(void *context,
                     ti_c54x_bus_word16_advance_phase phase,
                     uint64_t transaction_id,
                     const ti_c54x_bus_word16_request *request,
                     ti_c54x_bus_word16_observation *observation)
{
    delayed_push_script *script = (delayed_push_script *)context;

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
}

static ti_c54x_bus_word16_cancel_result
delayed_push_cancel(void *context, uint64_t transaction_id,
                    const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint
delayed_push_endpoint_for(delayed_push_script *script)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = script,
        .advance = delayed_push_advance,
        .cancel = delayed_push_cancel};
    return endpoint;
}

static ti_c54x_memory_instruction_result
delayed_push_go(ti_c54x_memory_instruction_transaction *transaction,
                delayed_push_script *script, ti_c54x_arch_state *state,
                ti_c54x_accumulator_state *accumulators,
                ti_c54x_t_register_state *t_register)
{
    const ti_c54x_bus_word16_endpoint endpoint =
        delayed_push_endpoint_for(script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           transaction, &endpoint, state, accumulators,
                           t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    return result;
}

/* Placeholder first word for the future W67C row; the seam consumes
 * no catalog identity (UNKNOWN id). */
static const uint16_t DELAYED_PUSH_WORD0 = UINT16_C(0xF274);

static ti_c54x_decoded_instruction
delayed_push_instruction(uint16_t pmad)
{
    const ti_c54x_decoded_instruction instruction = {
        .id = TI_C54X_OPCODE_ID_UNKNOWN,
        .operand_layout = TI_C54X_OPERAND_LAYOUT_NONE,
        .repeatability = TI_C54X_REPEATABILITY_INELIGIBLE,
        .input_resources = TI_C54X_INPUT_RESOURCES_STACK_POINTER,
        .bundle = {.program_words = {DELAYED_PUSH_WORD0, pmad},
                   .program_word_count = UINT8_C(2)}};
    return instruction;
}

static ti_c54x_op_effect
delayed_push_effect(void)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16;
    effect.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    /* The pure handler cannot know PC: the root stages the live PC+4
     * return address into the frozen copy at prepare. */
    effect.payload.memory.value_word = UINT16_C(0);
    effect.payload.memory.rmw_operation =
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE;
    effect.payload.memory.complete = NULL;
    return effect;
}

static ti_c54x_arch_state
delayed_push_state(uint16_t sp_value, uint32_t pc_value)
{
    ti_c54x_arch_state state = {0};

    /* CMPT/ARP/CPL stay unknown: success proves the root stages the
     * delayed push without mode reads. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&state, sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    state.pc_word = pc_value;
    return state;
}

static ti_c54x_memory_instruction_status
prepare_delayed_push(ti_c54x_memory_instruction_transaction *transaction,
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
test_prepare_stages_pc_plus_four(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction =
        delayed_push_instruction(UINT16_C(0x1000));
    const ti_c54x_op_effect effect = delayed_push_effect();
    ti_c54x_arch_state state =
        delayed_push_state(UINT16_C(0x1111), UINT32_C(0x0025));

    TI_C54X_TEST_CHECK(prepare_delayed_push(&transaction, &instruction,
                                            &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    /* SPRU172C §4 CALL[D] worked example: CALLD at 0025 stages TOS
     * 0029 (PC+4), not the 0027 sequential successor. */
    TI_C54X_TEST_CHECK(
        transaction.candidate.effect.payload.memory.value_word ==
        UINT16_C(0x0029));
    TI_C54X_TEST_CHECK(
        transaction.candidate.address_resolution.request.space ==
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(
        transaction.candidate.address_resolution.request.access ==
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(
        transaction.candidate.address_resolution.request.address_word,
        UINT16_C(0x1110));
    TI_C54X_TEST_CHECK_U64(
        transaction.candidate.address_resolution.request.write_value_word,
        UINT16_C(0x0029));
    TI_C54X_TEST_CHECK_U64(transaction.candidate.address_resolution
                               .address_commit.value_after,
                           UINT16_C(0x1110));
}

static void
test_wait_ready_defers_pc(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction =
        delayed_push_instruction(UINT16_C(0x1000));
    const ti_c54x_op_effect effect = delayed_push_effect();
    ti_c54x_arch_state state =
        delayed_push_state(UINT16_C(0x1111), UINT32_C(0x0025));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delayed_push_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.wait_once = 1;
    TI_C54X_TEST_CHECK(prepare_delayed_push(&transaction, &instruction,
                                            &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delayed_push_go(&transaction, &script, &state, &accumulators,
                             &t_register);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x1111));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x0025));
    TI_C54X_TEST_CHECK(!result.deferred_branch_armed);
    result = delayed_push_go(&transaction, &script, &state, &accumulators,
                             &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    /* Atomic stack commit with sequential PC: SP decremented, TOS
     * carries PC+4, PC advanced to the two-word successor while the
     * frozen pmad is reported deferred. */
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x1110));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x0027));
    TI_C54X_TEST_CHECK_U64(script.request.address_word, UINT16_C(0x1110));
    TI_C54X_TEST_CHECK_U64(script.request.write_value_word,
                           UINT16_C(0x0029));
    TI_C54X_TEST_CHECK(result.deferred_branch_armed);
    TI_C54X_TEST_CHECK_U64(result.deferred_branch_pc_word,
                           UINT16_C(0x1000));
}

static void
test_fault_rolls_back_disarmed(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction =
        delayed_push_instruction(UINT16_C(0x1000));
    const ti_c54x_op_effect effect = delayed_push_effect();
    ti_c54x_arch_state state =
        delayed_push_state(UINT16_C(0x1111), UINT32_C(0x0025));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delayed_push_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.fault = 1;
    TI_C54X_TEST_CHECK(prepare_delayed_push(&transaction, &instruction,
                                            &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delayed_push_go(&transaction, &script, &state, &accumulators,
                             &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x1111));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x0025));
    TI_C54X_TEST_CHECK(!result.deferred_branch_armed);
}

static void
test_top_of_memory_return_rejected(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction =
        delayed_push_instruction(UINT16_C(0x1000));
    const ti_c54x_op_effect effect = delayed_push_effect();
    /* Successor 0xFFFF is sequential-legal, but the PC+4 return
     * address 0x10001 leaves 16-bit program space: malformed
     * top-of-memory placement. */
    ti_c54x_arch_state state =
        delayed_push_state(UINT16_C(0x1111), UINT32_C(0xfffd));

    TI_C54X_TEST_CHECK(prepare_delayed_push(&transaction, &instruction,
                                            &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
}

static void
test_forged_spec_rejected(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction =
        delayed_push_instruction(UINT16_C(0x1000));
    ti_c54x_op_effect effect = delayed_push_effect();
    ti_c54x_arch_state state =
        delayed_push_state(UINT16_C(0x1111), UINT32_C(0x0025));

    /* Delayed pushes share the all-zero spec contract: any mode is
     * forged. */
    effect.payload.memory.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD1;
    TI_C54X_TEST_CHECK(prepare_delayed_push(&transaction, &instruction,
                                            &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
}

static void
test_immediate_push_retires_disarmed(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction =
        delayed_push_instruction(UINT16_C(0x1000));
    ti_c54x_op_effect effect = delayed_push_effect();
    ti_c54x_arch_state state =
        delayed_push_state(UINT16_C(0x1111), UINT32_C(0x0025));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    delayed_push_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    /* The immediate kind keeps its redirecting commit and never arms
     * the deferred report. */
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16;
    TI_C54X_TEST_CHECK(prepare_delayed_push(&transaction, &instruction,
                                            &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = delayed_push_go(&transaction, &script, &state, &accumulators,
                             &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x1000));
    TI_C54X_TEST_CHECK_U64(script.request.write_value_word,
                           UINT16_C(0x0027));
    TI_C54X_TEST_CHECK(!result.deferred_branch_armed);
    TI_C54X_TEST_CHECK_U64(result.deferred_branch_pc_word, UINT16_C(0));
}

int
main(void)
{
    test_prepare_stages_pc_plus_four();
    test_wait_ready_defers_pc();
    test_fault_rolls_back_disarmed();
    test_top_of_memory_return_rejected();
    test_forged_spec_rejected();
    test_immediate_push_retires_disarmed();
    puts("delayed_stack_push_root: ok");
    return 0;
}
