/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proofs for generic indexed WORD16
 * resolution. It proves ordered CMPT/ARP/ARx/AR0 reads, old-address MOD5/6
 * requests, and rejection without a commit candidate. Does not own: opcode
 * identity, handlers, memory visibility, endpoint callbacks, timing, CPU
 * lifecycle, diagnostics, or public ABI.
 */

#include "isa/ops/memory/memory_effect.h"
#include "sequencer/memory/address_resolution.h"
#include "address/indirect_indexed.h"
#include "state/arch_state.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_arch_state
indexed_state(uint8_t arf, uint16_t ar0, uint16_t ar_value)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, UINT8_C(0), ar0) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, arf, ar_value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_memory_address_spec
indexed_spec(ti_c54x_indirect_indexed_mode mode, uint8_t arf)
{
    const ti_c54x_op_memory_address_spec spec = {
        .kind = TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16,
        .mode = TI_C54X_INDIRECT_SINGLE_MOD0,
        .indexed_mode = mode,
        .arf = arf};
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
}

static void
check_mod5_old_address_and_post_subtraction(void)
{
    const uint8_t arf = UINT8_C(3);
    const uint16_t ar0 = UINT16_C(0x0010);
    const uint16_t old_ar = UINT16_C(0x2310);
    const ti_c54x_op_memory_address_spec spec = indexed_spec(
        TI_C54X_INDIRECT_INDEXED_MOD5, arf);
    ti_c54x_arch_state state = indexed_state(arf, ar0, old_ar);
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
    TI_C54X_TEST_CHECK_U64(resolution.request.address_word, old_ar);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR_INDEXED);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.source.value.ar_indexed.ar0_value_before,
        ar0);
    TI_C54X_TEST_CHECK(resolution.address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.ar_index, arf);
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.value_after,
                           (uint16_t)(old_ar - ar0));
}

static void
check_mod6_old_address_and_post_addition(void)
{
    const uint8_t arf = UINT8_C(5);
    const uint16_t ar0 = UINT16_C(0x0010);
    const uint16_t old_ar = UINT16_C(0x2310);
    const ti_c54x_op_memory_address_spec spec = indexed_spec(
        TI_C54X_INDIRECT_INDEXED_MOD6, arf);
    ti_c54x_arch_state state = indexed_state(arf, ar0, old_ar);
    ti_c54x_memory_address_resolution resolution = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                           UINT16_C(0xCAFE), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK_U64(resolution.request.address_word, old_ar);
    TI_C54X_TEST_CHECK_U64(resolution.request.write_value_word,
                           UINT16_C(0xCAFE));
    TI_C54X_TEST_CHECK(resolution.address_result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.value_after,
                           (uint16_t)(old_ar + ar0));
}

static void
check_rejections_are_candidate_free(void)
{
    const uint8_t arf = UINT8_C(3);
    const ti_c54x_op_memory_address_spec valid = indexed_spec(
        TI_C54X_INDIRECT_INDEXED_MOD5, arf);
    ti_c54x_memory_address_resolution resolution;
    ti_c54x_arch_state state;
    ti_c54x_arch_state before;

    /* Selected AR0 aliases are rejected before a bus or commit candidate. */
    state = indexed_state(UINT8_C(0), UINT16_C(0x0010), UINT16_C(0x2310));
    before = state;
    resolution = (ti_c54x_memory_address_resolution){0};
    {
        ti_c54x_op_memory_address_spec alias = valid;
        alias.arf = UINT8_C(0);
        TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                               &state, &alias,
                               TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                               UINT16_C(0), &resolution) !=
                           TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    }
    check_resolution_is_zero(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    /* Unknown CMPT/ARP and selected registers are all rejected without repair. */
    state = (ti_c54x_arch_state){0};
    before = state;
    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &valid,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) !=
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_resolution_is_zero(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    /* MOD5 underflow and MOD6 overflow must not leak planner candidates. */
    state = indexed_state(arf, UINT16_C(0x0011), UINT16_C(0x0010));
    before = state;
    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &valid,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) !=
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_resolution_is_zero(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    {
        ti_c54x_op_memory_address_spec overflow = indexed_spec(
            TI_C54X_INDIRECT_INDEXED_MOD6, arf);
        state = indexed_state(arf, UINT16_C(0x0010), UINT16_C(0xFFF8));
        before = state;
        resolution = (ti_c54x_memory_address_resolution){0};
        TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                               &state, &overflow,
                               TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                               UINT16_C(0xCAFE), &resolution) !=
                           TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
        check_resolution_is_zero(&resolution);
        TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
    }
}

int
main(void)
{
    check_mod5_old_address_and_post_subtraction();
    check_mod6_old_address_and_post_addition();
    check_rejections_are_candidate_free();
    return 0;
}
