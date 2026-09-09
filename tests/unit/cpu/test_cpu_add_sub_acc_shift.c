/* Module contract
 * Owns: CPU end-to-end retirement proofs for ADD_ACC_SHIFT and SUB_ACC_SHIFT,
 * including raw40 guard-bit shifting, destination routing, C/no-borrow,
 * OVM saturation, sticky destination overflow, PC, and retirement counters.
 * Does not own: CPU implementation, decode tables, pure handler arithmetic,
 * or status/effect mutation internals.
 */

#include <ti_c54x/cpu.h>

#include "../../../src/cpu/cpu_internal.h"
#include "../../../src/state/register_layout.h"
#include "../../../src/state/status/registers.h"
#include "../../../src/isa/decode/opcode_decode.h"
#include "../../../src/isa/ops/arithmetic/op_arithmetic.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct fixture {
    uint16_t word;
    ti_c54x_program_word_address expected_address;
    ti_c54x_bus_status fault_status;
} fixture;

static ti_c54x_bus_status read_word(void *context,
                                    ti_c54x_program_word_address address,
                                    uint16_t *value)
{
    fixture *f = (fixture *)context;
    TI_C54X_TEST_CHECK(address == f->expected_address);
    if (f->fault_status != TI_C54X_BUS_OK) {
        return f->fault_status;
    }
    *value = f->word;
    return TI_C54X_BUS_OK;
}

static uint16_t encode(uint16_t base, uint8_t source, uint8_t destination,
                       uint8_t shift)
{
    return (uint16_t)(base | ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U) | shift);
}

static void init_status(ti_c54x_cpu *cpu, uint8_t sxm, uint8_t ovm,
                        uint8_t ova, uint8_t ovb)
{
    cpu->arch_state.status.st0 = 0U;
    cpu->arch_state.status.st1 = 0U;
    cpu->arch_state.status.st0_known_mask = UINT16_MAX;
    cpu->arch_state.status.st1_known_mask = UINT16_MAX;
    if (ova != 0U) {
        cpu->arch_state.status.st0 |= TI_C54X_ST0_OVA_MASK;
    }
    if (ovb != 0U) {
        cpu->arch_state.status.st0 |= TI_C54X_ST0_OVB_MASK;
    }
    if (ovm != 0U) {
        cpu->arch_state.status.st1 |= TI_C54X_ST1_OVM_MASK;
    }
    if (sxm != 0U) {
        cpu->arch_state.status.st1 |= TI_C54X_ST1_SXM_MASK;
    }
}

static void run_guard_shift(void)
{
    fixture f = {encode(UINT16_C(0xF400), 0U, 1U, 0x1fU), 0U,
                 TI_C54X_BUS_OK};
    ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
    const ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x0000000100000000)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B, 0U) ==
                       TI_C54X_ACCUMULATOR_OK);
    init_status(cpu, 0U, 0U, 0U, 1U);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK_U64(state.accumulator_a_raw40,
                           UINT64_C(0x0000000100000000));
    TI_C54X_TEST_CHECK_U64(state.accumulator_b_raw40,
                           UINT64_C(0x0000000080000000));
    TI_C54X_TEST_CHECK(state.c == 0U);
    TI_C54X_TEST_CHECK(state.pc_word == 1U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 1U);
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & TI_C54X_ST0_OVA_MASK) ==
                       0U);
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & TI_C54X_ST0_OVB_MASK) !=
                       0U);
    ti_c54x_cpu_deinit(cpu);
}

