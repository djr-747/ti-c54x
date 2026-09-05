/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY tests for direct planner rejection,
 * exact zero effects, and caller-input ownership.
 * Does not own: valid TI address formation, CPU/register mutation, bus
 * mapping, opcode decoding, test harness implementation, or timing.
 */

#include "address/direct_dp.h"
#include "address/direct_sp.h"
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
    TI_C54X_TEST_CHECK(effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
    TI_C54X_TEST_CHECK_U64(effect->update.ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(effect->update.value_before, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(effect->update.value_after, UINT16_C(0));
}

static void
test_invalid_dp_fields(void)
{
    ti_c54x_direct_dp9 dp = {.bits = UINT16_C(0x0200)};
    ti_c54x_direct_dma7 dma = {.bits = UINT8_C(0)};
    ti_c54x_address_result result = ti_c54x_direct_dp_word(dp, dma);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_zero_effect(&result.effect);

    dp.bits = UINT16_C(0);
    dma.bits = UINT8_C(0x80);
    result = ti_c54x_direct_dp_word(dp, dma);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_zero_effect(&result.effect);

    dp.bits = UINT16_MAX;
    dma.bits = UINT8_MAX;
    result = ti_c54x_direct_dp_word(dp, dma);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_zero_effect(&result.effect);
}

static void
test_invalid_sp_dma(void)
{
    const ti_c54x_direct_sp16 sp = {.bits = UINT16_C(0)};
    const ti_c54x_direct_dma7 dma = {.bits = UINT8_C(0x80)};
    const ti_c54x_address_result result = ti_c54x_direct_sp_word(sp, dma);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_zero_effect(&result.effect);
}

static void
test_sp_boundary(void)
{
    ti_c54x_direct_sp16 sp = {.bits = UINT16_MAX};
    ti_c54x_direct_dma7 dma = {.bits = UINT8_C(1)};
    ti_c54x_address_result result = ti_c54x_direct_sp_word(sp, dma);

    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    check_zero_effect(&result.effect);

    sp.bits = UINT16_C(0xff80);
    dma.bits = UINT8_C(0x80);
    result = ti_c54x_direct_sp_word(sp, dma);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    check_zero_effect(&result.effect);
}

static void
test_inputs_are_copied(void)
{
    const ti_c54x_direct_dp9 dp = {.bits = UINT16_C(0x0123)};
    const ti_c54x_direct_dma7 dma = {.bits = UINT8_C(0x45)};
    const ti_c54x_direct_sp16 sp = {.bits = UINT16_C(0x3456)};
    const ti_c54x_address_result dp_result = ti_c54x_direct_dp_word(dp, dma);
    const ti_c54x_address_result sp_result = ti_c54x_direct_sp_word(sp, dma);

    TI_C54X_TEST_CHECK(dp_result.effect.source.value.direct_dp.dp9 ==
                       dp.bits);
    TI_C54X_TEST_CHECK(dp_result.effect.source.value.direct_dp.dma7 ==
                       dma.bits);
    TI_C54X_TEST_CHECK(sp_result.effect.source.value.direct_sp.sp16 ==
                       sp.bits);
    TI_C54X_TEST_CHECK(sp_result.effect.source.value.direct_sp.dma7 ==
                       dma.bits);
}

int
main(void)
{
    test_invalid_dp_fields();
    test_invalid_sp_dma();
    test_sp_boundary();
    test_inputs_are_copied();
    puts("direct_address_contract: ok");
    return 0;
}
