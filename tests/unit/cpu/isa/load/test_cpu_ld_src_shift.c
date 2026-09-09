/* Module contract
 * Owns: public-CPU RED proofs for `LD src[,SHIFT],dst`, including both
 * accumulators as source, all 32 shift encodings, SXM-dependent right
 * shifts, OVM saturation with overflow-bit staging, non-destination
 * preservation, and transactional rejection.
 * Does not own: production CPU behavior, catalog/decode or pure load
 * effects, address planning, shared fixture implementation, timing,
 * diagnostics, or test registration outside the load leaf.
 */

#include "../fixture.h"

#include "../../../../../src/foundation/arithmetic40.h"
#include "../../../../../src/state/register_layout.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* FORM-T2-19-11: source bit 9, destination bit 8, 5-bit shift in bits 4:0;
 * fixed bits 0xFCE0 hold 0xF440. Matches the shared SHIFT5 layout set. */
static uint16_t
ld_src_shift_word(uint8_t source, uint8_t destination, uint8_t shift_raw5)
{
    return (uint16_t)(UINT16_C(0xF440) |
                      ((uint16_t)(source & UINT8_C(1)) << 9U) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      (uint16_t)(shift_raw5 & UINT8_C(0x1F)));
}

/* Independent oracle: mirrors the staged handler value selection. */
static uint64_t
expected_shift(uint64_t source_raw40, uint8_t shift_raw5, uint8_t sxm,
               uint8_t ovm, bool *overflow_out)
{
    uint64_t value_raw40;
    uint64_t guard;
    uint8_t bit31;
    bool overflow;
    bool negative =
        (source_raw40 & TI_C54X_U40_SIGN_BIT) != UINT64_C(0);

    if (shift_raw5 <= UINT8_C(15)) {
        value_raw40 =
            ti_c54x_u40_logical_left_shift(source_raw40, shift_raw5);
    } else if (sxm != UINT8_C(0)) {
        value_raw40 = ti_c54x_u40_arithmetic_right_shift(
            source_raw40, (uint64_t)(UINT8_C(32) - shift_raw5));
    } else {
        value_raw40 = ti_c54x_u40_mask(
            source_raw40 >> (UINT8_C(32) - shift_raw5));
    }
    guard = (value_raw40 >> UINT64_C(32)) & UINT64_C(0xFF);
    bit31 = (uint8_t)((value_raw40 >> UINT64_C(31)) & UINT64_C(1));
    overflow =
        guard != (bit31 != UINT8_C(0) ? UINT64_C(0xFF) : UINT64_C(0));
    if (overflow && ovm != UINT8_C(0)) {
        value_raw40 =
            negative ? UINT64_C(0xFF80000000) : UINT64_C(0x007FFFFFFF);
    }
    *overflow_out = overflow;
    return value_raw40;
}

static ti_c54x_cpu *
init_load(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
          uint8_t source, uint8_t destination, uint8_t shift_raw5,
          uint64_t source_value, uint64_t other_value, uint8_t sxm,
          uint8_t ovm, ti_c54x_error *status)
{
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, fixture, false, status);

    /* FORM-T2-19-11 uses no address register; keep the shared seeder
     * referenced so the header stays warning-clean. */
    (void)ti_c54x_cpu_isa_seed_address;
    (void)destination;
    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_sxm(cpu, sxm);
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 &
                   (uint16_t)~TI_C54X_ST1_OVM_MASK);
    if (ovm != UINT8_C(0)) {
        cpu->arch_state.status.st1 = (uint16_t)(
            cpu->arch_state.status.st1 | TI_C54X_ST1_OVM_MASK);
    }
    cpu->arch_state.status.st1_known_mask = (uint16_t)(
        cpu->arch_state.status.st1_known_mask | TI_C54X_ST1_OVM_MASK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           source == UINT8_C(0) ? TI_C54X_ACCUMULATOR_A
                                                : TI_C54X_ACCUMULATOR_B,
                           source_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           source == UINT8_C(0) ? TI_C54X_ACCUMULATOR_B
                                                : TI_C54X_ACCUMULATOR_A,
                           other_value) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;

    /* Make C, TC, and both accumulator overflow bits visibly non-default. */
    cpu->arch_state.status.st0 = (uint16_t)(
        cpu->arch_state.status.st0 | TI_C54X_ST0_TC_MASK |
        TI_C54X_ST0_C_MASK | TI_C54X_ST0_OVA_MASK | TI_C54X_ST0_OVB_MASK);
    (void)shift_raw5;
    return cpu;
}

