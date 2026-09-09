/* Module contract
 * Owns: public-CPU RED proofs for the two-word `LD Smem[,SHIFT],dst`
 * load, including SXM extension, signed SHIFT with OVM saturation and
 * destination overflow, old-AR reads, postincrement, extension fetch
 * ordering, resumable completion, and transactional rejection.
 * Does not own: production CPU behavior, catalog/decode or pure load
 * effects, address planning, shared fixtures, timing, diagnostics, or
 * test registration outside the load leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/foundation/arithmetic40.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum ld_shift_memory_mode {
    LD_SHIFT_READY = 0,
    LD_SHIFT_WAIT_THEN_READY = 1,
    LD_SHIFT_FAULT = 2
} ld_shift_memory_mode;

typedef struct ld_shift_fixture {
    uint16_t words[2];
    ti_c54x_program_word_address start;
    ld_shift_memory_mode mode;
    uint16_t read_value_word;
    ti_c54x_program_word_address addresses[2];
    unsigned program_calls;
    unsigned endpoint_calls;
    unsigned begin_calls;
    unsigned resume_calls;
    unsigned ready_calls;
    unsigned cancel_calls;
    uint64_t ids[4];
    ti_c54x_bus_word16_request requests[4];
} ld_shift_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    ld_shift_fixture *fixture = (ld_shift_fixture *)context;
    const unsigned index = fixture->program_calls;

    TI_C54X_TEST_CHECK(index < 2U);
    TI_C54X_TEST_CHECK(address ==
                       fixture->start +
                           (ti_c54x_program_word_address)index);
    TI_C54X_TEST_CHECK(value != NULL);
    fixture->addresses[index] = address;
    *value = fixture->words[index];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    ld_shift_fixture *fixture = (ld_shift_fixture *)context;
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
    if (fixture->mode == LD_SHIFT_FAULT) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
    } else if (fixture->mode == LD_SHIFT_WAIT_THEN_READY &&
               fixture->endpoint_calls == 1U) {
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
    ld_shift_fixture *fixture = (ld_shift_fixture *)context;

    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    fixture->cancel_calls++;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_cpu *
init_cpu(ld_shift_fixture *fixture, unsigned char *storage,
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
ld_shift_word0(uint8_t mode, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x6F80) |
                      (uint16_t)(((uint16_t)(mode & UINT8_C(3))) << 3U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static uint16_t
ld_shift_word1(uint8_t destination, uint8_t shift)
{
    return (uint16_t)(UINT16_C(0x0C40) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (uint16_t)(shift & UINT8_C(0x1F)));
}

static void
seed_cpu(ti_c54x_cpu *cpu, uint8_t arf, uint16_t ar_value, uint8_t sxm,
         uint8_t ovm)
{
    cpu->arch_state.status.st0 = UINT16_C(0x0000);
    cpu->arch_state.status.st1 = (uint16_t)(
        (sxm != UINT8_C(0) ? TI_C54X_ST1_SXM_MASK : UINT16_C(0)) |
        (ovm != UINT8_C(0) ? TI_C54X_ST1_OVM_MASK : UINT16_C(0)));
    cpu->arch_state.status.pmst = UINT16_C(0x0000);
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
                           UINT64_C(0x1111222233)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000AABBCC)) == TI_C54X_ACCUMULATOR_OK);
}

/* Independent oracle mirroring the ops contract. */
static uint64_t
expected_shift(uint16_t read_value, uint8_t sxm, uint8_t shift_raw,
               uint8_t ovm, bool *overflow)
{
    uint64_t widened = (uint64_t)read_value;
    uint64_t value;
    uint64_t guard;
    uint8_t bit31;
    bool negative;

    if (sxm != UINT8_C(0) &&
        (read_value & UINT16_C(0x8000)) != UINT16_C(0)) {
        widened |= UINT64_C(0xFFFFFFFF0000);
    }
    negative = (widened & UINT64_C(0x8000000000)) != UINT64_C(0);
    if (shift_raw <= UINT8_C(15)) {
        value = (widened << shift_raw) & TI_C54X_U40_MASK;
    } else {
        const unsigned right = 32U - shift_raw;
        const int64_t signed_value =
            (int64_t)(negative
                          ? (widened | UINT64_C(0xFFFFFF0000000000))
                          : widened);

        value = ((uint64_t)(signed_value >> right)) & TI_C54X_U40_MASK;
    }
    guard = (value >> 32U) & UINT64_C(0xFF);
    bit31 = (uint8_t)((value >> 31U) & UINT64_C(1));
    *overflow =
        guard != (bit31 != UINT8_C(0) ? UINT64_C(0xFF) : UINT64_C(0));
    if (*overflow && ovm != UINT8_C(0)) {
        value = negative ? UINT64_C(0xFF80000000) : UINT64_C(0x007FFFFFFF);
    }
    return value;
}

