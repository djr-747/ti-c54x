/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage for the value-owned dual-indirect
 * MOD0 WORD16 planner and its rejected-input transaction contract.
 * Does not own: dual-opcode decoding, ARP/CMPT state, bus transfers,
 * sequencing, register mutation, timing, or architectural evidence.
 */

#include "address/indirect_dual.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static void
check_zero_effect(const ti_c54x_address_effect *effect)
{
    TI_C54X_TEST_CHECK_U64(effect->operand.width,
                           TI_C54X_DATA_OPERAND_WIDTH_INVALID);
    TI_C54X_TEST_CHECK_U64(effect->operand.first_word, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(effect->operand.second_word, UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->source.kind == TI_C54X_ADDRESS_SOURCE_NONE);
    TI_C54X_TEST_CHECK(effect->state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_NONE);
    TI_C54X_TEST_CHECK(effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
    TI_C54X_TEST_CHECK_U64(effect->update.ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(effect->update.value_before, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(effect->update.value_after, UINT16_C(0));
}

static void
check_zero_dual(const ti_c54x_indirect_dual_result *result,
                ti_c54x_address_status status)
{
    TI_C54X_TEST_CHECK(result->status == status);
    check_zero_effect(&result->x.effect);
    check_zero_effect(&result->y.effect);
}

static void
check_word16_read(const ti_c54x_address_result *result,
                  uint8_t ar_index,
                  uint16_t ar_value)
{
    TI_C54X_TEST_CHECK(result->status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(result->effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK_U64(result->effect.operand.first_word, ar_value);
    TI_C54X_TEST_CHECK_U64(result->effect.operand.second_word, UINT16_C(0));
    TI_C54X_TEST_CHECK(result->effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR);
    TI_C54X_TEST_CHECK_U64(result->effect.source.value.ar.index, ar_index);
    TI_C54X_TEST_CHECK_U64(result->effect.source.value.ar.value_before,
                           ar_value);
    TI_C54X_TEST_CHECK(result->effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_NONE);
    TI_C54X_TEST_CHECK(result->effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(result->effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
    TI_C54X_TEST_CHECK_U64(result->effect.update.ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(result->effect.update.value_before, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(result->effect.update.value_after, UINT16_C(0));
}

static void
test_actual_ar_domain_and_old_snapshots(void)
{
    static const uint16_t values[] = {
        UINT16_C(0), UINT16_C(1), UINT16_C(0x1234), UINT16_C(0x8000),
        UINT16_C(0xffff)};

    for (uint8_t x_index = UINT8_C(2); x_index <= UINT8_C(5); ++x_index) {
        for (uint8_t y_index = UINT8_C(2); y_index <= UINT8_C(5); ++y_index) {
            for (size_t x_value_index = 0;
                 x_value_index < sizeof(values) / sizeof(values[0]);
                 ++x_value_index) {
                for (size_t y_value_index = 0;
                     y_value_index < sizeof(values) / sizeof(values[0]);
                     ++y_value_index) {
                    const uint16_t x_value = values[x_value_index];
                    const uint16_t y_value = values[y_value_index];
                    const ti_c54x_indirect_dual_result result =
                        ti_c54x_indirect_dual_word16(
                            x_index, x_value, y_index, y_value,
                            TI_C54X_INDIRECT_DUAL_MOD0,
                            TI_C54X_INDIRECT_DUAL_MOD0,
                            TI_C54X_ADDRESS_ACCESS_READ);

                    if (x_index == y_index && x_value != y_value) {
                        check_zero_dual(&result,
                                        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
                        continue;
                    }
                    TI_C54X_TEST_CHECK(result.status ==
                                       TI_C54X_ADDRESS_STATUS_OK);
                    check_word16_read(&result.x, x_index, x_value);
                    check_word16_read(&result.y, y_index, y_value);
                }
            }
        }
    }
}

static void
test_invalid_selectors_and_access_are_transactional(void)
{
    const uint8_t valid = UINT8_C(2);
    const uint16_t value = UINT16_C(0x3456);

    {
        const ti_c54x_indirect_dual_result result =
            ti_c54x_indirect_dual_word16(
                UINT8_C(1), value, valid, value, TI_C54X_INDIRECT_DUAL_MOD0,
                TI_C54X_INDIRECT_DUAL_MOD0,
                TI_C54X_ADDRESS_ACCESS_READ);
        check_zero_dual(&result, TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
    {
        const ti_c54x_indirect_dual_result result =
            ti_c54x_indirect_dual_word16(
                valid, value, UINT8_C(6), value,
                TI_C54X_INDIRECT_DUAL_MOD0, TI_C54X_INDIRECT_DUAL_MOD0,
                TI_C54X_ADDRESS_ACCESS_READ);
        check_zero_dual(&result, TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
    {
        const ti_c54x_indirect_dual_result result =
            ti_c54x_indirect_dual_word16(
                valid, value, valid, value, TI_C54X_INDIRECT_DUAL_MOD0,
                TI_C54X_INDIRECT_DUAL_MOD0,
                (ti_c54x_address_access)UINT8_C(2));
        check_zero_dual(&result, TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
    {
        const ti_c54x_indirect_dual_result result =
            ti_c54x_indirect_dual_word16(
                valid, value, valid, value, TI_C54X_INDIRECT_DUAL_MOD0,
                TI_C54X_INDIRECT_DUAL_MOD0,
                TI_C54X_ADDRESS_ACCESS_WRITE);
        check_zero_dual(&result, TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
    }
}

static void
test_unsupported_modes_and_alias_mismatch(void)
{
    const uint8_t x_index = UINT8_C(2);
    const uint8_t y_index = UINT8_C(3);
    const uint16_t x_value = UINT16_C(0x1111);
    const uint16_t y_value = UINT16_C(0x2222);

    for (uint8_t mode = UINT8_C(1); mode <= UINT8_C(3); ++mode) {
        const ti_c54x_indirect_dual_result x_mode_result =
            ti_c54x_indirect_dual_word16(
                x_index, x_value, y_index, y_value,
                (ti_c54x_indirect_dual_mode)mode,
                TI_C54X_INDIRECT_DUAL_MOD0,
                TI_C54X_ADDRESS_ACCESS_READ);
        check_zero_dual(&x_mode_result,
                        TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);

        const ti_c54x_indirect_dual_result y_mode_result =
            ti_c54x_indirect_dual_word16(
                x_index, x_value, y_index, y_value,
                TI_C54X_INDIRECT_DUAL_MOD0,
                (ti_c54x_indirect_dual_mode)mode,
                TI_C54X_ADDRESS_ACCESS_READ);
        check_zero_dual(&y_mode_result,
                        TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
    }

    {
        const ti_c54x_indirect_dual_result result =
            ti_c54x_indirect_dual_word16(
                x_index, x_value, y_index, y_value,
                (ti_c54x_indirect_dual_mode)UINT8_C(4),
                TI_C54X_INDIRECT_DUAL_MOD0,
                TI_C54X_ADDRESS_ACCESS_READ);
        check_zero_dual(&result, TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
    {
        const ti_c54x_indirect_dual_result result =
            ti_c54x_indirect_dual_word16(
                x_index, x_value, y_index, y_value,
                TI_C54X_INDIRECT_DUAL_MOD0,
                (ti_c54x_indirect_dual_mode)UINT8_C(4),
                TI_C54X_ADDRESS_ACCESS_READ);
        check_zero_dual(&result, TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
    {
        const ti_c54x_indirect_dual_result result =
            ti_c54x_indirect_dual_word16(
                x_index, x_value, x_index, (uint16_t)(x_value + UINT16_C(1)),
                TI_C54X_INDIRECT_DUAL_MOD0,
                TI_C54X_INDIRECT_DUAL_MOD0,
                TI_C54X_ADDRESS_ACCESS_READ);
        check_zero_dual(&result, TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
}

int
main(void)
{
    test_actual_ar_domain_and_old_snapshots();
    test_invalid_selectors_and_access_are_transactional();
    test_unsupported_modes_and_alias_mismatch();
    puts("indirect_dual_mod0: ok");
    return 0;
}