static void
check_overflow_bit(const ti_c54x_cpu *cpu, uint8_t destination,
                   bool overflow, uint16_t before_st0)
{
    uint16_t mask = destination == UINT8_C(0) ? TI_C54X_ST0_OVA_MASK
                                              : TI_C54X_ST0_OVB_MASK;

    if (overflow) {
        TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & mask) !=
                           UINT16_C(0));
    } else {
        TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & mask) ==
                           (before_st0 & mask));
    }
}

static void
test_ready_all_sources_shifts_and_modes(void)
{
    static const uint64_t sources[] = {
        UINT64_C(0x0000000000), UINT64_C(0x0000000001),
        UINT64_C(0x007FFFFFFF), UINT64_C(0x0080000000),
        UINT64_C(0x00ABCDEF12), UINT64_C(0xFF80000000),
        UINT64_C(0xFFFFFFFFF0), UINT64_C(0xFFFFFFFFFF)};
    static const uint64_t others[] = {UINT64_C(0x1111222233),
                                      UINT64_C(0x0000AABBCC)};

    for (uint8_t source = 0U; source <= UINT8_C(1); ++source) {
        for (uint8_t destination = 0U; destination <= UINT8_C(1);
             ++destination) {
            for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
                for (uint8_t ovm = 0U; ovm <= UINT8_C(1); ++ovm) {
                    for (uint8_t shift = 0U; shift < UINT8_C(32);
                         ++shift) {
                        for (size_t source_index = 0U;
                             source_index < sizeof(sources) /
                                                sizeof(sources[0]);
                             ++source_index) {
                            ti_c54x_cpu_isa_memory_fixture fixture = {
                                .mode = TI_C54X_CPU_ISA_MEMORY_READY,
                                .program_word = ld_src_shift_word(
                                    source, destination, shift),
                                .read_value_word = UINT16_C(0),
                                .fault_status = TI_C54X_BUS_FAULT};
                            _Alignas(max_align_t)
                                unsigned char storage[1024] = {0};
                            ti_c54x_error status;
                            ti_c54x_cpu *cpu;
                            bool overflow = false;
                            uint64_t expected;
                            uint64_t other_value =
                                others[(size_t)(destination == source
                                                    ? 0U
                                                    : destination)];
                            cpu = init_load(
                                &fixture, storage, source, destination,
                                shift, sources[source_index],
                                other_value, sxm, ovm, &status);
                            expected = expected_shift(
                                sources[source_index], shift, sxm, ovm,
                                &overflow);
                            {
                                const ti_c54x_accumulator_state
                                    before_accumulators =
                                        cpu->accumulators;
                                const ti_c54x_status_register_state
                                    before_status =
                                        cpu->arch_state.status;
                                const ti_c54x_t_register_state before_t =
                                    cpu->t_register;
                                ti_c54x_run_result result =
                                    ti_c54x_cpu_run(cpu, UINT64_C(1));
                                ti_c54x_state state;

                                TI_C54X_TEST_CHECK(
                                    result.reason ==
                                    TI_C54X_STOP_BUDGET_EXHAUSTED);
                                TI_C54X_TEST_CHECK(result.bus_status ==
                                                   TI_C54X_BUS_OK);
                                TI_C54X_TEST_CHECK(
                                    result.instructions_attempted ==
                                    UINT64_C(1));
                                TI_C54X_TEST_CHECK(
                                    result.instructions_retired ==
                                    UINT64_C(1));
                                TI_C54X_TEST_CHECK(result.pc_word ==
                                                   UINT32_C(1));
                                TI_C54X_TEST_CHECK(
                                    result.opcode_valid);
                                TI_C54X_TEST_CHECK(
                                    fixture.program_reads == 1U);
                                if (destination == UINT8_C(0)) {
                                    TI_C54X_TEST_CHECK(
                                        cpu->accumulators.a_raw40 ==
                                        expected);
                                    TI_C54X_TEST_CHECK(
                                        cpu->accumulators.b_raw40 ==
                                        before_accumulators.b_raw40);
                                } else {
                                    TI_C54X_TEST_CHECK(
                                        cpu->accumulators.b_raw40 ==
                                        expected);
                                    TI_C54X_TEST_CHECK(
                                        cpu->accumulators.a_raw40 ==
                                        before_accumulators.a_raw40);
                                }
                                if (destination != source) {
                                    TI_C54X_TEST_CHECK(
                                        (source == UINT8_C(0)
                                             ? cpu->accumulators.a_raw40
                                             : cpu->accumulators
                                                   .b_raw40) ==
                                        sources[source_index]);
                                } else {
                                    TI_C54X_TEST_CHECK(
                                        (source == UINT8_C(0)
                                             ? cpu->accumulators.b_raw40
                                             : cpu->accumulators
                                                   .a_raw40) ==
                                        other_value);
                                }
                                check_overflow_bit(cpu, destination,
                                                   overflow,
                                                   before_status.st0);
                                TI_C54X_TEST_CHECK(
                                    (cpu->arch_state.status.st0 &
                                     (destination == UINT8_C(0)
                                          ? TI_C54X_ST0_OVB_MASK
                                          : TI_C54X_ST0_OVA_MASK)) ==
                                    (before_status.st0 &
                                     (destination == UINT8_C(0)
                                          ? TI_C54X_ST0_OVB_MASK
                                          : TI_C54X_ST0_OVA_MASK)));
                                TI_C54X_TEST_CHECK(
                                    (cpu->arch_state.status.st0 &
                                     (uint16_t)~(TI_C54X_ST0_OVA_MASK |
                                                 TI_C54X_ST0_OVB_MASK)) ==
                                    (before_status.st0 &
                                     (uint16_t)~(TI_C54X_ST0_OVA_MASK |
                                                 TI_C54X_ST0_OVB_MASK)));
                                TI_C54X_TEST_CHECK(
                                    cpu->arch_state.status.st1 ==
                                    before_status.st1);
                                TI_C54X_TEST_CHECK(
                                    cpu->arch_state.status
                                            .st1_known_mask ==
                                    before_status.st1_known_mask);
                                TI_C54X_TEST_CHECK(
                                    ti_c54x_cpu_inspect(cpu, &state) ==
                                    TI_C54X_ERROR_OK);
                                TI_C54X_TEST_CHECK(state.pc_word ==
                                                   UINT32_C(1));
                                TI_C54X_TEST_CHECK(
                                    state.instructions_retired_total ==
                                    UINT64_C(1));
                                TI_C54X_TEST_CHECK(
                                    memcmp(&cpu->t_register, &before_t,
                                           sizeof(before_t)) == 0);
                                TI_C54X_TEST_CHECK(
                                    state.accumulator_a_known &&
                                    state.accumulator_b_known);
                                TI_C54X_TEST_CHECK(
                                    ti_c54x_cpu_deinit(cpu) ==
                                    TI_C54X_ERROR_OK);
                            }
                        }
                    }
                }
            }
        }
    }
}

