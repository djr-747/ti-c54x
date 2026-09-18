/* Module contract
 * Owns: public-CPU RED proofs for FRAME retirement, including
 * SP+K publication with wraparound, sequential PC advance with no
 * bus traffic, and transactional rejection on unknown SP.
 * Does not own: production CPU behavior, catalog/decode or pure
 * FRAME effects, the SP-write leaf, shared fixtures, timing,
 * diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/arch_state.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct frame_fixture {
    uint16_t words[4];
    unsigned word_count;
    ti_c54x_program_word_address start;
    unsigned program_calls;
    unsigned endpoint_calls;
} frame_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    frame_fixture *fixture = (frame_fixture *)context;
    const unsigned index = fixture->program_calls;

    TI_C54X_TEST_CHECK(index < fixture->word_count);
    TI_C54X_TEST_CHECK(address ==
                       fixture->start + (ti_c54x_program_word_address)index);
    TI_C54X_TEST_CHECK(value != NULL);
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
    frame_fixture *fixture = (frame_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->endpoint_calls++;
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .bus_status = TI_C54X_BUS_OK,
        .read_value_word = UINT16_C(0),
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY};
}

static ti_c54x_bus_word16_cancel_result
cancel_data(void *context, uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_cpu *
init_cpu(frame_fixture *fixture, unsigned char *storage,
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
read_sp_value(const ti_c54x_cpu *cpu)
{
    uint16_t sp = UINT16_C(0xBEEF);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_sp(&cpu->arch_state, &sp) ==
                       TI_C54X_ARCH_STATE_OK);
    return sp;
}

static void
check_frame_retires(void)
{
    /* TI FRAME 10h example shape: SP 0x1000 -> 0x1010. */
    frame_fixture fixture = {.words = {UINT16_C(0xEE10)},
                             .word_count = 1U,
                             .start = 0U,
                             .program_calls = 0U,
                             .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  UINT16_C(0x1000)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(read_sp_value(cpu) == UINT16_C(0x1010));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_frame_wraps(void)
{
    frame_fixture fixture = {.words = {UINT16_C(0xEEFF)},
                             .word_count = 1U,
                             .start = 0U,
                             .program_calls = 0U,
                             .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  UINT16_C(0x0000)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(read_sp_value(cpu) == UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_unknown_sp_rejects(void)
{
    /* Fresh reset leaves SP unknown: nothing retires, PC frozen. */
    frame_fixture fixture = {.words = {UINT16_C(0xEE10)},
                             .word_count = 1U,
                             .start = 0U,
                             .program_calls = 0U,
                             .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    uint16_t sp = UINT16_C(0xBEEF);

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0100));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_sp(&cpu->arch_state, &sp) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    check_frame_retires();
    check_frame_wraps();
    check_unknown_sp_rejects();
    return 0;
}
