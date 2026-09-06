/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY tests for reverse-carry planner
 * validation, status precedence, exact all-zero failed effects, copied source
 * lifetime, and intrinsic-effect malformation rejection.
 * Does not own: TI architectural evidence, CPU state, bus mapping, opcode
 * decoding, valid-domain semantic coverage, or test-harness implementation.
 */

#include "address/indirect_reverse_carry.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static void
check_zero_effect(const ti_c54x_address_effect *effect)
{
    const unsigned char *bytes = (const unsigned char *)effect;
    size_t index;

    for (index = 0U; index < sizeof(*effect); ++index) {
        TI_C54X_TEST_CHECK_U64(bytes[index], 0U);
    }
}

static void
check_rejected(ti_c54x_address_result result,
               ti_c54x_address_status expected_status)
{
    TI_C54X_TEST_CHECK(result.status == expected_status);
    check_zero_effect(&result.effect);
}

static void
test_discriminant_precedence_and_ar0_alias(void)
{
    check_rejected(
        ti_c54x_indirect_reverse_carry_word16(
            UINT8_C(8), UINT16_C(0x60), UINT16_C(0),
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_reverse_carry_word16(
            UINT8_C(1), UINT16_C(0x60), UINT16_C(0x08),
            (ti_c54x_indirect_reverse_carry_mode)UINT8_C(5),
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_reverse_carry_word16(
            UINT8_C(1), UINT16_C(0x60), UINT16_C(0x08),
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,
            (ti_c54x_address_access)UINT8_C(2)),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);

    check_rejected(
        ti_c54x_indirect_reverse_carry_word16(
            UINT8_C(1), UINT16_C(0x60), UINT16_C(0),
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_reverse_carry_word16(
            UINT8_C(1), UINT16_C(0x60), UINT16_C(3),
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,
            TI_C54X_ADDRESS_ACCESS_WRITE),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_reverse_carry_word16(
            UINT8_C(1), UINT16_C(0x60), UINT16_C(0x0180),
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);

    /* AR0 syntax is admitted, but its same-register ordering is unresolved. */
    check_rejected(
        ti_c54x_indirect_reverse_carry_word16(
            UINT8_C(0), UINT16_C(0x60), UINT16_C(0x08),
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
    /* Invalid AR0 remains an invalid source before alias classification. */
    check_rejected(
        ti_c54x_indirect_reverse_carry_word16(
            UINT8_C(0), UINT16_C(0x60), UINT16_C(0),
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
}

static void
test_exact_escape_boundaries(void)
{
    check_rejected(
        ti_c54x_indirect_reverse_carry_word16(
            UINT8_C(3), UINT16_C(0xffff), UINT16_C(1),
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    check_rejected(
        ti_c54x_indirect_reverse_carry_word16(
            UINT8_C(3), UINT16_C(0), UINT16_C(1),
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
            TI_C54X_ADDRESS_ACCESS_WRITE),
        TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    check_rejected(
        ti_c54x_indirect_reverse_carry_word16(
            UINT8_C(3), UINT16_C(0x006f), UINT16_C(8),
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    check_rejected(
        ti_c54x_indirect_reverse_carry_word16(
            UINT8_C(3), UINT16_C(0x0060), UINT16_C(8),
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
            TI_C54X_ADDRESS_ACCESS_WRITE),
        TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
}

static void
test_source_snapshots_are_copied(void)
{
    uint8_t ar_index = UINT8_C(3);
    uint16_t ar_value = UINT16_C(0x60);
    uint16_t ar0_value = UINT16_C(0x08);
    ti_c54x_address_result result =
        ti_c54x_indirect_reverse_carry_word16(
            ar_index, ar_value, ar0_value,
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,
            TI_C54X_ADDRESS_ACCESS_WRITE);

    ar_index = UINT8_C(1);
    ar_value = UINT16_C(0);
    ar0_value = UINT16_C(0);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(result.effect.source.value.ar_reverse_carry.index,
                           UINT8_C(3));
    TI_C54X_TEST_CHECK_U64(
        result.effect.source.value.ar_reverse_carry.value_before,
        UINT16_C(0x60));
    TI_C54X_TEST_CHECK_U64(
        result.effect.source.value.ar_reverse_carry.ar0_value_before,
        UINT16_C(0x08));
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, UINT16_C(0x60));
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_after, UINT16_C(0x68));
}

static ti_c54x_address_result
valid_result(void)
{
    return ti_c54x_indirect_reverse_carry_word16(
        UINT8_C(3), UINT16_C(0x60), UINT16_C(0x08),
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,
        TI_C54X_ADDRESS_ACCESS_READ);
}

static void
test_validator_rejects_malformed_effects(void)
{
    const ti_c54x_address_result valid = valid_result();
    ti_c54x_address_effect malformed;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_indirect_reverse_carry_effect_is_valid(
        &valid.effect));
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(NULL));

    malformed = valid.effect;
    malformed.operand.width = TI_C54X_DATA_OPERAND_WIDTH_LONG32;
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.operand.second_word = UINT16_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.source.kind = TI_C54X_ADDRESS_SOURCE_AR;
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.source.value.ar_reverse_carry.index = UINT8_C(0);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.source.value.ar_reverse_carry.ar0_value_before = UINT16_C(3);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.source.value.ar_reverse_carry.modification = UINT8_C(5);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.state_policy = TI_C54X_ADDRESS_STATE_POLICY_NONE;
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.update.target = TI_C54X_ADDRESS_UPDATE_NONE;
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.update.phase = TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS;
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.update.ar_index = UINT8_C(4);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.update.value_before = UINT16_C(0x61);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.operand.first_word = UINT16_C(0x61);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.update.value_after = UINT16_C(0x69);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));

    /* An otherwise shaped effect cannot smuggle an escaping update through
     * the validator. */
    malformed = valid.effect;
    malformed.source.value.ar_reverse_carry.value_before = UINT16_C(0xffff);
    malformed.source.value.ar_reverse_carry.ar0_value_before = UINT16_C(1);
    malformed.operand.first_word = UINT16_C(0xffff);
    malformed.update.value_before = UINT16_C(0xffff);
    malformed.update.value_after = UINT16_C(0);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_reverse_carry_effect_is_valid(
        &malformed));
}

int
main(void)
{
    test_discriminant_precedence_and_ar0_alias();
    test_exact_escape_boundaries();
    test_source_snapshots_are_copied();
    test_validator_rejects_malformed_effects();
    puts("indirect_reverse_carry_contract: ok");
    return 0;
}
