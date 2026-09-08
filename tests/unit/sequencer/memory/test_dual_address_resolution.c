/*
 * Module contract
 *
 * Owns: executable W50A coverage for the dual-indirect MOD0/MOD0 WORD16
 * address-resolution carrier. It proves frozen-state dependency ordering,
 * independent X/Y DATA READ requests, same-AR aliasing, and transactional
 * rejection of malformed or unknown inputs.
 * Does not own: opcode identity or semantics, bus callbacks, memory
 * visibility, instruction sequencing, timing, architectural mutation, or
 * test discovery manifests.
 */

#include "sequencer/memory/dual_address_resolution.h"
#include "state/arch_state.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_arch_state
known_mode_state(void)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_memory_address_spec
dual_spec(uint8_t x_ar, uint8_t y_ar)
{
    const ti_c54x_op_memory_address_spec spec = {
        .kind = TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16,
        .dual = {
            .x_mode = UINT8_C(0),
            .x_ar = x_ar,
            .y_mode = UINT8_C(0),
            .y_ar = y_ar,
        },
    };

    return spec;
}

static void
check_zero_resolution(const ti_c54x_dual_memory_address_resolution *resolution)
{
    const ti_c54x_dual_memory_address_resolution zero = {0};

    TI_C54X_TEST_CHECK(memcmp(resolution, &zero, sizeof(zero)) == 0);
}

static void
check_word16_read(const ti_c54x_address_result *address_result,
                  const ti_c54x_bus_word16_request *request,
                  const ti_c54x_sequencer_address_commit *commit,
                  uint8_t ar_index,
                  uint16_t ar_value)
{
    TI_C54X_TEST_CHECK(address_result->status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(address_result->effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK_U64(address_result->effect.operand.first_word,
                           ar_value);
    TI_C54X_TEST_CHECK_U64(address_result->effect.operand.second_word,
                           UINT16_C(0));
    TI_C54X_TEST_CHECK(address_result->effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR);
    TI_C54X_TEST_CHECK_U64(address_result->effect.source.value.ar.index,
                           ar_index);
    TI_C54X_TEST_CHECK_U64(
        address_result->effect.source.value.ar.value_before, ar_value);
    TI_C54X_TEST_CHECK(address_result->effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_NONE);
    TI_C54X_TEST_CHECK(address_result->effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(address_result->effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
    TI_C54X_TEST_CHECK(commit->target == TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK_U64(commit->ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(commit->value_after, UINT16_C(0));
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK_U64(request->address_word, ar_value);
    TI_C54X_TEST_CHECK_U64(request->write_value_word, UINT16_C(0));
}

static void
test_mod0_pair_resolves_two_independent_reads(void)
{
    ti_c54x_arch_state state = known_mode_state();
    ti_c54x_dual_memory_address_resolution resolution = {0};
    const ti_c54x_op_memory_address_spec spec = dual_spec(2, 5);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x1234)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(5), UINT16_C(0xabcd)) ==
                       TI_C54X_ARCH_STATE_OK);

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_word16_read(&resolution.x_address_result, &resolution.x_request,
                      &resolution.x_address_commit, UINT8_C(2),
                      UINT16_C(0x1234));
    check_word16_read(&resolution.y_address_result, &resolution.y_request,
                      &resolution.y_address_commit, UINT8_C(5),
                      UINT16_C(0xabcd));
}

static void
test_same_ar_alias_uses_one_frozen_value(void)
{
    ti_c54x_arch_state state = known_mode_state();
    ti_c54x_dual_memory_address_resolution resolution = {0};
    const ti_c54x_op_memory_address_spec spec = dual_spec(3, 3);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x4567)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_word16_read(&resolution.x_address_result, &resolution.x_request,
                      &resolution.x_address_commit, UINT8_C(3),
                      UINT16_C(0x4567));
    check_word16_read(&resolution.y_address_result, &resolution.y_request,
                      &resolution.y_address_commit, UINT8_C(3),
                      UINT16_C(0x4567));
}

static void
test_cmpt_arp_are_not_dependencies_for_dual_mod0(void)
{
    ti_c54x_arch_state state = {0};
    ti_c54x_dual_memory_address_resolution resolution = {0};
    const ti_c54x_op_memory_address_spec spec = dual_spec(2, 5);

    /* Deliberately leave CMPT/ARP unknown: MOD0/MOD0 has no such claim. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x1234)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(5), UINT16_C(0xabcd)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);

    /* A nonzero mode state is equally irrelevant to this carrier. */
    state = known_mode_state();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(7)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x1234)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(5), UINT16_C(0xabcd)) ==
                       TI_C54X_ARCH_STATE_OK);
    resolution = (ti_c54x_dual_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
}

static void
test_unknown_ar_and_malformed_inputs_are_zero_on_error(void)
{
    ti_c54x_arch_state state = known_mode_state();
    ti_c54x_dual_memory_address_resolution resolution;
    ti_c54x_op_memory_address_spec spec;

    spec = dual_spec(UINT8_C(1), UINT8_C(5));
    resolution = (ti_c54x_dual_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);

    spec = dual_spec(UINT8_C(2), UINT8_C(5));
    spec.dual.x_mode = UINT8_C(1);
    resolution = (ti_c54x_dual_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);

    spec = dual_spec(UINT8_C(2), UINT8_C(5));
    spec.dual.reserved_word0 = UINT32_C(1);
    resolution = (ti_c54x_dual_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);

    spec = dual_spec(UINT8_C(2), UINT8_C(5));
    resolution = (ti_c54x_dual_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS);
    check_zero_resolution(&resolution);

    spec = dual_spec(UINT8_C(2), UINT8_C(5));
    resolution = (ti_c54x_dual_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE);
    check_zero_resolution(&resolution);
}

int
main(void)
{
    test_mod0_pair_resolves_two_independent_reads();
    test_same_ar_alias_uses_one_frozen_value();
    test_cmpt_arp_are_not_dependencies_for_dual_mod0();
    test_unknown_ar_and_malformed_inputs_are_zero_on_error();
    puts("dual_address_resolution: ok");
    return 0;
}
