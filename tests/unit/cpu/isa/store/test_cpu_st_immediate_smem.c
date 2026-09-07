/* Module contract
 * Owns: public-CPU RED proofs for the future two-word fixed-MOD2
 * `ST #lk,*ARx+` store, including extension fetch ordering, resumable data
 * completion, retirement, and transactional rejection paths.
 * Does not own: production CPU behavior, catalog/decode or pure store effects,
 * address planning, shared fixtures, timing, diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../../src/state/temporary/t_register.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum st_immediate_memory_mode {
    ST_IMMEDIATE_READY = 0,
    ST_IMMEDIATE_WAIT_THEN_READY = 1,
    ST_IMMEDIATE_FAULT = 2
} st_immediate_memory_mode;

typedef struct st_immediate_fixture {
    uint16_t words[2];
    ti_c54x_program_word_address start;
    bool fail_extension;
    ti_c54x_bus_status extension_status;
    st_immediate_memory_mode mode;
    ti_c54x_program_word_address addresses[2];
    unsigned program_calls;
    unsigned endpoint_calls;
    unsigned begin_calls;
    unsigned resume_calls;
    unsigned ready_calls;
    unsigned cancel_calls;
    uint64_t ids[4];
    ti_c54x_bus_word16_request requests[4];
} st_immediate_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    st_immediate_fixture *fixture = (st_immediate_fixture *)context;
    const unsigned index = fixture->program_calls;

    TI_C54X_TEST_CHECK(index < 2U);
    TI_C54X_TEST_CHECK(address == fixture->start + (ti_c54x_program_word_address)index);
    TI_C54X_TEST_CHECK(value != NULL);
    fixture->addresses[index] = address;
    *value = fixture->words[index];
    fixture->program_calls++;
    if (index == 1U && fixture->fail_extension) {
        return fixture->extension_status;
    }
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    st_immediate_fixture *fixture = (st_immediate_fixture *)context;
    const unsigned index = fixture->endpoint_calls;

    TI_C54X_TEST_CHECK(index < 4U);
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->ids[index] = transaction_id;
    fixture->requests[index] = *request;
    fixture->endpoint_calls++;
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        fixture->begin_calls++;
    } else {
        fixture->resume_calls++;
    }
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .bus_status = TI_C54X_BUS_OK,
        .read_value_word = UINT16_C(0),
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY};
    if (fixture->mode == ST_IMMEDIATE_FAULT) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
    } else if (fixture->mode == ST_IMMEDIATE_WAIT_THEN_READY &&
               fixture->endpoint_calls == 1U) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
    } else {
        fixture->ready_calls++;
    }
}

static ti_c54x_bus_word16_cancel_result
cancel_data(void *context, uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request)
{
    st_immediate_fixture *fixture = (st_immediate_fixture *)context;

    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    fixture->cancel_calls++;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_cpu *
init_cpu(st_immediate_fixture *fixture,
         unsigned char *storage,
         ti_c54x_program_word_address start)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = fixture, .advance = advance_data, .cancel = cancel_data};
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program,
                             .data_endpoint = endpoint};
    const ti_c54x_config config = {start, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    fixture->start = start;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

static uint16_t
st_immediate_word(uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x7690) | (uint16_t)(arf & UINT8_C(7)));
}

static void
seed_cpu(ti_c54x_cpu *cpu, uint8_t arf, uint16_t ar_value)
{
    cpu->arch_state.status.st0 = UINT16_C(0xA5C3);
    cpu->arch_state.status.st1 = UINT16_C(0x5A96);
    cpu->arch_state.status.pmst = UINT16_C(0xC35A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state, arf,
                                                   ar_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x123456789a)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x00abcdef01)) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;
}

static void
check_write_request(const st_immediate_fixture *fixture, uint16_t old_ar,
                    uint16_t expected_value)
{
    TI_C54X_TEST_CHECK(fixture->requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(fixture->requests[0].address_word == old_ar);
    TI_C54X_TEST_CHECK(fixture->requests[0].write_value_word ==
                       expected_value);
}

static void
test_ready_all_arfs_and_literals(void)
{
    static const uint16_t values[] = {UINT16_C(0x0000), UINT16_C(0x8000),
                                      UINT16_C(0xffff)};

    for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
        for (size_t value_index = 0U;
             value_index < sizeof(values) / sizeof(values[0]); ++value_index) {
            const uint16_t old_ar =
                (uint16_t)(UINT16_C(0x2100) + (uint16_t)(arf * 4U));
            st_immediate_fixture fixture = {
                .words = {st_immediate_word(arf), values[value_index]},
                .mode = ST_IMMEDIATE_READY};
            _Alignas(max_align_t) unsigned char storage[1024] = {0};
            ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0));
            const ti_c54x_status_register_state before_status =
                (seed_cpu(cpu, arf, old_ar), cpu->arch_state.status);
            const ti_c54x_accumulator_state before_accumulators =
                cpu->accumulators;
            const ti_c54x_t_register_state before_t = cpu->t_register;
            ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
            TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
            TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
            TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
            TI_C54X_TEST_CHECK(result.opcode_word == fixture.words[0]);
            TI_C54X_TEST_CHECK(result.opcode_valid);
            TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
            TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0));
            TI_C54X_TEST_CHECK(fixture.addresses[1] == UINT32_C(1));
            TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
            TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
            check_write_request(&fixture, old_ar, values[value_index]);
            TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + 1U);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == before_status.st0);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == before_status.st1);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.pmst == before_status.pmst);
            TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                      sizeof(before_accumulators)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                      sizeof(before_t)) == 0);
            TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
        }
    }
}

static void
test_wait_retires_without_refetch(void)
{
    const uint8_t arf = UINT8_C(3);
    const uint16_t old_ar = UINT16_C(0x3200);
    st_immediate_fixture fixture = {.words = {st_immediate_word(arf),
                                               UINT16_C(0x8000)},
                                    .mode = ST_IMMEDIATE_WAIT_THEN_READY};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0));
    seed_cpu(cpu, arf, old_ar);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.ids[0] == fixture.ids[1]);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + 1U);
    check_write_request(&fixture, old_ar, UINT16_C(0x8000));
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_data_fault_and_extension_fault_are_atomic(void)
{
    st_immediate_fixture fixture = {.words = {st_immediate_word(1U),
                                               UINT16_C(0x7fff)},
                                    .mode = ST_IMMEDIATE_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0));
    seed_cpu(cpu, UINT8_C(1), UINT16_C(0x4000));
    const ti_c54x_arch_state before_arch = cpu->arch_state;
    const ti_c54x_accumulator_state before_accumulators = cpu->accumulators;
    const ti_c54x_t_register_state before_t = cpu->t_register;
    const ti_c54x_status_register_state before_status = cpu->arch_state.status;
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                              sizeof(before_arch)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                              sizeof(before_accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t, sizeof(before_t)) ==
                       0);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state.status, &before_status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);

    fixture = (st_immediate_fixture){.words = {st_immediate_word(1U),
                                                UINT16_C(0xcafe)},
                                     .start = UINT32_C(0),
                                     .fail_extension = true,
                                     .extension_status = TI_C54X_BUS_FAULT};
    memset(storage, 0, sizeof(storage));
    cpu = init_cpu(&fixture, storage, UINT32_C(0));
    seed_cpu(cpu, UINT8_C(1), UINT16_C(0x4000));
    const ti_c54x_arch_state extension_before = cpu->arch_state;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &extension_before,
                              sizeof(extension_before)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_dependencies_and_ar_boundary_reject(void)
{
    for (unsigned dependency = 0U; dependency < 4U; ++dependency) {
        st_immediate_fixture fixture = {.words = {st_immediate_word(4U),
                                                   UINT16_C(0x0000)},
                                        .mode = ST_IMMEDIATE_READY};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0));
        seed_cpu(cpu, UINT8_C(4), dependency == 3U ? UINT16_MAX
                                                  : UINT16_C(0x4200));
        if (dependency == 0U) {
            cpu->arch_state.ar_known_mask = (uint8_t)(
                cpu->arch_state.ar_known_mask & (uint8_t)~(UINT8_C(1) << 4U));
        } else if (dependency == 1U) {
            cpu->arch_state.status.st0_known_mask = (uint16_t)(
                cpu->arch_state.status.st0_known_mask &
                (uint16_t)~TI_C54X_ST0_ARP_MASK);
        } else if (dependency == 2U) {
            cpu->arch_state.status.st1_known_mask = (uint16_t)(
                cpu->arch_state.status.st1_known_mask &
                (uint16_t)~TI_C54X_ST1_CMPT_MASK);
        }
        const ti_c54x_arch_state before = cpu->arch_state;
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before, sizeof(before)) ==
                           0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_two_word_pc_boundary(void)
{
    for (ti_c54x_program_word_address start = UINT32_C(0xFFFD);
         start <= UINT32_C(0xFFFE); ++start) {
        st_immediate_fixture fixture = {.words = {st_immediate_word(0U),
                                                   UINT16_C(1)},
                                        .start = start,
                                        .mode = ST_IMMEDIATE_READY};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage, start);
        seed_cpu(cpu, UINT8_C(0), UINT16_C(0x5000));
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        TI_C54X_TEST_CHECK(fixture.addresses[0] == start);
        TI_C54X_TEST_CHECK(fixture.addresses[1] == start + 1U);
        if (start == UINT32_C(0xFFFD)) {
            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
            TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
            TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0xFFFF));
        } else {
            /* MEMORY preparation routes the unresolved width-2 boundary
             * through the transactional invalid-candidate stop. */
            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
            TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
            TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start);
            TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        }
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

int
main(void)
{
    test_ready_all_arfs_and_literals();
    test_wait_retires_without_refetch();
    test_data_fault_and_extension_fault_are_atomic();
    test_unknown_dependencies_and_ar_boundary_reject();
    test_two_word_pc_boundary();
    return 0;
}
