/*
 * Module contract
 *
 * Owns: executable COMMON_TI tests for CMPT=0 single-indirect long32
 * planning, including complete valid AR domains for supported modes and the
 * documented MOD3 read restriction.
 * Does not own: state application, bus order, timing, opcode decoding,
 * unsupported address families, or test harness implementation.
 */

#include "address/indirect_single.h"
#include "../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static uint16_t
expected_second(uint16_t first)
{
    return (first & UINT16_C(1)) == UINT16_C(0)
               ? (uint16_t)(first + UINT16_C(1))
               : (uint16_t)(first - UINT16_C(1));
}

static void
check_no_update(const ti_c54x_address_update *update)
{
    TI_C54X_TEST_CHECK(update->target == TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(update->phase == TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
    TI_C54X_TEST_CHECK_U64(update->ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(update->value_before, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(update->value_after, UINT16_C(0));
}

static void
check_update(const ti_c54x_address_update *update,
             ti_c54x_address_update_phase phase,
             uint8_t ar_index,
             uint16_t before,
             uint16_t after)
{
    TI_C54X_TEST_CHECK(update->target == TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(update->phase == phase);
    TI_C54X_TEST_CHECK_U64(update->ar_index, ar_index);
    TI_C54X_TEST_CHECK_U64(update->value_before, before);
    TI_C54X_TEST_CHECK_U64(update->value_after, after);
}

static void
check_valid(uint8_t ar_index,
            uint16_t ar_value,
            ti_c54x_indirect_single_mode mode,
            ti_c54x_address_access access)
{
    const ti_c54x_address_result result = ti_c54x_indirect_single_long32(
        ar_index, ar_value, mode, access);
    uint16_t expected_first = ar_value;

    if (mode == TI_C54X_INDIRECT_SINGLE_MOD3) {
        expected_first = (uint16_t)(ar_value + UINT16_C(2));
    }
    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(result.effect.selected_ar_index, ar_index);
    TI_C54X_TEST_CHECK(result.effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0);
    TI_C54X_TEST_CHECK(result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_LONG32);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, expected_first);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word,
                           expected_second(expected_first));

    switch (mode) {
    case TI_C54X_INDIRECT_SINGLE_MOD0:
        check_no_update(&result.effect.update);
        break;
    case TI_C54X_INDIRECT_SINGLE_MOD1:
        check_update(&result.effect.update,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     ar_index, ar_value,
                     (uint16_t)(ar_value - UINT16_C(2)));
        break;
    case TI_C54X_INDIRECT_SINGLE_MOD2:
        check_update(&result.effect.update,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     ar_index, ar_value,
                     (uint16_t)(ar_value + UINT16_C(2)));
        break;
    case TI_C54X_INDIRECT_SINGLE_MOD3:
        TI_C54X_TEST_CHECK(access == TI_C54X_ADDRESS_ACCESS_WRITE);
        check_update(&result.effect.update,
                     TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                     ar_index, ar_value,
                     (uint16_t)(ar_value + UINT16_C(2)));
        break;
    default:
        TI_C54X_TEST_CHECK(false);
        break;
    }
}

static void
test_complete_valid_domain(void)
{
    uint32_t mode_value;

    for (mode_value = UINT32_C(0); mode_value <= UINT32_C(3); ++mode_value) {
        uint32_t access_value;

        for (access_value = UINT32_C(0); access_value <= UINT32_C(1);
             ++access_value) {
            const ti_c54x_indirect_single_mode mode =
                (ti_c54x_indirect_single_mode)mode_value;
            const ti_c54x_address_access access =
                (ti_c54x_address_access)access_value;
            uint32_t ar_index_value;
            uint32_t first_value;
            uint32_t last_value = UINT32_C(0xffff);

            if (mode == TI_C54X_INDIRECT_SINGLE_MOD3 &&
                access == TI_C54X_ADDRESS_ACCESS_READ) {
                continue;
            }
            first_value = mode == TI_C54X_INDIRECT_SINGLE_MOD1
                              ? UINT32_C(2)
                              : UINT32_C(0);
            if (mode == TI_C54X_INDIRECT_SINGLE_MOD2 ||
                mode == TI_C54X_INDIRECT_SINGLE_MOD3) {
                last_value = UINT32_C(0xfffd);
            }
            for (ar_index_value = UINT32_C(0);
                 ar_index_value <= UINT32_C(7); ++ar_index_value) {
                uint32_t ar_value;

                for (ar_value = first_value; ar_value <= last_value;
                     ++ar_value) {
                    check_valid((uint8_t)ar_index_value, (uint16_t)ar_value,
                                mode, access);
                }
            }
        }
    }
}

static void
test_mod3_read_is_unsupported(void)
{
    const ti_c54x_address_result result = ti_c54x_indirect_single_long32(
        UINT8_C(2), UINT16_C(0x1000), TI_C54X_INDIRECT_SINGLE_MOD3,
        TI_C54X_ADDRESS_ACCESS_READ);

    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
}

int
main(void)
{
    test_complete_valid_domain();
    test_mod3_read_is_unsupported();
    puts("indirect_single_long32: ok");
    return 0;
}
