/* Module contract
 * Owns: public-CPU execution proofs for the two-word ANDM, ORM, and XORM
 * WORD16 read/modify/write forms, including address modes, paired endpoint
 * transactions, resumable WAIT, faults, cancellation, and retirement.
 * Does not own: production semantics, decode/catalog metadata, shared
 * fixtures, benchmark policy, or non-CPU registration.
 */

#include <ti_c54x/cpu.h>

#include "../../../../src/cpu/cpu_internal.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum atomic_mode {
    ATOMIC_READY = 0,
    ATOMIC_WAIT_READ,
    ATOMIC_WAIT_WRITE,
    ATOMIC_FAULT_READ,
    ATOMIC_FAULT_WRITE,
    ATOMIC_WAIT_ALWAYS
} atomic_mode;

typedef struct atomic_fixture {
    uint16_t words[2];
    uint16_t read_value;
    atomic_mode mode;
    unsigned program_calls;
    unsigned endpoint_calls;
    unsigned begin_calls;
    unsigned resume_calls;
    unsigned cancel_calls;
    uint64_t ids[4];
    ti_c54x_bus_word16_request requests[4];
    uint64_t cancel_id;
    ti_c54x_bus_word16_request cancel_request;
} atomic_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    atomic_fixture *fixture = (atomic_fixture *)context;
    TI_C54X_TEST_CHECK(fixture->program_calls < 2U);
    TI_C54X_TEST_CHECK(address == fixture->program_calls);
    TI_C54X_TEST_CHECK(value != NULL);
    *value = fixture->words[fixture->program_calls++];
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    atomic_fixture *fixture = (atomic_fixture *)context;
    const unsigned index = fixture->endpoint_calls++;
    const bool read = request != NULL &&
                      request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;

    TI_C54X_TEST_CHECK(index < 4U);
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN ||
                       phase == TI_C54X_BUS_WORD16_ADVANCE_RESUME);
    fixture->ids[index] = transaction_id;
    fixture->requests[index] = *request;
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        fixture->begin_calls++;
    } else {
        fixture->resume_calls++;
    }
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .bus_status = TI_C54X_BUS_OK,
        .read_value_word = read ? fixture->read_value : UINT16_C(0)};
    if ((read && fixture->mode == ATOMIC_WAIT_READ) ||
        (!read && fixture->mode == ATOMIC_WAIT_WRITE) ||
        fixture->mode == ATOMIC_WAIT_ALWAYS) {
        /* The first occurrence is the intended wait; a resume is READY. */
        if (fixture->mode == ATOMIC_WAIT_ALWAYS || index == 0U ||
            (fixture->mode == ATOMIC_WAIT_WRITE && index == 1U)) {
            observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
            observation->read_value_word = UINT16_C(0);
        }
    }
    if ((read && fixture->mode == ATOMIC_FAULT_READ) ||
        (!read && fixture->mode == ATOMIC_FAULT_WRITE)) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
        observation->read_value_word = UINT16_C(0);
    }
}

static ti_c54x_bus_word16_cancel_result
cancel_data(void *context, uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request)
{
    atomic_fixture *fixture = (atomic_fixture *)context;
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    fixture->cancel_calls++;
    fixture->cancel_id = transaction_id;
    fixture->cancel_request = *request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_cpu *init_cpu(atomic_fixture *fixture, void *storage,
                             ti_c54x_error *status)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = fixture, .advance = advance_data, .cancel = cancel_data};
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program,
                             .data_endpoint = endpoint};
    const ti_c54x_config config = {UINT32_C(0), {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    *status = ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config, &bus,
                               &cpu);
    return cpu;
}

static uint16_t atomic_word(uint16_t base, uint8_t arf, uint8_t mode)
{
    return (uint16_t)(base | (uint16_t)(mode << 3U) | (arf & UINT8_C(7)));
}

static void seed_cpu(ti_c54x_cpu *cpu, uint8_t arf, uint16_t address)
{
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state, arf,
                                                   address) ==
                       TI_C54X_ARCH_STATE_OK);
}

static uint16_t transformed(uint8_t operation, uint16_t read, uint16_t mask)
{
    if (operation == 0U) {
        return (uint16_t)(read & mask);
    }
    if (operation == 1U) {
        return (uint16_t)(read | mask);
    }
    return (uint16_t)(read ^ mask);
}

