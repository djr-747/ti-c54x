/* Module contract
 * Owns: W31B1 root-transaction preparation, resume-before-fetch, cancellation,
 * monotonic IDs, WAIT/FAULT accounting, and architecture-commit proofs at the
 * CPU boundary. Preparation uses the generic private seam, never a test opcode.
 * Does not own: endpoint adapter internals, opcode dispatch, or public init
 * descriptor validation.
 */

#include "fixture.h"

#include "../../../../src/cpu/cpu_internal.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../../src/isa/ops/op_effect.h"
#include "../../../../src/isa/ops/op_input.h"
#include "../../../../src/isa/ops/op_status.h"
#include "../../../../src/sequencer/instruction/memory_instruction_transaction.h"

#include <stdbool.h>
#include <stddef.h>

static ti_c54x_decoded_instruction decoded_for_root_seed(void)
{
    return (ti_c54x_decoded_instruction){
        .id = TI_C54X_OPCODE_ID_NOP,
        .operand_layout = TI_C54X_OPERAND_LAYOUT_NONE,
        .repeatability = TI_C54X_REPEATABILITY_UNRESOLVED,
        .input_resources = TI_C54X_INPUT_RESOURCES_NONE,
        .bundle = {.program_words = {UINT16_C(0xf495), UINT16_C(0)},
                   .program_word_count = UINT8_C(1)}};
}

static ti_c54x_op_input_snapshot input_for_root_seed(void)
{
    return (ti_c54x_op_input_snapshot){
        .accumulator_a_raw40 = UINT64_C(0x1122),
        .accumulator_b_raw40 = UINT64_C(0x3344),
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_c = 0,
        .status_c_known = true,
        .status_tc = 0,
        .status_tc_known = true,
        .status_sxm = 0,
        .status_sxm_known = true};
}

static ti_c54x_op_status
complete_root_seed_read(const ti_c54x_decoded_instruction *instruction,
                        const ti_c54x_op_input_snapshot *input,
                        uint16_t read_value_word, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};
    TI_C54X_TEST_CHECK(instruction != NULL);
    TI_C54X_TEST_CHECK(input != NULL);
    TI_C54X_TEST_CHECK(effect != NULL);
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

static ti_c54x_op_effect
memory_effect_for_root_seed(ti_c54x_bus_word16_request_access access)
{
    ti_c54x_op_effect effect = {0};
    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
    effect.payload.memory.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD2;
    effect.payload.memory.address_spec.arf = UINT8_C(2);
    effect.payload.memory.access = access;
    effect.payload.memory.value_word =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ? UINT16_C(0)
                                                         : UINT16_C(0x00a5);
    effect.payload.memory.complete =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
            ? complete_root_seed_read
            : NULL;
    return effect;
}

