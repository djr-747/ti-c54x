/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for generic circular
 * WORD16 address resolution. It proves CMPT/ARP/AR/BK dependency ordering,
 * planner delegation, copied source snapshots, BK=0 no-update semantics, and
 * DATA request construction. Does not own: opcode identity, handlers, CPU
 * lifecycle, endpoint callbacks, timing, or architectural mutation.
 */

#include "isa/ops/memory/memory_effect.h"
#include "sequencer/memory/address_resolution.h"
#include "state/block_size_register.h"
#include "state/arch_state.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_arch_state
circular_state(uint8_t arf, uint16_t ar_value, uint16_t bk_value)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, arf, ar_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, bk_value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_memory_address_spec
circular_spec(uint8_t arf, ti_c54x_indirect_circular_mode mode)
{
    const ti_c54x_op_memory_address_spec spec = {
        .kind = TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16,
        .circular_mode = mode,
        .arf = arf};

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
}

static void
check_resolved_expected(uint8_t arf,
                        uint16_t old_ar,
                        uint16_t bk_value,
                        ti_c54x_indirect_circular_mode mode,
                        ti_c54x_bus_word16_request_access access,
                        uint16_t expected_after)
{
    ti_c54x_arch_state state = circular_state(arf, old_ar, bk_value);
    const ti_c54x_arch_state before = state;
    const ti_c54x_op_memory_address_spec spec = circular_spec(arf, mode);
    const uint16_t write_value =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE
            ? UINT16_C(0xCAFE)
            : UINT16_C(0);
    ti_c54x_memory_address_resolution resolution = {0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec, access, write_value, &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(resolution.request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(resolution.request.access == access);
    TI_C54X_TEST_CHECK_U64(resolution.request.address_word, old_ar);
    TI_C54X_TEST_CHECK_U64(resolution.request.write_value_word, write_value);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.source.value.ar_circular.index, arf);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.source.value.ar_circular.modification,
        (uint8_t)mode);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.source.value.ar_circular.value_before,
        old_ar);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.source.value.ar_circular.bk_value_before,
        bk_value);
    if (bk_value == UINT16_C(0)) {
        TI_C54X_TEST_CHECK(resolution.address_result.effect.update.target ==
                           TI_C54X_ADDRESS_UPDATE_NONE);
        TI_C54X_TEST_CHECK(resolution.address_result.effect.update.phase ==
                           TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
        TI_C54X_TEST_CHECK(resolution.address_commit.target ==
                           TI_C54X_ADDRESS_UPDATE_NONE);
    } else {
        TI_C54X_TEST_CHECK(resolution.address_result.effect.update.target ==
                           TI_C54X_ADDRESS_UPDATE_AR);
        TI_C54X_TEST_CHECK(resolution.address_result.effect.update.phase ==
                           TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
        TI_C54X_TEST_CHECK_U64(resolution.address_result.effect.update.ar_index,
                               arf);
        TI_C54X_TEST_CHECK_U64(
            resolution.address_result.effect.update.value_after, expected_after);
        TI_C54X_TEST_CHECK(resolution.address_commit.target ==
                           TI_C54X_ADDRESS_UPDATE_AR);
        TI_C54X_TEST_CHECK_U64(resolution.address_commit.ar_index, arf);
        TI_C54X_TEST_CHECK_U64(resolution.address_commit.value_after,
                               expected_after);
    }
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
check_resolved(uint8_t arf,
               uint16_t old_ar,
               uint16_t bk_value,
               ti_c54x_indirect_circular_mode mode,
               ti_c54x_bus_word16_request_access access)
{
    const uint16_t expected_after =
        mode == TI_C54X_INDIRECT_CIRCULAR_MOD8
            ? UINT16_C(0x1244)
            : UINT16_C(0x1241);

    check_resolved_expected(arf, old_ar, bk_value, mode, access,
                            expected_after);
}

static void
test_full_mode_selector_access_matrix(void)
{
    static const ti_c54x_indirect_circular_mode modes[] = {
        TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_INDIRECT_CIRCULAR_MOD10};
    static const ti_c54x_bus_word16_request_access accesses[] = {
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE};
    size_t mode_index;
    size_t access_index;
    uint8_t arf;

    for (mode_index = 0U; mode_index < 2U; ++mode_index) {
        for (access_index = 0U; access_index < 2U; ++access_index) {
            for (arf = UINT8_C(0); arf <= UINT8_C(7); ++arf) {
                check_resolved(arf, UINT16_C(0x1240), UINT16_C(5),
                               modes[mode_index], accesses[access_index]);
                check_resolved(arf, UINT16_C(0x1240), UINT16_C(0),
                               modes[mode_index], accesses[access_index]);
            }
        }
    }
}

static void
test_wraps_and_domain_boundaries(void)
{
    check_resolved_expected(UINT8_C(1), UINT16_C(0x1240), UINT16_C(5),
                            TI_C54X_INDIRECT_CIRCULAR_MOD8,
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                            UINT16_C(0x1244));
    check_resolved_expected(UINT8_C(1), UINT16_C(0x1244), UINT16_C(5),
                            TI_C54X_INDIRECT_CIRCULAR_MOD10,
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                            UINT16_C(0x1240));
}

static void
test_boundaries_and_invalid_inputs_reject_without_candidates(void)
{
    ti_c54x_arch_state state = circular_state(UINT8_C(3), UINT16_C(0x1245),
                                               UINT16_C(5));
    const ti_c54x_arch_state before = state;
    ti_c54x_memory_address_resolution resolution;
    ti_c54x_op_memory_address_spec spec = circular_spec(
        UINT8_C(3), TI_C54X_INDIRECT_CIRCULAR_MOD8);

    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED);
    check_zero_resolution(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    spec.kind = TI_C54X_OP_MEMORY_ADDRESS_INVALID;
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);

    spec = circular_spec(UINT8_C(8), TI_C54X_INDIRECT_CIRCULAR_MOD8);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);

    spec = circular_spec(UINT8_C(3),
                         (ti_c54x_indirect_circular_mode)UINT8_C(9));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);

    spec = circular_spec(UINT8_C(3), TI_C54X_INDIRECT_CIRCULAR_MOD8);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(1), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS);
    check_zero_resolution(&resolution);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           (ti_c54x_bus_word16_request_access)UINT8_C(9),
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS);
    check_zero_resolution(&resolution);
}

