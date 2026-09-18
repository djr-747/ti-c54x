/*
 * Module contract
 *
 * Owns: executable proof of the single-word Smem delay write-back
 * seam: a DATA WORD16 READ with write_smem_delay stages a second
 * bus WRITE of the read word to the resolved address plus one
 * (16-bit wraparound) after READY and before the architecture
 * commit; the LTD shape completes a T-register write while the
 * DELAY shape runs with no completion; WAIT resumes mid-write;
 * a delay-write FAULT publishes nothing; delay on WRITE/RMW
 * access and delay on MMR-routed reads reject transactionally.
 * Does not own: production behavior, CPU lifecycle, endpoint
 * behavior, or architecture.
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct delay_script {
    unsigned calls;
    unsigned cancel_calls;
    ti_c54x_bus_word16_request requests[4];
    uint16_t read_word;
    unsigned wait_read_begins;
    unsigned wait_write_begins;
    bool fault_read;
    bool fault_write;
} delay_script;

static ti_c54x_bus_word16_cancel_result
delay_cancel(void *context, uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request)
{
    delay_script *script = (delay_script *)context;

    (void)transaction_id;
    (void)request;
    ++script->cancel_calls;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static void
delay_advance(void *context, ti_c54x_bus_word16_advance_phase phase,
              uint64_t transaction_id,
              const ti_c54x_bus_word16_request *request,
              ti_c54x_bus_word16_observation *observation)
{
    delay_script *script = (delay_script *)context;

    TI_C54X_TEST_CHECK(script->calls < 4U);
    script->requests[script->calls++] = *request;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    if ((request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
         script->fault_read) ||
        (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
         script->fault_write)) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
        return;
    }
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        if ((request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
             script->wait_read_begins > 0U) ||
            (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
             script->wait_write_begins > 0U)) {
            if (request->access ==
                TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
                --script->wait_read_begins;
            } else {
                --script->wait_write_begins;
            }
            observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
            return;
        }
    }
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        observation->read_value_word = script->read_word;
    }
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
prepare_delay(ti_c54x_memory_instruction_transaction *transaction,
              const ti_c54x_op_effect *effect, ti_c54x_arch_state *state)
{
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = input_snapshot();

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, &instruction, &input, effect, state, 0, 0);
}

static ti_c54x_op_status
complete_t_write(const ti_c54x_decoded_instruction *instruction,
                 const ti_c54x_op_input_snapshot *input,
                 uint16_t read_value_word, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    (void)instruction;
    (void)input;
    staged.kind = TI_C54X_OP_EFFECT_T_REGISTER_WRITE;
    staged.payload.t_register_write.raw_value = read_value_word;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_effect
delay_read_effect(bool direct, uint8_t dma_or_arf,
                  ti_c54x_op_memory_complete_fn complete)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    effect.payload.memory.value_word = UINT16_C(0);
    effect.payload.memory.complete = complete;
    effect.payload.memory.write_smem_delay = true;
    if (direct) {
        effect.payload.memory.address_spec.kind =
            TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16;
        effect.payload.memory.address_spec.direct_dma7 = dma_or_arf;
    } else {
        effect.payload.memory.address_spec.kind =
            TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
        effect.payload.memory.address_spec.mode =
            TI_C54X_INDIRECT_SINGLE_MOD0;
        effect.payload.memory.address_spec.arf = dma_or_arf;
    }
    return effect;
}

static void
reset_states(ti_c54x_arch_state *state,
             ti_c54x_accumulator_state *accumulators,
             ti_c54x_t_register_state *t_register)
{
    *state = known_arch_state();
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(t_register) ==
                       TI_C54X_T_REGISTER_OK);
}

static ti_c54x_bus_word16_endpoint delay_endpoint_for(delay_script *script)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = script, .advance = delay_advance, .cancel = delay_cancel};
    return endpoint;
}

static void
test_ltd_shape_loads_t_and_delays(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_result result = {0};
    ti_c54x_op_effect effect;
    delay_script script = {0};
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_t_register_state t_before;

    reset_states(&state, &accumulators, &t_register);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x0100)) ==
                       TI_C54X_ARCH_STATE_OK);
    script.read_word = UINT16_C(0x6CAC);
    endpoint = delay_endpoint_for(&script);
    effect = delay_read_effect(false, UINT8_C(2), complete_t_write);
    t_before = t_register;
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(script.calls == 2U);
    TI_C54X_TEST_CHECK(script.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(script.requests[0].address_word == UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(script.requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(script.requests[1].address_word == UINT16_C(0x0101));
    TI_C54X_TEST_CHECK(script.requests[1].write_value_word ==
                       UINT16_C(0x6CAC));
    TI_C54X_TEST_CHECK(memcmp(&t_register, &t_before, sizeof(t_register)) !=
                       0);
}

static void
test_delay_shape_without_completion(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_result result = {0};
    ti_c54x_op_effect effect;
    delay_script script = {0};
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_t_register_state t_before;
    ti_c54x_accumulator_state acc_before;

    reset_states(&state, &accumulators, &t_register);
    script.read_word = UINT16_C(0x1234);
    endpoint = delay_endpoint_for(&script);
    effect = delay_read_effect(true, UINT8_C(0x20), NULL);
    t_before = t_register;
    acc_before = accumulators;
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.calls == 2U);
    TI_C54X_TEST_CHECK(script.requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(script.requests[1].address_word == UINT16_C(0x0021));
    TI_C54X_TEST_CHECK(script.requests[1].write_value_word ==
                       UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(memcmp(&t_register, &t_before, sizeof(t_register)) ==
                       0);
    TI_C54X_TEST_CHECK(memcmp(&accumulators, &acc_before,
                              sizeof(accumulators)) == 0);
}

static void
test_delay_write_wait_resumes(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_result result = {0};
    ti_c54x_op_effect effect;
    delay_script script = {0};
    ti_c54x_bus_word16_endpoint endpoint;

    reset_states(&state, &accumulators, &t_register);
    script.read_word = UINT16_C(0x6CAC);
    script.wait_read_begins = 1U;
    script.wait_write_begins = 1U;
    endpoint = delay_endpoint_for(&script);
    effect = delay_read_effect(true, UINT8_C(0x20), complete_t_write);
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.calls == 4U);
    TI_C54X_TEST_CHECK(script.requests[2].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(script.requests[2].address_word == UINT16_C(0x0021));
}

static void
test_delay_write_fault_publishes_nothing(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_result result = {0};
    ti_c54x_op_effect effect;
    delay_script script = {0};
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_t_register_state t_before;

    reset_states(&state, &accumulators, &t_register);
    script.read_word = UINT16_C(0x6CAC);
    script.fault_write = true;
    endpoint = delay_endpoint_for(&script);
    effect = delay_read_effect(true, UINT8_C(0x20), complete_t_write);
    t_before = t_register;
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(memcmp(&t_register, &t_before, sizeof(t_register)) ==
                       0);
}

static void
test_delay_rejects_write_and_rmw_access(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_op_effect effect;

    reset_states(&state, &accumulators, &t_register);
    effect = delay_read_effect(true, UINT8_C(0x20), complete_t_write);
    effect.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) !=
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    transaction = (ti_c54x_memory_instruction_transaction){0};
    effect = delay_read_effect(true, UINT8_C(0x20), complete_t_write);
    effect.payload.memory.rmw_operation =
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_ADD;
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) !=
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
}

static void
test_delay_rejects_mmr_routed_read(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state;
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_memory_instruction_result result = {0};
    ti_c54x_op_effect effect;
    delay_script script = {0};
    ti_c54x_bus_word16_endpoint endpoint;

    reset_states(&state, &accumulators, &t_register);
    script.read_word = UINT16_C(0x00FF);
    endpoint = delay_endpoint_for(&script);
    effect = delay_read_effect(true, UINT8_C(0x0E), complete_t_write);
    TI_C54X_TEST_CHECK(prepare_delay(&transaction, &effect, &state) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
    TI_C54X_TEST_CHECK(script.calls == 0U);
}

int
main(void)
{
    test_ltd_shape_loads_t_and_delays();
    test_delay_shape_without_completion();
    test_delay_write_wait_resumes();
    test_delay_write_fault_publishes_nothing();
    test_delay_rejects_write_and_rmw_access();
    test_delay_rejects_mmr_routed_read();
    return 0;
}
