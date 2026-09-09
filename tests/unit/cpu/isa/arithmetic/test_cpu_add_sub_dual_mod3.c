/* Module contract
 * Owns: public-CPU retirement proofs for the fourteen MOD3-involving
 * syntax-6 dual-memory ADD/SUB rows, including per-row AR post-access
 * publication through AR0/BK snapshots, same-AR Xmod precedence, and
 * destination routing. Shared fixed-shift arithmetic is owned by the MOD0
 * proofs and probed here only by value spots; WAIT persistence is owned by
 * the W51C contract.
 * Does not own: opcode semantics, decode tables, bus/reducer implementation,
 * CPU lifecycle, timing, observers, or production state mutation.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/block_size_register.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct dual_mod3_fixture {
    uint16_t program_word;
    uint16_t x_address;
    uint16_t y_address;
    uint16_t x_value;
    uint16_t y_value;
    uint16_t ar0_value;
    uint16_t bk_value;
    unsigned endpoint_calls;
    ti_c54x_bus_word16_request requests[4];
} dual_mod3_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    dual_mod3_fixture *fixture = (dual_mod3_fixture *)context;

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
    dual_mod3_fixture *fixture = (dual_mod3_fixture *)context;
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
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = is_x ? fixture->x_value : fixture->y_value,
        .bus_status = TI_C54X_BUS_OK};
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
init_cpu(dual_mod3_fixture *fixture, void *storage, ti_c54x_error *status_out)
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
    /* MOD3 sides require known AR0/BK snapshots in CPU state. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state, 0U,
                                                   fixture->ar0_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&cpu->arch_state,
                                                   fixture->bk_value) ==
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
        (cpu->arch_state.status.st1 &
        (uint16_t)~(TI_C54X_ST1_SXM_MASK | TI_C54X_ST1_OVM_MASK)));
    cpu->arch_state.status.st0_known_mask = (uint16_t)(
        (cpu->arch_state.status.st0_known_mask | TI_C54X_ST0_C_MASK));
    cpu->arch_state.status.st1_known_mask = (uint16_t)(
        (cpu->arch_state.status.st1_known_mask |
        (TI_C54X_ST1_SXM_MASK | TI_C54X_ST1_OVM_MASK)));
}

static void
run_ready_case(uint16_t opcode, uint16_t x_address, uint16_t y_address,
               uint16_t ar0_value, uint16_t bk_value, uint16_t expected_ar2,
               uint16_t expected_ar3, bool same_ar)
{
    dual_mod3_fixture fixture = {
        .program_word = opcode,
        .x_address = x_address,
        .y_address = y_address,
        .x_value = UINT16_C(1),
        .y_value = UINT16_C(2),
        .ar0_value = ar0_value,
        .bk_value = bk_value};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    const bool is_sub = (opcode & UINT16_C(0x0200)) != UINT16_C(0);

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
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[2] == expected_ar2);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[3] == expected_ar3);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_indexed_rows_publish_ar0_steps(void)
{
    /* BK=0 with AR0=2: every MOD3 side publishes address+2 while MOD0-2
     * sides keep their fixed updates. D=0, Xar=0 (AR2), Yar=1 (AR3). */
    static const struct {
        uint16_t base;
        uint16_t expected_ar2;
        uint16_t expected_ar3;
    } rows[] = {
        {UINT16_C(0xA00C), UINT16_C(0x1200), UINT16_C(0x1302)},
        {UINT16_C(0xA04C), UINT16_C(0x11FF), UINT16_C(0x1302)},
        {UINT16_C(0xA08C), UINT16_C(0x1201), UINT16_C(0x1302)},
        {UINT16_C(0xA0C0), UINT16_C(0x1202), UINT16_C(0x1300)},
        {UINT16_C(0xA0C4), UINT16_C(0x1202), UINT16_C(0x12FF)},
        {UINT16_C(0xA0C8), UINT16_C(0x1202), UINT16_C(0x1301)},
        {UINT16_C(0xA0CC), UINT16_C(0x1202), UINT16_C(0x1302)},
    };
    size_t index;

    for (index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        const uint16_t add_word =
            (uint16_t)(rows[index].base | UINT16_C(0x0001));
        const uint16_t sub_word =
            (uint16_t)(rows[index].base | UINT16_C(0x0201));

        run_ready_case(add_word, UINT16_C(0x1200), UINT16_C(0x1300),
                       UINT16_C(0x0002), UINT16_C(0), rows[index].expected_ar2,
                       rows[index].expected_ar3, false);
        run_ready_case(sub_word, UINT16_C(0x1200), UINT16_C(0x1300),
                       UINT16_C(0x0002), UINT16_C(0), rows[index].expected_ar2,
                       rows[index].expected_ar3, false);
    }
}

static void
test_circular_side_wraps_around_buffer(void)
{
    /* ADD M3_M0, BK=8, AR0=2: X 0x1207 wraps index 7 to 1 (0x1201) while
     * Y MOD0 holds 0x1300. */
    run_ready_case(UINT16_C(0xA0C1), UINT16_C(0x1207), UINT16_C(0x1300),
                   UINT16_C(0x0002), UINT16_C(8), UINT16_C(0x1201),
                   UINT16_C(0x1300), false);
}

static void
test_same_ar_commits_once_with_xmod_precedence(void)
{
    /* ADD M3_M3 with Xar=Yar=0: one X-owned AR0 step of AR2. */
    run_ready_case(UINT16_C(0xA0CC), UINT16_C(0x1200), UINT16_C(0x1200),
                   UINT16_C(0x0002), UINT16_C(0), UINT16_C(0x1202),
                   UINT16_C(0x1200), true);
}

int
main(void)
{
    test_indexed_rows_publish_ar0_steps();
    test_circular_side_wraps_around_buffer();
    test_same_ar_commits_once_with_xmod_precedence();
    return 0;
}
