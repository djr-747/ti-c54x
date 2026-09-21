/*
 * Module contract
 *
 * Owns: public-CPU RED proofs for `ABDST Xmem,Ymem` retirement,
 * including the TI worked example with ordered X/Y DATA reads and
 * MOD2 post-increment of both ARs, the accumulator pair commit
 * with OVA/OVB/C dispositions, OVM saturation, fault atomicity,
 * and unknown-input rejection.
 * Does not own: catalog/decode or pure distance effects, address
 * internals, endpoint behavior, production code, timing, or
 * diagnostics.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct abdst_fixture {
    uint16_t program_word;
    uint16_t x_address;
    uint16_t y_address;
    uint16_t x_value;
    uint16_t y_value;
    bool fault_data;
    unsigned program_reads;
    unsigned endpoint_calls;
    ti_c54x_bus_word16_request requests[4];
} abdst_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    abdst_fixture *fixture = (abdst_fixture *)context;

    TI_C54X_TEST_CHECK(address_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(value_word != NULL);
    TI_C54X_TEST_CHECK(fixture->program_reads == 0U);
    fixture->program_reads++;
    *value_word = fixture->program_word;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    abdst_fixture *fixture = (abdst_fixture *)context;
    const unsigned index = fixture->endpoint_calls++;
    bool is_x;

    TI_C54X_TEST_CHECK(index < 2U);
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN);
    fixture->requests[index] = *request;
    is_x = request->address_word == fixture->x_address;
    TI_C54X_TEST_CHECK(is_x || request->address_word == fixture->y_address);
    if (fixture->fault_data) {
        *observation = (ti_c54x_bus_word16_observation){
            .transaction_id = transaction_id,
            .kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT,
            .bus_status = TI_C54X_BUS_FAULT};
        return;
    }
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = is_x ? fixture->x_value : fixture->y_value,
        .bus_status = TI_C54X_BUS_OK};
}

static ti_c54x_bus_word16_cancel_result
cancel_data(void *context, uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request)
{
    (void)context;
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_cpu *
init_abdst(abdst_fixture *fixture, void *storage, bool seed_y,
           ti_c54x_error *status_out)
{
    const ti_c54x_config config = {UINT32_C(0), {0U, NULL, NULL}};
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = fixture, .advance = advance_data, .cancel = cancel_data};
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program,
                             .data_endpoint = endpoint};
    ti_c54x_cpu *cpu = NULL;

    *status_out = ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                   &bus, &cpu);
    TI_C54X_TEST_CHECK(*status_out == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state, 3U,
                                                   fixture->x_address) ==
                       TI_C54X_ARCH_STATE_OK);
    if (seed_y) {
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                               &cpu->arch_state, 4U,
                               fixture->y_address) ==
                           TI_C54X_ARCH_STATE_OK);
    }
    return cpu;
}

static void
seed_state(ti_c54x_cpu *cpu, uint64_t a_value, uint64_t b_value,
           uint8_t sxm, uint8_t frct, uint8_t ovm)
{
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           a_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           b_value) == TI_C54X_ACCUMULATOR_OK);
    if (sxm != UINT8_C(0)) {
        cpu->arch_state.status.st1 =
            (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_SXM_MASK);
    }
    if (frct != UINT8_C(0)) {
        cpu->arch_state.status.st1 =
            (uint16_t)(cpu->arch_state.status.st1 |
                       TI_C54X_ST1_FRCT_MASK);
    }
    if (ovm != UINT8_C(0)) {
        cpu->arch_state.status.st1 =
            (uint16_t)(cpu->arch_state.status.st1 |
                       TI_C54X_ST1_OVM_MASK);
    }
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_SXM_MASK | TI_C54X_ST1_FRCT_MASK |
                   TI_C54X_ST1_OVM_MASK);
    cpu->arch_state.status.st0 = UINT16_C(0xA5C3);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
}

static void
test_ti_worked_example_retires(void)
{
    /* 0xE39A: XMOD2 XAR=AR3, YMOD2 YAR=AR4, X=0x0055 Y=0x00AA,
     * A=FF ABCD 0000 B=0 SXM=1 FRCT=0 OVM=0:
     * A=FF FFAB 0000, B=00 1BB1 8229, both ARs post-increment. */
    abdst_fixture fixture = {.program_word = UINT16_C(0xE39A),
                             .x_address = UINT16_C(0x0100),
                             .y_address = UINT16_C(0x0200),
                             .x_value = UINT16_C(0x0055),
                             .y_value = UINT16_C(0x00AA),
                             .fault_data = false,
                             .program_reads = 0U,
                             .endpoint_calls = 0U,
                             .requests = {{0}}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_abdst(&fixture, storage, true, &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    seed_state(cpu, UINT64_C(0xFFABCD0000), UINT64_C(0), UINT8_C(1),
               UINT8_C(0), UINT8_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    /* X reads before Y over the one endpoint. */
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word ==
                       fixture.x_address);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word ==
                       fixture.y_address);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0xFFFFAB0000));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 ==
                       UINT64_C(0x5433));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[3] == UINT16_C(0x0101));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[4] == UINT16_C(0x0201));
    /* C/OVA/OVB hold around the clean commit. */
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == UINT16_C(0xA5C3));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_ovm_saturation_retires(void)
{
    /* X=0x7FFF Y=0x8000 diffs to 65535: OVA with OVM saturation
     * of A to 00 7FFF FFFF while B accumulates the zero absolute.
     * 0xE312 keeps MOD0 on AR3/AR4 (no AR update). */
    abdst_fixture fixture = {.program_word = UINT16_C(0xE312),
                             .x_address = UINT16_C(0x0100),
                             .y_address = UINT16_C(0x0200),
                             .x_value = UINT16_C(0x7FFF),
                             .y_value = UINT16_C(0x8000),
                             .fault_data = false,
                             .program_reads = 0U,
                             .endpoint_calls = 0U,
                             .requests = {{0}}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_abdst(&fixture, storage, true, &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    seed_state(cpu, UINT64_C(0), UINT64_C(0), UINT8_C(1), UINT8_C(0),
               UINT8_C(1));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == UINT64_C(0));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 &
                        TI_C54X_ST0_OVA_MASK) != UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_fault_does_not_commit(void)
{
    abdst_fixture fixture = {.program_word = UINT16_C(0xE39A),
                             .x_address = UINT16_C(0x0100),
                             .y_address = UINT16_C(0x0200),
                             .x_value = UINT16_C(0x0055),
                             .y_value = UINT16_C(0x00AA),
                             .fault_data = true,
                             .program_reads = 0U,
                             .endpoint_calls = 0U,
                             .requests = {{0}}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_abdst(&fixture, storage, true, &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    seed_state(cpu, UINT64_C(0xFFABCD0000), UINT64_C(0), UINT8_C(1),
               UINT8_C(0), UINT8_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0xFFABCD0000));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[3] == UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[4] == UINT16_C(0x0200));
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == UINT16_C(0xA5C3));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_y_rejects(void)
{
    /* Only the X AR is seeded; the reset-unknown Y AR gates
     * UNKNOWN_INPUT with nothing committed. */
    abdst_fixture fixture = {.program_word = UINT16_C(0xE39A),
                             .x_address = UINT16_C(0x0100),
                             .y_address = UINT16_C(0x0200),
                             .x_value = UINT16_C(0x0055),
                             .y_value = UINT16_C(0x00AA),
                             .fault_data = false,
                             .program_reads = 0U,
                             .endpoint_calls = 0U,
                             .requests = {{0}}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_abdst(&fixture, storage, false, &status);
    ti_c54x_run_result result;

    (void)status;
    seed_state(cpu, UINT64_C(0xFFABCD0000), UINT64_C(0), UINT8_C(1),
               UINT8_C(0), UINT8_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_ti_worked_example_retires();
    test_ovm_saturation_retires();
    test_fault_does_not_commit();
    test_unknown_y_rejects();
    return 0;
}
