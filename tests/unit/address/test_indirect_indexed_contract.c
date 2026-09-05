/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY tests for indexed planner rejection,
 * exact all-zero failed effects, and copied-source ownership.
 * Does not own: TI architectural evidence, CPU state, bus mapping, opcode
 * decoding, test harness implementation, or valid-domain semantic coverage.
 */

#include "address/indirect_indexed.h"
#include "../../support/test_harness.h"

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
    TI_C54X_TEST_CHECK(effect->update.target == TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
    TI_C54X_TEST_CHECK_U64(effect->update.ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(effect->update.value_before, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(effect->update.value_after, UINT16_C(0));
}

static void
check_rejected(ti_c54x_address_result result, ti_c54x_address_status status)
{
    TI_C54X_TEST_CHECK(result.status == status);
    check_zero_effect(&result.effect);
}

static void
test_invalid_arguments_and_precedence(void)
{
    check_rejected(
        ti_c54x_indirect_indexed_word(
            UINT8_C(8), UINT16_C(0x1000), UINT16_C(0x20),
            TI_C54X_INDIRECT_INDEXED_MOD5, TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_indexed_word(
            UINT8_MAX, UINT16_C(0x1000), UINT16_C(0x20),
            TI_C54X_INDIRECT_INDEXED_MOD6, TI_C54X_ADDRESS_ACCESS_WRITE),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_indexed_word(
            UINT8_C(1), UINT16_C(0x1000), UINT16_C(0x20),
            (ti_c54x_indirect_indexed_mode)UINT8_C(4),
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_indexed_word(
            UINT8_C(1), UINT16_C(0x1000), UINT16_C(0x20),
            (ti_c54x_indirect_indexed_mode)-1,
            TI_C54X_ADDRESS_ACCESS_WRITE),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_indexed_word(
            UINT8_C(1), UINT16_C(0x1000), UINT16_C(0x20),
            TI_C54X_INDIRECT_INDEXED_MOD5,
            (ti_c54x_address_access)UINT8_C(2)),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    /* Invalid mode/access still report INVALID_ARGUMENT before AR0 alias. */
    check_rejected(
        ti_c54x_indirect_indexed_word(
            UINT8_C(0), UINT16_C(0x1000), UINT16_C(0x20),
            (ti_c54x_indirect_indexed_mode)UINT8_C(4),
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_indexed_word(
            UINT8_C(0), UINT16_C(0x1000), UINT16_C(0x20),
            TI_C54X_INDIRECT_INDEXED_MOD5,
            (ti_c54x_address_access)-1),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
}

static void
test_selected_ar0_is_unsupported(void)
{
    check_rejected(
        ti_c54x_indirect_indexed_word(
            UINT8_C(0), UINT16_C(0x1000), UINT16_C(0x20),
            TI_C54X_INDIRECT_INDEXED_MOD5, TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
    check_rejected(
        ti_c54x_indirect_indexed_word(
            UINT8_C(0), UINT16_C(0x1000), UINT16_C(0x20),
            TI_C54X_INDIRECT_INDEXED_MOD6, TI_C54X_ADDRESS_ACCESS_WRITE),
        TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
}

static void
test_unresolved_boundaries(void)
{
    check_rejected(
        ti_c54x_indirect_indexed_word(
            UINT8_C(7), UINT16_C(0x1233), UINT16_C(0x1234),
            TI_C54X_INDIRECT_INDEXED_MOD5, TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    check_rejected(
        ti_c54x_indirect_indexed_word(
            UINT8_C(7), UINT16_C(0xfffe), UINT16_C(0x0002),
            TI_C54X_INDIRECT_INDEXED_MOD6, TI_C54X_ADDRESS_ACCESS_WRITE),
        TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
}

static void
test_source_snapshots_are_copied(void)
{
    uint8_t index = UINT8_C(3);
    uint16_t ar_value = UINT16_C(0x9000);
    uint16_t ar0_value = UINT16_C(0x0123);
    const ti_c54x_address_result result = ti_c54x_indirect_indexed_word(
        index, ar_value, ar0_value, TI_C54X_INDIRECT_INDEXED_MOD5,
        TI_C54X_ADDRESS_ACCESS_WRITE);

    index = UINT8_C(1);
    ar_value = UINT16_C(0);
    ar0_value = UINT16_C(0);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR_INDEXED);
    TI_C54X_TEST_CHECK_U64(result.effect.source.value.ar_indexed.index,
                           UINT8_C(3));
    TI_C54X_TEST_CHECK_U64(result.effect.source.value.ar_indexed.value_before,
                           UINT16_C(0x9000));
    TI_C54X_TEST_CHECK_U64(
        result.effect.source.value.ar_indexed.ar0_value_before,
        UINT16_C(0x0123));
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word,
                           UINT16_C(0x9000));
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_after,
                           UINT16_C(0x8edd));
}

int
main(void)
{
    test_invalid_arguments_and_precedence();
    test_selected_ar0_is_unsupported();
    test_unresolved_boundaries();
    test_source_snapshots_are_copied();
    puts("indirect_indexed_contract: ok");
    return 0;
}
