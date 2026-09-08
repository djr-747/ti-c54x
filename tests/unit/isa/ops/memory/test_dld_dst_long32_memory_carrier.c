/*
 * Module contract
 *
 * Owns: RED contracts for the DLD/DST CMPT=0 MOD13 LONG32 memory carrier and
 * its pure address-resolution seam.  It proves copied pair payloads, the
 * typed completion callback, canonical MSW/LSW addresses, and pre-access AR
 * staging without involving an opcode table or a CPU execution path.
 * Does not own: opcode decoding, catalog rows, bus lifecycle, architectural
 * mutation, timing, or diagnostics.
 */

#include "../../../../../src/address/indirect_long_offset.h"
#include "../../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/sequencer/instruction/memory_instruction_transaction.h"
#include "../../../../../src/sequencer/memory/address_resolution.h"
#include "../../../../../src/state/arch_state.h"
#include "../../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static unsigned completion_calls;

static ti_c54x_op_status complete_word16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    (void)instruction;
    (void)input;
    (void)read_value_word;
    *next_effect = (ti_c54x_op_effect){0};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status complete_long32(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t first_msw,
    uint16_t second_lsw,
    ti_c54x_op_effect *next_effect)
{
    TI_C54X_TEST_CHECK(instruction != NULL);
    TI_C54X_TEST_CHECK(input != NULL);
    TI_C54X_TEST_CHECK(next_effect != NULL);
    TI_C54X_TEST_CHECK(first_msw != second_lsw);
    *next_effect = (ti_c54x_op_effect){0};
    ++completion_calls;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_memory_effect valid_effect(
    ti_c54x_bus_word16_request_access access)
{
    ti_c54x_op_memory_effect effect = {0};

    effect.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32;
    effect.address_spec.mode = (ti_c54x_indirect_single_mode)0;
    effect.address_spec.indexed_mode = (ti_c54x_indirect_indexed_mode)0;
    effect.address_spec.circular_mode = (ti_c54x_indirect_circular_mode)0;
    effect.address_spec.reverse_carry_mode =
        (ti_c54x_indirect_reverse_carry_mode)0;
    effect.address_spec.arf = UINT8_C(2);
    effect.address_spec.long_offset_word = UINT16_C(2);
    effect.access = access;
    effect.first_value_msw = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE
                                  ? UINT16_C(0x1234)
                                  : UINT16_C(0);
    effect.second_value_lsw = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE
                                  ? UINT16_C(0x5678)
                                  : UINT16_C(0);
    effect.complete_long32 = access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                                 ? complete_long32
                                 : NULL;
    return effect;
}

static void check_carrier_shape_and_size(void)
{
    ti_c54x_op_memory_effect read =
        valid_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_op_memory_effect write =
        valid_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);

    TI_C54X_TEST_CHECK(sizeof(ti_c54x_op_memory_address_spec) == 24U);
    TI_C54X_TEST_CHECK(sizeof(ti_c54x_op_memory_effect) ==
                       (sizeof(void *) == 8U ? 40U : 36U));
    TI_C54X_TEST_CHECK(sizeof(ti_c54x_memory_instruction_transaction) ==
                       (sizeof(void *) == 8U ? 336U : 308U));
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&read));
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&write));
    TI_C54X_TEST_CHECK(read.first_value_msw == UINT16_C(0));
    TI_C54X_TEST_CHECK(read.second_value_lsw == UINT16_C(0));
    TI_C54X_TEST_CHECK(read.complete_long32 == complete_long32);
    TI_C54X_TEST_CHECK(write.complete_long32 == NULL);

    read.first_value_msw = UINT16_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&read));
    read = valid_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    read.second_value_lsw = UINT16_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&read));
    write.complete_long32 = complete_long32;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&write));
    write = valid_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    write.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&write));
}

