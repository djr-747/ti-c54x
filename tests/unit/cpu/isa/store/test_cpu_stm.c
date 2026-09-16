/* Module contract
 * Owns: public-CPU RED proofs for the two-word `STM #lk,direct`
 * store, including MMR-routed and DP0-RAM retirement with zero
 * versus one endpoint call, extension fetch ordering, page-0
 * mode-state independence, and the deferred indirect-form stop.
 * Does not own: production CPU behavior, catalog/decode or pure
 * store effects, address planning, shared fixtures, timing,
 * diagnostics, or test manifests.
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

typedef struct stm_fixture {
    uint16_t words[2];
    ti_c54x_program_word_address start;
    unsigned program_calls;
    unsigned endpoint_calls;
    ti_c54x_bus_word16_request requests[4];
    uint16_t ram[128];
} stm_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    stm_fixture *fixture = (stm_fixture *)context;
    const unsigned index = fixture->program_calls;

    TI_C54X_TEST_CHECK(index < 2U);
    TI_C54X_TEST_CHECK(address == fixture->start + (ti_c54x_program_word_address)index);
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
    stm_fixture *fixture = (stm_fixture *)context;
    const unsigned index = fixture->endpoint_calls;

    (void)phase;
    TI_C54X_TEST_CHECK(index < 4U);
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->requests[index] = *request;
    fixture->endpoint_calls++;
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
        request->address_word < 128U) {
        fixture->ram[request->address_word] = request->write_value_word;
    }
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
init_cpu(stm_fixture *fixture, unsigned char *storage,
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

static void check_stm_to_mmr(void)
{
    stm_fixture fixture = {.words = {UINT16_C(0x770E), UINT16_C(0x2468)},
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U,
                           .requests = {{0}},
                           .ram = {0}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;
    uint16_t t_value = UINT16_C(0);

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_read(&cpu->t_register,
                                               &t_value) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK_U64(t_value, UINT16_C(0x2468));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0102));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void check_stm_to_dp0_ram(void)
{
    stm_fixture fixture = {.words = {UINT16_C(0x7720), UINT16_C(0xABCD)},
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U,
                           .requests = {{0}},
                           .ram = {0}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    /* DP0 RAM: exactly one endpoint WRITE with address and value. */
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].address_word,
                           UINT32_C(0x0020));
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].write_value_word,
                           UINT16_C(0xABCD));
    TI_C54X_TEST_CHECK_U64(fixture.ram[0x20], UINT16_C(0xABCD));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0102));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void check_direct_ignores_cmpt(void)
{
    stm_fixture fixture = {.words = {UINT16_C(0x7720), UINT16_C(0xABCD)},
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U,
                           .requests = {{0}},
                           .ram = {0}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;

    /* Page-0 direct observes no mode state: a set CMPT must not
     * reject the store. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state,
                                                     UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].address_word,
                           UINT32_C(0x0020));
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].write_value_word,
                           UINT16_C(0xABCD));
    TI_C54X_TEST_CHECK_U64(fixture.ram[0x20], UINT16_C(0xABCD));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0102));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void check_indirect_form_stops(void)
{
    stm_fixture fixture = {.words = {UINT16_C(0x7790), UINT16_C(0x1234)},
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U,
                           .requests = {{0}},
                           .ram = {0}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    const ti_c54x_arch_state before_arch = cpu->arch_state;
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    /* Public CPU maps the semantic INVALID_OPERAND stop to
     * INVALID_STATE with nothing retired or issued. */
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == before_arch.pc_word);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                              sizeof(before_arch)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    check_stm_to_mmr();
    check_stm_to_dp0_ram();
    check_direct_ignores_cmpt();
    check_indirect_form_stops();
    return 0;
}