static void
test_dependency_order_and_unknown_state(void)
{
    const uint8_t arf = UINT8_C(2);
    const ti_c54x_op_memory_address_spec spec = circular_spec(
        arf, TI_C54X_INDIRECT_CIRCULAR_MOD10);
    ti_c54x_arch_state state = circular_state(arf, UINT16_C(0x1240),
                                               UINT16_C(5));
    ti_c54x_memory_address_resolution resolution;

    state.status.st1_known_mask = (uint16_t)(state.status.st1_known_mask &
                                             (uint16_t)~TI_C54X_ST1_CMPT_MASK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE);
    check_zero_resolution(&resolution);

    state = circular_state(arf, UINT16_C(0x1240), UINT16_C(5));
    state.status.st0_known_mask = (uint16_t)(state.status.st0_known_mask &
                                             (uint16_t)~TI_C54X_ST0_ARP_MASK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE);
    check_zero_resolution(&resolution);

    state = circular_state(arf, UINT16_C(0x1240), UINT16_C(5));
    state.ar_known_mask = (uint8_t)(state.ar_known_mask &
                                    (uint8_t)~(UINT8_C(1) << arf));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE);
    check_zero_resolution(&resolution);

    state = circular_state(arf, UINT16_C(0x1240), UINT16_C(5));
    state.scalar_known_mask = (uint8_t)(state.scalar_known_mask &
                                        (uint8_t)~TI_C54X_ARCH_STATE_KNOWN_BK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE);
    check_zero_resolution(&resolution);
}

int
main(void)
{
    test_full_mode_selector_access_matrix();
    test_wraps_and_domain_boundaries();
    test_boundaries_and_invalid_inputs_reject_without_candidates();
    test_dependency_order_and_unknown_state();
    return 0;
}
