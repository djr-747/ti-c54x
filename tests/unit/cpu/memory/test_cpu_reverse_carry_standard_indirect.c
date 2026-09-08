/* Module contract
 * Owns: public-CPU integration proofs for twenty reverse-carry MOD4/MOD7
 * WORD16 standard-indirect rows, including AR1/AR7 old-address access,
 * selected-AR publication, one-hot AR0 snapshots, resumable endpoint
 * behavior, stale/unknown dependency rejection, and ARF0 pre-bus rejection.
 * Does not own: catalog generation, decode metadata, pure opcode effects,
 * address planning, transaction internals, timing, diagnostics, or production
 * code.
 */

#include <ti_c54x/cpu.h>

#include "../../../../src/cpu/cpu_internal.h"
#include "../../../../src/state/accumulator/accumulator.h"
#include "../../../../src/state/register_layout.h"
#include "../../../../src/state/temporary/t_register.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum reverse_cpu_mode {
    REVERSE_CPU_READY = 0,
    REVERSE_CPU_WAIT_THEN_READY = 1,
    REVERSE_CPU_FAULT = 2,
    REVERSE_CPU_WAIT_ALWAYS = 3
} reverse_cpu_mode;

typedef struct reverse_cpu_fixture {
    uint16_t words[2];
    unsigned word_count;
    reverse_cpu_mode mode;
    uint16_t read_value_word;
    unsigned program_reads;
    unsigned endpoint_calls;
    unsigned begin_calls;
    unsigned resume_calls;
    unsigned ready_calls;
    unsigned cancel_calls;
    uint64_t ids[4];
    ti_c54x_bus_word16_request requests[4];
} reverse_cpu_fixture;

typedef struct reverse_row {
    uint16_t mod4_base;
    uint16_t mod7_base;
    bool read;
    bool two_words;
} reverse_row;

static const reverse_row reverse_rows[] = {
    {UINT16_C(0x10A0), UINT16_C(0x10B8), true, false},
    {UINT16_C(0x14A0), UINT16_C(0x14B8), true, false},
    {UINT16_C(0x80A0), UINT16_C(0x80B8), false, false},
    {UINT16_C(0x12A0), UINT16_C(0x12B8), true, false},
    {UINT16_C(0x44A0), UINT16_C(0x44B8), true, false},
    {UINT16_C(0x30A0), UINT16_C(0x30B8), true, false},
    {UINT16_C(0x76A0), UINT16_C(0x76B8), false, true},
    {UINT16_C(0x46A0), UINT16_C(0x46B8), true, false},
    {UINT16_C(0x32A0), UINT16_C(0x32B8), true, false},
    {UINT16_C(0x8CA0), UINT16_C(0x8CB8), false, false}
};

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    reverse_cpu_fixture *fixture = (reverse_cpu_fixture *)context;
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
    reverse_cpu_fixture *fixture = (reverse_cpu_fixture *)context;
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
    if (fixture->mode == REVERSE_CPU_FAULT) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
    } else if (fixture->mode == REVERSE_CPU_WAIT_ALWAYS ||
               (fixture->mode == REVERSE_CPU_WAIT_THEN_READY &&
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
    reverse_cpu_fixture *fixture = (reverse_cpu_fixture *)context;

    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    fixture->cancel_calls++;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_cpu *
init_cpu(reverse_cpu_fixture *fixture, void *storage)
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
seed_cpu(ti_c54x_cpu *cpu, uint8_t arf, uint16_t ar_value,
         uint16_t ar0_value)
{
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state,
                                                     UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state,
                                                    UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &cpu->arch_state, UINT8_C(0), ar0_value) ==
                       TI_C54X_ARCH_STATE_OK);
    if (arf != UINT8_C(0)) {
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                               &cpu->arch_state, arf, ar_value) ==
                           TI_C54X_ARCH_STATE_OK);
    }
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
    TI_C54X_TEST_CHECK(ti_c54x_t_register_write(
                           &cpu->t_register, UINT16_C(0x1357)) ==
                       TI_C54X_T_REGISTER_OK);
}

