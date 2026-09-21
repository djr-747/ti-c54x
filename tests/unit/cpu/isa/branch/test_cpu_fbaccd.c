/* Module contract
 * Owns: public-CPU RED proofs for the one-word `FBACCD` far
 * accumulator delayed branch, including the TI SPRU172C vector
 * (B 00 007F 2000 with the delay slot: PC 1000 to 2000, XPC 01
 * to 7F), XPC commit with the arming retirement,
 * deferred-branch take-effect, and full architectural
 * preservation outside PC/XPC without endpoint traffic.
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

typedef struct fbaccd_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
} fbaccd_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    fbaccd_fixture *fixture = (fbaccd_fixture *)context;
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
init_cpu(fbaccd_fixture *fixture, unsigned char *storage, uint32_t reset_pc)
{
    /* No DATA endpoint: FBACCD retires with sequential PC
     * advance, XPC commit, and countdown arming only, so any
     * endpoint traffic would fault here by construction. */
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* FORM-T2-12: 1111_01ZS_1110_0110 delayed row; S bit 8 selects
 * the source. */
static const uint16_t FBACCD_WORD_S1 = UINT16_C(0xF7E6);
static const uint16_t NOP_WORD0 = UINT16_C(0xF495);

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
                           UINT64_C(0x00007F2000)) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;
}

static void
test_fbaccd_vector(void)
{
    fbaccd_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    uint16_t xpc = UINT16_C(0xFFFF);

    /* TI SPRU172C FBACCD vector: B 00 007F 2000 with the delay
     * slot takes PC 1000 to 2000 with XPC 01 to 7F. Seed XPC to
     * 01 first: reset leaves it zero. */
    fixture.base = UINT32_C(0x1000);
    fixture.words[0] = FBACCD_WORD_S1;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_xpc(&cpu->arch_state,
                                                   UINT16_C(0x01)) ==
                       TI_C54X_ARCH_STATE_OK);

    /* FBACCD retires first: PC advances sequentially, XPC commits
     * with the arming retirement, and the countdown arms with
     * pmad deferred. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == FBACCD_WORD_S1);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1001));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(&cpu->arch_state, &xpc) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(xpc == UINT16_C(0x7F));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word ==
                       UINT16_C(0x2000));

    /* Both delay slots retire, then the deferred branch takes
     * effect with XPC held. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x2000));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2000));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(&cpu->arch_state, &xpc) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(xpc == UINT16_C(0x7F));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_fbaccd_vector();
    puts("cpu_fbaccd: ok");
    return 0;
}
