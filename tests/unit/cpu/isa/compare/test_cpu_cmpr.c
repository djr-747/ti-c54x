/* Module contract
 * Owns: public-CPU RED proofs for CMPR CC,ARx execution through the generic
 * fetch/decode/input/dispatch/status/retirement path.  The tests seed private
 * AR values, prove true and false TC publication, one-attempt/one-retire and
 * PC+1 behavior, preserve unrelated state, and reject reset-unknown AR input
 * transactionally.
 * Does not own: CMPR pure semantics, catalog/decode production, input
 * collection, status sequencing, CPU lifecycle, memory, timing, or tracing.
 */

#include "../fixture.h"

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/state/register_layout.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint16_t cmpr_word(uint8_t cc, uint8_t arx)
{
    return (uint16_t)(UINT16_C(0xF4A8) |
                      ((uint16_t)(cc & UINT8_C(3)) << 8U) |
                      (uint16_t)(arx & UINT8_C(7)));
}

static ti_c54x_cpu *init_cmpr_cpu(
    ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
    uint16_t program_word, ti_c54x_error *status)
{
    ti_c54x_cpu *cpu;

    fixture->program_word = program_word;
    cpu = ti_c54x_cpu_isa_init(storage, fixture, false, status);
    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

static void seed_known_auxiliaries(ti_c54x_cpu *cpu, uint16_t ar0,
                                   uint16_t selected, uint8_t arx)
{
    for (uint8_t index = 0U; index < UINT8_C(8); ++index) {
        const uint16_t value =
            index == UINT8_C(0) ? ar0 :
            index == (arx & UINT8_C(7)) ? selected :
            (uint16_t)(UINT16_C(0x2000) + index);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                               &cpu->arch_state, index, value) ==
                           TI_C54X_ARCH_STATE_OK);
    }
}

static void check_unrelated_state(const ti_c54x_arch_state *before,
                                  const ti_c54x_arch_state *after)
{
    TI_C54X_TEST_CHECK(after->ifr == before->ifr);
    TI_C54X_TEST_CHECK(after->rc == before->rc);
    TI_C54X_TEST_CHECK(after->bk == before->bk);
    TI_C54X_TEST_CHECK(after->sp == before->sp);
    TI_C54X_TEST_CHECK(after->scalar_known_mask == before->scalar_known_mask);
    TI_C54X_TEST_CHECK(after->ar_known_mask == before->ar_known_mask);
    for (uint8_t index = 0U; index < UINT8_C(8); ++index) {
        TI_C54X_TEST_CHECK(after->ar[index] == before->ar[index]);
    }
    TI_C54X_TEST_CHECK(after->status.st1 == before->status.st1);
    TI_C54X_TEST_CHECK(after->status.st1_known_mask ==
                       before->status.st1_known_mask);
    TI_C54X_TEST_CHECK(after->status.pmst == before->status.pmst);
    TI_C54X_TEST_CHECK(after->status.pmst_known_mask ==
                       before->status.pmst_known_mask);
    TI_C54X_TEST_CHECK((after->status.st0 & (uint16_t)~TI_C54X_ST0_TC_MASK) ==
                       (before->status.st0 & (uint16_t)~TI_C54X_ST0_TC_MASK));
    TI_C54X_TEST_CHECK(
        (after->status.st0_known_mask & (uint16_t)~TI_C54X_ST0_TC_MASK) ==
        (before->status.st0_known_mask & (uint16_t)~TI_C54X_ST0_TC_MASK));
}

static void check_retired_case(uint8_t cc, uint8_t arx, uint16_t selected,
                               uint16_t ar0, bool expected_tc)
{
    const uint16_t word = cmpr_word(cc, arx);
    const ti_c54x_decode_result decoded = ti_c54x_decode_first_word(word);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = word,
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu;
    ti_c54x_arch_state before;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(decoded.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(decoded.id == TI_C54X_OPCODE_ID_CMPR_AR);
    TI_C54X_TEST_CHECK(decoded.program_word_count == UINT8_C(1));
    cpu = init_cmpr_cpu(&fixture, storage, word, &status);
    /* Exercise the private seed helper before replacing the complete AR set. */
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(0), UINT16_C(0x1234));
    seed_known_auxiliaries(cpu, ar0, selected, arx);
    cpu->arch_state.status.st0 = UINT16_C(0xA5C3);
    cpu->arch_state.status.st1 = UINT16_C(0x5A96);
    cpu->arch_state.status.pmst = UINT16_C(0xC35A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    before = cpu->arch_state;

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == word);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & TI_C54X_ST0_TC_MASK) ==
                       (expected_tc ? TI_C54X_ST0_TC_MASK : UINT16_C(0)));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0_known_mask &
                        TI_C54X_ST0_TC_MASK) != UINT16_C(0));
    check_unrelated_state(&before, &cpu->arch_state);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void test_reset_unknown_is_nonretiring(void)
{
    const uint16_t word = cmpr_word(0U, 1U);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = word,
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_cmpr_cpu(&fixture, storage, word, &status);
    ti_c54x_arch_state before;
    ti_c54x_run_result result;

    cpu->arch_state.status.st0 = UINT16_C(0xA5C3);
    cpu->arch_state.status.st1 = UINT16_C(0x5A96);
    cpu->arch_state.status.pmst = UINT16_C(0xC35A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    before = cpu->arch_state;

    /* Reset deliberately leaves all ARs unknown; no CMPR effect may commit. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == before.status.st0);
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0_known_mask ==
                       before.status.st0_known_mask);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar_known_mask == UINT8_C(0));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    /* True and false vectors cover all CC encodings and unsigned boundaries. */
    for (uint8_t cc = 0U; cc < UINT8_C(4); ++cc) {
        check_retired_case(cc, 1U, UINT16_C(0x0000), UINT16_C(0xFFFF),
                           cc == 1U || cc == 3U);
        check_retired_case(cc, 7U, UINT16_C(0xFFFF), UINT16_C(0x0000),
                           cc == 2U || cc == 3U);
    }
    check_retired_case(0U, 0U, UINT16_C(0xFFFF), UINT16_C(0xFFFF), true);
    check_retired_case(2U, 0U, UINT16_C(0x0000), UINT16_C(0x0000), false);
    test_reset_unknown_is_nonretiring();
    return 0;
}
