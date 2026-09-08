/* Module contract
 * Owns: public-CPU retirement proofs for syntax-6 MOD0/MOD0 ADD and SUB,
 * including ordered X/Y DATA reads, resumable WAIT, atomic FAULT handling,
 * destination routing, SXM/OVM, carry/no-borrow, sticky OVA/OVB, PC, and AR
 * preservation.
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

typedef enum dual_mode {
    DUAL_READY = 0,
    DUAL_WAIT_X = 1,
    DUAL_WAIT_Y = 2,
    DUAL_FAULT_X = 3,
    DUAL_FAULT_Y = 4
} dual_mode;

typedef struct dual_fixture {
    dual_mode mode;
    uint16_t program_word;
    uint16_t x_address;
    uint16_t y_address;
    uint16_t x_value;
    uint16_t y_value;
    ti_c54x_bus_status fault_status;
    unsigned program_reads;
    unsigned endpoint_calls;
    unsigned begin_calls;
    unsigned resume_calls;
    unsigned ready_calls;
    uint64_t ids[4];
    ti_c54x_bus_word16_request requests[4];
} dual_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    dual_fixture *fixture = (dual_fixture *)context;

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
    dual_fixture *fixture = (dual_fixture *)context;
    const unsigned index = fixture->endpoint_calls++;
    bool is_x;
    bool wait;
    bool fault;

    TI_C54X_TEST_CHECK(index < sizeof(fixture->ids) / sizeof(fixture->ids[0]));
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(request->write_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN ||
                       phase == TI_C54X_BUS_WORD16_ADVANCE_RESUME);
    fixture->ids[index] = transaction_id;
    fixture->requests[index] = *request;
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        fixture->begin_calls++;
    } else {
        fixture->resume_calls++;
    }

    is_x = request->address_word == fixture->x_address;
    TI_C54X_TEST_CHECK(is_x || request->address_word == fixture->y_address);
    wait = (fixture->mode == DUAL_WAIT_X && index == 0U) ||
           (fixture->mode == DUAL_WAIT_Y && index == 1U);
    fault = fixture->mode == DUAL_FAULT_X ? is_x
                                          : (fixture->mode == DUAL_FAULT_Y &&
                                             !is_x);
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = wait ? TI_C54X_BUS_WORD16_OBSERVATION_WAIT
                     : (fault ? TI_C54X_BUS_WORD16_OBSERVATION_FAULT
                              : TI_C54X_BUS_WORD16_OBSERVATION_READY),
        .read_value_word = is_x ? fixture->x_value : fixture->y_value,
        .bus_status = fault ? fixture->fault_status : TI_C54X_BUS_OK};
    if (observation->kind == TI_C54X_BUS_WORD16_OBSERVATION_READY) {
        fixture->ready_calls++;
    } else {
        observation->read_value_word = UINT16_C(0);
    }
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

static ti_c54x_bus_word16_endpoint
make_endpoint(dual_fixture *fixture)
{
    return (ti_c54x_bus_word16_endpoint){
        .context = fixture, .advance = advance_data, .cancel = cancel_data};
}

static ti_c54x_cpu *
init_cpu(dual_fixture *fixture, void *storage, ti_c54x_error *status_out)
{
    const ti_c54x_config config = {UINT32_C(0), {0U, NULL, NULL}};
    const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(fixture);
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
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x0000001234)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000005678)) == TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static void
set_status(ti_c54x_cpu *cpu, uint8_t sxm, uint8_t ovm, uint8_t c,
           uint8_t ova, uint8_t ovb)
{
    cpu->arch_state.status.st0 = (uint16_t)(
        (cpu->arch_state.status.st0 & (uint16_t)~(TI_C54X_ST0_C_MASK |
                                                  TI_C54X_ST0_OVA_MASK |
                                                  TI_C54X_ST0_OVB_MASK)) |
        (c != 0U ? TI_C54X_ST0_C_MASK : 0U) |
        (ova != 0U ? TI_C54X_ST0_OVA_MASK : 0U) |
        (ovb != 0U ? TI_C54X_ST0_OVB_MASK : 0U));
    cpu->arch_state.status.st1 = (uint16_t)(
        (cpu->arch_state.status.st1 &
         (uint16_t)~(TI_C54X_ST1_SXM_MASK | TI_C54X_ST1_OVM_MASK)) |
        (sxm != 0U ? TI_C54X_ST1_SXM_MASK : 0U) |
        (ovm != 0U ? TI_C54X_ST1_OVM_MASK : 0U));
    cpu->arch_state.status.st0_known_mask = (uint16_t)(
        cpu->arch_state.status.st0_known_mask |
        (TI_C54X_ST0_C_MASK | TI_C54X_ST0_OVA_MASK | TI_C54X_ST0_OVB_MASK));
    cpu->arch_state.status.st1_known_mask = (uint16_t)(
        cpu->arch_state.status.st1_known_mask |
        (TI_C54X_ST1_SXM_MASK | TI_C54X_ST1_OVM_MASK));
}

static void
check_requests(const dual_fixture *fixture)
{
    TI_C54X_TEST_CHECK(fixture->requests[0].address_word ==
                       fixture->x_address);
    TI_C54X_TEST_CHECK(fixture->requests[1].address_word ==
                       fixture->y_address);
    TI_C54X_TEST_CHECK(fixture->requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[1].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture->requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture->ids[0] != UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture->ids[1] == fixture->ids[0] + UINT64_C(1));
}

static void
run_ready_case(uint16_t opcode, uint16_t x_value, uint16_t y_value,
               uint8_t sxm, uint8_t ovm, uint8_t c, uint8_t ova, uint8_t ovb,
               uint64_t expected, uint8_t expected_c, uint8_t expected_ova,
               uint8_t expected_ovb, uint8_t destination)
{
    dual_fixture fixture = {
        .mode = DUAL_READY,
        .program_word = opcode,
        .x_address = UINT16_C(0x1200),
        .y_address = UINT16_C(0x1300),
        .x_value = x_value,
        .y_value = y_value,
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, &status);
    const uint64_t other_before = destination == 0U
                                      ? cpu->accumulators.b_raw40
                                      : cpu->accumulators.a_raw40;
    ti_c54x_run_result result;

    set_status(cpu, sxm, ovm, c, ova, ovb);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == opcode);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 2U);
    check_requests(&fixture);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[2] == fixture.x_address);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[3] == fixture.y_address);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK((destination == 0U ? cpu->accumulators.a_raw40
                                          : cpu->accumulators.b_raw40) ==
                       expected);
    TI_C54X_TEST_CHECK((destination == 0U ? cpu->accumulators.b_raw40
                                          : cpu->accumulators.a_raw40) ==
                       other_before);
    TI_C54X_TEST_CHECK(((cpu->arch_state.status.st0 & TI_C54X_ST0_C_MASK) !=
                        0U) == (expected_c != 0U));
    TI_C54X_TEST_CHECK(((cpu->arch_state.status.st0 & TI_C54X_ST0_OVA_MASK) !=
                        0U) == (expected_ova != 0U));
    TI_C54X_TEST_CHECK(((cpu->arch_state.status.st0 & TI_C54X_ST0_OVB_MASK) !=
                        0U) == (expected_ovb != 0U));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_ready_forms_cover_routes_and_status(void)
{
    /* Ordinary ADD A/B, including both destination routes and C preserve. */
    run_ready_case(UINT16_C(0xA001), UINT16_C(1), UINT16_C(2), 0U, 0U, 0U,
                   1U, 0U, UINT64_C(0x00030000), 0U, 1U, 0U, 0U);
    run_ready_case(UINT16_C(0xA101), UINT16_C(0xffff), UINT16_C(1), 1U, 0U,
                   1U, 0U, 1U, UINT64_C(0x0000000000), 1U, 0U, 1U, 1U);
    /* ADD bit-32 carry is observable without a destination overflow. */
    run_ready_case(UINT16_C(0xA001), UINT16_C(0xffff), UINT16_C(0xffff), 1U,
                   0U, 0U, 0U, 1U, UINT64_C(0xfffffe0000), 1U, 0U, 1U,
                   0U);
    /* OVM clamps positive signed-32 overflow and sets only destination OVA. */
    run_ready_case(UINT16_C(0xA001), UINT16_C(0x7fff), UINT16_C(0x7fff), 0U,
                   1U, 0U, 0U, 1U, UINT64_C(0x007fffffff), 0U, 1U, 1U,
                   0U);
    /* SUB A/B, including no-borrow preservation and bit-32 borrow clear. */
    run_ready_case(UINT16_C(0xA201), UINT16_C(5), UINT16_C(2), 0U, 0U, 1U,
                   1U, 0U, UINT64_C(0x00030000), 1U, 1U, 0U, 0U);
    run_ready_case(UINT16_C(0xA301), UINT16_C(0), UINT16_C(1), 0U, 0U, 1U,
                   0U, 1U, UINT64_C(0xffffff0000), 0U, 0U, 1U, 1U);
    /* SXM sign extension plus OVM positive clamp and destination OVB. */
    run_ready_case(UINT16_C(0xA301), UINT16_C(0), UINT16_C(0x8000), 1U, 1U,
                   1U, 1U, 0U, UINT64_C(0x007fffffff), 0U, 1U, 1U, 1U);
}