static void check_reserved_and_malformed_fields(void)
{
    ti_c54x_op_memory_effect word16 = {0};
    ti_c54x_op_memory_effect effect =
        valid_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

    word16.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
    word16.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD0;
    word16.address_spec.arf = UINT8_C(2);
    word16.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    word16.complete = complete_word16;
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&word16));
    word16.transfer_reserved = UINT8_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&word16));

    effect.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD1;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = valid_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    effect.address_spec.indexed_mode = TI_C54X_INDIRECT_INDEXED_MOD5;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = valid_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    effect.address_spec.circular_mode = TI_C54X_INDIRECT_CIRCULAR_MOD8;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = valid_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    effect.address_spec.reverse_carry_mode =
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD4;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = valid_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    effect.address_spec.arf = UINT8_C(8);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = valid_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    effect.address_spec.kind = TI_C54X_OP_MEMORY_ADDRESS_INVALID;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(NULL));
}

static ti_c54x_arch_state known_state(uint8_t index, uint16_t ar_value)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, index, ar_value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static void check_planner_pairs(void)
{
    const uint16_t starts[] = {UINT16_C(0x2000), UINT16_C(0x2001),
                               UINT16_C(0x2004), UINT16_C(0x2005)};
    const uint16_t offsets[] = {UINT16_C(2), UINT16_C(2), UINT16_C(0xfffe),
                                UINT16_C(0xfffe)};

    for (size_t index = 0U; index < sizeof(starts) / sizeof(starts[0]);
         ++index) {
        const uint16_t start = starts[index];
        const uint16_t offset = offsets[index];
        const ti_c54x_address_result result =
            ti_c54x_indirect_long_offset_mod13_long32(
                UINT8_C(2), start, offset, TI_C54X_ADDRESS_ACCESS_READ);
        const uint16_t expected_first = (uint16_t)(start + offset);
        const uint16_t expected_second =
            (expected_first & UINT16_C(1)) == 0U
                ? (uint16_t)(expected_first + UINT16_C(1))
                : (uint16_t)(expected_first - UINT16_C(1));

        TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
        TI_C54X_TEST_CHECK(result.effect.operand.width ==
                           TI_C54X_DATA_OPERAND_WIDTH_LONG32);
        TI_C54X_TEST_CHECK(result.effect.operand.first_word == expected_first);
        TI_C54X_TEST_CHECK(result.effect.operand.second_word == expected_second);
        TI_C54X_TEST_CHECK(result.effect.update.phase ==
                           TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS);
        TI_C54X_TEST_CHECK(result.effect.update.value_after == expected_first);
        TI_C54X_TEST_CHECK(
            ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
    }
}

static void check_resolver_is_pure_and_uses_first_payload(void)
{
    ti_c54x_arch_state state = known_state(UINT8_C(2), UINT16_C(0x2100));
    const ti_c54x_arch_state before = state;
    const ti_c54x_op_memory_effect effect =
        valid_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    ti_c54x_memory_address_resolution resolution = {0};

    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_resolve_memory_address(
            &state, &effect.address_spec, effect.access, effect.first_value_msw,
            &resolution) == TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(resolution.request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(resolution.request.write_value_word ==
                       effect.first_value_msw);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_LONG32);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);

    {
        const ti_c54x_op_memory_effect read_effect =
            valid_effect(TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
        ti_c54x_memory_address_resolution read_resolution = {0};

        TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                               &state, &read_effect.address_spec,
                               read_effect.access, read_effect.first_value_msw,
                               &read_resolution) ==
                           TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
        TI_C54X_TEST_CHECK(read_resolution.request.access ==
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
        TI_C54X_TEST_CHECK(read_resolution.request.write_value_word ==
                           UINT16_C(0));
        TI_C54X_TEST_CHECK(read_resolution.request.address_word ==
                           UINT32_C(0x2102));
        TI_C54X_TEST_CHECK(read_resolution.address_result.effect.operand.second_word ==
                           UINT16_C(0x2103));
    }
}

int main(void)
{
    check_carrier_shape_and_size();
    check_reserved_and_malformed_fields();
    check_planner_pairs();
    check_resolver_is_pure_and_uses_first_payload();
    return 0;
}
