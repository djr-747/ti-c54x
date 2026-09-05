/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for MOD13 long-offset
 * rejection precedence, exact unresolved boundaries, copied source values,
 * all-zero failed effects, and intrinsic effect-shape validation.
 * Does not own: TI source interpretation, broad valid-domain coverage, CPU
 * state, sequencing, bus mapping, opcode decoding, or test-harness behavior.
 */

#include "address/indirect_long_offset.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static void
check_zero_effect(const ti_c54x_address_effect *effect)
{
    /* Do not inspect an inactive source union arm after SOURCE_NONE. */
    TI_C54X_TEST_CHECK(effect->operand.width ==
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
check_rejected(ti_c54x_address_result result, ti_c54x_address_status status)
{
    TI_C54X_TEST_CHECK(result.status == status);
    check_zero_effect(&result.effect);
}

static void
test_invalid_arguments_precede_boundary(void)
{
    check_rejected(
        ti_c54x_indirect_long_offset_mod13_word16(
            UINT8_C(8), UINT16_MAX, UINT16_C(1),
            TI_C54X_ADDRESS_ACCESS_READ),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_long_offset_mod13_long32(
            UINT8_MAX, UINT16_C(0), UINT16_C(0xffff),
            TI_C54X_ADDRESS_ACCESS_WRITE),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_long_offset_mod13_word16(
            UINT8_C(2), UINT16_MAX, UINT16_C(1),
            (ti_c54x_address_access)UINT8_C(2)),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_rejected(
        ti_c54x_indirect_long_offset_mod13_long32(
            UINT8_C(2), UINT16_C(0), UINT16_C(0xffff),
            (ti_c54x_address_access)-1),
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
}

static void
test_exact_unsafe_crossings(void)
{
    static const struct {
        uint16_t ar_value;
        uint16_t offset_word;
    } cases[] = {
        {UINT16_C(0xffff), UINT16_C(0x0001)}, /* +1 crosses 0xffff */
        {UINT16_C(0x0000), UINT16_C(0xffff)}, /* -1 crosses zero */
        {UINT16_C(0x8001), UINT16_C(0x7fff)}, /* +32767 crosses max */
        {UINT16_C(0x7fff), UINT16_C(0x8000)}}; /* -32768 crosses zero */
    size_t case_index;

    for (case_index = 0; case_index < sizeof(cases) / sizeof(cases[0]);
         ++case_index) {
        check_rejected(
            ti_c54x_indirect_long_offset_mod13_word16(
                UINT8_C(0), cases[case_index].ar_value,
                cases[case_index].offset_word,
                TI_C54X_ADDRESS_ACCESS_READ),
            TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
        check_rejected(
            ti_c54x_indirect_long_offset_mod13_long32(
                UINT8_C(7), cases[case_index].ar_value,
                cases[case_index].offset_word,
                TI_C54X_ADDRESS_ACCESS_WRITE),
            TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    }
}

static void
test_copied_source_lifetime(void)
{
    uint8_t ar_index = UINT8_C(5);
    uint16_t ar_value = UINT16_C(0x9000);
    uint16_t offset_word = UINT16_C(0xfff0);
    const ti_c54x_address_result result =
        ti_c54x_indirect_long_offset_mod13_long32(
            ar_index, ar_value, offset_word, TI_C54X_ADDRESS_ACCESS_WRITE);

    ar_index = UINT8_C(0);
    ar_value = UINT16_C(0);
    offset_word = UINT16_C(0);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(result.effect.source.value.ar_long_offset.index,
                           UINT8_C(5));
    TI_C54X_TEST_CHECK_U64(
        result.effect.source.value.ar_long_offset.modification,
        TI_C54X_INDIRECT_LONG_OFFSET_MOD13);
    TI_C54X_TEST_CHECK_U64(
        result.effect.source.value.ar_long_offset.value_before,
        UINT16_C(0x9000));
    TI_C54X_TEST_CHECK_U64(
        result.effect.source.value.ar_long_offset.long_offset_word,
        UINT16_C(0xfff0));
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word,
                           UINT16_C(0x8ff0));
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_after,
                           UINT16_C(0x8ff0));
}

static ti_c54x_address_result
valid_word(void)
{
    return ti_c54x_indirect_long_offset_mod13_word16(
        UINT8_C(3), UINT16_C(0x4000), UINT16_C(0xffff),
        TI_C54X_ADDRESS_ACCESS_READ);
}

static ti_c54x_address_result
valid_long32(void)
{
    return ti_c54x_indirect_long_offset_mod13_long32(
        UINT8_C(6), UINT16_C(0x4001), UINT16_C(0x7fff),
        TI_C54X_ADDRESS_ACCESS_WRITE);
}

static void
test_validator_accepts_planner_results(void)
{
    ti_c54x_address_result word = valid_word();
    ti_c54x_address_result long32 = valid_long32();

    TI_C54X_TEST_CHECK(word.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(long32.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(
        ti_c54x_indirect_long_offset_effect_is_valid(&word.effect));
    TI_C54X_TEST_CHECK(
        ti_c54x_indirect_long_offset_effect_is_valid(&long32.effect));
}

static void
test_validator_rejects_malformed_effects(void)
{
    ti_c54x_address_result result = valid_word();

    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(NULL));
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_AR;
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));

    result = valid_word();
    result.effect.source.value.ar_long_offset.modification = UINT8_C(12);
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
    result = valid_word();
    result.effect.source.value.ar_long_offset.index = UINT8_C(8);
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
    result = valid_word();
    result.effect.operand.width = TI_C54X_DATA_OPERAND_WIDTH_INVALID;
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
    result = valid_long32();
    result.effect.operand.second_word ^= UINT16_C(1);
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
    result = valid_word();
    result.effect.operand.first_word ^= UINT16_C(1);
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
    result = valid_word();
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_NONE;
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
    result = valid_word();
    result.effect.update.target = TI_C54X_ADDRESS_UPDATE_NONE;
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
    result = valid_word();
    result.effect.update.phase = TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS;
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
    result = valid_word();
    result.effect.update.ar_index = UINT8_C(4);
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
    result = valid_word();
    result.effect.update.value_before ^= UINT16_C(1);
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
    result = valid_word();
    result.effect.update.value_after ^= UINT16_C(1);
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
    result = valid_word();
    result.effect.source.value.ar_long_offset.long_offset_word =
        UINT16_C(0x0001);
    TI_C54X_TEST_CHECK(
        !ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
}

int
main(void)
{
    test_invalid_arguments_precede_boundary();
    test_exact_unsafe_crossings();
    test_copied_source_lifetime();
    test_validator_accepts_planner_results();
    test_validator_rejects_malformed_effects();
    puts("indirect_long_offset_contract: ok");
    return 0;
}