static void
test_wait_resumes_same_request_and_retires_once(void)
{
    dual_fixture fixture = {
        .mode = DUAL_WAIT_X,
        .program_word = UINT16_C(0xA001),
        .x_address = UINT16_C(0x1200),
        .y_address = UINT16_C(0x1300),
        .x_value = UINT16_C(1),
        .y_value = UINT16_C(2),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[2] == fixture.x_address);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[3] == fixture.y_address);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 3U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[1] == fixture.ids[0]);
    TI_C54X_TEST_CHECK(fixture.ids[2] == fixture.ids[0] + UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == fixture.x_address);
    TI_C54X_TEST_CHECK(fixture.requests[2].address_word == fixture.y_address);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_wait_on_y_resumes_same_request_and_retires_once(void)
{
    dual_fixture fixture = {
        .mode = DUAL_WAIT_Y,
        .program_word = UINT16_C(0xA101),
        .x_address = UINT16_C(0x1200),
        .y_address = UINT16_C(0x1300),
        .x_value = UINT16_C(1),
        .y_value = UINT16_C(2),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.ids[1] == fixture.ids[0] + UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == fixture.x_address);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == fixture.y_address);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 3U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[2] == fixture.ids[1]);
    TI_C54X_TEST_CHECK(fixture.requests[2].address_word == fixture.y_address);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 == UINT64_C(0x00030000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_fault_is_atomic_for_both_sides(void)
{
    const dual_mode modes[] = {DUAL_FAULT_X, DUAL_FAULT_Y};

    for (size_t index = 0U; index < sizeof(modes) / sizeof(modes[0]); ++index) {
        dual_fixture fixture = {
            .mode = modes[index],
            .program_word = UINT16_C(0xA101),
            .x_address = UINT16_C(0x1200),
            .y_address = UINT16_C(0x1300),
            .x_value = UINT16_C(1),
            .y_value = UINT16_C(2),
            .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage, &status);
        const uint64_t before_a = cpu->accumulators.a_raw40;
        const uint64_t before_b = cpu->accumulators.b_raw40;
        const ti_c54x_status_register_state before_status =
            cpu->arch_state.status;
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[2] == fixture.x_address);
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[3] == fixture.y_address);
        TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 == before_a);
        TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 == before_b);
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == before_status.st0);
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == before_status.st1);
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st0_known_mask ==
                           before_status.st0_known_mask);
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st1_known_mask ==
                           before_status.st1_known_mask);
        TI_C54X_TEST_CHECK(fixture.ready_calls ==
                           (modes[index] == DUAL_FAULT_Y ? 1U : 0U));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

int
main(void)
{
    test_ready_forms_cover_routes_and_status();
    test_wait_resumes_same_request_and_retires_once();
    test_wait_on_y_resumes_same_request_and_retires_once();
    test_fault_is_atomic_for_both_sides();
    return 0;
}
