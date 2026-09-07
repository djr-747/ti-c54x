/*
 * Module contract
 *
 * Owns: the project-contract proof that each CPU embeds one canonical
 * ti_c54x_arch_state, seeds common reset state before applying the configured
 * 32-bit reset carrier, and routes public PC/status observation and reset
 * through that state.  It also proves that a high-carrier NOP is fetched but
 * rejected transactionally by the existing common-PC bridge, and that two
 * CPU instances do not share architectural state.
 * Does not own: CPU implementation, reset policy, bridge implementation,
 * opcode decode, bus memory, timing, observers, diagnostics, or ABI layout.
 */

#include <stdint.h>
#include <stdlib.h>

#include <ti_c54x/cpu.h>

#include "../../../../src/cpu/cpu_internal.h"
#include "../../../../src/state/arch_state.h"
#include "../../../support/test_harness.h"

typedef struct nop_bus_fixture {
    unsigned reads;
    ti_c54x_program_word_address addresses[2];
    uint16_t opcode_word;
} nop_bus_fixture;

static ti_c54x_bus_status
read_nop(void *context,
         ti_c54x_program_word_address address_word,
         uint16_t *value_word)
{
    nop_bus_fixture *fixture = (nop_bus_fixture *)context;

    TI_C54X_TEST_CHECK(fixture->reads < 2U);
    TI_C54X_TEST_CHECK(value_word != NULL);
    fixture->addresses[fixture->reads] = address_word;
    fixture->reads++;
    *value_word = fixture->opcode_word;
    return TI_C54X_BUS_OK;
}

static ti_c54x_bus make_nop_bus(nop_bus_fixture *fixture)
{
    ti_c54x_bus bus = {0};

    bus.context = fixture;
    bus.program_read_word = read_nop;
    return bus;
}

static ti_c54x_config make_config(ti_c54x_program_word_address reset_pc_word)
{
    ti_c54x_config config = {reset_pc_word, {0U, NULL, NULL}};

    return config;
}

static void check_status_equal(const ti_c54x_status_register_state *actual,
                               const ti_c54x_status_register_state *expected)
{
    TI_C54X_TEST_CHECK(actual->st0 == expected->st0);
    TI_C54X_TEST_CHECK(actual->st1 == expected->st1);
    TI_C54X_TEST_CHECK(actual->pmst == expected->pmst);
    TI_C54X_TEST_CHECK(actual->st0_known_mask == expected->st0_known_mask);
    TI_C54X_TEST_CHECK(actual->st1_known_mask == expected->st1_known_mask);
    TI_C54X_TEST_CHECK(actual->pmst_known_mask == expected->pmst_known_mask);
}

static void check_arch_state_equal(const ti_c54x_arch_state *actual,
                                   const ti_c54x_arch_state *expected)
{
    size_t index;

    TI_C54X_TEST_CHECK(actual->pc_word == expected->pc_word);
    TI_C54X_TEST_CHECK(actual->ifr == expected->ifr);
    TI_C54X_TEST_CHECK(actual->rc == expected->rc);
    TI_C54X_TEST_CHECK(actual->bk == expected->bk);
    TI_C54X_TEST_CHECK(actual->sp == expected->sp);
    for (index = 0U; index < 8U; ++index) {
        TI_C54X_TEST_CHECK(actual->ar[index] == expected->ar[index]);
    }
    check_status_equal(&actual->status, &expected->status);
    TI_C54X_TEST_CHECK(actual->scalar_known_mask == expected->scalar_known_mask);
    TI_C54X_TEST_CHECK(actual->ar_known_mask == expected->ar_known_mask);
}