static void check_requests(const atomic_fixture *fixture, uint16_t address,
                           uint16_t value)
{
    TI_C54X_TEST_CHECK(fixture->requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture->requests[0].address_word == address);
    TI_C54X_TEST_CHECK(fixture->requests[0].write_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(fixture->requests[1].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(fixture->requests[1].address_word == address);
    TI_C54X_TEST_CHECK(fixture->requests[1].write_value_word == value);
}

static void check_totals(const ti_c54x_cpu *cpu, uint64_t attempted,
                         uint64_t retired)
{
    ti_c54x_state state;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == attempted);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == retired);
}

static void seed_nondefault_status(ti_c54x_cpu *cpu)
{
    cpu->arch_state.status = (ti_c54x_status_register_state){
        .st0 = UINT16_C(0xA5C3),
        .st1 = UINT16_C(0x5A96),
        .pmst = UINT16_C(0xC35A),
        .st0_known_mask = UINT16_C(0xFFFF),
        .st1_known_mask = UINT16_C(0xFFFF),
        .pmst_known_mask = UINT16_C(0xFFFF)};
    /* Keep the resolver's live CMPT/ARP dependencies aligned with seed_cpu. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(
                           &cpu->arch_state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(
                           &cpu->arch_state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
}

static void check_status_preserved(
    const ti_c54x_cpu *cpu, const ti_c54x_status_register_state *before)
{
    const ti_c54x_status_register_state *after = &cpu->arch_state.status;
    TI_C54X_TEST_CHECK(after->st0 == before->st0);
    TI_C54X_TEST_CHECK(after->st1 == before->st1);
    TI_C54X_TEST_CHECK(after->pmst == before->pmst);
    TI_C54X_TEST_CHECK(after->st0_known_mask == before->st0_known_mask);
    TI_C54X_TEST_CHECK(after->st1_known_mask == before->st1_known_mask);
    TI_C54X_TEST_CHECK(after->pmst_known_mask == before->pmst_known_mask);
}

static void test_ready_all_operations_and_modes(void)
{
    static const uint16_t bases[] = {UINT16_C(0x6880), UINT16_C(0x6980),
                                     UINT16_C(0x6A80)};
    for (uint8_t operation = 0U; operation < 3U; ++operation) {
        for (uint8_t mode = 0U; mode < 3U; ++mode) {
            const uint8_t arf = UINT8_C(3);
            const uint16_t address = UINT16_C(0x2400);
            const uint16_t read = UINT16_C(0xA55A);
            const uint16_t mask = UINT16_C(0x0F3C);
            atomic_fixture fixture = {
                .words = {atomic_word(bases[operation], arf, mode), mask},
                .read_value = read,
                .mode = ATOMIC_READY};
            _Alignas(max_align_t) unsigned char storage[1024] = {0};
            ti_c54x_error status;
            ti_c54x_cpu *cpu = init_cpu(&fixture, storage, &status);
            ti_c54x_run_result result;
            ti_c54x_status_register_state before_status;
            const uint16_t expected = transformed(operation, read, mask);
            const uint16_t expected_ar =
                mode == 0U ? address :
                mode == 1U ? (uint16_t)(address - UINT16_C(1)) :
                              (uint16_t)(address + UINT16_C(1));

            TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
            TI_C54X_TEST_CHECK(cpu != NULL);
            seed_cpu(cpu, arf, address);
            seed_nondefault_status(cpu);
            before_status = cpu->arch_state.status;
            result = ti_c54x_cpu_run(cpu, UINT64_C(1));
            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
            TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
            TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
            TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
            TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
            TI_C54X_TEST_CHECK(result.opcode_word == fixture.words[0]);
            TI_C54X_TEST_CHECK(result.opcode_valid);
            TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
            TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
            TI_C54X_TEST_CHECK(fixture.begin_calls == 2U);
            TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
            TI_C54X_TEST_CHECK(fixture.ids[0] != fixture.ids[1]);
            check_requests(&fixture, address, expected);
            TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == expected_ar);
            check_status_preserved(cpu, &before_status);
            check_totals(cpu, UINT64_C(1), UINT64_C(1));
            TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
        }
    }
}

static void test_wait_read_and_wait_write_resume_without_refetch(void)
{
    const uint16_t first = atomic_word(UINT16_C(0x6880), UINT8_C(2), 2U);
    const uint16_t mask = UINT16_C(0x00F0);
    const uint16_t read = UINT16_C(0xA55A);
    const uint16_t expected = (uint16_t)(read & mask);
    for (atomic_mode mode = ATOMIC_WAIT_READ; mode <= ATOMIC_WAIT_WRITE;
         ++mode) {
        atomic_fixture fixture = {
            .words = {first, mask}, .read_value = read, .mode = mode};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage, &status);
        ti_c54x_arch_state before;
        ti_c54x_run_result result;

        TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(cpu != NULL);
        seed_cpu(cpu, 2U, UINT16_C(0x2500));
        before = cpu->arch_state;
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before, sizeof(before)) ==
                           0);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        if (mode == ATOMIC_WAIT_READ) {
            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
            TI_C54X_TEST_CHECK(fixture.endpoint_calls == 3U);
            TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
            TI_C54X_TEST_CHECK(fixture.ids[0] == fixture.ids[1]);
            TI_C54X_TEST_CHECK(fixture.ids[1] != fixture.ids[2]);
        } else {
            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
            TI_C54X_TEST_CHECK(fixture.endpoint_calls == 3U);
            TI_C54X_TEST_CHECK(fixture.ids[1] == fixture.ids[2]);
            TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
        }
        TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
        TI_C54X_TEST_CHECK(fixture.requests[mode == ATOMIC_WAIT_READ ? 2U : 1U]
                               .write_value_word == expected);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(2));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[2] == UINT16_C(0x2501));
        check_totals(cpu, UINT64_C(1), UINT64_C(1));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void test_faults_and_cancellation_preserve_or_reset_state(void)
{
    for (atomic_mode mode = ATOMIC_FAULT_READ; mode <= ATOMIC_FAULT_WRITE;
         ++mode) {
        atomic_fixture fixture = {.words = {atomic_word(0x6980, 1U, 2U),
                                             UINT16_C(0xF0F0)},
                                   .read_value = UINT16_C(0xAAAA),
                                   .mode = mode};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage, &status);
        ti_c54x_arch_state before;
        ti_c54x_run_result result;

        TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(cpu != NULL);
        seed_cpu(cpu, 1U, UINT16_C(0x2600));
        before = cpu->arch_state;
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before, sizeof(before)) ==
                           0);
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == (mode == ATOMIC_FAULT_READ
                                                          ? 1U : 2U));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }

    for (atomic_mode mode = ATOMIC_WAIT_READ; mode <= ATOMIC_WAIT_WRITE;
         ++mode) {
        const uint8_t arf = UINT8_C(4);
        const uint16_t address = UINT16_C(0x2700);
        const uint16_t mask = UINT16_C(0x00FF);
        const uint16_t read = UINT16_C(0xF0F0);
        const unsigned pending_index = mode == ATOMIC_WAIT_READ ? 0U : 1U;
        atomic_fixture fixture = {
            .words = {atomic_word(UINT16_C(0x6A80), arf, 2U), mask},
            .read_value = read,
            .mode = mode};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage, &status);
        ti_c54x_run_result result;

        TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(cpu != NULL);
        seed_cpu(cpu, arf, address);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
        TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == pending_index + 1U);
        TI_C54X_TEST_CHECK(fixture.ids[pending_index] != UINT64_C(0));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(fixture.cancel_calls == 1U);
        TI_C54X_TEST_CHECK(fixture.cancel_id == fixture.ids[pending_index]);
        TI_C54X_TEST_CHECK(fixture.cancel_request.access ==
                           (mode == ATOMIC_WAIT_READ
                                ? TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                                : TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE));
        TI_C54X_TEST_CHECK(fixture.cancel_request.address_word == address);
        TI_C54X_TEST_CHECK(fixture.cancel_request.write_value_word ==
                           (mode == ATOMIC_WAIT_READ
                                ? UINT16_C(0)
                                : transformed(2U, read, mask)));
        TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(cpu->instructions_attempted == UINT64_C(0));
        TI_C54X_TEST_CHECK(cpu->instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == UINT16_C(0));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void test_mod3_rejected_before_data_issue(void)
{
    atomic_fixture fixture = {.words = {atomic_word(0x6880, 2U, 3U), 0xFFFF},
                               .mode = ATOMIC_READY};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(cpu != NULL);
    seed_cpu(cpu, 2U, UINT16_C(0x2800));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[2] == UINT16_C(0x2800));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    test_ready_all_operations_and_modes();
    test_wait_read_and_wait_write_resume_without_refetch();
    test_faults_and_cancellation_preserve_or_reset_state();
    test_mod3_rejected_before_data_issue();
    return 0;
}
