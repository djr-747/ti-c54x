/*
 * Module contract
 *
 * Owns: executable W50A/W51A coverage for dual-indirect WORD16 address
 * resolution in modes 0, 1, and 2. It proves frozen-state dependency
 * ordering, independent X/Y DATA READ requests, value-owned post-access
 * commits, Xmod precedence for same-AR aliases, and transactional rejection
 * of malformed, unsupported, or unresolved inputs.
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

/* W51A deliberately preserves W50A's requests-only persistent snapshot.
 * Commit persistence and atomic root publication are one later seam. */
_Static_assert(sizeof(ti_c54x_dual_memory_address_snapshot) == 32U,
               "W51A must not widen the persistent dual snapshot");

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

static ti_c54x_op_memory_address_spec
dual_spec_modes(uint8_t x_ar,
                uint8_t x_mode,
                uint8_t y_ar,
                uint8_t y_mode)
{
    const ti_c54x_op_memory_address_spec spec = {
        .kind = TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16,
        .dual = {
            .x_mode = x_mode,
            .x_ar = x_ar,
            .y_mode = y_mode,
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
check_post_update(const ti_c54x_address_result *address_result,
                  const ti_c54x_sequencer_address_commit *commit,
                  uint8_t ar_index,
                  uint16_t ar_value,
                  uint16_t value_after)
{
    TI_C54X_TEST_CHECK(address_result->status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(address_result->effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK_U64(address_result->effect.operand.first_word,
                           ar_value);
    TI_C54X_TEST_CHECK(address_result->effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR);
    TI_C54X_TEST_CHECK_U64(address_result->effect.source.value.ar.index,
                           ar_index);
    TI_C54X_TEST_CHECK_U64(
        address_result->effect.source.value.ar.value_before, ar_value);
    TI_C54X_TEST_CHECK(address_result->effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_NONE);
    TI_C54X_TEST_CHECK(address_result->effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(address_result->effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
    TI_C54X_TEST_CHECK_U64(address_result->effect.update.ar_index, ar_index);
    TI_C54X_TEST_CHECK_U64(address_result->effect.update.value_before,
                           ar_value);
    TI_C54X_TEST_CHECK_U64(address_result->effect.update.value_after,
                           value_after);
    TI_C54X_TEST_CHECK(commit->target == TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK_U64(commit->ar_index, ar_index);
    TI_C54X_TEST_CHECK_U64(commit->value_after, value_after);
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
test_distinct_mod1_mod2_publish_post_access_commits(void)
{
    ti_c54x_arch_state state = known_mode_state();
    ti_c54x_dual_memory_address_resolution resolution = {0};
    const ti_c54x_op_memory_address_spec spec =
        dual_spec_modes(2, UINT8_C(1), 5, UINT8_C(2));

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x1000)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(5), UINT16_C(0x2000)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_post_update(&resolution.x_address_result,
                      &resolution.x_address_commit, UINT8_C(2),
                      UINT16_C(0x1000), UINT16_C(0x0fff));
    check_post_update(&resolution.y_address_result,
                      &resolution.y_address_commit, UINT8_C(5),
                      UINT16_C(0x2000), UINT16_C(0x2001));
    TI_C54X_TEST_CHECK_U64(resolution.x_request.address_word,
                           UINT16_C(0x1000));
    TI_C54X_TEST_CHECK_U64(resolution.y_request.address_word,
                           UINT16_C(0x2000));

}

static void
test_same_ar_alias_preserves_x_mode_precedence(void)
{
    ti_c54x_arch_state state = known_mode_state();
    ti_c54x_dual_memory_address_resolution resolution = {0};
    const ti_c54x_op_memory_address_spec spec =
        dual_spec_modes(3, UINT8_C(2), 3, UINT8_C(1));

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x4567)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    check_post_update(&resolution.x_address_result,
                      &resolution.x_address_commit, UINT8_C(3),
                      UINT16_C(0x4567), UINT16_C(0x4568));
    check_word16_read(&resolution.y_address_result, &resolution.y_request,
                      &resolution.y_address_commit, UINT8_C(3),
                      UINT16_C(0x4567));
    TI_C54X_TEST_CHECK_U64(resolution.x_request.address_word,
                           UINT16_C(0x4567));
    TI_C54X_TEST_CHECK_U64(resolution.y_request.address_word,
                           UINT16_C(0x4567));

    /* Equal modes retain the same single-X-update rule. */
    resolution = (ti_c54x_dual_memory_address_resolution){0};
    {
        const ti_c54x_op_memory_address_spec equal_mode_spec =
            dual_spec_modes(3, UINT8_C(1), 3, UINT8_C(1));
        TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                               &state, &equal_mode_spec,
                               TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                               UINT16_C(0), &resolution) ==
                           TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    }
    check_post_update(&resolution.x_address_result,
                      &resolution.x_address_commit, UINT8_C(3),
                      UINT16_C(0x4567), UINT16_C(0x4566));
    check_word16_read(&resolution.y_address_result, &resolution.y_request,
                      &resolution.y_address_commit, UINT8_C(3),
                      UINT16_C(0x4567));
}

static void
test_mode_boundaries_and_mode3_are_rejected_transactionally(void)
{
    ti_c54x_arch_state state = known_mode_state();
    ti_c54x_dual_memory_address_resolution resolution;
    ti_c54x_op_memory_address_spec spec;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(5), UINT16_MAX) ==
                       TI_C54X_ARCH_STATE_OK);

    spec = dual_spec_modes(2, UINT8_C(1), 5, UINT8_C(0));
    resolution = (ti_c54x_dual_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED);
    check_zero_resolution(&resolution);

    spec = dual_spec_modes(2, UINT8_C(0), 5, UINT8_C(2));
    resolution = (ti_c54x_dual_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_ADDRESS_PLAN_REJECTED);
    check_zero_resolution(&resolution);

    spec = dual_spec_modes(2, UINT8_C(3), 5, UINT8_C(0));
    resolution = (ti_c54x_dual_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);
}

static void
test_cmpt_arp_are_not_dependencies_for_dual_modes(void)
{
    ti_c54x_arch_state state = {0};
    ti_c54x_dual_memory_address_resolution resolution = {0};
    ti_c54x_op_memory_address_spec spec =
        dual_spec_modes(2, UINT8_C(1), 5, UINT8_C(2));

    /* Dual fields select AR2..AR5 directly. Leave CMPT/ARP unknown while
     * resolving the admitted updating modes. */
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

    /* Known nonzero compatibility state is equally irrelevant. */
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
    spec = dual_spec_modes(2, UINT8_C(2), 5, UINT8_C(1));
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
    spec.dual.x_mode = UINT8_C(3);
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
    test_distinct_mod1_mod2_publish_post_access_commits();
    test_same_ar_alias_preserves_x_mode_precedence();
    test_mode_boundaries_and_mode3_are_rejected_transactionally();
    test_cmpt_arp_are_not_dependencies_for_dual_modes();
    test_unknown_ar_and_malformed_inputs_are_zero_on_error();
    puts("dual_address_resolution: ok");
    return 0;
}
