/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proofs for the
 * accumulator-targeted stack-push WORD16 root transaction. It checks
 * the PC+1/PC+3 return-address staging at prepare with the target
 * taken from the frozen input snapshot, WAIT/READY retirement with
 * atomic SP commit plus accumulator-target PC redirect (immediate)
 * or sequential-PC advance with accumulator-as-deferred reporting
 * (delayed), unknown-accumulator and forged-shape rejection before
 * any bus issue, and that immediate pushes retire disarmed. The
 * future CALA[D] rows bind the real catalog identities later; this
 * seam test synthesizes the bundle directly and consumes no catalog
 * identity. Does not own: opcode dispatch, catalog, endpoint
 * implementation, timing, observer policy, or CPU lifecycle.
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

typedef struct acc_push_script {
    int fault;
    int wait_once;
    bool waited;
    ti_c54x_bus_word16_request request;
    unsigned calls;
} acc_push_script;

static void
acc_push_advance(void *context,
                 ti_c54x_bus_word16_advance_phase phase,
                 uint64_t transaction_id,
                 const ti_c54x_bus_word16_request *request,
                 ti_c54x_bus_word16_observation *observation)
{
    acc_push_script *script = (acc_push_script *)context;

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
acc_push_cancel(void *context, uint64_t transaction_id,
                const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint
acc_push_endpoint_for(acc_push_script *script)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = script, .advance = acc_push_advance,
        .cancel = acc_push_cancel};
    return endpoint;
}

static ti_c54x_memory_instruction_result
acc_push_go(ti_c54x_memory_instruction_transaction *transaction,
            acc_push_script *script, ti_c54x_arch_state *state,
            ti_c54x_accumulator_state *accumulators,
            ti_c54x_t_register_state *t_register)
{
    const ti_c54x_bus_word16_endpoint endpoint =
        acc_push_endpoint_for(script);
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           transaction, &endpoint, state, accumulators,
                           t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    return result;
}

/* Placeholder first word for the future CALA[D] rows; the seam consumes
 * no catalog identity (UNKNOWN id, one program word). */
static const uint16_t ACC_PUSH_WORD0 = UINT16_C(0xF4E2);

static ti_c54x_decoded_instruction
acc_push_instruction(void)
{
    const ti_c54x_decoded_instruction instruction = {
        .id = TI_C54X_OPCODE_ID_UNKNOWN,
        .operand_layout = TI_C54X_OPERAND_LAYOUT_NONE,
        .repeatability = TI_C54X_REPEATABILITY_INELIGIBLE,
        .input_resources = TI_C54X_INPUT_RESOURCES_STACK_POINTER,
        .bundle = {.program_words = {ACC_PUSH_WORD0},
                   .program_word_count = UINT8_C(1)}};
    return instruction;
}

static ti_c54x_op_effect
acc_push_effect(ti_c54x_op_memory_address_kind kind, uint8_t select)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind = kind;
    effect.payload.memory.address_spec.arf = select;
    effect.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    /* The pure handler cannot know PC or the accumulator target: the
     * root stages the live return address into the frozen copy at
     * prepare and reads the target from the frozen input snapshot. */
    effect.payload.memory.value_word = UINT16_C(0);
    effect.payload.memory.rmw_operation =
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE;
    effect.payload.memory.complete = NULL;
    return effect;
}

static ti_c54x_op_input_snapshot
acc_push_input(bool a_known, uint64_t a_raw40, bool b_known,
               uint64_t b_raw40)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_known = a_known;
    input.accumulator_a_raw40 = a_raw40;
    input.accumulator_b_known = b_known;
    input.accumulator_b_raw40 = b_raw40;
    input.stack_pointer_known = true;
    return input;
}

static ti_c54x_arch_state
acc_push_state(uint16_t sp_value, uint32_t pc_value)
{
    ti_c54x_arch_state state = {0};

    /* CMPT/ARP/CPL stay unknown: success proves the root stages the
     * accumulator push without mode reads. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&state, sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    state.pc_word = pc_value;
    return state;
}

static ti_c54x_memory_instruction_status
prepare_acc_push(ti_c54x_memory_instruction_transaction *transaction,
                 const ti_c54x_decoded_instruction *instruction,
                 const ti_c54x_op_input_snapshot *input,
                 const ti_c54x_op_effect *effect, ti_c54x_arch_state *state)
{
    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, instruction, input, effect, state, UINT64_C(0),
        UINT64_C(0));
}

static void
test_prepare_stages_pc_plus_one(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction = acc_push_instruction();
    const ti_c54x_op_input_snapshot input =
        acc_push_input(true, UINT64_C(0x3000), true, UINT64_C(0x2000));
    const ti_c54x_op_effect effect = acc_push_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16,
        UINT8_C(0));
    ti_c54x_arch_state state =
        acc_push_state(UINT16_C(0x1111), UINT32_C(0x0025));

    TI_C54X_TEST_CHECK(prepare_acc_push(&transaction, &instruction, &input,
                                        &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    /* One-word form: TOS carries PC+1 (0x0026), not a bundle word. */
    TI_C54X_TEST_CHECK(
        transaction.candidate.effect.payload.memory.value_word ==
        UINT16_C(0x0026));
    TI_C54X_TEST_CHECK(
        transaction.candidate.address_resolution.request.space ==
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK_U64(
        transaction.candidate.address_resolution.request.address_word,
        UINT16_C(0x1110));
    TI_C54X_TEST_CHECK_U64(
        transaction.candidate.address_resolution.request.write_value_word,
        UINT16_C(0x0026));
}

static void
test_wait_ready_redirects_to_accumulator(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction = acc_push_instruction();
    /* B holds 0x2000: the B select must retire to the B target. */
    const ti_c54x_op_input_snapshot input =
        acc_push_input(true, UINT64_C(0x3000), true, UINT64_C(0x2000));
    const ti_c54x_op_effect effect = acc_push_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16,
        UINT8_C(1));
    ti_c54x_arch_state state =
        acc_push_state(UINT16_C(0x1111), UINT32_C(0x0025));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    acc_push_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.wait_once = 1;
    TI_C54X_TEST_CHECK(prepare_acc_push(&transaction, &instruction, &input,
                                        &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = acc_push_go(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x1111));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x0025));
    TI_C54X_TEST_CHECK(!result.deferred_branch_armed);
    result = acc_push_go(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    /* Atomic stack commit with accumulator redirect: SP decremented,
     * TOS carries PC+1, PC reaches the B target, RTN loads the
     * staged return, and no deferred branch arms. */
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x1110));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x2000));
    TI_C54X_TEST_CHECK_U64(state.rtn, UINT16_C(0x0026));
    TI_C54X_TEST_CHECK_U64(script.request.address_word, UINT16_C(0x1110));
    TI_C54X_TEST_CHECK_U64(script.request.write_value_word,
                           UINT16_C(0x0026));
    TI_C54X_TEST_CHECK(!result.deferred_branch_armed);
    TI_C54X_TEST_CHECK_U64(result.deferred_branch_pc_word, UINT16_C(0));
}

