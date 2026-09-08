/*
 * Module contract
 *
 * Owns: structural validation of the generic SINGLE_INDIRECT MOD0--MOD3,
 * INDEXED_AR0 MOD5/MOD6, CIRCULAR MOD8/MOD10, ARF-selected
 * REVERSE_CARRY MOD4/MOD7 WORD16, MOD13 LONG32, and dual-indirect
 * MOD0/MOD1/MOD2 WORD16 memory effect carriers. Dual MOD3 remains deferred
 * with the AR0/BK indexed/circular dependency.
 * Does not own: address state, bus execution, memory visibility, opcode
 * identity, timing, or diagnostics.
 */

#include "memory_effect.h"

#include <stddef.h>

static bool
ti_c54x_op_memory_rmw_operation_is_valid(uint8_t operation)
{
    return operation ==
               (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE ||
           operation == (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_AND ||
           operation == (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_OR ||
           operation == (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_XOR;
}

static bool
ti_c54x_op_memory_effect_access_shape_is_valid(
    const ti_c54x_op_memory_effect *effect)
{
    if (!ti_c54x_op_memory_rmw_operation_is_valid(effect->rmw_operation)) {
        return false;
    }
    if (effect->transfer_reserved != UINT8_C(0)) {
        return false;
    }
    if (effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        if (effect->rmw_operation ==
            (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE) {
            return effect->value_word == UINT16_C(0) &&
                   effect->complete != NULL;
        }
        return effect->complete == NULL;
    }
    return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
           effect->rmw_operation ==
               (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
           effect->complete == NULL;
}

static bool
ti_c54x_op_memory_dual_address_is_valid(
    const ti_c54x_op_memory_address_spec *address_spec)
{
    return address_spec->dual.x_mode <= UINT8_C(2) &&
           address_spec->dual.y_mode <= UINT8_C(2) &&
           address_spec->dual.x_ar >= UINT8_C(2) &&
           address_spec->dual.x_ar <= UINT8_C(5) &&
           address_spec->dual.y_ar >= UINT8_C(2) &&
           address_spec->dual.y_ar <= UINT8_C(5) &&
           address_spec->dual.reserved_word0 == UINT32_C(0) &&
           address_spec->dual.reserved_word1 == UINT32_C(0) &&
           address_spec->dual.reserved_word2 == UINT32_C(0) &&
           address_spec->dual.reserved_word3 == UINT32_C(0);
}

static bool
ti_c54x_op_memory_effect_dual_shape_is_valid(
    const ti_c54x_op_memory_effect *effect)
{
    return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           effect->value_word == UINT16_C(0) &&
           effect->rmw_operation ==
               (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE &&
           effect->transfer_reserved == UINT8_C(0) &&
           effect->complete_dual != NULL;
}

bool
ti_c54x_op_memory_effect_is_valid(const ti_c54x_op_memory_effect *effect)
{
    if (effect == NULL) {
        return false;
    }

    switch (effect->address_spec.kind) {
    case TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16:
        if (effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            effect->address_spec.mode < TI_C54X_INDIRECT_SINGLE_MOD0 ||
            effect->address_spec.mode > TI_C54X_INDIRECT_SINGLE_MOD3 ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        if (effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
            effect->address_spec.mode == TI_C54X_INDIRECT_SINGLE_MOD3) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16:
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            effect->address_spec.indexed_mode <
                TI_C54X_INDIRECT_INDEXED_MOD5 ||
            effect->address_spec.indexed_mode >
                TI_C54X_INDIRECT_INDEXED_MOD6 ||
            effect->address_spec.arf == UINT8_C(0) ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16:
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            (effect->address_spec.circular_mode !=
                 TI_C54X_INDIRECT_CIRCULAR_MOD8 &&
             effect->address_spec.circular_mode !=
                 TI_C54X_INDIRECT_CIRCULAR_MOD10) ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16:
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            (effect->address_spec.reverse_carry_mode !=
                 TI_C54X_INDIRECT_REVERSE_CARRY_MOD4 &&
             effect->address_spec.reverse_carry_mode !=
                 TI_C54X_INDIRECT_REVERSE_CARRY_MOD7) ||
            effect->address_spec.arf == UINT8_C(0) ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        return ti_c54x_op_memory_effect_access_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32:
        if (effect->address_spec.mode != 0 ||
            effect->address_spec.indexed_mode != 0 ||
            effect->address_spec.circular_mode != 0 ||
            effect->address_spec.reverse_carry_mode != 0 ||
            effect->address_spec.arf > UINT8_C(7)) {
            return false;
        }
        if (effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
            return effect->first_value_msw == UINT16_C(0) &&
                   effect->second_value_lsw == UINT16_C(0) &&
                   effect->complete_long32 != NULL;
        }
        return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
               effect->complete_long32 == NULL;

    case TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16:
        return ti_c54x_op_memory_dual_address_is_valid(
                   &effect->address_spec) &&
               ti_c54x_op_memory_effect_dual_shape_is_valid(effect);

    case TI_C54X_OP_MEMORY_ADDRESS_INVALID:
    default:
        return false;
    }
}
