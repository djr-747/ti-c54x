/* Module contract
 * Owns: public-CPU RED proofs for `MPY Xmem,Ymem` retirement,
 * including ordered X/Y DATA reads, the TI Example 2 STORE
 * vector with read-phase T load of the X-side word, MOD1/MOD2
 * AR updates, and accumulator plus overflow commit.
 * Does not own: catalog/decode or pure multiply effects,
 * address-planner internals, endpoint reducer behavior, production
 * code, timing, or diagnostics.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct mpy_dual_fixture {
    uint16_t program_word;
    uint16_t x_address;
    uint16_t y_address;
    uint16_t x_value;
    uint16_t y_value;
    uint8_t x_ar;
    uint8_t y_ar;
    unsigned program_reads;
    unsigned endpoint_calls;
    ti_c54x_bus_word16_request requests[4];
} mpy_dual_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    mpy_dual_fixture *fixture = (mpy_dual_fixture *)context;

    TI_C54X_TEST_CHECK(address_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(value_word != NULL);
    TI_C54X_TEST_CHECK(fixture->program_reads == 0U);
    fixture->program_reads++;
    *value_word = fixture->program_word;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    mpy_dual_fixture *fixture = (mpy_dual_fixture *)context;
    const unsigned index = fixture->endpoint_calls++;
    bool is_x;

    TI_C54X_TEST_CHECK(index < 2U);
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN);
    fixture->requests[index] = *request;
    is_x = request->address_word == fixture->x_address;
    TI_C54X_TEST_CHECK(is_x || request->address_word == fixture->y_address);
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = is_x ? fixture->x_value : fixture->y_value,
        .bus_status = TI_C54X_BUS_OK};
}

static ti_c54x_bus_word16_cancel_result
cancel_data(void *context, uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request)
{
    (void)context;
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_cpu *
init_mpy_dual(mpy_dual_fixture *fixture, void *storage,
              ti_c54x_error *status_out)
{
    const ti_c54x_config config = {UINT32_C(0), {0U, NULL, NULL}};
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = fixture, .advance = advance_data, .cancel = cancel_data};
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program,
                             .data_endpoint = endpoint};
    ti_c54x_cpu *cpu = NULL;

    *status_out = ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                   &bus, &cpu);
    TI_C54X_TEST_CHECK(*status_out == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state,
                                                   fixture->x_ar,
                                                   fixture->x_address) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state,
                                                   fixture->y_ar,
                                                   fixture->y_address) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0xFFFFFFE0)) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x0007);
    cpu->t_register.known = true;
    /* TI MPY Example 2 runs with FRCT=0: clear the bit and mark it known. */
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 & ~TI_C54X_ST1_FRCT_MASK);
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_FRCT_MASK);
    return cpu;
}

static void
run_case(uint16_t opcode, uint8_t x_ar, uint8_t y_ar, uint16_t x_address,
         uint16_t y_address, uint16_t x_value, uint16_t y_value,
         uint64_t expected_b, uint64_t expected_a, uint16_t expected_t,
         uint16_t expected_x_after, uint16_t expected_y_after)
{
    mpy_dual_fixture fixture = {.program_word = opcode,
                                .x_address = x_address,
                                .y_address = y_address,
                                .x_value = x_value,
                                .y_value = y_value,
                                .x_ar = x_ar,
                                .y_ar = y_ar,
                                .program_reads = 0U,
                                .endpoint_calls = 0U,
                                .requests = {{0}}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_mpy_dual(&fixture, storage, &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    /* X reads before Y over the one endpoint. */
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word ==
                       fixture.x_address);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word ==
                       fixture.y_address);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == expected_a);
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == expected_b);
    /* Read-phase T load takes the X-side word. */
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == expected_t);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[x_ar] == expected_x_after);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[y_ar] == expected_y_after);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_example2_retires(void)
{
    /* TI MPY Example 2: MPY *AR2-, *AR4+0%, B with B before
     * FF FFFF FFE0, FRCT 0, mem[01FF]=0010, mem[0300]=0002:
     * B=00 0000 0020 (STORE overwrites -32), T=0010,
     * AR2=01FE, AR4=0301. */
    run_case(UINT16_C(0xA54A), UINT8_C(2), UINT8_C(4), UINT16_C(0x01FF),
             UINT16_C(0x0300), UINT16_C(0x0010), UINT16_C(0x0002),
             UINT64_C(0x0020), UINT64_C(0), UINT16_C(0x0010),
             UINT16_C(0x01FE), UINT16_C(0x0301));
}

static void
test_m0_m0_stores_into_a(void)
{
    /* 0xA402: M0_M0, dst A, X=*AR2 (holds 0x0100), Y=*AR4 (holds
     * 0x0200): 0x0003 x 0x0005 = 0x000F into A, T=0003, ARs held. */
    run_case(UINT16_C(0xA402), UINT8_C(2), UINT8_C(4), UINT16_C(0x0100),
             UINT16_C(0x0200), UINT16_C(0x0003), UINT16_C(0x0005),
             UINT64_C(0xFFFFFFE0), UINT64_C(0x000F), UINT16_C(0x0003),
             UINT16_C(0x0100), UINT16_C(0x0200));
}

int
main(void)
{
    test_example2_retires();
    test_m0_m0_stores_into_a();
    return 0;
}