static void
test_delayed_defers_accumulator_target(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction = acc_push_instruction();
    const ti_c54x_op_input_snapshot input =
        acc_push_input(true, UINT64_C(0x3000), true, UINT64_C(0x2000));
    const ti_c54x_op_effect effect = acc_push_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16,
        UINT8_C(0));
    ti_c54x_arch_state state =
        acc_push_state(UINT16_C(0x1111), UINT32_C(0x0025));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    acc_push_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(prepare_acc_push(&transaction, &instruction, &input,
                                        &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    /* SPRU172C §4 CALA[D]: CALAD at 0025 stages TOS 0028 (PC+3). */
    TI_C54X_TEST_CHECK(
        transaction.candidate.effect.payload.memory.value_word ==
        UINT16_C(0x0028));
    result = acc_push_go(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    /* Sequential PC with the accumulator target deferred: the two
     * delay-slot words execute first. */
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x1110));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x0026));
    TI_C54X_TEST_CHECK_U64(script.request.write_value_word,
                           UINT16_C(0x0028));
    TI_C54X_TEST_CHECK(result.deferred_branch_armed);
    TI_C54X_TEST_CHECK_U64(result.deferred_branch_pc_word,
                           UINT16_C(0x3000));
}

static void
test_unknown_accumulator_rejected_before_bus(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction = acc_push_instruction();
    /* A selected but unknown: no bus issue may follow. */
    const ti_c54x_op_input_snapshot input =
        acc_push_input(false, UINT64_C(0x3000), true, UINT64_C(0x2000));
    const ti_c54x_op_effect effect = acc_push_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16,
        UINT8_C(0));
    ti_c54x_arch_state state =
        acc_push_state(UINT16_C(0x1111), UINT32_C(0x0025));
    acc_push_script script = {0};

    TI_C54X_TEST_CHECK(prepare_acc_push(&transaction, &instruction, &input,
                                        &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK_U64(script.calls, UINT64_C(0));
}

static void
test_forged_shapes_rejected(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction = acc_push_instruction();
    const ti_c54x_op_input_snapshot input =
        acc_push_input(true, UINT64_C(0x3000), true, UINT64_C(0x2000));
    ti_c54x_op_effect effect = acc_push_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16,
        UINT8_C(0));
    ti_c54x_arch_state state =
        acc_push_state(UINT16_C(0x1111), UINT32_C(0x0025));

    /* A select outside A/B is forged even with a valid shape gate. */
    effect.payload.memory.address_spec.arf = UINT8_C(2);
    TI_C54X_TEST_CHECK(prepare_acc_push(&transaction, &instruction, &input,
                                        &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);

    /* The accumulator kinds are one-word forms: a two-word bundle
     * cannot ride them. */
    effect = acc_push_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16,
        UINT8_C(0));
    {
        ti_c54x_decoded_instruction wide = instruction;
        wide.bundle.program_word_count = UINT8_C(2);
        TI_C54X_TEST_CHECK(prepare_acc_push(&transaction, &wide, &input,
                                            &effect, &state) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    }
}

static void
test_fault_rolls_back_disarmed(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    const ti_c54x_decoded_instruction instruction = acc_push_instruction();
    const ti_c54x_op_input_snapshot input =
        acc_push_input(true, UINT64_C(0x3000), true, UINT64_C(0x2000));
    const ti_c54x_op_effect effect = acc_push_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16,
        UINT8_C(0));
    ti_c54x_arch_state state =
        acc_push_state(UINT16_C(0x1111), UINT32_C(0x0025));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    acc_push_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.fault = 1;
    TI_C54X_TEST_CHECK(prepare_acc_push(&transaction, &instruction, &input,
                                        &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    result = acc_push_go(&transaction, &script, &state, &accumulators,
                         &t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK_U64(state.sp, UINT16_C(0x1111));
    TI_C54X_TEST_CHECK_U64(state.pc_word, UINT32_C(0x0025));
    TI_C54X_TEST_CHECK(!result.deferred_branch_armed);
}

int
main(void)
{
    test_prepare_stages_pc_plus_one();
    test_wait_ready_redirects_to_accumulator();
    test_delayed_defers_accumulator_target();
    test_unknown_accumulator_rejected_before_bus();
    test_forged_shapes_rejected();
    test_fault_rolls_back_disarmed();
    puts("accumulator_push_root: ok");
    return 0;
}
