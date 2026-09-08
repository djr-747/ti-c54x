/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for generic data-memory
 * address resolution across CMPT=0 single-indirect MOD0 through MOD3.
 * Does not own: opcode identity, memory visibility, instruction sequencing,
 * architectural mutation, timing, diagnostics, or test discovery manifests.
 */

#include "sequencer/memory/address_resolution.h"
#include "address/indirect_single.h"
#include "state/arch_state.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_arch_state
mode_state(uint8_t arf, uint16_t ar_value)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, arf, ar_value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_memory_address_spec
single_spec(uint8_t arf, ti_c54x_indirect_single_mode mode)
{
    const ti_c54x_op_memory_address_spec spec = {
        .kind = TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16,
        .mode = mode,
        .arf = arf
    };

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
    TI_C54X_TEST_CHECK(resolution->address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(resolution->address_result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_INVALID);
}

static void
check_resolved(uint8_t arf,
               uint16_t old_ar,
               ti_c54x_indirect_single_mode mode,
               ti_c54x_bus_word16_request_access access,
               uint16_t expected_address,
               ti_c54x_address_update_target expected_target,
               ti_c54x_address_update_phase expected_phase,
               uint16_t expected_after)
{
    ti_c54x_arch_state state = mode_state(arf, old_ar);
    ti_c54x_memory_address_resolution resolution = {0};
    const ti_c54x_op_memory_address_spec spec = single_spec(arf, mode);
    const uint16_t write_value =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE
            ? UINT16_C(0x00a5)
            : UINT16_C(0);
    const ti_c54x_memory_address_resolution_status status =
        ti_c54x_sequencer_resolve_memory_address(
            &state, &spec, access, write_value, &resolution);

    TI_C54X_TEST_CHECK(status == TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(resolution.request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(resolution.request.access == access);
    TI_C54X_TEST_CHECK_U64(resolution.request.address_word, expected_address);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.source.value.ar.value_before,
        old_ar);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.update.target ==
                       expected_target);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.update.phase ==
                       expected_phase);
    TI_C54X_TEST_CHECK_U64(resolution.address_result.effect.update.value_after,
                           expected_after);
    TI_C54X_TEST_CHECK(resolution.address_commit.target == expected_target);
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.ar_index,
                           expected_target == TI_C54X_ADDRESS_UPDATE_AR
                               ? arf
                               : UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.value_after,
                           expected_after);

    /* Resolution remains a plan and never mutates live AR state. */
    {
        uint16_t observed = UINT16_C(0);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, arf, &observed) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(observed, old_ar);
    }
}

static void
test_mod0_read_and_write_use_old_address_without_update(void)
{
    check_resolved(UINT8_C(2), UINT16_C(0x2310),
                   TI_C54X_INDIRECT_SINGLE_MOD0,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                   UINT16_C(0x2310), TI_C54X_ADDRESS_UPDATE_NONE,
                   TI_C54X_ADDRESS_UPDATE_PHASE_NONE, UINT16_C(0));
    check_resolved(UINT8_C(2), UINT16_C(0x2310),
                   TI_C54X_INDIRECT_SINGLE_MOD0,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT16_C(0x2310), TI_C54X_ADDRESS_UPDATE_NONE,
                   TI_C54X_ADDRESS_UPDATE_PHASE_NONE, UINT16_C(0));
}

static void
test_mod1_and_mod2_read_and_write_use_old_address_with_post_update(void)
{
    const ti_c54x_indirect_single_mode modes[] = {
        TI_C54X_INDIRECT_SINGLE_MOD1, TI_C54X_INDIRECT_SINGLE_MOD2};
    const uint16_t old_ar = UINT16_C(0x2310);
    size_t index;

    for (index = 0; index < sizeof(modes) / sizeof(modes[0]); ++index) {
        const ti_c54x_indirect_single_mode mode = modes[index];
        const uint16_t expected_after =
            mode == TI_C54X_INDIRECT_SINGLE_MOD1
                ? (uint16_t)(old_ar - UINT16_C(1))
                : (uint16_t)(old_ar + UINT16_C(1));

        check_resolved(UINT8_C(3), old_ar, mode,
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, old_ar,
                       TI_C54X_ADDRESS_UPDATE_AR,
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                       expected_after);
        check_resolved(UINT8_C(3), old_ar, mode,
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, old_ar,
                       TI_C54X_ADDRESS_UPDATE_AR,
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                       expected_after);
    }
}

static void
test_mod3_write_uses_incremented_address_with_pre_update(void)
{
    const uint16_t old_ar = UINT16_C(0x2310);

    check_resolved(UINT8_C(4), old_ar, TI_C54X_INDIRECT_SINGLE_MOD3,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   (uint16_t)(old_ar + UINT16_C(1)),
                   TI_C54X_ADDRESS_UPDATE_AR,
                   TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                   (uint16_t)(old_ar + UINT16_C(1)));
}

static void
test_mod3_read_and_boundaries_reject_without_candidates(void)
{
    const uint8_t arf = UINT8_C(1);
    const struct {
        ti_c54x_indirect_single_mode mode;
        ti_c54x_bus_word16_request_access access;
        uint16_t ar_value;
    } cases[] = {
        {TI_C54X_INDIRECT_SINGLE_MOD3,
         TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT16_C(0x2310)},
        {TI_C54X_INDIRECT_SINGLE_MOD1,
         TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT16_C(0)},
        {TI_C54X_INDIRECT_SINGLE_MOD2,
         TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT16_MAX},
        {TI_C54X_INDIRECT_SINGLE_MOD3,
         TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT16_MAX}};
    size_t index;

    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        ti_c54x_arch_state state = mode_state(arf, cases[index].ar_value);
        ti_c54x_arch_state before = state;
        ti_c54x_memory_address_resolution resolution = {0};
        const ti_c54x_op_memory_address_spec spec =
            single_spec(arf, cases[index].mode);

        TI_C54X_TEST_CHECK(
            ti_c54x_sequencer_resolve_memory_address(
                &state, &spec, cases[index].access, UINT16_C(0),
                &resolution) ==
            TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED);
        check_zero_resolution(&resolution);
        TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    }
}

int
main(void)
{
    test_mod0_read_and_write_use_old_address_without_update();
    test_mod1_and_mod2_read_and_write_use_old_address_with_post_update();
    test_mod3_write_uses_incremented_address_with_pre_update();
    test_mod3_read_and_boundaries_reject_without_candidates();
    (void)puts("memory_address_modes: ok");
    return 0;
}