static void prepare_pending_memory(ti_c54x_cpu *cpu,
                                   ti_c54x_bus_word16_request_access access)
{
    const ti_c54x_decoded_instruction instruction = decoded_for_root_seed();
    const ti_c54x_op_input_snapshot input = input_for_root_seed();
    const ti_c54x_op_effect effect = memory_effect_for_root_seed(access);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(
                           &cpu->arch_state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(
                           &cpu->arch_state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &cpu->arch_state, UINT8_C(2), UINT16_C(0x2110)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_prepare(
                           &cpu->instruction_transaction, &instruction, &input,
                           &effect, &cpu->arch_state,
                           cpu->instructions_attempted,
                           cpu->instructions_retired) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
}

static void seed_pending_memory(ti_c54x_cpu *cpu, cpu_memory_fixture *fixture,
                                ti_c54x_bus_word16_request_access access)
{
    const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(fixture);
    ti_c54x_memory_instruction_result result = {0};
    prepare_pending_memory(cpu, access);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &cpu->instruction_transaction, &endpoint,
                           &cpu->arch_state, &cpu->accumulators,
                           &cpu->t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    cpu->instructions_attempted++;
}

static void test_prepared_and_pending_work_resume_before_fetch(void)
{
    cpu_memory_fixture fixture = {
        .mode = CPU_ENDPOINT_WAIT,
        .read_value_word = UINT16_C(0xbeef),
        .fault_status = TI_C54X_BUS_FAULT,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
    ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    ti_c54x_trace_sink trace = {TI_C54X_TRACE_MASK_ALL, &fixture, emit_trace};
    ti_c54x_error status;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(storage, &fixture, &endpoint, &trace, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    prepare_pending_memory(cpu, TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.fetch_calls == 0U);
    fixture.mode = CPU_ENDPOINT_READY;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.fetch_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.stop_events == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void test_pending_ready_resume_uses_remaining_budget_for_fetch(void)
{
    cpu_memory_fixture fixture = {
        .mode = CPU_ENDPOINT_WAIT,
        .read_value_word = UINT16_C(0xbeef),
        .fault_status = TI_C54X_BUS_FAULT,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
    ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    ti_c54x_error status;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(storage, &fixture, &endpoint, NULL, &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    seed_pending_memory(cpu, &fixture,
                        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    fixture.mode = CPU_ENDPOINT_READY;

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(result.opcode_word == UINT16_C(0xf495));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.advance_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.fetch_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.stop_events == 0U);
    TI_C54X_TEST_CHECK(fixture.ids[0] != UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.ids[1] == fixture.ids[0]);
    TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == UINT32_C(0x2110));
    TI_C54X_TEST_CHECK(fixture.requests[1].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == UINT32_C(0x2110));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(state.last_fetched_word == UINT16_C(0xf495));
    TI_C54X_TEST_CHECK(state.last_fetch_valid);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0xbeef));
    TI_C54X_TEST_CHECK(state.accumulator_a_known);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == UINT64_C(2));
    TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[2] == UINT16_C(0x2111));
    TI_C54X_TEST_CHECK(cpu->instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void test_reset_and_deinit_cancel_pending_with_same_id_and_retry(void)
{
    cpu_memory_fixture fixture = {
        .mode = CPU_ENDPOINT_WAIT,
        .fault_status = TI_C54X_BUS_FAULT,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_FAILED};
    ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    ti_c54x_error status;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(storage, &fixture, &endpoint, NULL, &status);

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    seed_pending_memory(cpu, &fixture,
                        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture.ids[0] != UINT64_C(0));
    fixture.reentry_cpu = cpu;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) ==
                       TI_C54X_ERROR_BUS_CANCEL_FAILED);
    TI_C54X_TEST_CHECK(fixture.reentry_status == TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(fixture.cancel_calls == 1U);
    fixture.reentry_cpu = NULL;
    fixture.cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    fixture.mode = CPU_ENDPOINT_WAIT;
    seed_pending_memory(cpu, &fixture,
                        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture.ids[1] > fixture.ids[0]);
    fixture.cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_FAILED;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                       TI_C54X_ERROR_BUS_CANCEL_FAILED);
    fixture.cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void test_pending_fault_does_not_commit_architecture(void)
{
    cpu_memory_fixture fixture = {
        .mode = CPU_ENDPOINT_WAIT,
        .fault_status = TI_C54X_BUS_FAULT,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
    ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    ti_c54x_error status;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(storage, &fixture, &endpoint, NULL, &status);
    ti_c54x_state before;
    ti_c54x_state after;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    seed_pending_memory(cpu, &fixture,
                        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &before) ==
                       TI_C54X_ERROR_OK);
    fixture.mode = CPU_ENDPOINT_FAULT;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &after) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(after.pc_word == before.pc_word);
    TI_C54X_TEST_CHECK(after.dp == before.dp);
    TI_C54X_TEST_CHECK(after.arp == before.arp);
    TI_C54X_TEST_CHECK(after.accumulator_a_raw40 == before.accumulator_a_raw40);
    TI_C54X_TEST_CHECK(after.accumulator_b_raw40 == before.accumulator_b_raw40);
    TI_C54X_TEST_CHECK(after.instructions_attempted_total ==
                       before.instructions_attempted_total);
    TI_C54X_TEST_CHECK(after.instructions_retired_total ==
                       before.instructions_retired_total);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void test_missing_data_endpoint_faults_before_external_issue(void)
{
    cpu_memory_fixture fixture = {.mode = CPU_ENDPOINT_WAIT,
                                   .fault_status = TI_C54X_BUS_FAULT,
                                   .cancel_result =
                                       TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_bus absent_bus = make_bus(&fixture, NULL);
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(
                           storage, ti_c54x_cpu_size(), NULL, &absent_bus,
                           &cpu) == TI_C54X_ERROR_OK);
    prepare_pending_memory(cpu, TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(cpu->instruction_transaction.bus_transaction.transaction_id ==
                       UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->instruction_transaction.last_transaction_id ==
                       UINT64_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_NO_CALLBACK);
    TI_C54X_TEST_CHECK(fixture.advance_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->instruction_transaction.bus_transaction.transaction_id ==
                       UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->instruction_transaction.last_transaction_id ==
                       UINT64_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    test_prepared_and_pending_work_resume_before_fetch();
    test_pending_ready_resume_uses_remaining_budget_for_fetch();
    test_reset_and_deinit_cancel_pending_with_same_id_and_retry();
    test_pending_fault_does_not_commit_architecture();
    test_missing_data_endpoint_faults_before_external_issue();
    return 0;
}
