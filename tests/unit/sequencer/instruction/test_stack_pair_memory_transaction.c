/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT executable proof of the S52 stack-pair
 * root transaction hook: PSHD retires Smem READ then TOS WRITE
 * carrying the observed Smem word, POPD retires TOS READ then
 * Smem WRITE carrying the observed TOS word, PSHM/POPM serve
 * the MMR leg synchronously through the S1 seam with endpoint
 * traffic only for the TOS leg, malformed arms reject at
 * prepare, leg faults stay transactional with no second issue,
 * stale SP rejects on resume, and in-flight cancel reaches the
 * endpoint. Endpoint scripts are local.
 * Does not own: stack-leg planning, DATA-leg resolution, MMR
 * routing, endpoint implementation, opcode dispatch, CPU
 * lifecycle, or timing.
 */

#include "isa/ops/memory/memory_effect.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "state/accumulator/accumulator.h"
#include "state/arch_state.h"
#include "state/mmr_file.h"
#include "state/temporary/t_register.h"
#include "../../../support/test_harness.h"

#include <ti_c54x/bus_endpoint.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct pair_slot {
    bool seen;
    ti_c54x_bus_word16_request request;
    uint64_t transaction_id;
} pair_slot;

typedef struct pair_script {
    pair_slot slots[2];
    unsigned begin_count;
    /* -1 always READY; >=0 WAIT that many begins, then READY. */
    int wait_begins;
    /* Fault the named slot index on begin (-1 never). */
    int fault_slot;
    uint16_t read_values[2];
    unsigned cancel_calls;
} pair_script;

static int
pair_slot_for(const pair_script *script,
              const ti_c54x_bus_word16_request *request)
{
    if (script->slots[0].seen &&
        script->slots[0].request.space == request->space &&
        script->slots[0].request.address_word == request->address_word &&
        script->slots[0].request.access == request->access) {
        return 0;
    }
    if (script->slots[1].seen &&
        script->slots[1].request.space == request->space &&
        script->slots[1].request.address_word == request->address_word &&
        script->slots[1].request.access == request->access) {
        return 1;
    }
    if (!script->slots[0].seen) {
        return 0;
    }
    if (!script->slots[1].seen) {
        return 1;
    }
    return -1;
}

static void
pair_script_advance(void *context, ti_c54x_bus_word16_advance_phase phase,
                    uint64_t transaction_id,
                    const ti_c54x_bus_word16_request *request,
                    ti_c54x_bus_word16_observation *observation)
{
    pair_script *script = (pair_script *)context;
    const int slot = pair_slot_for(script, request);

    TI_C54X_TEST_CHECK(slot >= 0);
    TI_C54X_TEST_CHECK(script->slots[slot].transaction_id == UINT64_C(0) ||
                       script->slots[slot].transaction_id == transaction_id);
    script->slots[slot].seen = true;
    script->slots[slot].request = *request;
    script->slots[slot].transaction_id = transaction_id;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    if (slot == script->fault_slot) {
        /* The endpoint contract carries no read value on FAULT. */
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
        return;
    }
    /* The endpoint contract requires WAIT observations to carry a
     * zero read value: tests that WAIT must leave the slot's read
     * value zero. */
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        observation->read_value_word = script->read_values[slot];
    }
    if (script->wait_begins < 0) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        observation->bus_status = TI_C54X_BUS_OK;
        return;
    }
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        ++script->begin_count;
    }
    if ((int)script->begin_count <= script->wait_begins) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        observation->bus_status = TI_C54X_BUS_OK;
        return;
    }
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
    observation->bus_status = TI_C54X_BUS_OK;
}

static ti_c54x_bus_word16_cancel_result
pair_script_cancel(void *context, uint64_t transaction_id,
                   const ti_c54x_bus_word16_request *request)
{
    pair_script *script = (pair_script *)context;

    (void)transaction_id;
    (void)request;
    ++script->cancel_calls;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint
pair_endpoint_for(pair_script *script)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = script, .advance = pair_script_advance,
        .cancel = pair_script_cancel};
    return endpoint;
}

static ti_c54x_op_effect
pair_effect(ti_c54x_op_memory_address_kind kind,
            ti_c54x_bus_word16_request_access access, uint8_t indirect,
            uint8_t smem_dma7)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind = kind;
    effect.payload.memory.address_spec.stack_pair.indirect = indirect;
    effect.payload.memory.address_spec.stack_pair.smem_dma7 = smem_dma7;
    effect.payload.memory.access = access;
    effect.payload.memory.value_word = UINT16_C(0);
    effect.payload.memory.complete = NULL;
    return effect;
}

