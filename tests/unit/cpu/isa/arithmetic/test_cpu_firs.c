/*
 * Module contract
 *
 * Owns: public-CPU proofs for `FIRS Xmem,Ymem,pmad`
 * retirement, including the ordered X/Y DATA reads plus the
 * PROGRAM coefficient read, MOD2 post-increment of both ARs,
 * the accumulator pair commit, fault atomicity, and
 * unknown-input rejection.
 * Does not own: catalog/decode or pure FIRS effects, address
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

typedef struct firs_fixture {
    uint16_t program_words[2];
    uint16_t x_address;
    uint16_t y_address;
    uint16_t x_value;
    uint16_t y_value;
    uint16_t pmad;
    uint16_t coeff_value;
    bool fault_data;
    unsigned program_reads;
    unsigned endpoint_calls;
    ti_c54x_bus_word16_request requests[4];
} firs_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    firs_fixture *fixture = (firs_fixture *)context;

    TI_C54X_TEST_CHECK(value_word != NULL);
    TI_C54X_TEST_CHECK(fixture->program_reads < 2U);
    TI_C54X_TEST_CHECK(address_word == fixture->program_reads);
    fixture->program_reads++;
    *value_word = fixture->program_words[address_word];
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    firs_fixture *fixture = (firs_fixture *)context;
    const unsigned index = fixture->endpoint_calls++;

    TI_C54X_TEST_CHECK(index < 3U);
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN);
    fixture->requests[index] = *request;
    if (index < 2U) {
        const bool is_x =
            request->address_word == fixture->x_address;

        TI_C54X_TEST_CHECK(request->space ==
                           TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
        TI_C54X_TEST_CHECK(is_x ||
                           request->address_word == fixture->y_address);
        *observation = (ti_c54x_bus_word16_observation){
            .transaction_id = transaction_id,
            .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
            .read_value_word =
                is_x ? fixture->x_value : fixture->y_value,
            .bus_status = TI_C54X_BUS_OK};
        return;
    }
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM);
    TI_C54X_TEST_CHECK(request->address_word == fixture->pmad);
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = fixture->coeff_value,
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
init_firs(firs_fixture *fixture, void *storage, ti_c54x_error *status_out)
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
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state, 4U,
                                                   fixture->y_address) ==
                       TI_C54X_ARCH_STATE_OK);
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
test_basic_step_retires(void)
{
    /* 0xE09A 0x1000: XMOD2 XAR=AR3, YMOD2 YAR=AR4, X=3 Y=5,
     * coef=3, A_old=00 0004 0000 B=7 SXM=0 FRCT=0 OVM=0:
     * A=00 0008 0000, B=19, both ARs post-increment, PC+2. */
    firs_fixture fixture = {.program_words = {UINT16_C(0xE09A),
                                              UINT16_C(0x1000)},
                            .x_address = UINT16_C(0x0100),
                            .y_address = UINT16_C(0x0200),
                            .x_value = UINT16_C(3),
                            .y_value = UINT16_C(5),
                            .pmad = UINT16_C(0x1000),
                            .coeff_value = UINT16_C(3),
                            .fault_data = false,
                            .program_reads = 0U,
                            .endpoint_calls = 0U,
                            .requests = {{0}}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status = TI_C54X_ERROR_OK;
    ti_c54x_cpu *cpu = init_firs(&fixture, storage, &status);
    ti_c54x_run_result result;
    uint64_t a_value = UINT64_C(0);
    uint64_t b_value = UINT64_C(0);

    seed_state(cpu, UINT64_C(0x00040000), UINT64_C(7), UINT8_C(0),
               UINT8_C(0), UINT8_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 3U);
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word ==
                       UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word ==
                       UINT16_C(0x0200));
    TI_C54X_TEST_CHECK(fixture.requests[2].address_word ==
                       UINT16_C(0x1000));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           &a_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(a_value == UINT64_C(0x00080000));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           &b_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(b_value == UINT64_C(19));
}

int
main(void)
{
    test_basic_step_retires();
    return 0;
}
