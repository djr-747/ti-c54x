/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage for CMPT=0 signed MOD13 long-offset
 * planning across WORD16/LONG32 widths, READ/WRITE access, every AR index,
 * complete single-source dimensions, and an independent arithmetic oracle.
 * Does not own: production address semantics, CPU state, sequencing, bus
 * mapping, opcode decoding, test-harness implementation, or rejection policy.
 */

#include "address/indirect_long_offset.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static uint16_t
oracle_value_after(uint16_t ar_value, uint16_t long_offset_word)
{
    const int32_t signed_offset =
        (long_offset_word & UINT16_C(0x8000)) != UINT16_C(0)
            ? (int32_t)long_offset_word - INT32_C(0x10000)
            : (int32_t)long_offset_word;
    const int32_t sum = (int32_t)ar_value + signed_offset;

    TI_C54X_TEST_CHECK(sum >= INT32_C(0));
    TI_C54X_TEST_CHECK(sum <= INT32_C(0xffff));
    return (uint16_t)sum;
}

static ti_c54x_address_result
plan_case(uint8_t ar_index,
          uint16_t ar_value,
          uint16_t long_offset_word,
          ti_c54x_data_operand_width width,
          ti_c54x_address_access access)
{
    if (width == TI_C54X_DATA_OPERAND_WIDTH_WORD16) {
        return ti_c54x_indirect_long_offset_mod13_word16(
            ar_index, ar_value, long_offset_word, access);
    }
    return ti_c54x_indirect_long_offset_mod13_long32(
        ar_index, ar_value, long_offset_word, access);
}

static void
check_success(uint8_t ar_index,
              uint16_t ar_value,
              uint16_t long_offset_word,
              ti_c54x_data_operand_width width,
              ti_c54x_address_access access)
{
    const uint16_t expected_after =
        oracle_value_after(ar_value, long_offset_word);
    const ti_c54x_address_result result = plan_case(
        ar_index, ar_value, long_offset_word, width, access);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(result.effect.operand.width == width);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, expected_after);
    if (width == TI_C54X_DATA_OPERAND_WIDTH_WORD16) {
        TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word,
                               UINT16_C(0));
    } else {
        const uint16_t expected_pair =
            (expected_after & UINT16_C(1)) == UINT16_C(0)
                ? (uint16_t)(expected_after + UINT16_C(1))
                : (uint16_t)(expected_after - UINT16_C(1));
        TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word,
                               expected_pair);
    }
    TI_C54X_TEST_CHECK(result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET);
    TI_C54X_TEST_CHECK_U64(result.effect.source.value.ar_long_offset.index,
                           ar_index);
    TI_C54X_TEST_CHECK_U64(
        result.effect.source.value.ar_long_offset.modification,
        TI_C54X_INDIRECT_LONG_OFFSET_MOD13);
    TI_C54X_TEST_CHECK_U64(
        result.effect.source.value.ar_long_offset.value_before, ar_value);
    TI_C54X_TEST_CHECK_U64(
        result.effect.source.value.ar_long_offset.long_offset_word,
        long_offset_word);
    TI_C54X_TEST_CHECK(result.effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0);
    TI_C54X_TEST_CHECK(result.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS);
    TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index, ar_index);
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_before, ar_value);
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_after, expected_after);
    TI_C54X_TEST_CHECK(
        ti_c54x_indirect_long_offset_effect_is_valid(&result.effect));
}

/* Exhaust one architectural source dimension at a time; this intentionally
 * does not claim the full AR-by-offset Cartesian product. */
static void
test_complete_single_source_dimensions(void)
{
    static const ti_c54x_data_operand_width widths[] = {
        TI_C54X_DATA_OPERAND_WIDTH_WORD16,
        TI_C54X_DATA_OPERAND_WIDTH_LONG32};
    static const ti_c54x_address_access accesses[] = {
        TI_C54X_ADDRESS_ACCESS_READ, TI_C54X_ADDRESS_ACCESS_WRITE};
    uint32_t index_value;

    for (index_value = UINT32_C(0); index_value <= UINT32_C(7);
         ++index_value) {
        size_t width_index;
        for (width_index = 0; width_index < 2; ++width_index) {
            size_t access_index;
            for (access_index = 0; access_index < 2; ++access_index) {
                uint32_t ar_value;
                for (ar_value = UINT32_C(0);
                     ar_value <= UINT32_C(0xffff); ++ar_value) {
                    check_success((uint8_t)index_value, (uint16_t)ar_value,
                                  UINT16_C(0), widths[width_index],
                                  accesses[access_index]);
                }

                uint32_t offset_value;
                for (offset_value = UINT32_C(0);
                     offset_value <= UINT32_C(0xffff); ++offset_value) {
                    check_success((uint8_t)index_value, UINT16_C(0x8000),
                                  (uint16_t)offset_value, widths[width_index],
                                  accesses[access_index]);
                }
            }
        }
    }
}

static void
test_representative_cross_source_and_parity_vectors(void)
{
    static const uint16_t vectors[][2] = {
        {UINT16_C(0x1000), UINT16_C(0xffff)},
        {UINT16_C(0x1001), UINT16_C(0x0001)},
        {UINT16_C(0x7fff), UINT16_C(0x0001)},
        {UINT16_C(0x8000), UINT16_C(0x7fff)},
        {UINT16_C(0x8000), UINT16_C(0x8000)},
        {UINT16_C(0xfffe), UINT16_C(0xffff)},
        {UINT16_C(0x0001), UINT16_C(0x7fff)}};
    size_t vector_index;

    for (vector_index = 0; vector_index < sizeof(vectors) / sizeof(vectors[0]);
         ++vector_index) {
        check_success((uint8_t)(vector_index % 8), vectors[vector_index][0],
                      vectors[vector_index][1],
                      TI_C54X_DATA_OPERAND_WIDTH_WORD16,
                      TI_C54X_ADDRESS_ACCESS_READ);
        check_success((uint8_t)(7 - (vector_index % 8)),
                      vectors[vector_index][0], vectors[vector_index][1],
                      TI_C54X_DATA_OPERAND_WIDTH_LONG32,
                      TI_C54X_ADDRESS_ACCESS_WRITE);
    }
}

static void
test_exact_safe_boundaries(void)
{
    static const uint16_t cases[][2] = {
        {UINT16_C(0x0000), UINT16_C(0x0000)},
        {UINT16_C(0xffff), UINT16_C(0x0000)},
        {UINT16_C(0xfffe), UINT16_C(0x0001)},
        {UINT16_C(0x0001), UINT16_C(0xffff)},
        {UINT16_C(0x8000), UINT16_C(0x7fff)},
        {UINT16_C(0x8000), UINT16_C(0x8000)}};
    size_t case_index;

    for (case_index = 0; case_index < sizeof(cases) / sizeof(cases[0]);
         ++case_index) {
        check_success(UINT8_C(4), cases[case_index][0], cases[case_index][1],
                      TI_C54X_DATA_OPERAND_WIDTH_WORD16,
                      TI_C54X_ADDRESS_ACCESS_READ);
        check_success(UINT8_C(4), cases[case_index][0], cases[case_index][1],
                      TI_C54X_DATA_OPERAND_WIDTH_LONG32,
                      TI_C54X_ADDRESS_ACCESS_WRITE);
    }
}

int
main(void)
{
    test_complete_single_source_dimensions();
    test_representative_cross_source_and_parity_vectors();
    test_exact_safe_boundaries();
    puts("indirect_long_offset_mod13: ok");
    return 0;
}
