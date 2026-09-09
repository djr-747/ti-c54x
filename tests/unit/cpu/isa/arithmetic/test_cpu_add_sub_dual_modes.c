/* Module contract
 * Owns: public-CPU retirement proofs for the sixteen non-MOD0/MOD0
 * syntax-6 dual-memory ADD/SUB rows, including per-row AR post-access
 * publication, same-AR Xmod precedence, WAIT persistence of staged commits,
 * and destination routing. Shared fixed-shift arithmetic is owned by the
 * MOD0 proofs and probed here only by value spots.
 * Does not own: opcode semantics, decode tables, bus/reducer implementation,
 * CPU lifecycle, timing, observers, or production state mutation.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct dual_modes_fixture {
    uint16_t program_word;
    uint16_t x_address;
    uint16_t y_address;
    uint16_t x_value;
    uint16_t y_value;
    bool wait_y;
    unsigned endpoint_calls;
    ti_c54x_bus_word16_request requests[4];
} dual_modes_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    dual_modes_fixture *fixture = (dual_modes_fixture *)context;

    TI_C54X_TEST_CHECK(address_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(value_word != NULL);
    *value_word = fixture->program_word;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    dual_modes_fixture *fixture = (dual_modes_fixture *)context;
    const unsigned index = fixture->endpoint_calls++;
    const bool is_x = request->address_word == fixture->x_address;
    const bool is_y = request->address_word == fixture->y_address;

    TI_C54X_TEST_CHECK(index < 4U);
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(request->write_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(is_x || is_y);
    fixture->requests[index] = *request;
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = (fixture->wait_y && index == 1U)
                    ? TI_C54X_BUS_WORD16_OBSERVATION_WAIT
                    : TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = is_x ? fixture->x_value : fixture->y_value,
        .bus_status = TI_C54X_BUS_OK};
    if (observation->kind == TI_C54X_BUS_WORD16_OBSERVATION_WAIT) {
        observation->read_value_word = UINT16_C(0);
    }
    (void)phase;
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
init_cpu(dual_modes_fixture *fixture, void *storage, ti_c54x_error *status_out)
{
    const ti_c54x_config config = {UINT32_C(0), {0U, NULL, NULL}};
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = fixture, .advance = advance_data, .cancel = cancel_data};
    const ti_c54x_bus bus = {
        .context = fixture,
        .program_read_word = read_program,
        .data_endpoint = endpoint};
    ti_c54x_cpu *cpu = NULL;

    *status_out = ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config, &bus,
                                   &cpu);
    TI_C54X_TEST_CHECK(*status_out == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state, 2U,
                                                   fixture->x_address) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state, 3U,
                                                   fixture->y_address) ==
                       TI_C54X_ARCH_STATE_OK);
    return cpu;
}

static void
set_status(ti_c54x_cpu *cpu)
{
    cpu->arch_state.status.st0 = (uint16_t)(
        (cpu->arch_state.status.st0 & (uint16_t)~TI_C54X_ST0_C_MASK) |
        TI_C54X_ST0_C_MASK);
    cpu->arch_state.status.st1 = (uint16_t)(
        cpu->arch_state.status.st1 &
        (uint16_t)~(TI_C54X_ST1_SXM_MASK | TI_C54X_ST1_OVM_MASK));
    cpu->arch_state.status.st0_known_mask = (uint16_t)(
        cpu->arch_state.status.st0_known_mask | TI_C54X_ST0_C_MASK);
    cpu->arch_state.status.st1_known_mask = (uint16_t)(
        cpu->arch_state.status.st1_known_mask |
        (TI_C54X_ST1_SXM_MASK | TI_C54X_ST1_OVM_MASK));
}

static int16_t mode_delta(uint8_t mode)
{
    if (mode == UINT8_C(1)) {
        return -1;
    }
    if (mode == UINT8_C(2)) {
        return 1;
    }
    return 0;
}

static void
run_ready_case(uint16_t opcode, uint8_t x_mode, uint8_t y_mode, bool same_ar)
{
    dual_modes_fixture fixture = {
        .program_word = opcode,
        .x_address = UINT16_C(0x1200),
        .y_address = UINT16_C(0x1300),
        .x_value = UINT16_C(1),
        .y_value = UINT16_C(2),
        .wait_y = false};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    const bool is_sub = (opcode & UINT16_C(0x0200)) != UINT16_C(0);
    uint16_t expected_ar2;
    uint16_t expected_ar3;

    if (is_sub) {
        fixture.x_value = UINT16_C(5);
    }
    if (same_ar) {
        fixture.y_address = fixture.x_address;
        fixture.y_value = fixture.x_value;
    }
    cpu = init_cpu(&fixture, storage, &status);
    set_status(cpu);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == opcode);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == fixture.x_address);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == fixture.y_address);
    /* (1+2)<<16 and (5-2)<<16 share the 0x00030000 fixed-shift spot value;
     * the same-AR alias reads one address twice, so it spots (1+1)<<16. */
    TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                       (same_ar ? UINT64_C(0x00020000)
                                : UINT64_C(0x00030000)));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & TI_C54X_ST0_C_MASK) !=
                       UINT16_C(0));
    expected_ar2 = (uint16_t)(fixture.x_address + mode_delta(x_mode));
    expected_ar3 = same_ar ? fixture.y_address
                           : (uint16_t)(fixture.y_address +
                                        mode_delta(y_mode));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[2] == expected_ar2);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[3] == expected_ar3);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_ready_rows_publish_post_access_updates(void)
{
    static const struct {
        uint16_t base;
        uint8_t x_mode;
        uint8_t y_mode;
    } rows[] = {
        {UINT16_C(0xA004), UINT8_C(0), UINT8_C(1)},
        {UINT16_C(0xA008), UINT8_C(0), UINT8_C(2)},
        {UINT16_C(0xA040), UINT8_C(1), UINT8_C(0)},
        {UINT16_C(0xA044), UINT8_C(1), UINT8_C(1)},
        {UINT16_C(0xA048), UINT8_C(1), UINT8_C(2)},
        {UINT16_C(0xA080), UINT8_C(2), UINT8_C(0)},
        {UINT16_C(0xA084), UINT8_C(2), UINT8_C(1)},
        {UINT16_C(0xA088), UINT8_C(2), UINT8_C(2)},
    };
    size_t index;

    for (index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        /* D=0, Xar=0 (AR2), Yar=1 (AR3). */
        const uint16_t add_word = (uint16_t)(rows[index].base | UINT16_C(1));
        const uint16_t sub_word =
            (uint16_t)(rows[index].base | UINT16_C(0x0201));

        run_ready_case(add_word, rows[index].x_mode, rows[index].y_mode,
                       false);
        run_ready_case(sub_word, rows[index].x_mode, rows[index].y_mode,
                       false);
    }
}

static void
test_same_ar_commits_once_with_xmod_precedence(void)
{
    /* ADD M1M2 with Xar=Yar=0: one X-owned decrement of AR2. */
    run_ready_case(UINT16_C(0xA048), UINT8_C(1), UINT8_C(2), true);
}

static void
test_wait_y_persists_commits_until_ready(void)
{
    /* ADD M1M2, D=0, Xar=0, Yar=1 with Y WAIT on first issue. */
    dual_modes_fixture fixture = {
        .program_word = UINT16_C(0xA049),
        .x_address = UINT16_C(0x1200),
        .y_address = UINT16_C(0x1300),
        .x_value = UINT16_C(1),
        .y_value = UINT16_C(2),
        .wait_y = true};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, &status);
    ti_c54x_run_result result;

    set_status(cpu);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[2] == fixture.x_address);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[3] == fixture.y_address);
    fixture.wait_y = false;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 == UINT64_C(0x00030000));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[2] == UINT16_C(0x11ff));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[3] == UINT16_C(0x1301));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_ready_rows_publish_post_access_updates();
    test_same_ar_commits_once_with_xmod_precedence();
    test_wait_y_persists_commits_until_ready();
    return 0;
}
