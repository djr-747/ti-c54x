/* Module contract
 * Owns: public-CPU RED proofs for the one-word `FBACC` far
 * accumulator branch, including the TI SPRU172C vector (A
 * 00 0001 3000: PC 1000 to 3000, XPC 00 to 01) for both S
 * source selects, immediate PC-plus-XPC commit, and full
 * architectural preservation outside PC/XPC without endpoint
 * traffic.
 * Does not own: production CPU behavior, catalog/decode or pure
 * branch effects, address planning, shared fixtures, timing,
 * diagnostics, or test registration outside the branch leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/program_counter_extension.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct fbacc_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
} fbacc_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    fbacc_fixture *fixture = (fbacc_fixture *)context;
    uint32_t offset;

    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address >= fixture->base);
    offset = address - fixture->base;
    TI_C54X_TEST_CHECK(offset < fixture->word_count);
    *value = fixture->words[offset];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static ti_c54x_cpu *
init_cpu(fbacc_fixture *fixture, unsigned char *storage, uint32_t reset_pc)
{
    /* No DATA endpoint: FBACC retires with immediate PC-plus-XPC
     * commit only, so any endpoint traffic would fault here by
     * construction. */
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* FORM-T2-12: 1111_01ZS_1110_0110; S bit 8 selects the source. */
static const uint16_t FBACC_WORD_S0 = UINT16_C(0xF4E6);
static const uint16_t FBACC_WORD_S1 = UINT16_C(0xF5E6);

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
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;
}

static void
check_vector(uint16_t word, bool use_b, uint64_t target, uint32_t expect_pc,
             uint16_t expect_xpc)
{
    fbacc_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    uint16_t xpc = UINT16_C(0xFFFF);

    fixture.base = UINT32_C(0x1000);
    fixture.words[0] = word;
    fixture.word_count = 1U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           use_b ? TI_C54X_ACCUMULATOR_B
                                 : TI_C54X_ACCUMULATOR_A,
                           target) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           use_b ? TI_C54X_ACCUMULATOR_A
                                 : TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x2ABCDEF012)) ==
                       TI_C54X_ACCUMULATOR_OK);

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == word);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == expect_pc);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(&cpu->arch_state, &xpc) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(xpc == expect_xpc);
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_fbacc_vectors(void)
{
    /* TI SPRU172C FBACC vector: A 00 0001 3000 takes PC 1000 to
     * 3000 with XPC 00 to 01. */
    check_vector(FBACC_WORD_S0, false, UINT64_C(0x0000013000),
                 UINT32_C(0x3000), UINT16_C(0x01));
    /* B-source sibling with a full-page target. */
    check_vector(FBACC_WORD_S1, true, UINT64_C(0x00007F2000),
                 UINT32_C(0x2000), UINT16_C(0x7F));
}

int
main(void)
{
    test_fbacc_vectors();
    puts("cpu_fbacc: ok");
    return 0;
}