static void test_common_seed_then_configured_high_carrier(void)
{
    nop_bus_fixture fixture = {0U, {0U, 0U}, UINT16_C(0x1234)};
    ti_c54x_bus bus = make_nop_bus(&fixture);
    const ti_c54x_config config = make_config(UINT32_C(0x10042));
    ti_c54x_arch_state common_seed;
    ti_c54x_state observed;
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&common_seed) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);

    /* The private composition seam is the canonical owner; no second CPU PC
     * or status value is consulted by public inspection. */
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x10042));
    check_status_equal(&cpu->arch_state.status, &common_seed.status);
    TI_C54X_TEST_CHECK(cpu->arch_state.ifr == common_seed.ifr);
    TI_C54X_TEST_CHECK(cpu->arch_state.rc == common_seed.rc);
    TI_C54X_TEST_CHECK(cpu->arch_state.scalar_known_mask ==
                       common_seed.scalar_known_mask);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar_known_mask ==
                       common_seed.ar_known_mask);

    /* Mutating the canonical state must be visible through the public API. */
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_dp(
                           &cpu->arch_state.status, UINT16_C(0x1AB)) ==
                       TI_C54X_STATUS_REGISTER_OK);
    cpu->arch_state.pc_word = UINT32_C(0x10055);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &observed) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(observed.pc_word == UINT32_C(0x10055));
    TI_C54X_TEST_CHECK(observed.dp_known);
    TI_C54X_TEST_CHECK(observed.dp == UINT16_C(0x1AB));

    /* Reset re-seeds the common state, then reapplies the configured carrier. */
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x10042));
    check_status_equal(&cpu->arch_state.status, &common_seed.status);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &observed) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(observed.pc_word == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(observed.dp_known);
    TI_C54X_TEST_CHECK(observed.dp == UINT16_C(0));

    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void test_high_nop_fetch_result_and_reset_are_transactional(void)
{
    nop_bus_fixture fixture = {0U, {0U, 0U}, UINT16_C(0xF495)};
    ti_c54x_bus bus = make_nop_bus(&fixture);
    const ti_c54x_config config = make_config(UINT32_C(0x10042));
    ti_c54x_state observed;
    ti_c54x_state after_reset;
    ti_c54x_arch_state before_arch;
    ti_c54x_run_result result;
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    before_arch = cpu->arch_state;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == UINT16_C(0xF495));
    TI_C54X_TEST_CHECK(fixture.reads == 1U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &observed) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(observed.pc_word == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(observed.instructions_attempted_total == 1U);
    TI_C54X_TEST_CHECK(observed.instructions_retired_total == 0U);
    check_arch_state_equal(&cpu->arch_state, &before_arch);

    /* Reset retains the full carrier and the next public fetch uses it again. */
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &after_reset) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(after_reset.pc_word == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(!after_reset.last_fetch_valid);
    TI_C54X_TEST_CHECK(after_reset.instructions_attempted_total == 0U);
    TI_C54X_TEST_CHECK(after_reset.instructions_retired_total == 0U);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(fixture.reads == 2U);
    TI_C54X_TEST_CHECK(fixture.addresses[1] == UINT32_C(0x10042));

    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void test_instances_have_isolated_canonical_state(void)
{
    nop_bus_fixture first_fixture = {0U, {0U, 0U}, UINT16_C(0xF495)};
    nop_bus_fixture second_fixture = {0U, {0U, 0U}, UINT16_C(0xF495)};
    ti_c54x_bus first_bus = make_nop_bus(&first_fixture);
    ti_c54x_bus second_bus = make_nop_bus(&second_fixture);
    const ti_c54x_config first_config = make_config(UINT32_C(0x10042));
    const ti_c54x_config second_config = make_config(UINT32_C(0x0022));
    ti_c54x_state first_state;
    ti_c54x_state second_state;
    ti_c54x_run_result result;
    void *first_storage = malloc(ti_c54x_cpu_size());
    void *second_storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *first = NULL;
    ti_c54x_cpu *second = NULL;

    TI_C54X_TEST_CHECK(first_storage != NULL);
    TI_C54X_TEST_CHECK(second_storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(
                           first_storage, ti_c54x_cpu_size(), &first_config,
                           &first_bus, &first) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(
                           second_storage, ti_c54x_cpu_size(), &second_config,
                           &second_bus, &second) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(&first->arch_state != &second->arch_state);

    result = ti_c54x_cpu_step(first);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS);
    result = ti_c54x_cpu_step(second);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x0023));
    TI_C54X_TEST_CHECK(first_fixture.addresses[0] == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(second_fixture.addresses[0] == UINT32_C(0x0022));

    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(first, &first_state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(second, &second_state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(first_state.pc_word == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(second_state.pc_word == UINT32_C(0x0023));
    TI_C54X_TEST_CHECK(first_state.instructions_retired_total == 0U);
    TI_C54X_TEST_CHECK(second_state.instructions_retired_total == 1U);

    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(first) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(second, &second_state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(second_state.pc_word == UINT32_C(0x0023));
    TI_C54X_TEST_CHECK(second_state.instructions_retired_total == 1U);

    ti_c54x_cpu_deinit(first);
    ti_c54x_cpu_deinit(second);
    free(first_storage);
    free(second_storage);
}

int main(void)
{
    test_common_seed_then_configured_high_carrier();
    test_high_nop_fetch_result_and_reset_are_transactional();
    test_instances_have_isolated_canonical_state();
    return 0;
}
