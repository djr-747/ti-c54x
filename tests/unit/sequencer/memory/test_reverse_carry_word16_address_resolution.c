/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for generic reverse-carry
 * WORD16 address resolution. It proves exact MOD4/MOD7 planner delegation,
 * ordered CMPT/ARP/ARx/AR0 dependencies, candidate-free rejection, and a
 * selected-AR POST_ACCESS commit without BK or AR0 mutation. Does not own:
 * opcode identity, handlers, memory visibility, endpoint callbacks, timing,
 * CPU lifecycle, diagnostics, or public ABI.
 */

#include "isa/ops/memory/memory_effect.h"
#include "sequencer/memory/address_resolution.h"
#include "address/indirect_reverse_carry.h"
#include "state/arch_state.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_arch_state
reverse_state(uint8_t arf, uint16_t ar_value, uint16_t ar0_value)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, arf, ar_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, UINT8_C(0),
                                                   ar0_value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_memory_address_spec
reverse_spec(uint8_t arf, ti_c54x_indirect_reverse_carry_mode mode)
{
    const ti_c54x_op_memory_address_spec spec = {
        .kind = TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16,
        .reverse_carry_mode = mode,
        .arf = arf};

    return spec;
}

static void
check_zero_resolution(const ti_c54x_memory_address_resolution *resolution)
{
    const ti_c54x_memory_address_resolution zero = {0};

    TI_C54X_TEST_CHECK(memcmp(resolution, &zero, sizeof(*resolution)) == 0);
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
    TI_C54X_TEST_CHECK(resolution->address_result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_NONE);
    TI_C54X_TEST_CHECK(resolution->address_result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
}

static ti_c54x_memory_address_resolution
sentinel_resolution(void)
{
    ti_c54x_memory_address_resolution resolution;

    (void)memset(&resolution, 0xA5, sizeof(resolution));
    return resolution;
}

static void
check_resolved(uint8_t arf,
               uint16_t old_ar,
               uint16_t ar0_value,
               ti_c54x_indirect_reverse_carry_mode mode,
               ti_c54x_bus_word16_request_access access,
               uint16_t expected_after)
{
    ti_c54x_arch_state state = reverse_state(arf, old_ar, ar0_value);
    const ti_c54x_arch_state before = state;
    const ti_c54x_op_memory_address_spec spec = reverse_spec(arf, mode);
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
                       TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.source.value.ar_reverse_carry.index,
        arf);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.source.value.ar_reverse_carry.modification,
        (uint8_t)mode);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.source.value.ar_reverse_carry.value_before,
        old_ar);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.source.value.ar_reverse_carry.ar0_value_before,
        ar0_value);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
    TI_C54X_TEST_CHECK_U64(resolution.address_result.effect.update.ar_index,
                           arf);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.update.value_before, old_ar);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.update.value_after, expected_after);
    TI_C54X_TEST_CHECK(resolution.address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.ar_index, arf);
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.value_after,
                           expected_after);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.operand.first_word, old_ar);
    TI_C54X_TEST_CHECK(ti_c54x_indirect_reverse_carry_effect_is_valid(
        &resolution.address_result.effect));
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
test_modes_accesses_and_selected_registers(void)
{
    static const ti_c54x_indirect_reverse_carry_mode modes[] = {
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7};
    static const ti_c54x_bus_word16_request_access accesses[] = {
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE};
    static const uint8_t arfs[] = {UINT8_C(1), UINT8_C(7)};
    static const uint16_t old_ar[] = {UINT16_C(0x0068), UINT16_C(0x0060)};
    static const uint16_t expected_after[] = {UINT16_C(0x0060),
                                              UINT16_C(0x0068)};
    size_t mode_index;
    size_t access_index;
    size_t arf_index;

    for (mode_index = 0U; mode_index < 2U; ++mode_index) {
        for (access_index = 0U; access_index < 2U; ++access_index) {
            for (arf_index = 0U; arf_index < 2U; ++arf_index) {
                check_resolved(arfs[arf_index], old_ar[mode_index],
                               UINT16_C(0x0008), modes[mode_index],
                               accesses[access_index],
                               expected_after[mode_index]);
            }
        }
    }
}

