/* Module contract
 * Owns: public-CPU RED proofs for the two-word `B pmad`
 * unconditional branch, including verbatim pmad target commit from
 * nonzero origins, the worked-example vector, two-word retirement
 * with fetch ordering, second-word fault transactionality, and full
 * architectural preservation without endpoint traffic.
 * Does not own: production CPU behavior, catalog/decode or pure
 * branch effects, address planning, shared fixtures, timing,
 * diagnostics, or test registration outside the branch leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct b_pmad_fixture {
    uint16_t words[2];
    unsigned program_calls;
    ti_c54x_program_word_address addresses[2];
    bool fail_second;
} b_pmad_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    b_pmad_fixture *fixture = (b_pmad_fixture *)context;
    const unsigned index = fixture->program_calls;

    TI_C54X_TEST_CHECK(index < 2U);
    TI_C54X_TEST_CHECK(value != NULL);
    if (fixture->fail_second && index == 1U) {
        return TI_C54X_BUS_FAULT;
    }
    fixture->addresses[index] = address;
    *value = fixture->words[index];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static ti_c54x_cpu *
init_cpu(b_pmad_fixture *fixture, unsigned char *storage,
         uint32_t reset_pc)
{
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* FORM-T2-12-01: 1111_00Z0_0011_0011 with Z=0; word1 is the copied
 * raw pmad. */
static const uint16_t B_PMAD_WORD0 = UINT16_C(0xF073);

static void
seed_cpu(ti_c54x_cpu *cpu)
{
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
    cpu->arch_state.status.pmst = UINT16_C(0x3C3C);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x1111222233)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000AABBCC)) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;
}

static void
check_ready_case(uint32_t start_pc, uint16_t pmad)
{
    b_pmad_fixture fixture = {.words = {B_PMAD_WORD0, pmad},
                              .program_calls = 0U,
                              .addresses = {0U, 0U},
                              .fail_second = false};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, start_pc);
    ti_c54x_run_result result;
    ti_c54x_state state;

    seed_cpu(cpu);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;
        const ti_c54x_t_register_state before_t = cpu->t_register;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.opcode_word == B_PMAD_WORD0);
        TI_C54X_TEST_CHECK(result.opcode_valid);
        TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
        TI_C54X_TEST_CHECK(fixture.addresses[0] == start_pc);
        TI_C54X_TEST_CHECK(fixture.addresses[1] == start_pc + UINT32_C(1));
        /* The branch commits the verbatim target, not PC+2. */
        TI_C54X_TEST_CHECK(result.pc_word == (uint32_t)pmad);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)pmad);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                  sizeof(before_t)) == 0);

        /* Only the program counter may move. */
        {
            ti_c54x_arch_state expect_arch = before_arch;

            expect_arch.pc_word = (uint32_t)pmad;
            TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &expect_arch,
                                      sizeof(expect_arch)) == 0);
        }
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == (uint32_t)pmad);
        TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(1));
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_ready_commits_verbatim_target(void)
{
    /* Degenerate pmad values 0/1/2 exercise targets at and below the
     * sequential successor without taking it. */
    static const uint16_t boundaries[] = {
        UINT16_C(0x0000), UINT16_C(0x0001), UINT16_C(0x0002),
        UINT16_C(0x0003), UINT16_C(0x00FF), UINT16_C(0x1F44),
        UINT16_C(0x1F45), UINT16_C(0x2000), UINT16_C(0x7FFF),
        UINT16_C(0x8000), UINT16_C(0xFEDC), UINT16_C(0xFFFE),
        UINT16_C(0xFFFF)};
    size_t index;

    for (index = 0U;
         index < sizeof(boundaries) / sizeof(boundaries[0]); ++index) {
        check_ready_case(UINT32_C(0), boundaries[index]);
    }
    /* Prime-stride sweep covers the remaining pmad space. */
    for (uint32_t raw = 0U; raw <= UINT32_C(0xFFFF); raw += 257U) {
        check_ready_case(UINT32_C(0), (uint16_t)raw);
    }
}

static void
test_worked_example_from_nonzero_origin(void)
{
    /* TI SPRU172C B 2000h example: PC 1F45 becomes PC 2000. */
    check_ready_case(UINT32_C(0x1F45), UINT16_C(0x2000));
}

static void
test_second_word_fault_is_transactional(void)
{
    b_pmad_fixture fixture = {.words = {B_PMAD_WORD0, UINT16_C(0x2000)},
                              .program_calls = 0U,
                              .addresses = {0U, 0U},
                              .fail_second = true};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1F45));
    ti_c54x_run_result result;

    seed_cpu(cpu);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    test_ready_commits_verbatim_target();
    test_worked_example_from_nonzero_origin();
    test_second_word_fault_is_transactional();
    return 0;
}
