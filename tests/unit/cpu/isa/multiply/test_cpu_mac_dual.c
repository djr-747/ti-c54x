/* Module contract
 * Owns: public-CPU RED proofs for `MAC Xmem,Ymem` retirement,
 * including ordered X/Y DATA reads, the TI Example 4/6-derived
 * vectors with read-phase T load of the X-side word, MACR
 * rounding, MOD2 post-increment of both ARs, and accumulator
 * plus overflow commit.
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

typedef struct mac_dual_fixture {
    uint16_t program_word;
    uint16_t x_address;
    uint16_t y_address;
    uint16_t x_value;
    uint16_t y_value;
    unsigned program_reads;
    unsigned endpoint_calls;
    ti_c54x_bus_word16_request requests[4];
} mac_dual_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    mac_dual_fixture *fixture = (mac_dual_fixture *)context;

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
    mac_dual_fixture *fixture = (mac_dual_fixture *)context;
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
init_mac_dual(mac_dual_fixture *fixture, void *storage,
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
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state, 5U,
                                                   fixture->x_address) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state, 4U,
                                                   fixture->y_address) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x1000)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0004)) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x0008);
    cpu->t_register.known = true;
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_FRCT_MASK);
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_FRCT_MASK);
    return cpu;
}

static void
run_case(uint16_t opcode, uint64_t expected_b, uint16_t expected_x_after,
         uint16_t expected_y_after)
{
    /* TI Example 4/6-derived vectors: X=mem[AR5]=0x5678,
     * Y=mem[AR4]=0x1234, src A=0x1000, B before=4, T before=8,
     * FRCT=1. */
    mac_dual_fixture fixture = {.program_word = opcode,
                                .x_address = UINT16_C(0x0100),
                                .y_address = UINT16_C(0x0200),
                                .x_value = UINT16_C(0x5678),
                                .y_value = UINT16_C(0x1234),
                                .program_reads = 0U,
                                .endpoint_calls = 0U,
                                .requests = {{0}}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_mac_dual(&fixture, storage, &status);
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
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0x1000));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == expected_b);
    /* Read-phase T load takes the X-side word. */
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x5678));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[5] == expected_x_after);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[4] == expected_y_after);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_example4_derived_retires(void)
{
    /* 0xB132: M0_M0, R=0, src A, dst B, X=*AR5 (MOD0 holds
     * 0x0100), Y=*AR4 (MOD0 holds 0x0200):
     * (0x5678<<1) x 0x1234 + 0x1000 = 0x0C4C10C0. */
    run_case(UINT16_C(0xB132), UINT64_C(0x0C4C10C0), UINT16_C(0x0100),
             UINT16_C(0x0200));
}

static void
test_macr_mod2_postincrements(void)
{
    /* 0xB5BA: M2_M2, R=1, src A, dst B, X=*AR5+0-style
     * post-increment, Y likewise: rounded 0x0C4C0000 with
     * AR5=0x0101 and AR4=0x0201. */
    run_case(UINT16_C(0xB5BA), UINT64_C(0x0C4C0000), UINT16_C(0x0101),
             UINT16_C(0x0201));
}

int
main(void)
{
    test_example4_derived_retires();
    test_macr_mod2_postincrements();
    return 0;
}
