/*
 * Module contract
 *
 * Owns: executable COMMON_TI tests for CMPT=0 WORD16 reverse-carry MOD4/MOD7
 * planning, including the TI worked sequence, both access kinds, every
 * selected AR1..AR7, and old-address/POST_ACCESS effect shape.
 * Does not own: production address semantics, CPU state, bus mapping, opcode
 * decoding, rejected-input contracts, or test-harness implementation.
 */

#include "address/indirect_reverse_carry.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static void
check_case(uint8_t ar_index,
           uint16_t ar_value,
           uint16_t ar0_value,
           ti_c54x_indirect_reverse_carry_mode mode,
           ti_c54x_address_access access,
           uint16_t expected_after)
{
    const ti_c54x_address_result result =
        ti_c54x_indirect_reverse_carry_word16(
            ar_index, ar_value, ar0_value, mode, access);
    const ti_c54x_address_source_ar_reverse_carry *source =
        &result.effect.source.value.ar_reverse_carry;

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, ar_value);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word, UINT16_C(0));
    TI_C54X_TEST_CHECK(result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY);
    TI_C54X_TEST_CHECK_U64(source->index, ar_index);
    TI_C54X_TEST_CHECK_U64(source->modification, (uint8_t)mode);
    TI_C54X_TEST_CHECK_U64(source->value_before, ar_value);
    TI_C54X_TEST_CHECK_U64(source->ar0_value_before, ar0_value);
    TI_C54X_TEST_CHECK(result.effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0);
    TI_C54X_TEST_CHECK(result.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
    TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index, ar_index);
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_before, ar_value);
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_after, expected_after);
    TI_C54X_TEST_CHECK(ti_c54x_indirect_reverse_carry_effect_is_valid(
        &result.effect));
}

static void
test_ti_mod7_sequence_for_all_selected_registers_and_accesses(void)
{
    static const uint16_t sequence[] = {
        UINT16_C(0x60), UINT16_C(0x68), UINT16_C(0x64), UINT16_C(0x6c),
        UINT16_C(0x62), UINT16_C(0x6a), UINT16_C(0x66), UINT16_C(0x6e)
    };
    static const ti_c54x_address_access accesses[] = {
        TI_C54X_ADDRESS_ACCESS_READ, TI_C54X_ADDRESS_ACCESS_WRITE
    };
    uint8_t ar_index;
    size_t access_index;

    for (ar_index = UINT8_C(1); ar_index <= UINT8_C(7); ++ar_index) {
        for (access_index = 0U; access_index < 2U; ++access_index) {
            size_t step;
            for (step = 0U;
                 step + 1U < sizeof(sequence) / sizeof(sequence[0]);
                 ++step) {
                check_case(ar_index, sequence[step], UINT16_C(0x0008),
                           TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,
                           accesses[access_index], sequence[step + 1U]);
            }
        }
    }
}

static void
test_safe_mod4_inverse_for_all_selected_registers_and_accesses(void)
{
    static const uint16_t sequence[] = {
        UINT16_C(0x60), UINT16_C(0x68), UINT16_C(0x64), UINT16_C(0x6c),
        UINT16_C(0x62), UINT16_C(0x6a), UINT16_C(0x66), UINT16_C(0x6e)
    };
    static const ti_c54x_address_access accesses[] = {
        TI_C54X_ADDRESS_ACCESS_READ, TI_C54X_ADDRESS_ACCESS_WRITE
    };
    uint8_t ar_index;
    size_t access_index;

    for (ar_index = UINT8_C(1); ar_index <= UINT8_C(7); ++ar_index) {
        for (access_index = 0U; access_index < 2U; ++access_index) {
            size_t step;
            for (step = 1U; step < sizeof(sequence) / sizeof(sequence[0]);
                 ++step) {
                check_case(ar_index, sequence[step], UINT16_C(0x0008),
                           TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
                           accesses[access_index], sequence[step - 1U]);
            }
        }
    }
}

int
main(void)
{
    test_ti_mod7_sequence_for_all_selected_registers_and_accesses();
    test_safe_mod4_inverse_for_all_selected_registers_and_accesses();
    puts("indirect_reverse_carry_word: ok");
    return 0;
}
