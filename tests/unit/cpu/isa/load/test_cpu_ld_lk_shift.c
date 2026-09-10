/* Module contract
 * Owns: public-CPU RED proofs for the two-word `LD #lk[,SHFT],dst`
 * load, including SXM extension of the copied extension immediate,
 * unsigned 4-bit shift with OVM saturation and sticky destination
 * overflow, C/TC preservation, extension fetch ordering, two-word
 * retirement, and transactional unknown-SXM/OVM rejection without
 * endpoint traffic.
 * Does not own: production CPU behavior, catalog/decode or pure load
 * effects, address planning, shared fixtures, timing, diagnostics, or
 * test registration outside the load leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/foundation/arithmetic40.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct ld_lk_shift_fixture {
    uint16_t words[2];
    unsigned program_calls;
    ti_c54x_program_word_address addresses[2];
} ld_lk_shift_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    ld_lk_shift_fixture *fixture = (ld_lk_shift_fixture *)context;
    const unsigned index = fixture->program_calls;

    TI_C54X_TEST_CHECK(index < 2U);
    TI_C54X_TEST_CHECK(address ==
                       (ti_c54x_program_word_address)index);
    TI_C54X_TEST_CHECK(value != NULL);
    fixture->addresses[index] = address;
    *value = fixture->words[index];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static ti_c54x_cpu *
init_cpu(ld_lk_shift_fixture *fixture, unsigned char *storage)
{
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program};
    const ti_c54x_config config = {UINT32_C(0), {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* FORM-T2-19-08: 1111_000D_0010_SHFT: destination bit 8, unsigned
 * 4-bit SHFT in bits 3:0, all other first-word bits fixed; word1 is
 * the copied raw lk. */
static uint16_t
ld_lk_shift_word0(uint8_t destination, uint8_t shift)
{
    return (uint16_t)(UINT16_C(0xF020) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (shift & UINT8_C(0x0F)));
}

static void
seed_cpu(ti_c54x_cpu *cpu, uint8_t sxm, uint8_t ovm)
{
    cpu->arch_state.status.st0 = UINT16_C(0x0000);
    cpu->arch_state.status.st1 = (uint16_t)(
        (sxm != UINT8_C(0) ? TI_C54X_ST1_SXM_MASK : UINT16_C(0)) |
        (ovm != UINT8_C(0) ? TI_C54X_ST1_OVM_MASK : UINT16_C(0)));
    cpu->arch_state.status.pmst = UINT16_C(0x0000);
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

/* Independent oracle mirroring the ops contract. */
static uint64_t
expected_lk_shift(uint16_t immediate, uint8_t sxm, uint8_t shift,
                  uint8_t ovm, bool *overflow)
{
    uint64_t widened = (uint64_t)immediate;
    uint64_t value;
    uint64_t guard;
    uint8_t bit31;
    bool negative;

    if (sxm != UINT8_C(0) &&
        (immediate & UINT16_C(0x8000)) != UINT16_C(0)) {
        widened |= (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF));
    }
    negative = (widened & TI_C54X_U40_SIGN_BIT) != UINT64_C(0);
    value = (widened << shift) & TI_C54X_U40_MASK;
    guard = (value >> UINT64_C(32)) & UINT64_C(0xFF);
    bit31 = (uint8_t)((value >> UINT64_C(31)) & UINT64_C(1));
    *overflow = guard != (bit31 != UINT8_C(0) ? UINT64_C(0xFF)
                                              : UINT64_C(0));
    if (*overflow && ovm != UINT8_C(0)) {
        value = negative ? UINT64_C(0xFF80000000) : UINT64_C(0x007FFFFFFF);
    }
    return value;
}

static void
check_ov(const ti_c54x_cpu *cpu, uint8_t destination, bool overflow)
{
    const uint16_t mask = destination == UINT8_C(0) ? TI_C54X_ST0_OVA_MASK
                                                   : TI_C54X_ST0_OVB_MASK;

    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & mask) ==
                       (overflow ? mask : UINT16_C(0)));
}

