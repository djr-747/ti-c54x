/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proof of circular planner argument
 * precedence, strict initial-buffer rejection, copied source snapshots,
 * canonical BK=0 effects, and intrinsic-effect validation. Every rejected
 * planner result must carry an all-zero effect.
 * Does not own: TI source evidence, CPU state, sequencer dependency order,
 * architectural mutation, bus mapping, opcode decoding, or valid-domain
 * semantic coverage.
 */

#include "address/indirect_circular.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static void check_zero_effect(const ti_c54x_address_effect *effect)
{
    const unsigned char *bytes = (const unsigned char *)effect;
    size_t index;

    for (index = 0U; index < sizeof(*effect); ++index) {
        TI_C54X_TEST_CHECK_U64(bytes[index], 0U);
    }
}

static void check_rejected(ti_c54x_address_result result,
                           ti_c54x_address_status expected_status)
{
    TI_C54X_TEST_CHECK(result.status == expected_status);
    check_zero_effect(&result.effect);
}

static void test_argument_precedence(void)
{
    check_rejected(
        ti_c54x_indirect_circular_word16(
            UINT8_C(8), UINT16_C(0x1200), UINT16_C(8),
            TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_circular_word16(
            UINT8_C(1), UINT16_C(0x1200), UINT16_C(8),
            (ti_c54x_indirect_circular_mode)UINT8_C(9),
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_circular_word16(
            UINT8_C(1), UINT16_C(0x1200), UINT16_C(8),
            TI_C54X_INDIRECT_CIRCULAR_MOD10,
            (ti_c54x_address_access)UINT8_C(2)),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
}

static void test_initial_address_must_be_inside_buffer(void)
{
    /* BK=5 aligns to an eight-word EFB; index 5 and 7 are outside. */
    check_rejected(
        ti_c54x_indirect_circular_word16(
            UINT8_C(2), UINT16_C(0x1245), UINT16_C(5),
            TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    check_rejected(
        ti_c54x_indirect_circular_word16(
            UINT8_C(2), UINT16_C(0xffff), UINT16_C(5),
            TI_C54X_INDIRECT_CIRCULAR_MOD10, TI_C54X_ADDRESS_ACCESS_WRITE),
        TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
}

static void test_source_snapshots_are_copied(void)
{
    uint8_t ar_index = UINT8_C(3);
    uint16_t ar_value = UINT16_C(0x1242);
    uint16_t bk_value = UINT16_C(5);
    ti_c54x_address_result result = ti_c54x_indirect_circular_word16(
        ar_index, ar_value, bk_value, TI_C54X_INDIRECT_CIRCULAR_MOD10,
        TI_C54X_ADDRESS_ACCESS_WRITE);
    const ti_c54x_address_source_ar_circular *source =
        &result.effect.source.value.ar_circular;

    ar_index = UINT8_C(1);
    ar_value = UINT16_C(0);
    bk_value = UINT16_C(0);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(source->index == UINT8_C(3));
    TI_C54X_TEST_CHECK(source->modification ==
                       (uint8_t)TI_C54X_INDIRECT_CIRCULAR_MOD10);
    TI_C54X_TEST_CHECK(source->value_before == UINT16_C(0x1242));
    TI_C54X_TEST_CHECK(source->bk_value_before == UINT16_C(5));
    TI_C54X_TEST_CHECK(result.effect.operand.first_word == UINT16_C(0x1242));
    TI_C54X_TEST_CHECK(result.effect.update.value_after == UINT16_C(0x1243));
}

static ti_c54x_address_result valid_positive_result(void)
{
    return ti_c54x_indirect_circular_word16(
        UINT8_C(3), UINT16_C(0x1242), UINT16_C(5),
        TI_C54X_INDIRECT_CIRCULAR_MOD10, TI_C54X_ADDRESS_ACCESS_READ);
}

static ti_c54x_address_result valid_zero_result(void)
{
    return ti_c54x_indirect_circular_word16(
        UINT8_C(3), UINT16_C(0x5502), UINT16_C(0),
        TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_ADDRESS_ACCESS_WRITE);
}

static void test_intrinsic_validator_rejects_malformed_effects(void)
{
    const ti_c54x_address_result valid = valid_positive_result();
    ti_c54x_address_effect malformed;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_indirect_circular_effect_is_valid(
        &valid.effect));
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(NULL));

    malformed = valid.effect;
    malformed.operand.width = TI_C54X_DATA_OPERAND_WIDTH_LONG32;
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.operand.first_word = UINT16_C(0x1243);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.source.kind = TI_C54X_ADDRESS_SOURCE_AR;
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.source.value.ar_circular.index = UINT8_C(8);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.source.value.ar_circular.modification = UINT8_C(9);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.source.value.ar_circular.bk_value_before = UINT16_C(0);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.state_policy = TI_C54X_ADDRESS_STATE_POLICY_NONE;
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.update.target = TI_C54X_ADDRESS_UPDATE_NONE;
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.update.phase = TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS;
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.update.ar_index = UINT8_C(4);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.update.value_before = UINT16_C(0x1243);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));
    malformed = valid.effect;
    malformed.update.value_after = UINT16_C(0x1244);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));

    /* An in-range source cannot smuggle a non-circular update through. */
    malformed = valid.effect;
    malformed.source.value.ar_circular.value_before = UINT16_C(0x1245);
    malformed.operand.first_word = UINT16_C(0x1245);
    malformed.update.value_before = UINT16_C(0x1245);
    malformed.update.value_after = UINT16_C(0x1240);
    TI_C54X_TEST_CHECK(!ti_c54x_indirect_circular_effect_is_valid(
        &malformed));
}

static void test_bk_zero_validator_shape(void)
{
    const ti_c54x_address_result result = valid_zero_result();

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_indirect_circular_effect_is_valid(
        &result.effect));
    TI_C54X_TEST_CHECK(result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR);
    TI_C54X_TEST_CHECK(result.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
}

int main(void)
{
    test_argument_precedence();
    test_initial_address_must_be_inside_buffer();
    test_source_snapshots_are_copied();
    test_intrinsic_validator_rejects_malformed_effects();
    test_bk_zero_validator_shape();
    puts("indirect_circular_contract: ok");
    return 0;
}
