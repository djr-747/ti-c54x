/*
 * Module contract
 *
 * Owns: executable COMMON_TI tests for CMPT=0 AR0-indexed WORD16 MOD5/MOD6
 * planning, including independent arithmetic and copied-source snapshots.
 * Does not own: production address semantics, CPU state, bus mapping, opcode
 * decoding, test harness implementation, or rejected-input contract checks.
 */

#include "address/indirect_indexed.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static uint16_t
oracle_value_after(uint16_t ar_value,
                   uint16_t ar0_value,
                   ti_c54x_indirect_indexed_mode mode)
{
    if (mode == TI_C54X_INDIRECT_INDEXED_MOD5) {
        TI_C54X_TEST_CHECK(ar_value >= ar0_value);
        return (uint16_t)(ar_value - ar0_value);
    }

    {
        const uint32_t sum = (uint32_t)ar_value + (uint32_t)ar0_value;
        TI_C54X_TEST_CHECK(sum <= UINT32_C(0xffff));
        return (uint16_t)sum;
    }
}

static void
check_case(uint8_t ar_index,
           uint16_t ar_value,
           uint16_t ar0_value,
           ti_c54x_indirect_indexed_mode mode,
           ti_c54x_address_access access)
{
    const uint16_t expected_after =
        oracle_value_after(ar_value, ar0_value, mode);
    const ti_c54x_address_result result = ti_c54x_indirect_indexed_word(
        ar_index, ar_value, ar0_value, mode, access);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, ar_value);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word, UINT16_C(0));
    TI_C54X_TEST_CHECK(result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR_INDEXED);
    TI_C54X_TEST_CHECK_U64(result.effect.source.value.ar_indexed.index,
                           ar_index);
    TI_C54X_TEST_CHECK_U64(
        result.effect.source.value.ar_indexed.value_before, ar_value);
    TI_C54X_TEST_CHECK_U64(
        result.effect.source.value.ar_indexed.ar0_value_before, ar0_value);
    TI_C54X_TEST_CHECK(
        result.effect.state_policy ==
        TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0);
    TI_C54X_TEST_CHECK(result.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
    TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index, ar_index);
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_before, ar_value);
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_after, expected_after);
}

/* For each mode, sweep one complete 16-bit source dimension while choosing
 * the opposite operand so every case remains inside the admitted domain. */
static void
test_full_single_operand_dimensions(void)
{
    uint32_t index_value;
    for (index_value = UINT32_C(1); index_value <= UINT32_C(7);
         ++index_value) {
        uint32_t ar_value;
        for (ar_value = UINT32_C(0); ar_value <= UINT32_C(0xffff);
             ++ar_value) {
            check_case((uint8_t)index_value, (uint16_t)ar_value,
                       UINT16_C(0x0000), TI_C54X_INDIRECT_INDEXED_MOD5,
                       TI_C54X_ADDRESS_ACCESS_READ);
            check_case((uint8_t)index_value, (uint16_t)ar_value,
                       UINT16_C(0x0000), TI_C54X_INDIRECT_INDEXED_MOD6,
                       TI_C54X_ADDRESS_ACCESS_WRITE);
        }

        uint32_t ar0_value;
        for (ar0_value = UINT32_C(0); ar0_value <= UINT32_C(0xffff);
             ++ar0_value) {
            check_case((uint8_t)index_value, UINT16_C(0xffff),
                       (uint16_t)ar0_value,
                       TI_C54X_INDIRECT_INDEXED_MOD5,
                       TI_C54X_ADDRESS_ACCESS_WRITE);
            check_case((uint8_t)index_value, UINT16_C(0x0000),
                       (uint16_t)ar0_value,
                       TI_C54X_INDIRECT_INDEXED_MOD6,
                       TI_C54X_ADDRESS_ACCESS_READ);
        }
    }
}

static void
test_representative_mode_access_matrix(void)
{
    static const uint16_t values[] = {UINT16_C(0),    UINT16_C(1),
                                      UINT16_C(0x1234), UINT16_C(0x7fff),
                                      UINT16_C(0xfffe), UINT16_C(0xffff)};
    uint32_t index_value;

    for (index_value = UINT32_C(1); index_value <= UINT32_C(7);
         ++index_value) {
        size_t ar_value_index;
        for (ar_value_index = 0;
             ar_value_index < sizeof(values) / sizeof(values[0]);
             ++ar_value_index) {
            size_t ar0_value_index;
            for (ar0_value_index = 0;
                 ar0_value_index < sizeof(values) / sizeof(values[0]);
                 ++ar0_value_index) {
                const uint16_t ar_value = values[ar_value_index];
                const uint16_t ar0_value = values[ar0_value_index];
                if (ar_value >= ar0_value) {
                    check_case((uint8_t)index_value, ar_value, ar0_value,
                               TI_C54X_INDIRECT_INDEXED_MOD5,
                               TI_C54X_ADDRESS_ACCESS_READ);
                    check_case((uint8_t)index_value, ar_value, ar0_value,
                               TI_C54X_INDIRECT_INDEXED_MOD5,
                               TI_C54X_ADDRESS_ACCESS_WRITE);
                }
                if ((uint32_t)ar_value + (uint32_t)ar0_value <=
                    UINT32_C(0xffff)) {
                    check_case((uint8_t)index_value, ar_value, ar0_value,
                               TI_C54X_INDIRECT_INDEXED_MOD6,
                               TI_C54X_ADDRESS_ACCESS_READ);
                    check_case((uint8_t)index_value, ar_value, ar0_value,
                               TI_C54X_INDIRECT_INDEXED_MOD6,
                               TI_C54X_ADDRESS_ACCESS_WRITE);
                }
            }
        }
    }
}

static void
test_exact_admitted_boundaries(void)
{
    const uint8_t index = UINT8_C(4);
    const uint16_t ar0 = UINT16_C(0x1357);
    const ti_c54x_address_access accesses[] = {
        TI_C54X_ADDRESS_ACCESS_READ, TI_C54X_ADDRESS_ACCESS_WRITE};
    size_t access_index;

    for (access_index = 0; access_index < 2; ++access_index) {
        const ti_c54x_address_access access = accesses[access_index];
        check_case(index, ar0, ar0, TI_C54X_INDIRECT_INDEXED_MOD5, access);
        check_case(index, UINT16_MAX - ar0, ar0,
                   TI_C54X_INDIRECT_INDEXED_MOD6, access);
    }
}

int
main(void)
{
    test_full_single_operand_dimensions();
    test_representative_mode_access_matrix();
    test_exact_admitted_boundaries();
    puts("indirect_indexed_word: ok");
    return 0;
}