static void
check_ready_case(uint8_t destination, uint8_t shift, uint8_t sxm,
                 uint8_t ovm, uint16_t immediate)
{
    ld_lk_shift_fixture fixture = {
        .words = {ld_lk_shift_word0(destination, shift), immediate},
        .program_calls = 0U,
        .addresses = {0U, 0U}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
    bool overflow = false;
    const uint64_t expected =
        expected_lk_shift(immediate, sxm, shift, ovm, &overflow);
    ti_c54x_run_result result;
    ti_c54x_state state;

    seed_cpu(cpu, sxm, ovm);
    {
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;
        const uint16_t before_st1 = cpu->arch_state.status.st1;
        const ti_c54x_t_register_state before_t = cpu->t_register;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.opcode_word == fixture.words[0]);
        TI_C54X_TEST_CHECK(result.opcode_valid);
        TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
        TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0));
        TI_C54X_TEST_CHECK(fixture.addresses[1] == UINT32_C(1));
        TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
        if (destination == UINT8_C(0)) {
            TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 == expected);
            TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                               before_accumulators.b_raw40);
        } else {
            TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 == expected);
            TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                               before_accumulators.a_raw40);
        }
        /* C/TC/ST1 preserved; only the destination OV flag may set. */
        TI_C54X_TEST_CHECK(
            (cpu->arch_state.status.st0 &
             (uint16_t)(UINT16_C(0xFFFF) & ~(TI_C54X_ST0_OVA_MASK |
                                             TI_C54X_ST0_OVB_MASK))) ==
            UINT16_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == before_st1);
        check_ov(cpu, destination, overflow);
        TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                  sizeof(before_t)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(2));
        TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(1));
        TI_C54X_TEST_CHECK((destination == UINT8_C(0)
                                ? state.accumulator_a_raw40
                                : state.accumulator_b_raw40) == expected);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_ready_retires_all_shifts(void)
{
    static const uint16_t boundaries[] = {
        UINT16_C(0x0000), UINT16_C(0x0001), UINT16_C(0x00FF),
        UINT16_C(0x0100), UINT16_C(0x4000), UINT16_C(0x7FFF),
        UINT16_C(0x8000), UINT16_C(0x8001), UINT16_C(0xC000),
        UINT16_C(0xFEDC), UINT16_C(0xFFFF)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        for (uint8_t shift = 0U; shift <= UINT8_C(15); ++shift) {
            for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
                for (uint8_t ovm = 0U; ovm <= UINT8_C(1); ++ovm) {
                    size_t index;

                    for (index = 0U;
                         index <
                         sizeof(boundaries) / sizeof(boundaries[0]);
                         ++index) {
                        check_ready_case(destination, shift, sxm, ovm,
                                         boundaries[index]);
                    }
                    /* Prime-stride sweep covers the remaining lk
                     * space, including saturation corners. */
                    for (uint32_t raw = 0U; raw <= UINT32_C(0xFFFF);
                         raw += 257U) {
                        check_ready_case(destination, shift, sxm, ovm,
                                         (uint16_t)raw);
                    }
                }
            }
        }
    }
}

static void
test_unknown_inputs_reject(void)
{
    ld_lk_shift_fixture fixture = {
        .words = {ld_lk_shift_word0(UINT8_C(0), UINT8_C(4)),
                  UINT16_C(0x1234)},
        .program_calls = 0U,
        .addresses = {0U, 0U}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT8_C(1), UINT8_C(1));
    cpu->arch_state.status.st1_known_mask = (uint16_t)(
        cpu->arch_state.status.st1_known_mask &
        (uint16_t)~TI_C54X_ST1_SXM_MASK);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }

    seed_cpu(cpu, UINT8_C(1), UINT8_C(1));
    fixture.program_calls = 0U;
    fixture.addresses[0] = 0U;
    fixture.addresses[1] = 0U;
    cpu->arch_state.status.st1_known_mask = (uint16_t)(
        cpu->arch_state.status.st1_known_mask &
        (uint16_t)~TI_C54X_ST1_OVM_MASK);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
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
    test_ready_retires_all_shifts();
    test_unknown_inputs_reject();
    return 0;
}
