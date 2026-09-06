/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage for CMPT=0 single-operand WORD16
 * circular MOD8/MOD10 planning. It proves the strict BK-sized domain, EFB
 * alignment, fixed-step lower/upper wrapping, interior movement, BK=0's
 * canonical no-update result, every AR selector, and both access kinds.
 * Does not own: production address semantics, CPU state, sequencer reads or
 * writes, bus mapping, opcode decoding, timing, or rejection contracts.
 */

#include "address/indirect_circular.h"
#include "../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static void check_positive_case(uint8_t ar_index,
                                uint16_t ar_value,
                                uint16_t bk_value,
                                ti_c54x_indirect_circular_mode mode,
                                ti_c54x_address_access access,
                                uint16_t expected_after)
{
    const ti_c54x_address_result result = ti_c54x_indirect_circular_word16(
        ar_index, ar_value, bk_value, mode, access);
    const ti_c54x_address_source_ar_circular *source =
        &result.effect.source.value.ar_circular;

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, ar_value);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word, UINT16_C(0));
    TI_C54X_TEST_CHECK(result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR);
    TI_C54X_TEST_CHECK_U64(source->index, ar_index);
    TI_C54X_TEST_CHECK_U64(source->modification, (uint8_t)mode);
    TI_C54X_TEST_CHECK_U64(source->value_before, ar_value);
    TI_C54X_TEST_CHECK_U64(source->bk_value_before, bk_value);
    TI_C54X_TEST_CHECK(result.effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0);
    TI_C54X_TEST_CHECK(result.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
    TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index, ar_index);
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_before, ar_value);
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_after, expected_after);
    TI_C54X_TEST_CHECK(ti_c54x_indirect_circular_effect_is_valid(
        &result.effect));
}

static void test_fixed_step_domains_and_wraps(void)
{
    static const ti_c54x_address_access accesses[] = {
        TI_C54X_ADDRESS_ACCESS_READ, TI_C54X_ADDRESS_ACCESS_WRITE};
    static const ti_c54x_indirect_circular_mode modes[] = {
        TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_INDIRECT_CIRCULAR_MOD10};
    static const uint8_t selectors[] = {UINT8_C(0), UINT8_C(1), UINT8_C(2),
                                        UINT8_C(3), UINT8_C(4), UINT8_C(5),
                                        UINT8_C(6), UINT8_C(7)};
    size_t mode_index;
    size_t access_index;
    size_t selector_index;

    for (mode_index = 0U; mode_index < 2U; ++mode_index) {
        for (access_index = 0U; access_index < 2U; ++access_index) {
            for (selector_index = 0U; selector_index < 8U;
                 ++selector_index) {
                const uint8_t selector = selectors[selector_index];
                const ti_c54x_address_access access = accesses[access_index];
                const ti_c54x_indirect_circular_mode mode = modes[mode_index];
                const bool decrement =
                    mode == TI_C54X_INDIRECT_CIRCULAR_MOD8;

                /* BK=1 exercises the degenerate one-element buffer. */
                check_positive_case(selector, UINT16_C(0x1200), UINT16_C(1),
                                    mode, access, UINT16_C(0x1200));
                /* Power-of-two size: lower, upper, and interior movement. */
                check_positive_case(selector, UINT16_C(0x1230), UINT16_C(8),
                                    mode, access,
                                    decrement ? UINT16_C(0x1237)
                                              : UINT16_C(0x1231));
                check_positive_case(selector, UINT16_C(0x1237), UINT16_C(8),
                                    mode, access,
                                    decrement ? UINT16_C(0x1236)
                                              : UINT16_C(0x1230));
                check_positive_case(selector, UINT16_C(0x1233), UINT16_C(8),
                                    mode, access,
                                    decrement ? UINT16_C(0x1232)
                                              : UINT16_C(0x1234));
                /* Non-power-of-two size: strict index < BK is observable. */
                check_positive_case(selector, UINT16_C(0x1240), UINT16_C(5),
                                    mode, access,
                                    decrement ? UINT16_C(0x1244)
                                              : UINT16_C(0x1241));
                check_positive_case(selector, UINT16_C(0x1244), UINT16_C(5),
                                    mode, access,
                                    decrement ? UINT16_C(0x1243)
                                              : UINT16_C(0x1240));
                check_positive_case(selector, UINT16_C(0x1242), UINT16_C(5),
                                    mode, access,
                                    decrement ? UINT16_C(0x1241)
                                              : UINT16_C(0x1243));
            }
        }
    }
}

