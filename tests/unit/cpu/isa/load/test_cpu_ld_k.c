/* Module contract
 * Owns: public-CPU RED proofs for `LD #K,dst`, including both
 * destinations, all 256 immediates, zero extension independent of SXM,
 * full status preservation (no OVM/OV effect), non-destination
 * preservation, and single-word retirement without endpoint traffic.
 * Does not own: production CPU behavior, catalog/decode or pure load
 * effects, address planning, shared fixture implementation, timing,
 * diagnostics, or test registration outside the load leaf.
 */

#include "../fixture.h"

#include "../../../../../src/state/register_layout.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* FORM-T2-19-07: 1110_100D_KKKK_KKKK: destination bit 8, unsigned K in
 * bits 7:0; fixed bits 0xFE00 hold 0xE800. */
static uint16_t
ld_k_word(uint8_t destination, uint8_t immediate)
{
    return (uint16_t)(UINT16_C(0xE800) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      (uint16_t)immediate);
}

static ti_c54x_cpu *
init_load(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
          uint8_t destination, uint64_t other_value,
          ti_c54x_error *status)
{
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, fixture, false, status);

    /* FORM-T2-19-07 uses no address register and no status inputs; keep
     * the shared seeders referenced so the header stays warning-clean,
     * and leave SXM/OVM in their reset states. */
    (void)ti_c54x_cpu_isa_seed_address;
    (void)ti_c54x_cpu_isa_seed_sxm;
    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           destination == UINT8_C(0) ? UINT64_C(0)
                                                     : other_value) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           destination == UINT8_C(1) ? UINT64_C(0)
                                                     : other_value) ==
                       TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;

    /* Make C, TC, SXM, OVM, and both accumulator overflow bits visibly
     * non-default: none may change. */
    cpu->arch_state.status.st0 = (uint16_t)(
        cpu->arch_state.status.st0 | TI_C54X_ST0_TC_MASK |
        TI_C54X_ST0_C_MASK | TI_C54X_ST0_OVA_MASK | TI_C54X_ST0_OVB_MASK);
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_SXM_MASK |
                   TI_C54X_ST1_OVM_MASK);
    (void)destination;
    return cpu;
}

static void
test_ready_all_destinations_and_immediates(void)
{
    static const uint64_t others[] = {UINT64_C(0x1111222233),
                                      UINT64_C(0xFFFFFFFFFF)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        for (uint32_t raw = 0U; raw <= UINT32_C(0xFF); ++raw) {
            const uint8_t immediate = (uint8_t)raw;
            for (size_t other_index = 0U;
                 other_index <
                 sizeof(others) / sizeof(others[0]);
                 ++other_index) {
                ti_c54x_cpu_isa_memory_fixture fixture = {
                    .mode = TI_C54X_CPU_ISA_MEMORY_READY,
                    .program_word =
                        ld_k_word(destination, immediate),
                    .read_value_word = UINT16_C(0),
                    .fault_status = TI_C54X_BUS_FAULT};
                _Alignas(max_align_t) unsigned char storage[1024] = {0};
                ti_c54x_error status;
                ti_c54x_cpu *cpu =
                    init_load(&fixture, storage, destination,
                              others[other_index], &status);
                const ti_c54x_accumulator_state before_accumulators =
                    cpu->accumulators;
                const ti_c54x_status_register_state before_status =
                    cpu->arch_state.status;
                const ti_c54x_t_register_state before_t =
                    cpu->t_register;
                ti_c54x_run_result result =
                    ti_c54x_cpu_run(cpu, UINT64_C(1));
                ti_c54x_state state;

                TI_C54X_TEST_CHECK(result.reason ==
                                   TI_C54X_STOP_BUDGET_EXHAUSTED);
                TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
                TI_C54X_TEST_CHECK(result.instructions_attempted ==
                                   UINT64_C(1));
                TI_C54X_TEST_CHECK(result.instructions_retired ==
                                   UINT64_C(1));
                TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
                TI_C54X_TEST_CHECK(result.opcode_valid);
                TI_C54X_TEST_CHECK(result.opcode_word ==
                                   fixture.program_word);
                TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
                TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
                if (destination == UINT8_C(0)) {
                    TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                                       (uint64_t)immediate);
                    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                                       before_accumulators.b_raw40);
                } else {
                    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                                       (uint64_t)immediate);
                    TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                                       before_accumulators.a_raw40);
                }
                TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state.status,
                                          &before_status,
                                          sizeof(before_status)) == 0);
                TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                          sizeof(before_t)) == 0);
                TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                                   TI_C54X_ERROR_OK);
                TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(1));
                TI_C54X_TEST_CHECK(state.instructions_retired_total ==
                                   UINT64_C(1));
                TI_C54X_TEST_CHECK(state.accumulator_a_known &&
                                   state.accumulator_b_known);
                TI_C54X_TEST_CHECK((destination == UINT8_C(0)
                                        ? state.accumulator_a_raw40
                                        : state.accumulator_b_raw40) ==
                                   (uint64_t)immediate);
                TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                                   TI_C54X_ERROR_OK);
                (void)status;
            }
        }
    }
}

int
main(void)
{
    test_ready_all_destinations_and_immediates();
    return 0;
}
