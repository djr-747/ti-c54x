/* Module contract
 * Owns: public-CPU integration proofs for the twenty W44B circular WORD16
 * standard-indirect rows (ten families, MOD8 and MOD10), including AR0/AR7,
 * BK=0, positive-BK wrap, resumable endpoint publication, rollback, stale
 * dependencies, and pre-issue rejection. Does not own: catalog generation,
 * pure opcode effects, address planning, transaction internals, timing,
 * diagnostics, or production code.
 */

#include <ti_c54x/cpu.h>

#include "../../../../src/cpu/cpu_internal.h"
#include "../../../../src/state/accumulator/accumulator.h"
#include "../../../../src/state/block_size_register.h"
#include "../../../../src/state/register_layout.h"
#include "../../../../src/state/temporary/t_register.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum circular_cpu_mode {
    CIRCULAR_CPU_READY = 0,
    CIRCULAR_CPU_WAIT_THEN_READY = 1,
    CIRCULAR_CPU_FAULT = 2,
    CIRCULAR_CPU_WAIT_ALWAYS = 3
} circular_cpu_mode;

typedef struct circular_cpu_fixture {
    uint16_t words[2];
    unsigned word_count;
    circular_cpu_mode mode;
    uint16_t read_value_word;
    unsigned program_reads;
    unsigned endpoint_calls;
    unsigned begin_calls;
    unsigned resume_calls;
    unsigned ready_calls;
    unsigned cancel_calls;
    uint64_t ids[4];
    ti_c54x_bus_word16_request requests[4];
} circular_cpu_fixture;

typedef struct circular_row {
    uint16_t mod8_base;
    uint16_t mod10_base;
    bool read;
    bool two_words;
} circular_row;

static const circular_row circular_rows[] = {
    {UINT16_C(0x10C0), UINT16_C(0x10D0), true, false},
    {UINT16_C(0x14C0), UINT16_C(0x14D0), true, false},
    {UINT16_C(0x80C0), UINT16_C(0x80D0), false, false},
    {UINT16_C(0x12C0), UINT16_C(0x12D0), true, false},
    {UINT16_C(0x30C0), UINT16_C(0x30D0), true, false},
    {UINT16_C(0x76C0), UINT16_C(0x76D0), false, true},
    {UINT16_C(0x46C0), UINT16_C(0x46D0), true, false},
    {UINT16_C(0x32C0), UINT16_C(0x32D0), true, false},
    {UINT16_C(0x44C0), UINT16_C(0x44D0), true, false},
    {UINT16_C(0x8CC0), UINT16_C(0x8CD0), false, false},
};

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    circular_cpu_fixture *fixture = (circular_cpu_fixture *)context;
    const unsigned index = fixture->program_reads;

    TI_C54X_TEST_CHECK(value_word != NULL);
    TI_C54X_TEST_CHECK(index < fixture->word_count);
    TI_C54X_TEST_CHECK(address_word == (ti_c54x_program_word_address)index);
    *value_word = fixture->words[index];
    fixture->program_reads++;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    circular_cpu_fixture *fixture = (circular_cpu_fixture *)context;
    const unsigned index = fixture->endpoint_calls;

    TI_C54X_TEST_CHECK(index < sizeof(fixture->ids) / sizeof(fixture->ids[0]));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
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
    if (fixture->mode == CIRCULAR_CPU_FAULT) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
    } else if (fixture->mode == CIRCULAR_CPU_WAIT_ALWAYS ||
               (fixture->mode == CIRCULAR_CPU_WAIT_THEN_READY &&
                fixture->endpoint_calls == 1U)) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
    } else {
        fixture->ready_calls++;
        if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
            observation->read_value_word = fixture->read_value_word;
        }
    }
}

static ti_c54x_bus_word16_cancel_result
cancel_data(void *context, uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request)
{
    circular_cpu_fixture *fixture = (circular_cpu_fixture *)context;

    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    fixture->cancel_calls++;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_cpu *
init_cpu(circular_cpu_fixture *fixture, void *storage)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = fixture, .advance = advance_data, .cancel = cancel_data};
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program,
                             .data_endpoint = endpoint};
    const ti_c54x_config config = {UINT32_C(0), {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

static void
seed_cpu(ti_c54x_cpu *cpu, uint8_t arf, uint16_t ar_value, uint16_t bk_value)
{
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state,
                                                     UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state,
                                                    UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state, arf,
                                                   ar_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&cpu->arch_state, bk_value) ==
                       TI_C54X_ARCH_STATE_OK);
    cpu->arch_state.status.st1 = (uint16_t)(
        cpu->arch_state.status.st1 & (uint16_t)~TI_C54X_ST1_SXM_MASK);
    cpu->arch_state.status.st1_known_mask = (uint16_t)(
        cpu->arch_state.status.st1_known_mask | TI_C54X_ST1_SXM_MASK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x00000000CAFE)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x000000005AA5)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_write(&cpu->t_register,
                                                UINT16_C(0x1357)) ==
                       TI_C54X_T_REGISTER_OK);
}