/* OVM-off must preserve the raw shifted value even on overflow. */
static void
test_ovm_off_preserves_raw_shifted_value(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = ld_src_shift_word(0U, 1U, 15U),
        .read_value_word = UINT16_C(0),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_load(&fixture, storage, 0U, 1U, 15U, UINT64_C(0x0000400000),
                  UINT64_C(0x0000AABBCC), 0U, 0U, &status);
    const uint16_t before_st0 = cpu->arch_state.status.st0;
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    bool overflow = false;
    uint64_t expected = expected_shift(UINT64_C(0x0000400000), 15U, 0U,
                                       0U, &overflow);

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(overflow);
    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 == expected);
    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 !=
                       UINT64_C(0x007FFFFFFF));
    check_overflow_bit(cpu, 1U, true, before_st0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    (void)status;
}

/* Unknown source accumulator rejects transactionally. */
static void
test_unknown_source_rejects_transactionally(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = ld_src_shift_word(1U, 0U, 3U),
        .read_value_word = UINT16_C(0),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_load(&fixture, storage, 1U, 0U, 3U, UINT64_C(0x00000000FF),
                  UINT64_C(0x1111222233), 0U, 0U, &status);
    ti_c54x_run_result result;

    cpu->accumulators.known_mask = (uint8_t)(
        cpu->accumulators.known_mask & (uint8_t)~UINT8_C(2));
    {
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;
        const ti_c54x_status_register_state before_status =
            cpu->arch_state.status;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state.status, &before_status,
                                  sizeof(before_status)) == 0);
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
        (void)status;
    }
}

int
main(void)
{
    test_ready_all_sources_shifts_and_modes();
    test_ovm_off_preserves_raw_shifted_value();
    test_unknown_source_rejects_transactionally();
    return 0;
}