static ti_c54x_arch_state
pair_state(uint16_t sp_value, uint32_t pc_value)
{
    ti_c54x_arch_state state = {0};

    /* CMPT/ARP/CPL/DP stay unknown: success proves both legs
     * resolve without mode reads. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&state, sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    state.pc_word = pc_value;
    return state;
}

static ti_c54x_decoded_instruction
decoded_nop(void)
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

static ti_c54x_op_input_snapshot
pair_input(void)
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
        .status_sxm_known = 1,
        .status_dp = 0,
        .status_dp_known = 1};
    return input;
}

static ti_c54x_memory_instruction_status
prepare_pair(ti_c54x_memory_instruction_transaction *transaction,
             const ti_c54x_op_effect *effect, ti_c54x_arch_state *state)
{
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = pair_input();

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, &instruction, &input, effect, state, UINT64_C(0),
        UINT64_C(0));
}

static void
reset_side_state(ti_c54x_accumulator_state *accumulators,
                 ti_c54x_t_register_state *t_register)
{
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(t_register) ==
                       TI_C54X_T_REGISTER_OK);
}

static void
drive_to_retired(ti_c54x_memory_instruction_transaction *transaction,
                 pair_script *script, ti_c54x_arch_state *state,
                 ti_c54x_accumulator_state *accumulators,
                 ti_c54x_t_register_state *t_register,
                 ti_c54x_memory_instruction_result *result)
{
    ti_c54x_bus_word16_endpoint endpoint = pair_endpoint_for(script);

    for (unsigned step = 0U; step < 6U; ++step) {
        TI_C54X_TEST_CHECK(
            ti_c54x_memory_instruction_transaction_advance(
                transaction, &endpoint, state, accumulators, t_register,
                result) == TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
        if (result->status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
            return;
        }
        TI_C54X_TEST_CHECK(result->status ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    }
    TI_C54X_TEST_CHECK(false);
}

static uint16_t
read_sp(const ti_c54x_arch_state *state)
{
    uint16_t sp = UINT16_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_sp(state, &sp) ==
                       TI_C54X_ARCH_STATE_OK);
    return sp;
}

static void
test_pshd_direct_retires(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = pair_state(UINT16_C(0x1111), UINT32_C(0x20));
    ti_c54x_op_effect effect = pair_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT8_C(0),
        UINT8_C(0x60));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    reset_side_state(&accumulators, &t_register);
    script.wait_begins = -1;
    script.fault_slot = -1;
    script.read_values[0] = UINT16_C(0xBEEF);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    drive_to_retired(&transaction, &script, &state, &accumulators,
                     &t_register, &result);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    /* Smem DATA READ first, TOS DATA WRITE second with the
     * observed word at old SP - 1. */
    TI_C54X_TEST_CHECK(script.slots[0].seen && script.slots[1].seen);
    TI_C54X_TEST_CHECK(script.slots[0].request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.slots[0].request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(script.slots[0].request.address_word ==
                       UINT16_C(0x0060));
    TI_C54X_TEST_CHECK(script.slots[1].request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.slots[1].request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(script.slots[1].request.address_word ==
                       UINT16_C(0x1110));
    TI_C54X_TEST_CHECK(script.slots[1].request.write_value_word ==
                       UINT16_C(0xBEEF));
    TI_C54X_TEST_CHECK(script.slots[0].transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(script.slots[1].transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(script.slots[0].transaction_id !=
                       script.slots[1].transaction_id);
    TI_C54X_TEST_CHECK(read_sp(&state) == UINT16_C(0x1110));
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x21));
}

static void
test_popd_direct_retires(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = pair_state(UINT16_C(0x1110), UINT32_C(0x20));
    ti_c54x_op_effect effect = pair_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT8_C(0),
        UINT8_C(0x60));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    reset_side_state(&accumulators, &t_register);
    script.wait_begins = -1;
    script.fault_slot = -1;
    script.read_values[0] = UINT16_C(0x1234);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    drive_to_retired(&transaction, &script, &state, &accumulators,
                     &t_register, &result);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* TOS DATA READ first at old SP, Smem DATA WRITE second with
     * the observed word. */
    TI_C54X_TEST_CHECK(script.slots[0].seen && script.slots[1].seen);
    TI_C54X_TEST_CHECK(script.slots[0].request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.slots[0].request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(script.slots[0].request.address_word ==
                       UINT16_C(0x1110));
    TI_C54X_TEST_CHECK(script.slots[1].request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.slots[1].request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(script.slots[1].request.address_word ==
                       UINT16_C(0x0060));
    TI_C54X_TEST_CHECK(script.slots[1].request.write_value_word ==
                       UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(read_sp(&state) == UINT16_C(0x1111));
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x21));
}

static void
test_pshm_mmr_leg_routes_with_tos_endpoint_traffic(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = pair_state(UINT16_C(0x2000), UINT32_C(0x20));
    ti_c54x_op_effect effect = pair_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT8_C(0),
        UINT8_C(0x1A));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    reset_side_state(&accumulators, &t_register);
    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_write_brc(
                           &state.mmr_file, UINT16_C(0x1234)) ==
                       TI_C54X_MMR_FILE_OK);
    script.wait_begins = -1;
    script.fault_slot = -1;
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    drive_to_retired(&transaction, &script, &state, &accumulators,
                     &t_register, &result);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* The BRC read routes internally: only the TOS WRITE reaches
     * the endpoint, carrying the MMR observation. */
    TI_C54X_TEST_CHECK(!script.slots[1].seen);
    TI_C54X_TEST_CHECK(script.slots[0].seen);
    TI_C54X_TEST_CHECK(script.slots[0].request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.slots[0].request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(script.slots[0].request.address_word ==
                       UINT16_C(0x1FFF));
    TI_C54X_TEST_CHECK(script.slots[0].request.write_value_word ==
                       UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(read_sp(&state) == UINT16_C(0x1FFF));
}

static void
test_popm_mmr_leg_routes_with_tos_endpoint_traffic(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = pair_state(UINT16_C(0x03F0), UINT32_C(0x20));
    ti_c54x_op_effect effect = pair_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT8_C(0),
        UINT8_C(0x15));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_script script = {0};
    ti_c54x_memory_instruction_result result = {0};
    uint16_t ar5 = UINT16_C(0);

    reset_side_state(&accumulators, &t_register);
    script.wait_begins = -1;
    script.fault_slot = -1;
    script.read_values[0] = UINT16_C(0x0060);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    drive_to_retired(&transaction, &script, &state, &accumulators,
                     &t_register, &result);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* The TOS READ reaches the endpoint; the AR5 write routes
     * internally with the TOS observation. */
    TI_C54X_TEST_CHECK(!script.slots[1].seen);
    TI_C54X_TEST_CHECK(script.slots[0].seen);
    TI_C54X_TEST_CHECK(script.slots[0].request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(script.slots[0].request.address_word ==
                       UINT16_C(0x03F0));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, UINT8_C(5),
                                                  &ar5) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ar5 == UINT16_C(0x0060));
    TI_C54X_TEST_CHECK(read_sp(&state) == UINT16_C(0x03F1));
}