static uint16_t
row_word(const circular_row *row, bool mod10, uint8_t arf)
{
    return (uint16_t)((mod10 ? row->mod10_base : row->mod8_base) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static void
check_request(const circular_cpu_fixture *fixture, bool read,
              uint16_t old_ar, uint16_t write_value)
{
    TI_C54X_TEST_CHECK(fixture->requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[0].access ==
                       (read ? TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                             : TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE));
    TI_C54X_TEST_CHECK(fixture->requests[0].address_word == old_ar);
    if (!read) {
        TI_C54X_TEST_CHECK(fixture->requests[0].write_value_word ==
                           write_value);
    }
}

static uint16_t
expected_after(bool mod10, uint16_t old_ar)
{
    const uint16_t block_base = (uint16_t)(old_ar & UINT16_C(0xFFF8));
    const uint16_t index = (uint16_t)(old_ar & UINT16_C(0x0007));
    const uint16_t moved = (uint16_t)(mod10
                                          ? (index + UINT16_C(1)) % UINT16_C(5)
                                          : (index == UINT16_C(0)
                                                 ? UINT16_C(4)
                                                 : index - UINT16_C(1)));
    return (uint16_t)(block_base + moved);
}

static void
test_all_twenty_rows_decode_dispatch_and_update(void)
{
    for (size_t row_index = 0U;
         row_index < sizeof(circular_rows) / sizeof(circular_rows[0]);
         ++row_index) {
        const circular_row *row = &circular_rows[row_index];

        for (unsigned mod10_value = 0U; mod10_value <= 1U; ++mod10_value) {
            for (unsigned arf_value = 0U; arf_value <= 7U; arf_value += 7U) {
                const bool mod10 = mod10_value != 0U;
                const uint8_t arf = (uint8_t)arf_value;
                const uint16_t old_ar = UINT16_C(0x1240);
                circular_cpu_fixture fixture = {
                    .words = {row_word(row, mod10, arf), UINT16_C(0xA55A)},
                    .word_count = row->two_words ? 2U : 1U,
                    .mode = CIRCULAR_CPU_READY,
                    .read_value_word = UINT16_C(0x8123)};
                _Alignas(max_align_t) unsigned char storage[1024] = {0};
                ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
                const uint16_t write_value = row->two_words
                                                  ? UINT16_C(0xA55A)
                                                  : (row_index == 2U
                                                         ? UINT16_C(0xCAFE)
                                                         : UINT16_C(0x1357));
                ti_c54x_run_result result;

                seed_cpu(cpu, arf, old_ar, UINT16_C(5));
                result = ti_c54x_cpu_run(cpu, UINT64_C(1));
                TI_C54X_TEST_CHECK(result.reason ==
                                   TI_C54X_STOP_BUDGET_EXHAUSTED);
                TI_C54X_TEST_CHECK(result.instructions_attempted ==
                                   UINT64_C(1));
                TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
                TI_C54X_TEST_CHECK(result.opcode_valid);
                TI_C54X_TEST_CHECK(fixture.program_reads == fixture.word_count);
                TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
                TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
                check_request(&fixture, row->read, old_ar, write_value);
                TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] ==
                                   expected_after(mod10, old_ar));
                TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
            }
        }
    }
}

