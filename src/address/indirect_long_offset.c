/*
 * Module contract
 *
 * Owns: checked CMPT=0 signed long-offset MOD13 effective-address planning
 * for WORD16 and LONG32 operands, and intrinsic validation of the resulting
 * value-owned effect shape.
 * Does not own: CPU/register storage, ARP/CMPT validation, instruction fetch,
 * opcode decoding, bus transfers, wrapping, MOD12/MOD14/MOD15 modes,
 * sequencing, timing, or diagnostics.
 */

#include "indirect_long_offset.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static ti_c54x_address_result
ti_c54x_long_offset_result_with_status(ti_c54x_address_status status)
{
    const ti_c54x_address_result result = {
        .status = status,
        .effect = {.operand = {0}}
    };
    return result;
}

/* Convert the raw instruction word without an implementation-defined
 * uint16_t-to-signed conversion. */
static int32_t
ti_c54x_long_offset_sign_extend(uint16_t long_offset_word)
{
    if ((long_offset_word & UINT16_C(0x8000)) != UINT16_C(0)) {
        return (int32_t)long_offset_word - INT32_C(0x10000);
    }
    return (int32_t)long_offset_word;
}

static bool
ti_c54x_long_offset_sum(uint16_t ar_value,
                        uint16_t long_offset_word,
                        uint16_t *value_after)
{
    const int32_t sum = (int32_t)ar_value +
                        ti_c54x_long_offset_sign_extend(long_offset_word);

    if (sum < INT32_C(0) || sum > INT32_C(0xffff)) {
        return false;
    }
    *value_after = (uint16_t)sum;
    return true;
}

static bool
ti_c54x_long_offset_access_valid(ti_c54x_address_access access)
{
    return access == TI_C54X_ADDRESS_ACCESS_READ ||
           access == TI_C54X_ADDRESS_ACCESS_WRITE;
}

static ti_c54x_address_result
ti_c54x_long_offset_plan(uint8_t ar_index,
                         uint16_t ar_value,
                         uint16_t long_offset_word,
                         ti_c54x_address_access access,
                         ti_c54x_data_operand_width width)
{
    ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {.operand = {0}}
    };
    uint16_t value_after;

    if (ar_index > UINT8_C(7) || !ti_c54x_long_offset_access_valid(access)) {
        return ti_c54x_long_offset_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
    if (!ti_c54x_long_offset_sum(ar_value, long_offset_word, &value_after)) {
        return ti_c54x_long_offset_result_with_status(
            TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    }

    result.effect.operand = width == TI_C54X_DATA_OPERAND_WIDTH_WORD16
                                ? ti_c54x_data_operand_word16(value_after)
                                : ti_c54x_data_operand_long32(value_after);
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET;
    result.effect.source.value.ar_long_offset.index = ar_index;
    result.effect.source.value.ar_long_offset.modification =
        (uint8_t)TI_C54X_INDIRECT_LONG_OFFSET_MOD13;
    result.effect.source.value.ar_long_offset.value_before = ar_value;
    result.effect.source.value.ar_long_offset.long_offset_word =
        long_offset_word;
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0;
    result.effect.update.target = TI_C54X_ADDRESS_UPDATE_AR;
    result.effect.update.phase = TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS;
    result.effect.update.ar_index = ar_index;
    result.effect.update.value_before = ar_value;
    result.effect.update.value_after = value_after;
    return result;
}

ti_c54x_address_result
ti_c54x_indirect_long_offset_mod13_word16(uint8_t ar_index,
                                          uint16_t ar_value,
                                          uint16_t long_offset_word,
                                          ti_c54x_address_access access)
{
    return ti_c54x_long_offset_plan(
        ar_index, ar_value, long_offset_word, access,
        TI_C54X_DATA_OPERAND_WIDTH_WORD16);
}

ti_c54x_address_result
ti_c54x_indirect_long_offset_mod13_long32(uint8_t ar_index,
                                          uint16_t ar_value,
                                          uint16_t long_offset_word,
                                          ti_c54x_address_access access)
{
    return ti_c54x_long_offset_plan(
        ar_index, ar_value, long_offset_word, access,
        TI_C54X_DATA_OPERAND_WIDTH_LONG32);
}

bool
ti_c54x_indirect_long_offset_effect_is_valid(
    const ti_c54x_address_effect *effect)
{
    uint16_t expected_after;
    const ti_c54x_address_source_ar_long_offset *source;

    if (effect == NULL || !ti_c54x_data_operand_is_valid(&effect->operand) ||
        effect->source.kind != TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET ||
        effect->state_policy != TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0) {
        return false;
    }

    source = &effect->source.value.ar_long_offset;
    if (source->index > UINT8_C(7) ||
        source->modification !=
            (uint8_t)TI_C54X_INDIRECT_LONG_OFFSET_MOD13 ||
        effect->update.target != TI_C54X_ADDRESS_UPDATE_AR ||
        effect->update.phase != TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS ||
        effect->update.ar_index != source->index ||
        effect->update.value_before != source->value_before ||
        !ti_c54x_long_offset_sum(source->value_before,
                                  source->long_offset_word,
                                  &expected_after) ||
        effect->operand.first_word != expected_after ||
        effect->update.value_after != expected_after) {
        return false;
    }
    return true;
}