static void test_sixteen_bit_alignment_case(void)
{
    /* BK=0x8000 requires N=16 because the comparison is strictly >. */
    check_positive_case(UINT8_C(0), UINT16_C(0x0000), UINT16_C(0x8000),
                        TI_C54X_INDIRECT_CIRCULAR_MOD8,
                        TI_C54X_ADDRESS_ACCESS_READ, UINT16_C(0x7fff));
    check_positive_case(UINT8_C(7), UINT16_C(0x7fff), UINT16_C(0x8000),
                        TI_C54X_INDIRECT_CIRCULAR_MOD10,
                        TI_C54X_ADDRESS_ACCESS_WRITE, UINT16_C(0x0000));
    /* The largest positive 16-bit BK also uses N=16. */
    check_positive_case(UINT8_C(2), UINT16_C(0x0000), UINT16_C(0xffff),
                        TI_C54X_INDIRECT_CIRCULAR_MOD8,
                        TI_C54X_ADDRESS_ACCESS_WRITE, UINT16_C(0xfffe));
    check_positive_case(UINT8_C(5), UINT16_C(0xfffe), UINT16_C(0xffff),
                        TI_C54X_INDIRECT_CIRCULAR_MOD10,
                        TI_C54X_ADDRESS_ACCESS_READ, UINT16_C(0x0000));
}

static void test_bk_zero_is_canonical_no_update(void)
{
    static const ti_c54x_indirect_circular_mode modes[] = {
        TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_INDIRECT_CIRCULAR_MOD10};
    static const ti_c54x_address_access accesses[] = {
        TI_C54X_ADDRESS_ACCESS_READ, TI_C54X_ADDRESS_ACCESS_WRITE};
    size_t mode_index;
    size_t access_index;
    uint8_t selector;

    for (mode_index = 0U; mode_index < 2U; ++mode_index) {
        for (access_index = 0U; access_index < 2U; ++access_index) {
            for (selector = UINT8_C(0); selector <= UINT8_C(7); ++selector) {
                const uint16_t old_ar =
                    (uint16_t)(UINT16_C(0xa500) + selector);
                const ti_c54x_address_result result =
                    ti_c54x_indirect_circular_word16(
                        selector, old_ar, UINT16_C(0), modes[mode_index],
                        accesses[access_index]);
                const ti_c54x_address_source_ar_circular *source =
                    &result.effect.source.value.ar_circular;

                TI_C54X_TEST_CHECK(result.status ==
                                   TI_C54X_ADDRESS_STATUS_OK);
                TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word,
                                       old_ar);
                TI_C54X_TEST_CHECK(result.effect.source.kind ==
                                   TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR);
                TI_C54X_TEST_CHECK_U64(source->index, selector);
                TI_C54X_TEST_CHECK_U64(source->modification,
                                       (uint8_t)modes[mode_index]);
                TI_C54X_TEST_CHECK_U64(source->value_before, old_ar);
                TI_C54X_TEST_CHECK_U64(source->bk_value_before, UINT16_C(0));
                TI_C54X_TEST_CHECK(result.effect.update.target ==
                                   TI_C54X_ADDRESS_UPDATE_NONE);
                TI_C54X_TEST_CHECK(result.effect.update.phase ==
                                   TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
                TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index,
                                       UINT8_C(0));
                TI_C54X_TEST_CHECK_U64(result.effect.update.value_before,
                                       UINT16_C(0));
                TI_C54X_TEST_CHECK_U64(result.effect.update.value_after,
                                       UINT16_C(0));
                TI_C54X_TEST_CHECK(ti_c54x_indirect_circular_effect_is_valid(
                    &result.effect));
            }
        }
    }
}

int main(void)
{
    test_fixed_step_domains_and_wraps();
    test_sixteen_bit_alignment_case();
    test_bk_zero_is_canonical_no_update();
    puts("indirect_circular_word16: ok");
    return 0;
}