static void
check_read_request(const ld_shift_fixture *fixture, uint16_t old_ar)
{
    TI_C54X_TEST_CHECK(fixture->requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture->requests[0].address_word == old_ar);
    TI_C54X_TEST_CHECK(fixture->requests[0].write_value_word == UINT16_C(0));
}

static void
check_ov(const ti_c54x_cpu *cpu, uint8_t destination, bool overflow)
{
    const uint16_t mask = destination == UINT8_C(0) ? TI_C54X_ST0_OVA_MASK
                                                   : TI_C54X_ST0_OVB_MASK;

    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & mask) ==
                       (overflow ? mask : UINT16_C(0)));
}

static void
test_ready_retires_shift_and_postincrements(void)
{
    static const uint16_t reads[] = {UINT16_C(0x0000), UINT16_C(0x7FFF),
                                     UINT16_C(0x8000), UINT16_C(0xFFFF)};
    static const uint8_t shifts[] = {UINT8_C(0), UINT8_C(15), UINT8_C(16),
                                     UINT8_C(31)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1); ++destination) {
        for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
            for (uint8_t ovm = 0U; ovm <= UINT8_C(1); ++ovm) {
                for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
                    for (size_t shift_index = 0U;
                         shift_index < sizeof(shifts) / sizeof(shifts[0]);
                         ++shift_index) {
                        for (size_t value_index = 0U;
                             value_index < sizeof(reads) / sizeof(reads[0]);
                             ++value_index) {
                            const uint16_t old_ar = (uint16_t)(
                                UINT16_C(0x2100) +
                                (uint16_t)(arf * UINT8_C(4)));
                            ld_shift_fixture fixture = {
                                .words = {ld_shift_word0(UINT8_C(2), arf),
                                          ld_shift_word1(destination,
                                                         shifts[shift_index])},
                                .mode = LD_SHIFT_READY,
                                .read_value_word = reads[value_index]};
                            _Alignas(max_align_t)
                                unsigned char storage[1024] = {0};
                            ti_c54x_cpu *cpu =
                                init_cpu(&fixture, storage, UINT32_C(0));
                            const ti_c54x_accumulator_state
                                before_accumulators =
                                    (seed_cpu(cpu, arf, old_ar, sxm, ovm),
                                     cpu->accumulators);
                            bool overflow = false;
                            const uint64_t expected = expected_shift(
                                reads[value_index], sxm,
                                shifts[shift_index], ovm, &overflow);
                            ti_c54x_run_result result =
                                ti_c54x_cpu_run(cpu, UINT64_C(1));

                            TI_C54X_TEST_CHECK(
                                result.reason ==
                                TI_C54X_STOP_BUDGET_EXHAUSTED);
                            TI_C54X_TEST_CHECK(result.bus_status ==
                                               TI_C54X_BUS_OK);
                            TI_C54X_TEST_CHECK(
                                result.instructions_attempted ==
                                UINT64_C(1));
                            TI_C54X_TEST_CHECK(
                                result.instructions_retired == UINT64_C(1));
                            TI_C54X_TEST_CHECK(result.opcode_word ==
                                               fixture.words[0]);
                            TI_C54X_TEST_CHECK(result.opcode_valid);
                            TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
                            TI_C54X_TEST_CHECK(fixture.addresses[0] ==
                                               UINT32_C(0));
                            TI_C54X_TEST_CHECK(fixture.addresses[1] ==
                                               UINT32_C(1));
                            TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
                            TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
                            check_read_request(&fixture, old_ar);
                            TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] ==
                                               (uint16_t)(old_ar +
                                                          UINT16_C(1)));
                            TI_C54X_TEST_CHECK(result.pc_word ==
                                               UINT32_C(2));
                            check_ov(cpu, destination, overflow);
                            if (destination == UINT8_C(0)) {
                                TI_C54X_TEST_CHECK(
                                    cpu->accumulators.a_raw40 == expected);
                                TI_C54X_TEST_CHECK(
                                    cpu->accumulators.b_raw40 ==
                                    before_accumulators.b_raw40);
                            } else {
                                TI_C54X_TEST_CHECK(
                                    cpu->accumulators.b_raw40 == expected);
                                TI_C54X_TEST_CHECK(
                                    cpu->accumulators.a_raw40 ==
                                    before_accumulators.a_raw40);
                            }
                            TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                                               TI_C54X_ERROR_OK);
                        }
                    }
                }
            }
        }
    }
}

