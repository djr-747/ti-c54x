/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage for dual-indirect WORD16 MOD3
 * resolution: AR0/BK knownness-gated reads, DATA READ request and commit
 * publication, AR0/BK snapshot shadows, lazy BK reads, and CMPT/ARP
 * invariance.
 * Evidence: E-TI-C54X-SPRU131G-S5-5-4-T5-8-P5-20-5-21-DUAL-MOD3,
 * E-TI-C54X-SPRU131G-S5-5-4-2-P5-21-5-22-DUAL-MOD3-INDEXED,
 * E-TI-C54X-SPRU131G-S5-5-4-3-P5-21-5-22-DUAL-MOD3-CIRCULAR,
 * E-TI-C54X-SPRU131G-S5-5-3-4-P5-15-5-16-CIRCULAR-STEP-RULE, and
 * E-TI-C54X-SPRU131G-S5-5-4-T5-6-T5-7-S5-5-5-P5-19-5-22-DUAL-SELECTORS.
 * Does not own: opcode identity or semantics, bus callbacks, memory
 * visibility, instruction sequencing, timing, architectural mutation, or
 * test discovery manifests.
 */

#include "sequencer/memory/dual_address_resolution.h"
#include "state/arch_state.h"
#include "state/block_size_register.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* W52B widens the persistent snapshot with copied AR0/BK shadows so the
 * root validator can recheck MOD3 commits. */
_Static_assert(sizeof(ti_c54x_dual_memory_address_snapshot) == 52U,
               "W52B carries requests, commits, and AR0/BK shadows");

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

static ti_c54x_arch_state
mod3_state(uint8_t cmpt, uint8_t arp)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, cmpt) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, arp) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, UINT8_C(0),
                                                  UINT16_C(0x0002)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, UINT8_C(2),
                                                  UINT16_C(0x0107)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, UINT8_C(3),
                                                  UINT16_C(0x2000)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, UINT16_C(8)) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static void
check_zero_resolution(const ti_c54x_dual_memory_address_resolution *resolution)
{
    const ti_c54x_dual_memory_address_resolution zero = {0};

    TI_C54X_TEST_CHECK(memcmp(resolution, &zero, sizeof(zero)) == 0);
}

/* MOD3 circular side resolves the old address with a post-access commit;
 * the untouched side behaves as before. */
static void
test_mod3_circular_resolution(void)
{
    ti_c54x_arch_state state = mod3_state(UINT8_C(0), UINT8_C(0));
    const ti_c54x_op_memory_address_spec spec =
        dual_spec_modes(UINT8_C(2), UINT8_C(3), UINT8_C(3), UINT8_C(0));
    ti_c54x_dual_memory_address_resolution out = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &out) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK_U64(out.x_request.address_word, UINT16_C(0x0107));
    TI_C54X_TEST_CHECK_U64(out.y_request.address_word, UINT16_C(0x2000));
    TI_C54X_TEST_CHECK(out.x_address_commit.target == TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK_U64(out.x_address_commit.ar_index, UINT8_C(2));
    TI_C54X_TEST_CHECK_U64(out.x_address_commit.value_after, UINT16_C(0x0101));
    TI_C54X_TEST_CHECK(out.y_address_commit.target == TI_C54X_ADDRESS_UPDATE_NONE);
}

/* MOD3 indexed side (BK=0) stages the plain signed addition. */
static void
test_mod3_indexed_resolution(void)
{
    ti_c54x_arch_state state = mod3_state(UINT8_C(0), UINT8_C(0));
    const ti_c54x_op_memory_address_spec spec =
        dual_spec_modes(UINT8_C(2), UINT8_C(0), UINT8_C(3), UINT8_C(3));
    ti_c54x_dual_memory_address_resolution out = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, UINT16_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &out) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK_U64(out.y_request.address_word, UINT16_C(0x2000));
    TI_C54X_TEST_CHECK(out.y_address_commit.target == TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK_U64(out.y_address_commit.ar_index, UINT8_C(3));
    TI_C54X_TEST_CHECK_U64(out.y_address_commit.value_after, UINT16_C(0x2002));
    TI_C54X_TEST_CHECK(out.x_address_commit.target == TI_C54X_ADDRESS_UPDATE_NONE);
}