static void
test_malformed_prepare_rejects(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = pair_state(UINT16_C(0x1111), UINT32_C(0x20));
    ti_c54x_op_effect effect = pair_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT8_C(1),
        UINT8_C(0x60));

    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
}

static void
test_leg_fault_is_transactional(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = pair_state(UINT16_C(0x1111), UINT32_C(0x20));
    ti_c54x_op_effect effect = pair_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT8_C(0),
        UINT8_C(0x60));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_script script = {0};
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_memory_instruction_result result = {0};

    reset_side_state(&accumulators, &t_register);
    script.wait_begins = -1;
    script.fault_slot = 0;
    script.read_values[0] = UINT16_C(0xBEEF);
    endpoint = pair_endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(!script.slots[1].seen);
    TI_C54X_TEST_CHECK(read_sp(&state) == UINT16_C(0x1111));
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x20));
}

static void
test_stale_sp_rejects_on_resume(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = pair_state(UINT16_C(0x1111), UINT32_C(0x20));
    ti_c54x_op_effect effect = pair_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT8_C(0),
        UINT8_C(0x60));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_script script = {0};
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_memory_instruction_result result = {0};

    reset_side_state(&accumulators, &t_register);
    script.wait_begins = 1;
    script.fault_slot = -1;
    /* No read value: the only observation is WAIT, and the resume
     * rejects before any READY. */
    endpoint = pair_endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&state,
                                                  UINT16_C(0x1112)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(!script.slots[1].seen);
    TI_C54X_TEST_CHECK(read_sp(&state) == UINT16_C(0x1112));
}

static void
test_cancel_in_flight(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = pair_state(UINT16_C(0x1111), UINT32_C(0x20));
    ti_c54x_op_effect effect = pair_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT8_C(0),
        UINT8_C(0x60));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    pair_script script = {0};
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_memory_instruction_result result = {0};

    reset_side_state(&accumulators, &t_register);
    script.wait_begins = 100;
    script.fault_slot = -1;
    endpoint = pair_endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare_pair(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(script.cancel_calls == 1U);
}

int
main(void)
{
    test_pshd_direct_retires();
    test_popd_direct_retires();
    test_pshm_mmr_leg_routes_with_tos_endpoint_traffic();
    test_popm_mmr_leg_routes_with_tos_endpoint_traffic();
    test_malformed_prepare_rejects();
    test_leg_fault_is_transactional();
    test_stale_sp_rejects_on_resume();
    test_cancel_in_flight();
    return 0;
}
