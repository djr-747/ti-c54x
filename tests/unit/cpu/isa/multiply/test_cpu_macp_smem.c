/* Module contract
 * Owns: public-CPU RED proofs for `MACP Smem,Pmad` retirement,
 * including ordered Smem-DATA/coefficient-PROGRAM reads, the
 * independently recomputed oracle vector with read-phase Smem T
 * load, the FRCT doubling lane, MOD2 post-increment of the Smem
 * AR, and accumulator plus overflow commit.
 * Does not own: catalog/decode or pure multiply effects,
 * address-planner internals, endpoint reducer behavior, production
 * code, timing, or diagnostics.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>

typedef struct macp_fixture {
    uint16_t program_words[2];
    uint16_t smem_address;
    uint16_t smem_value;
    uint16_t pmad;
    uint16_t coeff_value;
    unsigned program_reads;
    unsigned endpoint_calls;
    ti_c54x_bus_word16_request requests[4];
} macp_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    macp_fixture *fixture = (macp_fixture *)context;

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
    macp_fixture *fixture = (macp_fixture *)context;
    const unsigned index = fixture->endpoint_calls++;

    TI_C54X_TEST_CHECK(index < 2U);
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN);
    fixture->requests[index] = *request;
    if (index == 0U) {
        TI_C54X_TEST_CHECK(request->space ==
                           TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
        TI_C54X_TEST_CHECK(request->address_word == fixture->smem_address);
    } else {
        TI_C54X_TEST_CHECK(request->space ==
                           TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM);
        TI_C54X_TEST_CHECK(request->address_word == fixture->pmad);
    }
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word =
            index == 0U ? fixture->smem_value : fixture->coeff_value,
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
init_macp(macp_fixture *fixture, void *storage, ti_c54x_error *status_out,
          bool frct)
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
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state, 5U,
                                                   fixture->smem_address) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0)) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x0000);
    cpu->t_register.known = true;
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_FRCT_MASK);
    if (frct) {
        cpu->arch_state.status.st1 =
            (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_FRCT_MASK);
    }
    return cpu;
}

static void
run_case(uint16_t opcode, uint16_t pmad, uint16_t smem_address,
         uint64_t expected_b, uint16_t expected_ar5, bool frct)
{
    /* Oracle shape: B=0 plus Smem 0x4000 times coefficient 0x4000
     * equals 0x10000000 (FRCT=0) or 0x20000000 (FRCT=1); T takes
     * the Smem word. */
    macp_fixture fixture = {.program_words = {opcode, pmad},
                            .smem_address = smem_address,
                            .smem_value = UINT16_C(0x4000),
                            .pmad = pmad,
                            .coeff_value = UINT16_C(0x4000),
                            .program_reads = 0U,
                            .endpoint_calls = 0U,
                            .requests = {{0}}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_macp(&fixture, storage, &status, frct);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(fixture.program_reads == 2U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    /* Smem DATA reads before the PROGRAM coefficient over the one
     * endpoint. */
    TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[1].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == expected_b);
    /* Read-phase T load takes the Smem word. */
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x4000));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[5] == expected_ar5);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_indirect_mod2_retires(void)
{
    /* 0x7995: S=1 (dst B), *AR5+ MOD2 over 0x0100: B=0x10000000
     * with AR5=0x0101. */
    run_case(UINT16_C(0x7995), UINT16_C(0x0100), UINT16_C(0x0100),
             UINT64_C(0x10000000), UINT16_C(0x0101), false);
}

static void
test_frct_doubles(void)
{
    /* Same encoding with FRCT=1: B=0x20000000. */
    run_case(UINT16_C(0x7995), UINT16_C(0x0100), UINT16_C(0x0100),
             UINT64_C(0x20000000), UINT16_C(0x0101), true);
}

static void
test_direct_retires(void)
{
    /* 0x7834: S=0... direct dma 0x34: A accumulates, AR5 untouched. */
    macp_fixture fixture = {.program_words = {UINT16_C(0x7834),
                                              UINT16_C(0x0100)},
                            .smem_address = UINT16_C(0x0034),
                            .smem_value = UINT16_C(0x4000),
                            .pmad = UINT16_C(0x0100),
                            .coeff_value = UINT16_C(0x4000),
                            .program_reads = 0U,
                            .endpoint_calls = 0U,
                            .requests = {{0}}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_macp(&fixture, storage, &status, false);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(fixture.program_reads == 2U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    /* S=0 selects A: A=0x10000000, B stays 0. */
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0x10000000));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x4000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_indirect_mod2_retires();
    test_frct_doubles();
    test_direct_retires();
    return 0;
}