/* Reset leaves every source unknown; the caller selects which sources to
 * make known so each missing MOD3 dependency rejects in isolation. */
static ti_c54x_arch_state
partial_state(int with_ar0, int with_bk)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, UINT8_C(2),
                                                  UINT16_C(0x0107)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, UINT8_C(3),
                                                  UINT16_C(0x2000)) ==
                       TI_C54X_ARCH_STATE_OK);
    if (with_ar0) {
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, UINT8_C(0),
                                                      UINT16_C(0x0002)) ==
                           TI_C54X_ARCH_STATE_OK);
    }
    if (with_bk) {
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, UINT16_C(8)) ==
                           TI_C54X_ARCH_STATE_OK);
    }
    return state;
}

/* Unknown AR0 or BK rejects when a MOD3 side needs them. */
static void
test_unknown_ar0_bk_reject(void)
{
    const ti_c54x_op_memory_address_spec spec =
        dual_spec_modes(UINT8_C(2), UINT8_C(3), UINT8_C(3), UINT8_C(0));
    ti_c54x_arch_state no_ar0 = partial_state(0, 1);
    ti_c54x_arch_state no_bk = partial_state(1, 0);
    ti_c54x_dual_memory_address_resolution out = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &no_ar0, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &out) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE);
    check_zero_resolution(&out);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &no_bk, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &out) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE);
    check_zero_resolution(&out);
}

/* BK is read lazily: unknown BK without a MOD3 side still resolves. */
static void
test_bk_ignored_without_mod3(void)
{
    ti_c54x_arch_state state = {0};
    const ti_c54x_op_memory_address_spec spec =
        dual_spec_modes(UINT8_C(2), UINT8_C(1), UINT8_C(3), UINT8_C(2));
    ti_c54x_dual_memory_address_resolution out = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, UINT8_C(2),
                                                  UINT16_C(0x1000)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, UINT8_C(3),
                                                  UINT16_C(0x2000)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &out) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK_U64(out.x_address_commit.value_after, UINT16_C(0x0FFF));
    TI_C54X_TEST_CHECK_U64(out.y_address_commit.value_after, UINT16_C(0x2001));
}

/* Identical specs resolve identically under any CMPT/ARP state: the dual
 * path admits no CMPT/ARP input. */
static void
test_cmpt_arp_invariance(void)
{
    ti_c54x_arch_state standard = mod3_state(UINT8_C(0), UINT8_C(0));
    ti_c54x_arch_state compat = mod3_state(UINT8_C(1), UINT8_C(7));
    const ti_c54x_op_memory_address_spec mod3_spec =
        dual_spec_modes(UINT8_C(2), UINT8_C(3), UINT8_C(3), UINT8_C(0));
    const ti_c54x_op_memory_address_spec plain_spec =
        dual_spec_modes(UINT8_C(2), UINT8_C(1), UINT8_C(3), UINT8_C(2));
    ti_c54x_dual_memory_address_resolution first = {0};
    ti_c54x_dual_memory_address_resolution second = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &standard, &mod3_spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &first) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &compat, &mod3_spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &second) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(memcmp(&first, &second, sizeof(first)) == 0);
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &standard, &plain_spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &first) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_dual_memory_address(
                           &compat, &plain_spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &second) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(memcmp(&first, &second, sizeof(first)) == 0);
}

int
main(void)
{
    test_mod3_circular_resolution();
    test_mod3_indexed_resolution();
    test_unknown_ar0_bk_reject();
    test_bk_ignored_without_mod3();
    test_cmpt_arp_invariance();
    puts("dual_mod3_resolution: ok");
    return 0;
}