static void run_saturating_subtract(void)
{
    fixture f = {encode(UINT16_C(0xF420), 1U, 0U, 0U), 0U,
                 TI_C54X_BUS_OK};
    ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
    const ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    ti_c54x_status_register_state before;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x000000ff80000000)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000000000000001)) ==
                       TI_C54X_ACCUMULATOR_OK);
    init_status(cpu, 0U, 1U, 0U, 1U);
    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 == UINT64_C(1));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 & TI_C54X_ST1_OVM_MASK) !=
                       0U);
    {
        const ti_c54x_decode_result first = ti_c54x_decode_first_word(f.word);
        const ti_c54x_instruction_word_bundle bundle = {
            .program_words = {f.word, 0U}, .program_word_count = 1U};
        ti_c54x_decoded_instruction instruction;
        ti_c54x_op_input_snapshot input = {
            .accumulator_a_raw40 = UINT64_C(0x000000ff80000000),
            .accumulator_b_raw40 = UINT64_C(1),
            .accumulator_a_known = true,
            .accumulator_b_known = true,
            .status_sxm = 0U,
            .status_sxm_known = true,
            .status_ovm = 1U,
            .status_ovm_known = true};
        ti_c54x_op_effect effect = {0};
        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &first, &bundle, &instruction) ==
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_op_sub_acc_shift(&instruction, &input,
                                                    &effect) ==
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK_U64(effect.payload.accumulator_write.value_raw40,
                               UINT64_C(0xff80000000));
    }
    before = cpu->arch_state.status;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK_U64(state.accumulator_a_raw40,
                           UINT64_C(0x00ff80000000));
    TI_C54X_TEST_CHECK_U64(state.accumulator_b_raw40,
                           UINT64_C(1));
    TI_C54X_TEST_CHECK(state.c == 1U);
    TI_C54X_TEST_CHECK(state.pc_word == 1U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 1U);
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & TI_C54X_ST0_OVA_MASK) !=
                       0U);
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & TI_C54X_ST0_OVB_MASK) !=
                       0U);
    TI_C54X_TEST_CHECK((uint16_t)(cpu->arch_state.status.st0 &
                                 (uint16_t)~(TI_C54X_ST0_OVA_MASK |
                                             TI_C54X_ST0_C_MASK)) ==
                       (uint16_t)(before.st0 &
                                  (uint16_t)~(TI_C54X_ST0_OVA_MASK |
                                              TI_C54X_ST0_C_MASK)));
    ti_c54x_cpu_deinit(cpu);
}

static void run_positive_clamp(void)
{
    fixture f = {encode(UINT16_C(0xF400), 1U, 0U, 0U), 0U,
                 TI_C54X_BUS_OK};
    ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
    const ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x7fffffff)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B, 1U) ==
                       TI_C54X_ACCUMULATOR_OK);
    init_status(cpu, 0U, 1U, 1U, 0U);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK_U64(state.accumulator_a_raw40,
                           UINT64_C(0x7fffffff));
    TI_C54X_TEST_CHECK_U64(state.accumulator_b_raw40, UINT64_C(1));
    TI_C54X_TEST_CHECK(state.c == 0U);
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & TI_C54X_ST0_OVA_MASK) !=
                       0U);
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & TI_C54X_ST0_OVB_MASK) ==
                       0U);
    ti_c54x_cpu_deinit(cpu);
}

static void run_rejected_input(uint8_t reject_kind)
{
    fixture f = {encode(UINT16_C(0xF400), 0U, 1U, 0U), 0U,
                 TI_C54X_BUS_OK};
    ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
    const ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_status_register_state before_status;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x123456789a)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000000001)) == TI_C54X_ACCUMULATOR_OK);
    init_status(cpu, 0U, 0U, 1U, 1U);
    if (reject_kind == 0U) {
        cpu->arch_state.status.st1_known_mask = (uint16_t)(
            cpu->arch_state.status.st1_known_mask &
            (uint16_t)~TI_C54X_ST1_SXM_MASK);
    } else if (reject_kind == 1U) {
        cpu->arch_state.status.st1_known_mask = (uint16_t)(
            cpu->arch_state.status.st1_known_mask &
            (uint16_t)~TI_C54X_ST1_OVM_MASK);
    } else {
        cpu->accumulators.known_mask = (uint8_t)(
            cpu->accumulators.known_mask & (uint8_t)~UINT8_C(1));
    }
    before_accumulators = cpu->accumulators;
    before_status = cpu->arch_state.status;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == 0U);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_program_fault(void)
{
    fixture f = {encode(UINT16_C(0xF400), 0U, 1U, 0U), 0U,
                 TI_C54X_BUS_FAULT};
    ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
    const ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_status_register_state before_status;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    init_status(cpu, 0U, 0U, 1U, 1U);
    before_accumulators = cpu->accumulators;
    before_status = cpu->arch_state.status;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == 0U);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_pc_boundary(void)
{
    fixture f = {encode(UINT16_C(0xF400), 0U, 1U, 0U), UINT32_C(0xFFFF),
                 TI_C54X_BUS_OK};
    ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
    const ti_c54x_config config = {UINT32_C(0xFFFF), {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_status_register_state before_status;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A, 3U) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B, 1U) ==
                       TI_C54X_ACCUMULATOR_OK);
    init_status(cpu, 0U, 0U, 1U, 1U);
    before_accumulators = cpu->accumulators;
    before_status = cpu->arch_state.status;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason ==
                       TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0xFFFF));
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

int main(void)
{
    run_guard_shift();
    run_saturating_subtract();
    run_positive_clamp();
    run_rejected_input(0U);
    run_rejected_input(1U);
    run_rejected_input(2U);
    run_program_fault();
    run_pc_boundary();
    return 0;
}
