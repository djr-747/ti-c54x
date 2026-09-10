/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for stack-pop WORD16
 * address resolution. It proves mode independence (no CMPT/ARP/CPL read),
 * planner delegation with copied SP snapshots, POST_ACCESS SP commit
 * staging, READ-only access, DATA request construction, and transactional
 * output discipline. Does not own: opcode identity, handlers, CPU
 * lifecycle, endpoint callbacks, timing, or architectural mutation.
 */

#include "isa/ops/memory/memory_effect.h"
#include "sequencer/memory/address_resolution.h"
#include "state/arch_state.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_arch_state
pop_state(uint16_t sp_value)
{
    ti_c54x_arch_state state = {0};

    /* Only SP is known: CMPT/ARP/CPL and every AR stay unknown, so any
     * mode read would reject and success proves mode independence. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&state, sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_memory_address_spec
pop_spec(void)
{
    const ti_c54x_op_memory_address_spec spec = {
        .kind = TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16};
    return spec;
}

static void
check_zero_resolution(const ti_c54x_memory_address_resolution *resolution)
{
    TI_C54X_TEST_CHECK(resolution->request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID);
    TI_C54X_TEST_CHECK(resolution->request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID);
    TI_C54X_TEST_CHECK_U64(resolution->request.address_word, UINT32_C(0));
    TI_C54X_TEST_CHECK_U64(resolution->request.write_value_word, UINT16_C(0));
    TI_C54X_TEST_CHECK(resolution->address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(resolution->address_result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_INVALID);
    TI_C54X_TEST_CHECK_U64(resolution->ar0_shadow, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(resolution->bk_shadow, UINT16_C(0));
}

static void
check_resolved_expected(uint16_t sp_before)
{
    ti_c54x_arch_state state = pop_state(sp_before);
    const ti_c54x_arch_state before = state;
    const ti_c54x_op_memory_address_spec spec = pop_spec();
    const uint16_t expected_after = (uint16_t)(sp_before + UINT16_C(1));
    ti_c54x_memory_address_resolution resolution = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(resolution.request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(resolution.request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    /* The TOS slot is the pre-update register. */
    TI_C54X_TEST_CHECK_U64(resolution.request.address_word, sp_before);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_STACK_PUSH);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.source.value.stack_push.sp_before,
        sp_before);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.operand.first_word, sp_before);
    TI_C54X_TEST_CHECK(resolution.address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_SP);
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.value_after,
                           expected_after);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
test_resolve_vectors(void)
{
    check_resolved_expected(UINT16_C(0x0300));
    check_resolved_expected(UINT16_C(0x0000));
    check_resolved_expected(UINT16_C(0xfffe));
    check_resolved_expected(UINT16_C(0xffff));
}

static void
test_write_rejected(void)
{
    ti_c54x_arch_state state = pop_state(UINT16_C(0x0300));
    const ti_c54x_op_memory_address_spec spec = pop_spec();
    ti_c54x_memory_address_resolution resolution = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS);
    check_zero_resolution(&resolution);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                           UINT16_C(0x0001), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS);
    check_zero_resolution(&resolution);
}

static void
test_unknown_sp_rejected(void)
{
    ti_c54x_arch_state state = {0};
    const ti_c54x_op_memory_address_spec spec = pop_spec();
    ti_c54x_memory_address_resolution resolution = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE);
    check_zero_resolution(&resolution);
}

static void
test_malformed_spec_rejected(void)
{
    ti_c54x_arch_state state = pop_state(UINT16_C(0x0300));
    ti_c54x_op_memory_address_spec spec = pop_spec();
    ti_c54x_memory_address_resolution resolution = {0};

    spec.arf = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);
    spec = pop_spec();
    spec.mode = TI_C54X_INDIRECT_SINGLE_MOD1;
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);
    spec = pop_spec();
    spec.reverse_carry_mode = TI_C54X_INDIRECT_REVERSE_CARRY_MOD4;
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);
}

int
main(void)
{
    test_resolve_vectors();
    test_write_rejected();
    test_unknown_sp_rejected();
    test_malformed_spec_rejected();
    puts("stack_pop_resolution: ok");
    return 0;
}