static void
test_dependency_order_and_unknownness(void)
{
    const uint8_t arf = UINT8_C(3);
    const ti_c54x_op_memory_address_spec spec = reverse_spec(
        arf, TI_C54X_INDIRECT_REVERSE_CARRY_MOD7);
    ti_c54x_arch_state state;
    ti_c54x_arch_state before;
    ti_c54x_memory_address_resolution resolution;

    /* CMPT is the first dependency and dominates all later unknowns. */
    state = (ti_c54x_arch_state){0};
    before = state;
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE);
    check_zero_resolution(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    /* ARP is read only after a known CMPT=0. */
    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE);
    check_zero_resolution(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    /* Selected AR is read before AR0. */
    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, UINT8_C(0),
                                                   UINT16_C(0x0008)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE);
    check_zero_resolution(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    /* AR0 is the last source dependency and must be known. */
    state = reverse_state(arf, UINT16_C(0x0060), UINT16_C(0x0008));
    state.ar_known_mask = (uint8_t)(state.ar_known_mask &
                                    (uint8_t)~UINT8_C(1));
    before = state;
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE);
    check_zero_resolution(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    state = reverse_state(arf, UINT16_C(0x0060), UINT16_C(0x0008));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE);
    check_zero_resolution(&resolution);

    state = reverse_state(arf, UINT16_C(0x0060), UINT16_C(0x0008));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE);
    check_zero_resolution(&resolution);
}

static void
test_prevalidation_and_ar0_domain_rejection(void)
{
    const ti_c54x_op_memory_address_spec valid = reverse_spec(
        UINT8_C(3), TI_C54X_INDIRECT_REVERSE_CARRY_MOD4);
    ti_c54x_op_memory_address_spec malformed;
    ti_c54x_arch_state state = reverse_state(UINT8_C(3), UINT16_C(0x0068),
                                              UINT16_C(0x0008));
    ti_c54x_memory_address_resolution resolution;

    /* Unrelated fields and discriminants are prevalidated before any reads. */
    malformed = valid;
    malformed.mode = TI_C54X_INDIRECT_SINGLE_MOD1;
    resolution = sentinel_resolution();
    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &malformed,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);

    malformed = valid;
    malformed.indexed_mode = TI_C54X_INDIRECT_INDEXED_MOD5;
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &malformed,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);

    malformed = valid;
    malformed.circular_mode = TI_C54X_INDIRECT_CIRCULAR_MOD8;
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &malformed,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);

    malformed = valid;
    malformed.reverse_carry_mode =
        (ti_c54x_indirect_reverse_carry_mode)UINT8_C(5);
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &malformed,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);

    malformed = valid;
    malformed.arf = UINT8_C(0);
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &malformed,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);

    malformed = valid;
    malformed.arf = UINT8_C(8);
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &malformed,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);

    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &valid,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(1), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS);
    check_zero_resolution(&resolution);

    malformed = valid;
    malformed.arf = UINT8_C(3);
    state = reverse_state(UINT8_C(3), UINT16_C(0x0068), UINT16_C(0));
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &malformed,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED);
    check_zero_resolution(&resolution);

    state = reverse_state(UINT8_C(3), UINT16_C(0x0068), UINT16_C(3));
    resolution = sentinel_resolution();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &valid,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                           UINT16_C(0xCAFE), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED);
    check_zero_resolution(&resolution);
}

static void
test_boundary_propagation_rejects_candidate(void)
{
    const struct {
        ti_c54x_indirect_reverse_carry_mode mode;
        uint16_t old_ar;
        uint16_t ar0;
        ti_c54x_bus_word16_request_access access;
    } cases[] = {
        {TI_C54X_INDIRECT_REVERSE_CARRY_MOD4, UINT16_C(0), UINT16_C(1),
         TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ},
        {TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, UINT16_MAX, UINT16_C(1),
         TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE}};
    size_t index;

    for (index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        ti_c54x_arch_state state = reverse_state(UINT8_C(7),
                                                  cases[index].old_ar,
                                                  cases[index].ar0);
        const ti_c54x_arch_state before = state;
        const ti_c54x_op_memory_address_spec spec = reverse_spec(
            UINT8_C(7), cases[index].mode);
        ti_c54x_memory_address_resolution resolution = sentinel_resolution();
        const uint16_t write_value =
            cases[index].access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE
                ? UINT16_C(0xCAFE)
                : UINT16_C(0);

        TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                               &state, &spec, cases[index].access, write_value,
                               &resolution) ==
                           TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED);
        check_zero_resolution(&resolution);
        TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    }
}

int
main(void)
{
    test_modes_accesses_and_selected_registers();
    test_dependency_order_and_unknownness();
    test_prevalidation_and_ar0_domain_rejection();
    test_boundary_propagation_rejects_candidate();
    return 0;
}
