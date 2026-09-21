/* Module contract
 * Owns: public-CPU proofs for `RESET` retirement, including the TI
 * field list (ST0/ST1/PMST-IPTR, IFR clear, IPTR-derived PC),
 * preservation of unlisted state, pipeline-latch clearing, no
 * DATA endpoint traffic, and unknown-IPTR transactionality.
 * Does not own: catalog/decode or pure reset effects, applier
 * internals, production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/state/interrupt_flag_register.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static ti_c54x_cpu *
init_reset(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
           ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, true, status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    return cpu;
}

/* Dirty every RESET-listed field away from its reset value while
 * keeping IPTR known at 0x100; set sentinels on preserved state;
 * latch pipeline state. */
static void
dirty_for_reset(ti_c54x_cpu *cpu)
{
    cpu->arch_state.status.st0 = UINT16_C(0xE7FF);
    cpu->arch_state.status.st1 = UINT16_C(0x0400);
    cpu->arch_state.status.pmst = UINT16_C(0x8000);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ifr(&cpu->arch_state,
                                                   UINT16_C(0x00FF)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x123456789A)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0FEDCBA987)) ==
                       TI_C54X_ACCUMULATOR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(3), UINT16_C(0x5555));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  UINT16_C(0x7777)) ==
                       TI_C54X_ARCH_STATE_OK);
    /* NMI/maskable takes preempt fetch at the instruction boundary and
     * active repeat re-drives its body, so those latches cannot be
     * co-observed with a RESET retirement; the inert routing table
     * and frozen take source prove the latch-clearing block. */
    cpu->interrupt_vectors[2] = UINT16_C(0x1234);
    cpu->interrupt_vectors_known = UINT16_C(0x0004);
    cpu->take_source_bit = UINT8_C(5);
    cpu->take_source_is_maskable = true;
}

static void
test_reset_retires_with_manual_field_list(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0xF7E0),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_reset(&fixture, storage, &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    dirty_for_reset(cpu);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* IPTR 0x100 shifted left 7. */
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x8000));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT16_C(0x8000));
    /* ST0: TC/C set, all else clear. */
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == UINT16_C(0x1800));
    /* ST1: XF/INTM/SXM set, reserved bit 10 preserved, all else
     * clear. */
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == UINT16_C(0x2D00));
    /* IPTR preserved; IFR cleared and known. */
    TI_C54X_TEST_CHECK(cpu->arch_state.status.pmst == UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(cpu->arch_state.ifr == UINT16_C(0));
    /* Unlisted state preserved. */
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0x123456789A));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 ==
                       UINT64_C(0x0FEDCBA987));
    {
        uint16_t ar3 = UINT16_C(0);
        uint16_t sp = UINT16_C(0);

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                               &cpu->arch_state, UINT8_C(3), &ar3) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(ar3 == UINT16_C(0x5555));
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_sp(&cpu->arch_state,
                                                     &sp) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(sp == UINT16_C(0x7777));
    }
    /* Pipeline latches cleared. */
    TI_C54X_TEST_CHECK(cpu->interrupt_vectors[2] == UINT16_C(0));
    TI_C54X_TEST_CHECK(cpu->interrupt_vectors_known == UINT16_C(0));
    TI_C54X_TEST_CHECK(cpu->take_source_bit == UINT8_C(0));
    TI_C54X_TEST_CHECK(cpu->take_source_is_maskable == false);
    /* No DATA endpoint traffic for the operand-less reset. */
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_iptr_stops_without_retirement(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0xF7E0),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_reset(&fixture, storage, &status);
    ti_c54x_run_result result;

    uint16_t before_st0;

    (void)status;
    dirty_for_reset(cpu);
    before_st0 = cpu->arch_state.status.st0;
    cpu->arch_state.status.pmst_known_mask = (uint16_t)(
        cpu->arch_state.status.pmst_known_mask &
        (uint16_t)~TI_C54X_PMST_IPTR_MASK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    /* Nothing applied: dirt and latches intact. */
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == before_st0);
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == UINT16_C(0x0400));
    TI_C54X_TEST_CHECK(cpu->arch_state.ifr == UINT16_C(0x00FF));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(cpu->interrupt_vectors[2] == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_reset_retires_with_manual_field_list();
    test_unknown_iptr_stops_without_retirement();
    return 0;
}
