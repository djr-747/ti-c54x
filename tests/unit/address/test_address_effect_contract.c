/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY tests for checked planner status and
 * deterministic zero effects on rejected inputs.
 * Does not own: TI architectural evidence, CPU state, bus mapping, opcode
 * decoding, test harness implementation, or valid-mode semantic coverage.
 */

#include "address/indirect_single.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static void
check_zero_effect(const ti_c54x_address_effect *effect)
{
    TI_C54X_TEST_CHECK_U64(effect->effective_address_word, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(effect->selected_ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK(effect->update.target == TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(effect->update.phase == TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
    TI_C54X_TEST_CHECK_U64(effect->update.ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(effect->update.value_before, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(effect->update.value_after, UINT16_C(0));
}

static void
check_status_and_zero(ti_c54x_address_result result,
                      ti_c54x_address_status expected)
{
    TI_C54X_TEST_CHECK(result.status == expected);
    check_zero_effect(&result.effect);
}

static void
test_invalid_arguments(void)
{
    check_status_and_zero(
        ti_c54x_indirect_single_word(
            UINT8_C(8), UINT16_C(0x1000), TI_C54X_INDIRECT_SINGLE_MOD0,
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_status_and_zero(
        ti_c54x_indirect_single_word(
            UINT8_C(0), UINT16_C(0x1000),
            (ti_c54x_indirect_single_mode)UINT8_C(4),
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_status_and_zero(
        ti_c54x_indirect_single_word(
            UINT8_C(0), UINT16_C(0x1000), TI_C54X_INDIRECT_SINGLE_MOD0,
            (ti_c54x_address_access)UINT8_C(2)),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_status_and_zero(
        ti_c54x_indirect_single_word(
            UINT8_C(0), UINT16_C(0x1000),
            (ti_c54x_indirect_single_mode)-1,
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_status_and_zero(
        ti_c54x_indirect_single_word(
            UINT8_C(0), UINT16_C(0x1000), TI_C54X_INDIRECT_SINGLE_MOD0,
            (ti_c54x_address_access)-1),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
}

static void
test_unsupported_access(void)
{
    check_status_and_zero(
        ti_c54x_indirect_single_word(
            UINT8_C(2), UINT16_C(0x1000), TI_C54X_INDIRECT_SINGLE_MOD3,
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
}

static void
test_unresolved_boundaries(void)
{
    check_status_and_zero(
        ti_c54x_indirect_single_word(
            UINT8_C(1), UINT16_C(0), TI_C54X_INDIRECT_SINGLE_MOD1,
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    check_status_and_zero(
        ti_c54x_indirect_single_word(
            UINT8_C(1), UINT16_MAX, TI_C54X_INDIRECT_SINGLE_MOD2,
            TI_C54X_ADDRESS_ACCESS_WRITE),
        TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    check_status_and_zero(
        ti_c54x_indirect_single_word(
            UINT8_C(1), UINT16_MAX, TI_C54X_INDIRECT_SINGLE_MOD3,
            TI_C54X_ADDRESS_ACCESS_WRITE),
        TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
}

int
main(void)
{
    test_invalid_arguments();
    test_unsupported_access();
    test_unresolved_boundaries();
    puts("address_effect_contract: ok");
    return 0;
}
