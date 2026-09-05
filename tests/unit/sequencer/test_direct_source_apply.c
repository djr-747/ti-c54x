/*
 * Module contract
 *
 * Owns: executable COMMON_TI proof that planner-produced direct DP/SP WORD16
 * effects apply with their tagged mode/source state and do not touch AR state.
 * Does not own: direct-address arithmetic, architectural register layout,
 * malformed payload precedence, timing, bus transfers, opcode decoding, or
 * diagnostics.
 */

#include "sequencer/address_effect_apply.h"
#include "address/direct_dp.h"
#include "address/direct_sp.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
test_direct_dp_planner_effects_apply_without_ar_access(void)
{
    uint32_t dp_value;
    uint32_t dma_value;

    for (dp_value = UINT32_C(0); dp_value <= UINT32_C(0x1ff); ++dp_value) {
        for (dma_value = UINT32_C(0); dma_value <= UINT32_C(0x7f);
             ++dma_value) {
            const ti_c54x_direct_dp9 dp = {(uint16_t)dp_value};
            const ti_c54x_direct_dma7 dma = {(uint8_t)dma_value};
            const ti_c54x_address_result result =
                ti_c54x_direct_dp_word(dp, dma);
            ti_c54x_arch_state state;
            ti_c54x_arch_state before;

            TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
            TI_C54X_TEST_CHECK(result.effect.source.kind ==
                               TI_C54X_ADDRESS_SOURCE_DIRECT_DP);
            TI_C54X_TEST_CHECK(result.effect.state_policy ==
                               TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0);
            TI_C54X_TEST_CHECK_U64(
                result.effect.operand.first_word,
                (uint16_t)((dp_value << UINT32_C(7)) | dma_value));
            TI_C54X_TEST_CHECK(result.effect.update.target ==
                               TI_C54X_ADDRESS_UPDATE_NONE);

            TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                               TI_C54X_ARCH_STATE_OK);
            TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(
                                   &state, UINT8_C(0)) ==
                               TI_C54X_ARCH_STATE_OK);
            TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(
                                   &state, (uint16_t)dp_value) ==
                               TI_C54X_ARCH_STATE_OK);
            before = state;
            TI_C54X_TEST_CHECK(
                ti_c54x_sequencer_apply_address_result(
                    &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE) ==
                TI_C54X_ADDRESS_APPLY_OK);
            TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
        }
    }
}

static void
test_direct_sp_planner_effects_apply_without_ar_access(void)
{
    uint32_t sp_value;
    uint32_t dma_value;

    for (sp_value = UINT32_C(0); sp_value <= UINT32_C(0xffff); ++sp_value) {
        for (dma_value = UINT32_C(0); dma_value <= UINT32_C(0x7f);
             ++dma_value) {
            const uint32_t sum = sp_value + dma_value;
            const ti_c54x_direct_sp16 sp = {(uint16_t)sp_value};
            const ti_c54x_direct_dma7 dma = {(uint8_t)dma_value};
            ti_c54x_address_result result;
            ti_c54x_arch_state state;
            ti_c54x_arch_state before;

            if (sum > UINT32_C(0xffff)) {
                continue;
            }
            result = ti_c54x_direct_sp_word(sp, dma);
            TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
            TI_C54X_TEST_CHECK(result.effect.source.kind ==
                               TI_C54X_ADDRESS_SOURCE_DIRECT_SP);
            TI_C54X_TEST_CHECK(result.effect.state_policy ==
                               TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL1);
            TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word,
                                   (uint16_t)sum);
            TI_C54X_TEST_CHECK(result.effect.update.target ==
                               TI_C54X_ADDRESS_UPDATE_NONE);

            TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                               TI_C54X_ARCH_STATE_OK);
            TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(
                                   &state, UINT8_C(1)) ==
                               TI_C54X_ARCH_STATE_OK);
            TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(
                                   &state, (uint16_t)sp_value) ==
                               TI_C54X_ARCH_STATE_OK);
            before = state;
            TI_C54X_TEST_CHECK(
                ti_c54x_sequencer_apply_address_result(
                    &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE) ==
                TI_C54X_ADDRESS_APPLY_OK);
            TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
        }
    }
}

int
main(void)
{
    test_direct_dp_planner_effects_apply_without_ar_access();
    test_direct_sp_planner_effects_apply_without_ar_access();
    puts("direct_source_apply: ok");
    return 0;
}
