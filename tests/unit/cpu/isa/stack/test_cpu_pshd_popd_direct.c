/* Module contract
 * Owns: public-CPU RED proofs for PSHD/POPD/PSHM/POPM
 * retirement, including Smem/TOS observation handoff through
 * endpoint RAM, MMR-leg routing with TOS-only endpoint traffic,
 * SP publication, sequential PC advance, and transactional
 * rejection on unknown SP.
 * Does not own: production CPU behavior, catalog/decode or pure
 * pair effects, the pair root, shared fixtures, timing,
 * diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/arch_state.h"
#include "../../../../../src/state/mmr_file.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PAIR_DATA_WORDS UINT32_C(0x2000)

typedef struct pair_fixture {
    uint16_t words[4];
    unsigned word_count;
    ti_c54x_program_word_address start;
    unsigned program_calls;
    unsigned endpoint_calls;
    uint16_t data[0x2000];
} pair_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    pair_fixture *fixture = (pair_fixture *)context;
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
    pair_fixture *fixture = (pair_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->address_word < PAIR_DATA_WORDS);
    fixture->endpoint_calls++;
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .bus_status = TI_C54X_BUS_OK,
        .read_value_word = UINT16_C(0),
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY};
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        observation->read_value_word =
            fixture->data[request->address_word];
    } else {
        fixture->data[request->address_word] =
            request->write_value_word;
    }
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
init_cpu(pair_fixture *fixture, unsigned char *storage,
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
check_pshd_retires(void)
{
    /* TI PSHD *AR3+ example shape: Smem word lands at old SP - 1. */
    pair_fixture fixture = {.words = {UINT16_C(0x4B60)},
                            .word_count = 1U,
                            .start = 0U,
                            .program_calls = 0U,
                            .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;

    fixture.data[UINT16_C(0x0060)] = UINT16_C(0xBEEF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  UINT16_C(0x1111)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.data[UINT16_C(0x1110)] ==
                       UINT16_C(0xBEEF));
    TI_C54X_TEST_CHECK(read_sp_value(cpu) == UINT16_C(0x1110));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_popd_retires(void)
{
    /* TI POPD 10 example shape: (TOS) lands in Smem, SP + 1. */
    pair_fixture fixture = {.words = {UINT16_C(0x8B60)},
                            .word_count = 1U,
                            .start = 0U,
                            .program_calls = 0U,
                            .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;

    fixture.data[UINT16_C(0x1110)] = UINT16_C(0x1234);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  UINT16_C(0x1110)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.data[UINT16_C(0x0060)] ==
                       UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(read_sp_value(cpu) == UINT16_C(0x1111));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_pshm_routes_mmr(void)
{
    /* TI PSHM BRC example shape: BRC lands at old SP - 1 with the
     * MMR read served internally. */
    pair_fixture fixture = {.words = {UINT16_C(0x4A1A)},
                            .word_count = 1U,
                            .start = 0U,
                            .program_calls = 0U,
                            .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_write_brc(
                           &cpu->arch_state.mmr_file,
                           UINT16_C(0x1234)) == TI_C54X_MMR_FILE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  UINT16_C(0x2000)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.data[UINT16_C(0x1FFF)] ==
                       UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(read_sp_value(cpu) == UINT16_C(0x1FFF));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_popm_routes_mmr(void)
{
    /* TI POPM AR5 example shape: (TOS) lands in AR5, SP + 1. */
    pair_fixture fixture = {.words = {UINT16_C(0x8A15)},
                            .word_count = 1U,
                            .start = 0U,
                            .program_calls = 0U,
                            .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;
    uint16_t ar5 = UINT16_C(0);

    fixture.data[UINT16_C(0x03F0)] = UINT16_C(0x0060);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  UINT16_C(0x03F0)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state,
                                                  UINT8_C(5),
                                                  &ar5) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ar5 == UINT16_C(0x0060));
    TI_C54X_TEST_CHECK(read_sp_value(cpu) == UINT16_C(0x03F1));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
seed_indirect_data_leg(ti_c54x_cpu *cpu, uint8_t ar_index, uint16_t ar_value)
{
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state,
                                                    UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state,
                                                   UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state,
                                                  ar_index,
                                                  ar_value) ==
                       TI_C54X_ARCH_STATE_OK);
}

static uint16_t
read_ar_value(const ti_c54x_cpu *cpu, uint8_t index)
{
    uint16_t value = UINT16_C(0xBEEF);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state, index,
                                                 &value) ==
                       TI_C54X_ARCH_STATE_OK);
    return value;
}

static void
check_pshd_indirect_retires(void)
{
    /* PSHD *AR3 (MOD1): Smem READ at old AR3, TOS WRITE, AR3 - 1. */
    pair_fixture fixture = {.words = {UINT16_C(0x4B8B)},
                            .word_count = 1U,
                            .start = 0U,
                            .program_calls = 0U,
                            .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;

    fixture.data[UINT16_C(0x0400)] = UINT16_C(0xCAFE);
    seed_indirect_data_leg(cpu, UINT8_C(3), UINT16_C(0x0400));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  UINT16_C(0x1111)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.data[UINT16_C(0x1110)] ==
                       UINT16_C(0xCAFE));
    TI_C54X_TEST_CHECK(read_ar_value(cpu, UINT8_C(3)) == UINT16_C(0x03FF));
    TI_C54X_TEST_CHECK(read_sp_value(cpu) == UINT16_C(0x1110));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_popd_indirect_retires(void)
{
    /* POPD *AR2 (MOD2): TOS READ, Smem WRITE at old AR2, AR2 + 1. */
    pair_fixture fixture = {.words = {UINT16_C(0x8B92)},
                            .word_count = 1U,
                            .start = 0U,
                            .program_calls = 0U,
                            .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;

    fixture.data[UINT16_C(0x1110)] = UINT16_C(0x1234);
    seed_indirect_data_leg(cpu, UINT8_C(2), UINT16_C(0x0500));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  UINT16_C(0x1110)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.data[UINT16_C(0x0500)] ==
                       UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(read_ar_value(cpu, UINT8_C(2)) == UINT16_C(0x0501));
    TI_C54X_TEST_CHECK(read_sp_value(cpu) == UINT16_C(0x1111));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_unknown_sp_rejects(void)
{
    /* Fresh reset leaves SP unknown: nothing retires, PC frozen. */
    pair_fixture fixture = {.words = {UINT16_C(0x4B60)},
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
    check_pshd_retires();
    check_popd_retires();
    check_pshd_indirect_retires();
    check_popd_indirect_retires();
    check_pshm_routes_mmr();
    check_popm_routes_mmr();
    check_unknown_sp_rejects();
    return 0;
}