static void
test_wait_retires_without_refetch(void)
{
    const uint8_t arf = UINT8_C(3);
    const uint16_t old_ar = UINT16_C(0x3200);
    ld_shift_fixture fixture = {
        .words = {ld_shift_word0(UINT8_C(2), arf),
                  ld_shift_word1(UINT8_C(1), UINT8_C(8))},
        .mode = LD_SHIFT_WAIT_THEN_READY,
        .read_value_word = UINT16_C(0x8000)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0));
    bool overflow = false;
    const uint64_t expected =
        expected_shift(UINT16_C(0x8000), UINT8_C(1), UINT8_C(8),
                       UINT8_C(0), &overflow);
    ti_c54x_run_result result;

    seed_cpu(cpu, arf, old_ar, UINT8_C(1), UINT8_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[0] == fixture.ids[1]);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + UINT16_C(1));
    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 == expected);
    check_ov(cpu, UINT8_C(1), overflow);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_fault_rolls_back(void)
{
    ld_shift_fixture fixture = {
        .words = {ld_shift_word0(UINT8_C(2), UINT8_C(1)),
                  ld_shift_word1(UINT8_C(0), UINT8_C(3))},
        .mode = LD_SHIFT_FAULT,
        .read_value_word = UINT16_C(0)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT8_C(1), UINT16_C(0x4000), UINT8_C(1), UINT8_C(0));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_sxm_rejects(void)
{
    ld_shift_fixture fixture = {
        .words = {ld_shift_word0(UINT8_C(2), UINT8_C(1)),
                  ld_shift_word1(UINT8_C(0), UINT8_C(3))},
        .mode = LD_SHIFT_READY,
        .read_value_word = UINT16_C(0x1234)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT8_C(1), UINT16_C(0x4000), UINT8_C(1), UINT8_C(0));
    cpu->arch_state.status.st1_known_mask = (uint16_t)(
        cpu->arch_state.status.st1_known_mask &
        (uint16_t)~TI_C54X_ST1_SXM_MASK);
    {
        const ti_c54x_arch_state before = cpu->arch_state;
        const unsigned endpoint_calls = fixture.endpoint_calls;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == endpoint_calls);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before,
                                  sizeof(before)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    test_ready_retires_shift_and_postincrements();
    test_wait_retires_without_refetch();
    test_fault_rolls_back();
    test_unknown_sxm_rejects();
    return 0;
}
