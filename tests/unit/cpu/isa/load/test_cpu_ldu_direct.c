/* Module contract
 * Owns: public-CPU RED proofs for the DP-relative direct `LDU Smem,dst`
 * load, including zero-extension, SXM independence, DP-unknown
 * transactional rejection, and resumable endpoint completion.
 * Does not own: catalog/decode or pure LDU effects, address-planner internals,
 * endpoint reducer behavior, production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint16_t
ldu_word(uint8_t destination, uint8_t dma)
{
    return (uint16_t)(UINT16_C(0x1200) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      (uint16_t)(dma & UINT8_C(0x7F)));
}

static ti_c54x_cpu *
init_ldu_direct(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
                bool with_endpoint, uint16_t dp, ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, with_endpoint,
                                            status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    /* AR0 seeds CMPT0/ARP0 liveness only; the direct form uses no AR. */
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(0), UINT16_C(0x0002));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&cpu->arch_state, dp) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x1111222233)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000aabbcc)) == TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static void
test_ready_forms_zero_extend(void)
{
    static const uint16_t values[] = {
        UINT16_C(0x0000), UINT16_C(0x8000), UINT16_C(0xffff)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1); ++destination) {
        for (size_t value_index = 0U;
             value_index < sizeof(values) / sizeof(values[0]); ++value_index) {
            /* DP 006 with dma 5 addresses 0x0305. */
            ti_c54x_cpu_isa_memory_fixture fixture = {
                .mode = TI_C54X_CPU_ISA_MEMORY_READY,
                .program_word = ldu_word(destination, UINT8_C(0x05)),
                .read_value_word = values[value_index],
                .fault_status = TI_C54X_BUS_FAULT};
            _Alignas(max_align_t) unsigned char storage[1024] = {0};
            ti_c54x_error status;
            ti_c54x_cpu *cpu = init_ldu_direct(&fixture, storage, true,
                                              UINT16_C(0x006), &status);
            ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

            (void)status;
            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
            TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
            TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
            TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
            TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
            TI_C54X_TEST_CHECK(result.opcode_word == fixture.program_word);
            TI_C54X_TEST_CHECK(result.opcode_valid);
            TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
            TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
            TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                               TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
            TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                               TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
            TI_C54X_TEST_CHECK_U64(fixture.requests[0].address_word,
                                   UINT64_C(0x0305));
            TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                               (destination == UINT8_C(0)
                                    ? (uint64_t)values[value_index]
                                    : UINT64_C(0x1111222233)));
            TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                               (destination == UINT8_C(1)
                                    ? (uint64_t)values[value_index]
                                    : UINT64_C(0x0000aabbcc)));
            TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
        }
    }
}

static void
test_dp_unknown_rejects(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = ldu_word(UINT8_C(0), UINT8_C(0x05)),
        .read_value_word = UINT16_C(0x1234),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    /* Reset leaves DP known-zero; clear it to prove the gate. */
    cpu->arch_state.status.st0_known_mask = (uint16_t)(
        cpu->arch_state.status.st0_known_mask &
        (uint16_t)~TI_C54X_ST0_DP_MASK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_ready_forms_zero_extend();
    test_dp_unknown_rejects();
    return 0;
}
