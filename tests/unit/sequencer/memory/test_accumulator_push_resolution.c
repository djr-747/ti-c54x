/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for
 * accumulator-targeted stack-push WORD16 address resolution. It proves
 * the shared TOS contract with immediate pushes (mode independence,
 * PRE_ACCESS SP commit staging, WRITE-only DATA requests) for both
 * accumulator selects and both immediacy kinds, plus transactional
 * output discipline. Does not own: opcode identity, accumulator
 * values or target planning, handlers, CPU lifecycle, endpoint
 * callbacks, timing, or architectural mutation.
 */

#include "isa/ops/memory/memory_effect.h"
#include "sequencer/memory/address_resolution.h"
#include "state/arch_state.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_arch_state
push_state(uint16_t sp_value)
{
    ti_c54x_arch_state state = {0};

    /* Only SP is known: CMPT/ARP/CPL and every AR stay unknown, so any
     * mode read would reject and success proves mode independence. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&state, sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_memory_address_spec
push_spec(ti_c54x_op_memory_address_kind kind, uint8_t select)
{
    ti_c54x_op_memory_address_spec spec = {0};

    spec.kind = kind;
    spec.arf = select;
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
check_resolved_expected(ti_c54x_op_memory_address_kind kind, uint8_t select,
                        uint16_t sp_before, uint16_t write_value)
{
    ti_c54x_arch_state state = push_state(sp_before);
    const ti_c54x_arch_state before = state;
    const ti_c54x_op_memory_address_spec spec = push_spec(kind, select);
    const uint16_t expected_address = (uint16_t)(sp_before - UINT16_C(1));
    ti_c54x_memory_address_resolution resolution = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                           write_value, &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(resolution.request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(resolution.request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(resolution.request.address_word,
                           expected_address);
    TI_C54X_TEST_CHECK_U64(resolution.request.write_value_word, write_value);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_STACK_PUSH);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.source.value.stack_push.sp_before,
        sp_before);
    TI_C54X_TEST_CHECK_U64(
        resolution.address_result.effect.operand.first_word,
        expected_address);
    TI_C54X_TEST_CHECK(resolution.address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_SP);
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(resolution.address_commit.value_after,
                           expected_address);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
test_resolve_vectors(void)
{
    static const ti_c54x_op_memory_address_kind kinds[] = {
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16,
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16};
    static const uint16_t sps[] = {UINT16_C(0x1111), UINT16_C(0x0001),
                                   UINT16_C(0x0000)};
    size_t kind_index;
    uint8_t select;
    size_t sp_index;

    for (kind_index = 0U;
         kind_index < sizeof(kinds) / sizeof(kinds[0]); ++kind_index) {
        for (select = UINT8_C(0); select <= UINT8_C(1); ++select) {
            for (sp_index = 0U;
                 sp_index < sizeof(sps) / sizeof(sps[0]); ++sp_index) {
                check_resolved_expected(kinds[kind_index], select,
                                        sps[sp_index], UINT16_C(0xCAFE));
            }
        }
    }
}

static void
test_rejections_are_candidate_free(void)
{
    ti_c54x_arch_state state = push_state(UINT16_C(0x1111));
    const ti_c54x_arch_state before = state;
    ti_c54x_op_memory_address_spec spec = push_spec(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16,
        UINT8_C(0));
    ti_c54x_memory_address_resolution resolution = {0};

    /* A select outside A/B rejects before any candidate. */
    spec.arf = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                           UINT16_C(0xCAFE), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    /* Stray first-arm fields reject. */
    spec = push_spec(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16,
        UINT8_C(1));
    spec.long_offset_word = UINT16_C(1);
    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                           UINT16_C(0xCAFE), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    check_zero_resolution(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    /* READ access rejects. */
    spec = push_spec(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16,
        UINT8_C(0));
    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS);
    check_zero_resolution(&resolution);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    /* Unknown SP rejects without repair. */
    state = (ti_c54x_arch_state){0};
    resolution = (ti_c54x_memory_address_resolution){0};
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                           UINT16_C(0xCAFE), &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_SOURCE);
    check_zero_resolution(&resolution);
}

int
main(void)
{
    test_resolve_vectors();
    test_rejections_are_candidate_free();
    puts("accumulator_push_resolution: ok");
    return 0;
}