static uint16_t
row_word(const reverse_row *row, bool mod7, uint8_t arf)
{
    return (uint16_t)((mod7 ? row->mod7_base : row->mod4_base) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static uint16_t
write_value_for_row(size_t row_index)
{
    if (row_index == 2U) {
        return UINT16_C(0xCAFE);
    }
    if (row_index == 6U) {
        return UINT16_C(0xA55A);
    }
    return UINT16_C(0x1357);
}

static uint16_t
old_address(bool mod7)
{
    return mod7 ? UINT16_C(0x0060) : UINT16_C(0x0068);
}

static uint16_t
updated_address(bool mod7)
{
    return mod7 ? UINT16_C(0x0068) : UINT16_C(0x0060);
}

static void
check_common_result(const ti_c54x_run_result *result,
                    ti_c54x_stop_reason reason, uint64_t attempted,
                    uint64_t retired)
{
    TI_C54X_TEST_CHECK(result->reason == reason);
    TI_C54X_TEST_CHECK(result->bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result->instructions_attempted == attempted);
    TI_C54X_TEST_CHECK(result->instructions_retired == retired);
}

static void
check_request(const reverse_cpu_fixture *fixture, const reverse_row *row,
              size_t row_index, bool mod7, uint16_t old_ar)
{
    TI_C54X_TEST_CHECK(fixture->requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[0].access ==
                       (row->read ? TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                                  : TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE));
    TI_C54X_TEST_CHECK(fixture->requests[0].address_word == old_ar);
    if (!row->read) {
        TI_C54X_TEST_CHECK(fixture->requests[0].write_value_word ==
                           write_value_for_row(row_index));
    }
    (void)mod7;
}

static void
test_all_twenty_rows_ar1_ar7_wait_ready(void)
{
    static const uint8_t arfs[] = {UINT8_C(1), UINT8_C(7)};

    for (size_t row_index = 0U;
         row_index < sizeof(reverse_rows) / sizeof(reverse_rows[0]);
         ++row_index) {
        const reverse_row *row = &reverse_rows[row_index];

        for (unsigned mod7_value = 0U; mod7_value <= 1U; ++mod7_value) {
            const bool mod7 = mod7_value != 0U;

            for (size_t arf_index = 0U; arf_index < sizeof(arfs); ++arf_index) {
                const uint8_t arf = arfs[arf_index];
                const uint16_t old_ar = old_address(mod7);
                reverse_cpu_fixture fixture = {
                    .words = {row_word(row, mod7, arf), UINT16_C(0xA55A)},
                    .word_count = row->two_words ? 2U : 1U,
                    .mode = REVERSE_CPU_WAIT_THEN_READY,
                    .read_value_word = UINT16_C(0x8123)};
                _Alignas(max_align_t) unsigned char storage[1024] = {0};
                ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
                ti_c54x_run_result result;

                seed_cpu(cpu, arf, old_ar, UINT16_C(0x0008));
                result = ti_c54x_cpu_run(cpu, UINT64_C(1));
                check_common_result(&result, TI_C54X_STOP_BUS_WAIT, 1U, 0U);
                TI_C54X_TEST_CHECK(result.opcode_valid);
                TI_C54X_TEST_CHECK(fixture.program_reads == fixture.word_count);
                TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
                TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
                TI_C54X_TEST_CHECK(fixture.requests[0].address_word == old_ar);
                TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
                TI_C54X_TEST_CHECK(cpu->arch_state.ar[0] == UINT16_C(0x0008));

                result = ti_c54x_cpu_run(cpu, UINT64_C(1));
                check_common_result(&result, TI_C54X_STOP_BUDGET_EXHAUSTED,
                                    0U, 1U);
                TI_C54X_TEST_CHECK(fixture.program_reads == fixture.word_count);
                TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
                TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
                TI_C54X_TEST_CHECK(fixture.ids[0] == fixture.ids[1]);
                check_request(&fixture, row, row_index, mod7, old_ar);
                TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] ==
                                   updated_address(mod7));
                TI_C54X_TEST_CHECK(cpu->arch_state.ar[0] == UINT16_C(0x0008));
                TI_C54X_TEST_CHECK((cpu->arch_state.scalar_known_mask &
                                    TI_C54X_ARCH_STATE_KNOWN_BK) == 0U);
                TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
            }
        }
    }
}

