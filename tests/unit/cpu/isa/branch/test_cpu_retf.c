/* Module contract
 * Owns: public-CPU RED proofs for the one-word `RETF` fast
 * interrupt return, including the worked-example vector with RTN
 * capture, single-word fetch ordering with zero data-endpoint
 * traffic, unknown-RTN and unknown-SP transactionality, and the
 * CALL-then-RETF chain proving push-commit RTN staging, with
 * full architectural preservation outside SP/PC/INTM.
 * Does not own: production CPU behavior, catalog/decode or pure
 * return effects, address planning, shared fixtures, timing,
 * diagnostics, or test registration outside the branch leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/arch_state.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct retf_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
    ti_c54x_bus_word16_request data_request;
    unsigned data_calls;
} retf_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    retf_fixture *fixture = (retf_fixture *)context;
    uint32_t offset;

    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address >= fixture->base);
    offset = address - fixture->base;
    TI_C54X_TEST_CHECK(offset < fixture->word_count);
    *value = fixture->words[offset];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    retf_fixture *fixture = (retf_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->data_request = *request;
    fixture->data_calls++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
}

static ti_c54x_bus_word16_cancel_result
cancel_data(void *context, uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_cpu *
init_cpu(retf_fixture *fixture, unsigned char *storage,
         uint32_t reset_pc)
{
    const ti_c54x_bus bus = {
        .context = fixture,
        .program_read_word = read_program,
        .data_endpoint = {.context = fixture,
                          .advance = advance_data,
                          .cancel = cancel_data}};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* Table 2-15 syntax 6 (SPRU172C pp. 4-140–4-141): 1111_01Z0_1001_1011
 * with Z=0. */
static const uint16_t RETF_WORD0 = UINT16_C(0xF49B);
/* CALL pmad (FORM-T2-13-02): 0xF074 followed by the 16-bit target. */
static const uint16_t CALL_WORD0 = UINT16_C(0xF074);

static void
seed_cpu(ti_c54x_cpu *cpu, uint16_t sp_value, uint16_t rtn_value)
{
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
    cpu->arch_state.status.pmst = UINT16_C(0x3C3C);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rtn(&cpu->arch_state,
                                                   rtn_value) ==
                       TI_C54X_ARCH_STATE_OK);
    /* CMPT/ARP stay unknown: RTN pops are mode-independent end to end. */
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x1111222233)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000AABBCC)) ==
                       TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;
}

static void
check_ready_case(uint32_t start_pc, uint16_t sp_before,
                 uint16_t rtn_value)
{
    retf_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_state state;
    const uint16_t expected_sp = (uint16_t)(sp_before + UINT16_C(1));

    fixture.base = start_pc;
    fixture.words[0] = RETF_WORD0;
    fixture.word_count = 1U;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_cpu(cpu, sp_before, rtn_value);
    const ti_c54x_arch_state before_arch = cpu->arch_state;
    const ti_c54x_accumulator_state before_accumulators =
        cpu->accumulators;
    const ti_c54x_t_register_state before_t = cpu->t_register;

    /* RETF retires at once with no data-endpoint traffic: PC takes
     * RTN, SP advances past the discarded slot, and INTM clears. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == RETF_WORD0);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
    TI_C54X_TEST_CHECK(result.pc_word == (uint32_t)rtn_value);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)rtn_value);
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_sp);
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == UINT16_C(0x525A));
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                              sizeof(before_accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                              sizeof(before_t)) == 0);

    /* Only SP, PC, and ST1.INTM may move; RTN is preserved. */
    {
        ti_c54x_arch_state expect_arch = before_arch;

        expect_arch.pc_word = (uint32_t)rtn_value;
        expect_arch.sp = expected_sp;
        expect_arch.status.st1 = UINT16_C(0x525A);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &expect_arch,
                                  sizeof(expect_arch)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == (uint32_t)rtn_value);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_origins_and_targets(void)
{
    /* TI SPRU172C RETF example (p. 4-141): PC 01C3 and SP 2001
     * become PC 0110 and SP 2002 with ST1 bit 11 (INTM) cleared. */
    check_ready_case(UINT32_C(0x01C3), UINT16_C(0x2001),
                     UINT16_C(0x0110));
    check_ready_case(UINT32_C(0x1000), UINT16_C(0xFFFF),
                     UINT16_C(0x2000));
    check_ready_case(UINT32_C(0x0000), UINT16_C(0x0000),
                     UINT16_C(0x0000));
}

static void
test_call_then_retf_chain(void)
{
    /* A CALL stages RTN at push-commit time; the chained RETF then
     * returns to the pushed address with no stack read. */
    retf_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    uint16_t rtn_value = UINT16_C(0);

    fixture.base = UINT32_C(0x1000);
    fixture.words[0] = CALL_WORD0;
    fixture.words[1] = UINT16_C(0x1002);
    fixture.words[2] = RETF_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, UINT16_C(0x0300), UINT16_C(0x0000));
    /* RTN starts unknown: the reset seed carries no return. */
    cpu->arch_state.scalar_known_mask = (uint8_t)(
        cpu->arch_state.scalar_known_mask &
        (uint8_t)~TI_C54X_ARCH_STATE_KNOWN_RTN);

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == CALL_WORD0);
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1002));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x02FF));
    /* The push wrote PC+2 at old SP - 1 and staged the same
     * address into RTN. */
    TI_C54X_TEST_CHECK(fixture.data_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.data_request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.data_request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(fixture.data_request.address_word ==
                       UINT16_C(0x02FF));
    TI_C54X_TEST_CHECK(fixture.data_request.write_value_word ==
                       UINT16_C(0x1002));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rtn(&cpu->arch_state,
                                                  &rtn_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(rtn_value == UINT16_C(0x1002));
    /* CALL leaves INTM set. */
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == UINT16_C(0x5A5A));

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == RETF_WORD0);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_calls == 3U);
    /* No new data-endpoint traffic: the return comes from RTN. */
    TI_C54X_TEST_CHECK(fixture.data_calls == 1U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1002));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0300));
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == UINT16_C(0x525A));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_rtn_rejects(void)
{
    retf_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x1000);
    fixture.words[0] = RETF_WORD0;
    fixture.word_count = 1U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, UINT16_C(0x0300), UINT16_C(0x0110));
    cpu->arch_state.scalar_known_mask = (uint8_t)(
        cpu->arch_state.scalar_known_mask &
        (uint8_t)~TI_C54X_ARCH_STATE_KNOWN_RTN);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        /* RETF with unknown RTN rejects without endpoint traffic. */
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_sp_rejects(void)
{
    retf_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x1000);
    fixture.words[0] = RETF_WORD0;
    fixture.word_count = 1U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));

    /* Seed everything except SP: reset leaves SP unknown. */
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rtn(&cpu->arch_state,
                                                   UINT16_C(0x0110)) ==
                       TI_C54X_ARCH_STATE_OK);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    test_origins_and_targets();
    test_call_then_retf_chain();
    test_unknown_rtn_rejects();
    test_unknown_sp_rejects();
    return 0;
}
