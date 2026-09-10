/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY tests for the stack-pop WORD16
 * planner over the full 16-bit SP domain, including the SP=0xFFFF
 * wrap pin.
 * Does not own: planner validation, CPU/SP storage, bus mapping, opcode
 * decoding, test harness implementation, or TI stack-underflow policy.
 */

#include "address/stack_pop.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static void
check_spot(uint16_t sp_before, uint16_t expected_after)
{
    const ti_c54x_address_result result =
        ti_c54x_stack_pop_word16(sp_before);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, sp_before);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word, UINT16_C(0));
    TI_C54X_TEST_CHECK(result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_STACK_PUSH);
    TI_C54X_TEST_CHECK_U64(result.effect.source.value.stack_push.sp_before,
                           sp_before);
    TI_C54X_TEST_CHECK(result.effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_NONE);
    TI_C54X_TEST_CHECK(result.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_SP);
    TI_C54X_TEST_CHECK(result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
    TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_before, sp_before);
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_after,
                           expected_after);
}

static void
test_spot_vectors(void)
{
    check_spot(UINT16_C(0x0300), UINT16_C(0x0301));
    check_spot(UINT16_C(0x0000), UINT16_C(0x0001));
    check_spot(UINT16_C(0xfffe), UINT16_C(0xffff));
    /* SP=0xFFFF wraps mod 65536: the TOS slot is 0xFFFF and SP after is
     * 0x0000. The manual states no stack status, so wrap is an explicit
     * project-interpretation pin, not a TI claim. */
    check_spot(UINT16_C(0xffff), UINT16_C(0x0000));
}

static void
test_complete_domain(void)
{
    uint32_t sp_value;

    for (sp_value = UINT32_C(0); sp_value <= UINT32_C(0xffff); ++sp_value) {
        const uint16_t expected =
            (uint16_t)((sp_value + UINT32_C(1)) & UINT32_C(0xffff));
        const ti_c54x_address_result result =
            ti_c54x_stack_pop_word16((uint16_t)sp_value);

        TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
        TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word,
                               (uint16_t)sp_value);
        TI_C54X_TEST_CHECK_U64(result.effect.update.value_after, expected);
    }
}

int
main(void)
{
    test_spot_vectors();
    test_complete_domain();
    puts("stack_pop_planner: ok");
    return 0;
}