static void
test_ready_immediate_read_and_write(void)
{
    static const struct {
        size_t row_index;
        bool mod7;
        uint8_t arf;
    } cases[] = {{0U, false, UINT8_C(1)}, {2U, true, UINT8_C(7)}};

    for (size_t case_index = 0U; case_index < sizeof(cases) / sizeof(cases[0]);
         ++case_index) {
        const size_t row_index = cases[case_index].row_index;
        const reverse_row *row = &reverse_rows[row_index];
        const bool mod7 = cases[case_index].mod7;
        const uint8_t arf = cases[case_index].arf;
        const uint16_t old_ar = old_address(mod7);
        reverse_cpu_fixture fixture = {
            .words = {row_word(row, mod7, arf), UINT16_C(0xA55A)},
            .word_count = row->two_words ? 2U : 1U,
            .mode = REVERSE_CPU_READY,
            .read_value_word = UINT16_C(0x8123)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
        const ti_c54x_run_result result =
            (seed_cpu(cpu, arf, old_ar, UINT16_C(0x0008)),
             ti_c54x_cpu_run(cpu, UINT64_C(1)));

        check_common_result(&result, TI_C54X_STOP_BUDGET_EXHAUSTED, 1U, 1U);
        TI_C54X_TEST_CHECK(result.opcode_valid);
        TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
        TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
        TI_C54X_TEST_CHECK(fixture.program_reads == fixture.word_count);
        check_request(&fixture, row, row_index, mod7, old_ar);
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == updated_address(mod7));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[0] == UINT16_C(0x0008));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_fault_and_cancel_leave_architecture_unpublished(void)
{
    reverse_cpu_fixture fixture = {
        .words = {row_word(&reverse_rows[0], false, UINT8_C(7)), UINT16_C(0)},
        .word_count = 1U,
        .mode = REVERSE_CPU_FAULT,
        .read_value_word = UINT16_C(0xBEEF)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
    const uint16_t old_ar = UINT16_C(0x0068);
    ti_c54x_arch_state before;
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT8_C(7), old_ar, UINT16_C(0x0008));
    before = cpu->arch_state;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before, sizeof(before)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);

    fixture = (reverse_cpu_fixture){
        .words = {row_word(&reverse_rows[9], true, UINT8_C(1)), UINT16_C(0)},
        .word_count = 1U,
        .mode = REVERSE_CPU_WAIT_ALWAYS};
    memset(storage, 0, sizeof(storage));
    cpu = init_cpu(&fixture, storage);
    seed_cpu(cpu, UINT8_C(1), UINT16_C(0x0060), UINT16_C(0x0008));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[1] == UINT16_C(0x0060));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(fixture.cancel_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_stale_selected_ar_and_ar0_reject_before_resume(void)
{
    for (unsigned mutate_ar0 = 0U; mutate_ar0 <= 1U; ++mutate_ar0) {
        reverse_cpu_fixture fixture = {
            .words = {row_word(&reverse_rows[0], false, UINT8_C(7)), UINT16_C(0)},
            .word_count = 1U,
            .mode = REVERSE_CPU_WAIT_THEN_READY,
            .read_value_word = UINT16_C(0xBEEF)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
        ti_c54x_arch_state before;
        ti_c54x_run_result result;

        seed_cpu(cpu, UINT8_C(7), UINT16_C(0x0068), UINT16_C(0x0008));
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
        if (mutate_ar0 != 0U) {
            TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                   &cpu->arch_state, UINT8_C(0),
                                   UINT16_C(0x0010)) ==
                               TI_C54X_ARCH_STATE_OK);
        } else {
            TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                                   &cpu->arch_state, UINT8_C(7),
                                   UINT16_C(0x0060)) ==
                               TI_C54X_ARCH_STATE_OK);
        }
        before = cpu->arch_state;
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before, sizeof(before)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_unknown_dependency_order_rejects_before_data_bus(void)
{
    for (unsigned dependency = 0U; dependency < 4U; ++dependency) {
        reverse_cpu_fixture fixture = {
            .words = {row_word(&reverse_rows[0], false, UINT8_C(1)), UINT16_C(0)},
            .word_count = 1U,
            .mode = REVERSE_CPU_READY};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
        ti_c54x_arch_state before;
        ti_c54x_run_result result;

        seed_cpu(cpu, UINT8_C(1), UINT16_C(0x0068), UINT16_C(0x0008));
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
                cpu->arch_state.ar_known_mask & (uint8_t)~(UINT8_C(1) << 1U));
        } else {
            cpu->arch_state.ar_known_mask = (uint8_t)(
                cpu->arch_state.ar_known_mask & (uint8_t)~UINT8_C(1));
        }
        before = cpu->arch_state;
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.opcode_valid);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before, sizeof(before)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_arf0_decodes_but_rejects_before_data_bus(void)
{
    for (size_t row_index = 0U;
         row_index < sizeof(reverse_rows) / sizeof(reverse_rows[0]);
         ++row_index) {
        for (unsigned mod7_value = 0U; mod7_value <= 1U; ++mod7_value) {
            const reverse_row *row = &reverse_rows[row_index];
            reverse_cpu_fixture fixture = {
                .words = {row_word(row, mod7_value != 0U, UINT8_C(0)),
                          UINT16_C(0xA55A)},
                .word_count = row->two_words ? 2U : 1U,
                .mode = REVERSE_CPU_READY};
            _Alignas(max_align_t) unsigned char storage[1024] = {0};
            ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
            ti_c54x_arch_state before;
            ti_c54x_run_result result;

            seed_cpu(cpu, UINT8_C(0), UINT16_C(0x0068), UINT16_C(0x0008));
            before = cpu->arch_state;
            result = ti_c54x_cpu_run(cpu, UINT64_C(1));
            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
            TI_C54X_TEST_CHECK(result.opcode_valid);
            TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
            TI_C54X_TEST_CHECK(fixture.program_reads == fixture.word_count);
            TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
            TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before,
                                      sizeof(before)) == 0);
            TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
        }
    }
}

int
main(void)
{
    test_all_twenty_rows_ar1_ar7_wait_ready();
    test_ready_immediate_read_and_write();
    test_fault_and_cancel_leave_architecture_unpublished();
    test_stale_selected_ar_and_ar0_reject_before_resume();
    test_unknown_dependency_order_rejects_before_data_bus();
    test_arf0_decodes_but_rejects_before_data_bus();
    return 0;
}
