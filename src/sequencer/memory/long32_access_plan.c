/*
 * Module contract
 *
 * Owns: the pure single-word Lmem LONG32 read leg plan: arm
 * validation plus direct-DP concatenation or landed indirect
 * LONG32 resolution into a canonical operand. Does not own:
 * request issue, endpoint traffic, the pair-transaction commit
 * root, opcode semantics, CPU lifecycle, timing, diagnostics,
 * allocation, or callbacks. All inputs are borrowed for one call.
 */

#include "long32_access_plan.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "address/direct_dp.h"
#include "address/indirect_single.h"

static ti_c54x_long32_single_access
invalid_output(ti_c54x_long32_single_access_status status)
{
    const ti_c54x_long32_single_access plan = {
        .status = status, .effect = {.operand = {0}}};

    return plan;
}

static bool
long32_single_arm_is_valid(const ti_c54x_op_memory_address_spec *spec)
{
    const uint8_t indirect = spec->long32_single.indirect;

    if (spec->kind != TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE ||
        indirect > UINT8_C(1) ||
        spec->long32_single.reserved_word0 != UINT32_C(0) ||
        spec->long32_single.reserved_word1 != UINT32_C(0) ||
        spec->long32_single.reserved_word2 != UINT32_C(0)) {
        return false;
    }
    if (indirect == UINT8_C(0)) {
        return spec->long32_single.mode == UINT8_C(0) &&
               spec->long32_single.arf == UINT8_C(0) &&
               spec->long32_single.dma7 <= UINT8_C(0x7F) &&
               spec->long32_single.dp_page <= UINT16_C(511);
    }
    return spec->long32_single.mode <= UINT8_C(3) &&
           spec->long32_single.arf <= UINT8_C(7) &&
           spec->long32_single.dma7 == UINT8_C(0) &&
           spec->long32_single.dp_page == UINT16_C(0);
}

static ti_c54x_long32_single_access
plan_direct_leg(const ti_c54x_op_memory_address_spec *spec)
{
    const ti_c54x_direct_dp9 dp = {.bits = spec->long32_single.dp_page};
    const ti_c54x_direct_dma7 dma = {.bits = spec->long32_single.dma7};
    const ti_c54x_address_result resolved = ti_c54x_direct_dp_word(dp, dma);
    ti_c54x_long32_single_access plan = invalid_output(
        TI_C54X_LONG32_SINGLE_ACCESS_ADDRESS_PLAN_REJECTED);

    if (resolved.status != TI_C54X_ADDRESS_STATUS_OK) {
        return plan;
    }
    plan.status = TI_C54X_LONG32_SINGLE_ACCESS_OK;
    plan.effect = resolved.effect;
    plan.effect.operand = ti_c54x_data_operand_long32(
        resolved.effect.operand.first_word);
    return plan;
}

static ti_c54x_long32_single_access
plan_indirect_leg(const ti_c54x_op_memory_address_spec *spec,
                  uint16_t ar_value,
                  ti_c54x_address_access access)
{
    const ti_c54x_address_result resolved = ti_c54x_indirect_single_long32(
        spec->long32_single.arf, ar_value,
        (ti_c54x_indirect_single_mode)spec->long32_single.mode, access);
    ti_c54x_long32_single_access plan = invalid_output(
        TI_C54X_LONG32_SINGLE_ACCESS_ADDRESS_PLAN_REJECTED);

    if (resolved.status != TI_C54X_ADDRESS_STATUS_OK) {
        return plan;
    }
    plan.status = TI_C54X_LONG32_SINGLE_ACCESS_OK;
    plan.effect = resolved.effect;
    return plan;
}

ti_c54x_long32_single_access ti_c54x_sequencer_plan_long32_single_access(
    const ti_c54x_op_memory_address_spec *spec,
    uint16_t ar_value,
    ti_c54x_bus_word16_request_access access)
{
    ti_c54x_address_access resolved_access;

    if (spec == NULL) {
        return invalid_output(
            TI_C54X_LONG32_SINGLE_ACCESS_INVALID_ARGUMENT);
    }
    if (access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        resolved_access = TI_C54X_ADDRESS_ACCESS_READ;
    } else if (access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        resolved_access = TI_C54X_ADDRESS_ACCESS_WRITE;
    } else {
        return invalid_output(
            TI_C54X_LONG32_SINGLE_ACCESS_INVALID_ARGUMENT);
    }
    if (!long32_single_arm_is_valid(spec)) {
        return invalid_output(TI_C54X_LONG32_SINGLE_ACCESS_INVALID_SPEC);
    }
    if (spec->long32_single.indirect == UINT8_C(0)) {
        (void)ar_value;
        return plan_direct_leg(spec);
    }
    return plan_indirect_leg(spec, ar_value, resolved_access);
}
