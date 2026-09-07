/* Module contract
 * Owns: public-CPU RED proofs for reset-runnable SSBX/RSBX status-bit
 * execution through the generic fetch/decode/dispatch/retirement path.
 * Does not own: status-bit production semantics, opcode catalog/decode,
 * effect sequencing, CPU lifecycle, shared fixtures, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/state/register_layout.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint16_t status_bit_word(bool set, uint8_t status, uint8_t bit)
{
    const uint16_t base = set ? UINT16_C(0xF5B0) : UINT16_C(0xF4B0);

    return (uint16_t)(base | ((uint16_t)(status & UINT8_C(1)) << 9U) |
                      (uint16_t)(bit & UINT8_C(0x0F)));
}

static ti_c54x_cpu *init_status_cpu(
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

static void seed_status_with_selected_unknown(ti_c54x_cpu *cpu,
                                              uint8_t status, uint8_t bit,
                                              bool set)
{
    const uint16_t selected_mask = (uint16_t)(UINT16_C(1) << bit);
    ti_c54x_status_register_state *state = &cpu->arch_state.status;

    state->st0 = UINT16_C(0xA5C3);
    state->st1 = UINT16_C(0x5A96);
    state->pmst = UINT16_C(0xC35A);
    state->st0_known_mask = UINT16_C(0xFFFF);
    state->st1_known_mask = UINT16_C(0xFFFF);
    state->pmst_known_mask = UINT16_C(0xFFFF);
    if (status == UINT8_C(0)) {
        state->st0 = set ? (uint16_t)(state->st0 & (uint16_t)~selected_mask)
                         : (uint16_t)(state->st0 | selected_mask);
        state->st0_known_mask =
            (uint16_t)(state->st0_known_mask & (uint16_t)~selected_mask);
    } else {
        state->st1 = set ? (uint16_t)(state->st1 & (uint16_t)~selected_mask)
                         : (uint16_t)(state->st1 | selected_mask);
        state->st1_known_mask =
            (uint16_t)(state->st1_known_mask & (uint16_t)~selected_mask);
    }
}

static void check_unrelated_arch_state(const ti_c54x_arch_state *before,
                                       const ti_c54x_arch_state *after,
                                       uint8_t status, uint8_t bit, bool set)
{
    const uint16_t selected_mask = (uint16_t)(UINT16_C(1) << bit);
    const uint16_t expected_selected = set ? selected_mask : UINT16_C(0);

    TI_C54X_TEST_CHECK(after->ifr == before->ifr);
    TI_C54X_TEST_CHECK(after->rc == before->rc);
    TI_C54X_TEST_CHECK(after->bk == before->bk);
    TI_C54X_TEST_CHECK(after->sp == before->sp);
    TI_C54X_TEST_CHECK(after->scalar_known_mask == before->scalar_known_mask);
    TI_C54X_TEST_CHECK(after->ar_known_mask == before->ar_known_mask);
    for (uint8_t index = 0U; index < UINT8_C(8); ++index) {
        TI_C54X_TEST_CHECK(after->ar[index] == before->ar[index]);
    }
    TI_C54X_TEST_CHECK(after->status.pmst == before->status.pmst);
    TI_C54X_TEST_CHECK(after->status.pmst_known_mask ==
                       before->status.pmst_known_mask);
    if (status == UINT8_C(0)) {
        TI_C54X_TEST_CHECK(after->status.st1 == before->status.st1);
        TI_C54X_TEST_CHECK(after->status.st1_known_mask ==
                           before->status.st1_known_mask);
        TI_C54X_TEST_CHECK((after->status.st0 & (uint16_t)~selected_mask) ==
                           (before->status.st0 & (uint16_t)~selected_mask));
        TI_C54X_TEST_CHECK(after->status.st0_known_mask ==
                           (uint16_t)(before->status.st0_known_mask |
                                      selected_mask));
        TI_C54X_TEST_CHECK((after->status.st0 & selected_mask) ==
                           expected_selected);
    } else {
        TI_C54X_TEST_CHECK(after->status.st0 == before->status.st0);
        TI_C54X_TEST_CHECK(after->status.st0_known_mask ==
                           before->status.st0_known_mask);
        TI_C54X_TEST_CHECK((after->status.st1 & (uint16_t)~selected_mask) ==
                           (before->status.st1 & (uint16_t)~selected_mask));
        TI_C54X_TEST_CHECK(after->status.st1_known_mask ==
                           (uint16_t)(before->status.st1_known_mask |
                                      selected_mask));
        TI_C54X_TEST_CHECK((after->status.st1 & selected_mask) ==
                           expected_selected);
    }
}

static void check_one_status_bit(bool set, uint8_t status, uint8_t bit)
{
    const uint16_t word = status_bit_word(set, status, bit);
    const ti_c54x_decode_result decoded = ti_c54x_decode_first_word(word);
    const ti_c54x_opcode_id expected_id =
        set ? TI_C54X_OPCODE_ID_SSBX_STATUS : TI_C54X_OPCODE_ID_RSBX_STATUS;
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = word,
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status_code;
    ti_c54x_cpu *cpu;
    ti_c54x_arch_state before;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(decoded.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(decoded.id == expected_id);
    TI_C54X_TEST_CHECK(decoded.program_word_count == UINT8_C(1));

    cpu = init_status_cpu(&fixture, storage, word, &status_code);
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(0), UINT16_C(0x1234));
    seed_status_with_selected_unknown(cpu, status, bit, set);
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
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(1));
    check_unrelated_arch_state(&before, &cpu->arch_state, status, bit, set);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void test_ssbx_rsbx_boundaries_and_reserved_bit(void)
{
    static const uint8_t bits[] = {UINT8_C(0), UINT8_C(15), UINT8_C(10)};

    for (uint8_t set_value = 0U; set_value <= UINT8_C(1); ++set_value) {
        const bool set = set_value != UINT8_C(0);
        for (uint8_t status = 0U; status <= UINT8_C(1); ++status) {
            const size_t limit = status == UINT8_C(1) ? 3U : 2U;
            for (size_t index = 0U; index < limit; ++index) {
                check_one_status_bit(set, status, bits[index]);
            }
        }
    }
}

int main(void)
{
    test_ssbx_rsbx_boundaries_and_reserved_bit();
    return 0;
}