static void
test_bk_zero_ready_and_wait_publish_without_update(void)
{
    const uint16_t old_ar = UINT16_C(0x1240);
    const uint8_t arf = UINT8_C(0);
    circular_cpu_fixture fixture = {
        .words = {row_word(&circular_rows[0], false, arf), UINT16_C(0)},
        .word_count = 1U,
        .mode = CIRCULAR_CPU_READY,
        .read_value_word = UINT16_C(0xBEEF)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
    ti_c54x_run_result result;

    seed_cpu(cpu, arf, old_ar, UINT16_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);

    fixture = (circular_cpu_fixture){
        .words = {row_word(&circular_rows[0], true, UINT8_C(7)), UINT16_C(0)},
        .word_count = 1U,
        .mode = CIRCULAR_CPU_WAIT_THEN_READY,
        .read_value_word = UINT16_C(0xBEEF)};
    memset(storage, 0, sizeof(storage));
    cpu = init_cpu(&fixture, storage);
    seed_cpu(cpu, UINT8_C(7), old_ar, UINT16_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[7] == old_ar);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[0] == fixture.ids[1]);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[7] == old_ar);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_fault_and_cancel_leave_architecture_unpublished(void)
{
    circular_cpu_fixture fixture = {
        .words = {row_word(&circular_rows[0], false, UINT8_C(7)), UINT16_C(0)},
        .word_count = 1U,
        .mode = CIRCULAR_CPU_FAULT,
        .read_value_word = UINT16_C(0xBEEF)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
    const uint16_t old_ar = UINT16_C(0x1240);
    const ti_c54x_arch_state before =
        (seed_cpu(cpu, UINT8_C(7), old_ar, UINT16_C(5)), cpu->arch_state);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before, sizeof(before)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);

    fixture = (circular_cpu_fixture){
        .words = {row_word(&circular_rows[9], true, UINT8_C(0)), UINT16_C(0)},
        .word_count = 1U,
        .mode = CIRCULAR_CPU_WAIT_ALWAYS};
    memset(storage, 0, sizeof(storage));
    cpu = init_cpu(&fixture, storage);
    seed_cpu(cpu, UINT8_C(0), old_ar, UINT16_C(5));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(fixture.cancel_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_stale_dependencies_reject_before_resume(void)
{
    for (unsigned mutate_bk = 0U; mutate_bk <= 1U; ++mutate_bk) {
        circular_cpu_fixture fixture = {
            .words = {row_word(&circular_rows[0], false, UINT8_C(2)), UINT16_C(0)},
            .word_count = 1U,
            .mode = CIRCULAR_CPU_WAIT_THEN_READY,
            .read_value_word = UINT16_C(0xBEEF)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
        const uint16_t old_ar = UINT16_C(0x1240);
        ti_c54x_run_result result;

        seed_cpu(cpu, UINT8_C(2), old_ar, UINT16_C(5));
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
        if (mutate_bk != 0U) {
            TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(
                                   &cpu->arch_state, UINT16_C(6)) ==
                               TI_C54X_ARCH_STATE_OK);
        } else {
            TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                   &cpu->arch_state, UINT8_C(2),
                                   UINT16_C(0x1241)) == TI_C54X_ARCH_STATE_OK);
        }
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_unknown_dependencies_and_planner_boundary_reject_before_issue(void)
{
    for (unsigned dependency = 0U; dependency < 4U; ++dependency) {
        circular_cpu_fixture fixture = {
            .words = {row_word(&circular_rows[0], false, UINT8_C(3)), UINT16_C(0)},
            .word_count = 1U,
            .mode = CIRCULAR_CPU_READY};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
        ti_c54x_run_result result;

        seed_cpu(cpu, UINT8_C(3), UINT16_C(0x1240), UINT16_C(5));
        if (dependency == 0U) {
            cpu->arch_state.status.st1_known_mask = (uint16_t)(
                cpu->arch_state.status.st1_known_mask &
                (uint16_t)~TI_C54X_ST1_CMPT_MASK);
        } else if (dependency == 1U) {
            cpu->arch_state.status.st0_known_mask = (uint16_t)(
                cpu->arch_state.status.st0_known_mask &
                (uint16_t)~TI_C54X_ST0_ARP_MASK);
        } else if (dependency == 2U) {
            cpu->arch_state.ar_known_mask = (uint8_t)(
                cpu->arch_state.ar_known_mask & (uint8_t)~(UINT8_C(1) << 3U));
        } else {
            cpu->arch_state.scalar_known_mask = (uint8_t)(
                cpu->arch_state.scalar_known_mask &
                (uint8_t)~TI_C54X_ARCH_STATE_KNOWN_BK);
        }
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }

    {
        circular_cpu_fixture fixture = {
            .words = {row_word(&circular_rows[1], true, UINT8_C(3)), UINT16_C(0)},
            .word_count = 1U,
            .mode = CIRCULAR_CPU_READY};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
        ti_c54x_run_result result;

        seed_cpu(cpu, UINT8_C(3), UINT16_C(0x1245), UINT16_C(5));
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[3] == UINT16_C(0x1245));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

int
main(void)
{
    test_all_twenty_rows_decode_dispatch_and_update();
    test_bk_zero_ready_and_wait_publish_without_update();
    test_fault_and_cancel_leave_architecture_unpublished();
    test_stale_dependencies_reject_before_resume();
    test_unknown_dependencies_and_planner_boundary_reject_before_issue();
    return 0;
}
