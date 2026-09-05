/*
 * Module contract
 *
 * Owns: executable COMMON_TI tests for CMPT=0 single-indirect word planning.
 * Does not own: production address semantics, CPU state, bus mapping, opcode
 * decoding, test harness implementation, or unresolved boundary policy.
 */

#include "address/indirect_single.h"
#include "../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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
check_ar_update(const ti_c54x_address_update *update,
                ti_c54x_address_update_phase phase,
                uint8_t ar_index,
                uint16_t value_before,
                uint16_t value_after)
{
    TI_C54X_TEST_CHECK(update->target == TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(update->phase == phase);
    TI_C54X_TEST_CHECK_U64(update->ar_index, ar_index);
    TI_C54X_TEST_CHECK_U64(update->value_before, value_before);
    TI_C54X_TEST_CHECK_U64(update->value_after, value_after);
}

static void
check_valid(uint8_t ar_index,
            uint16_t ar_value,
            ti_c54x_indirect_single_mode mode,
            ti_c54x_address_access access)
{
    const ti_c54x_address_result result = ti_c54x_indirect_single_word(
        ar_index, ar_value, mode, access);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(result.effect.selected_ar_index, ar_index);
    TI_C54X_TEST_CHECK(
        result.effect.state_policy ==
        TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0);
    TI_C54X_TEST_CHECK(result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word, UINT16_C(0));

    switch (mode) {
    case TI_C54X_INDIRECT_SINGLE_MOD0:
        TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, ar_value);
        check_no_update(&result.effect.update);
        break;
    case TI_C54X_INDIRECT_SINGLE_MOD1:
        TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, ar_value);
        check_ar_update(&result.effect.update,
                        TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                        ar_index,
                        ar_value,
                        (uint16_t)(ar_value - UINT16_C(1)));
        break;
    case TI_C54X_INDIRECT_SINGLE_MOD2:
        TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, ar_value);
        check_ar_update(&result.effect.update,
                        TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                        ar_index,
                        ar_value,
                        (uint16_t)(ar_value + UINT16_C(1)));
        break;
    case TI_C54X_INDIRECT_SINGLE_MOD3:
        TI_C54X_TEST_CHECK(access == TI_C54X_ADDRESS_ACCESS_WRITE);
        TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word,
                               (uint16_t)(ar_value + UINT16_C(1)));
        check_ar_update(&result.effect.update,
                        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                        ar_index,
                        ar_value,
                        (uint16_t)(ar_value + UINT16_C(1)));
        break;
    default:
        TI_C54X_TEST_CHECK(false);
        break;
    }
}

static void
test_complete_valid_word_domain(void)
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

            if (mode == TI_C54X_INDIRECT_SINGLE_MOD3 &&
                access == TI_C54X_ADDRESS_ACCESS_READ) {
                continue;
            }
            for (ar_index_value = UINT32_C(0);
                 ar_index_value <= UINT32_C(7);
                 ++ar_index_value) {
                uint32_t ar_value;

                for (ar_value = UINT32_C(0); ar_value <= UINT32_C(0xffff);
                     ++ar_value) {
                    if ((mode == TI_C54X_INDIRECT_SINGLE_MOD1 &&
                         ar_value == UINT32_C(0)) ||
                        ((mode == TI_C54X_INDIRECT_SINGLE_MOD2 ||
                          mode == TI_C54X_INDIRECT_SINGLE_MOD3) &&
                         ar_value == UINT32_C(0xffff))) {
                        continue;
                    }
                    check_valid((uint8_t)ar_index_value,
                                (uint16_t)ar_value,
                                mode,
                                access);
                }
            }
        }
    }
}

static void
test_mod3_is_write_only(void)
{
    const ti_c54x_address_result result = ti_c54x_indirect_single_word(
        UINT8_C(3), UINT16_C(0x3456), TI_C54X_INDIRECT_SINGLE_MOD3,
        TI_C54X_ADDRESS_ACCESS_READ);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.width,
                           TI_C54X_DATA_OPERAND_WIDTH_INVALID);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(result.effect.selected_ar_index, UINT8_C(0));
    check_no_update(&result.effect.update);
}

int
main(void)
{
    test_complete_valid_word_domain();
    test_mod3_is_write_only();
    puts("indirect_single_word: ok");
    return 0;
}
