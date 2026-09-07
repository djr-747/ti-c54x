/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT_ONLY proof that the generic data-memory sequencer
 * resolves a SINGLE_INDIRECT_WORD16 MOD2 operand from copied architectural
 * state and returns value-owned bus/address commit candidates.
 * Does not own: opcode identity or handlers, memory visibility, endpoint
 * callbacks, timing, CPU lifecycle, diagnostics, or public ABI.
 */

#include "isa/ops/memory/memory_effect.h"
#include "sequencer/address_effect_apply.h"
#include "sequencer/memory/address_resolution.h"
#include "address/indirect_single.h"
#include "state/arch_state.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_arch_state
mode_state(void)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_memory_address_spec
mod2_spec(uint8_t arf)
{
    const ti_c54x_op_memory_address_spec spec = {
        .kind = TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16,
        .mode = TI_C54X_INDIRECT_SINGLE_MOD2,
        .arf = arf
    };

    return spec;
}

static void
check_resolution_is_zero(const ti_c54x_memory_address_resolution *resolution)
{
    TI_C54X_TEST_CHECK(resolution->request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID);
    TI_C54X_TEST_CHECK(resolution->request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID);
    TI_C54X_TEST_CHECK(resolution->request.address_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(resolution->request.write_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(resolution->address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(resolution->address_result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_INVALID);
    TI_C54X_TEST_CHECK(resolution->address_result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_NONE);
    TI_C54X_TEST_CHECK(resolution->address_result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
    TI_C54X_TEST_CHECK(resolution->address_commit.ar_index == UINT8_C(0));
    TI_C54X_TEST_CHECK(resolution->address_commit.value_after == UINT16_C(0));
}

static void
check_address_result_equal(const ti_c54x_address_result *actual,
                           const ti_c54x_address_result *expected)
{
    TI_C54X_TEST_CHECK(actual->status == expected->status);
    TI_C54X_TEST_CHECK(actual->effect.operand.width ==
                       expected->effect.operand.width);
    TI_C54X_TEST_CHECK(actual->effect.operand.first_word ==
                       expected->effect.operand.first_word);
    TI_C54X_TEST_CHECK(actual->effect.operand.second_word ==
                       expected->effect.operand.second_word);
    TI_C54X_TEST_CHECK(actual->effect.source.kind ==
                       expected->effect.source.kind);
    TI_C54X_TEST_CHECK(actual->effect.source.value.ar.index ==
                       expected->effect.source.value.ar.index);
    TI_C54X_TEST_CHECK(actual->effect.source.value.ar.value_before ==
                       expected->effect.source.value.ar.value_before);
    TI_C54X_TEST_CHECK(actual->effect.state_policy ==
                       expected->effect.state_policy);
    TI_C54X_TEST_CHECK(actual->effect.update.target ==
                       expected->effect.update.target);
    TI_C54X_TEST_CHECK(actual->effect.update.phase ==
                       expected->effect.update.phase);
    TI_C54X_TEST_CHECK(actual->effect.update.ar_index ==
                       expected->effect.update.ar_index);
    TI_C54X_TEST_CHECK(actual->effect.update.value_before ==
                       expected->effect.update.value_before);
    TI_C54X_TEST_CHECK(actual->effect.update.value_after ==
                       expected->effect.update.value_after);
}

static void
test_mod2_uses_old_ar_and_stages_postincrement(void)
{
    ti_c54x_arch_state state = mode_state();
    ti_c54x_memory_address_resolution resolution = {0};
    const uint8_t arf = UINT8_C(3);
    const uint16_t old_ar = UINT16_C(0x2310);
    const ti_c54x_address_result expected_address =
        ti_c54x_indirect_single_word(arf, old_ar,
                                     TI_C54X_INDIRECT_SINGLE_MOD2,
                                     TI_C54X_ADDRESS_ACCESS_READ);
    const ti_c54x_op_memory_address_spec spec =
        mod2_spec(arf);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, arf, old_ar) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_address_result_equal(&resolution.address_result, &expected_address);
    TI_C54X_TEST_CHECK(resolution.request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(resolution.request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK_U64(resolution.request.address_word, old_ar);
    TI_C54X_TEST_CHECK_U64(resolution.request.write_value_word, UINT16_C(0));
    TI_C54X_TEST_CHECK(resolution.address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.ar_index, arf);
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.value_after,
                           (uint16_t)(old_ar + UINT16_C(1)));
    TI_C54X_TEST_CHECK_U64(resolution.address_result.effect.operand.first_word,
                           old_ar);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.update.value_after,
        (uint16_t)(old_ar + UINT16_C(1)));
    /* Resolution is a pure plan: the live AR remains the old effective word. */
    {
        uint16_t observed = UINT16_C(0);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&state, arf, &observed) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(observed, old_ar);
    }
}

static void
test_mod2_write_maps_store_word_without_state_mutation(void)
{
    ti_c54x_arch_state state = mode_state();
    ti_c54x_memory_address_resolution resolution = {0};
    const uint8_t arf = UINT8_C(6);
    const uint16_t old_ar = UINT16_C(0x0042);
    const ti_c54x_op_memory_address_spec spec =
        mod2_spec(arf);
    ti_c54x_arch_state before;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, arf, old_ar) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                           UINT16_C(0x00a5), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(resolution.request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(resolution.request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(resolution.request.address_word, old_ar);
    TI_C54X_TEST_CHECK_U64(resolution.request.write_value_word,
                           UINT16_C(0x00a5));
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
test_dependency_order_rejects_mode_and_source_without_repair(void)
{
    const uint8_t arf = UINT8_C(2);
    const ti_c54x_op_memory_address_spec spec =
        mod2_spec(arf);
    ti_c54x_arch_state state = {0};
    ti_c54x_arch_state before;
    ti_c54x_memory_address_resolution resolution;

    /* CMPT is checked before ARP and selected AR. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, arf,
                                                   UINT16_C(0x2000)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) !=
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_resolution_is_zero(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    /* A known nonzero ARP is rejected without consulting or changing AR. */
    state = mode_state();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, arf,
                                                   UINT16_C(0x2000)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) !=
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_resolution_is_zero(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    /* CMPT=0/ARP=0 still rejects an unknown selected AR. */
    state = mode_state();
    before = state;
    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) !=
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_resolution_is_zero(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
test_invalid_spec_and_boundary_are_rejected_before_candidates(void)
{
    ti_c54x_arch_state state = mode_state();
    ti_c54x_memory_address_resolution resolution;
    ti_c54x_op_memory_address_spec spec;
    ti_c54x_arch_state before;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), UINT16_C(0x2400)) ==
                       TI_C54X_ARCH_STATE_OK);

    spec = mod2_spec(UINT8_C(0));
    spec.kind = TI_C54X_OP_MEMORY_ADDRESS_INVALID;
    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) !=
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_resolution_is_zero(&resolution);

    spec = mod2_spec(UINT8_C(8));
    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) !=
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_resolution_is_zero(&resolution);

    spec = mod2_spec(UINT8_C(0));
    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           (ti_c54x_bus_word16_request_access)UINT8_C(0xff),
                           UINT16_C(0), &resolution) !=
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_resolution_is_zero(&resolution);

    /* The checked MOD2 planner rejects postincrement past the common word. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), UINT16_MAX) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    spec = mod2_spec(UINT8_C(0));
    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) !=
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_resolution_is_zero(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

int
main(void)
{
    test_mod2_uses_old_ar_and_stages_postincrement();
    test_mod2_write_maps_store_word_without_state_mutation();
    test_dependency_order_rejects_mode_and_source_without_repair();
    test_invalid_spec_and_boundary_are_rejected_before_candidates();
    return 0;
}
