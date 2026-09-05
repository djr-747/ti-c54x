/*
 * Module contract
 *
 * Owns: executable COMMON_TI tests for the CPL=1 direct SP16+dma7 WORD16
 * planner over every safe input pair.
 * Does not own: planner validation, CPU/SP storage, bus mapping, opcode
 * decoding, test harness implementation, or unresolved-boundary policy.
 */

#include "address/direct_sp.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static void
check_safe_pair(uint16_t sp_value, uint8_t dma_value)
{
    const ti_c54x_direct_sp16 sp = {.bits = sp_value};
    const ti_c54x_direct_dma7 dma = {.bits = dma_value};
    const ti_c54x_address_result result = ti_c54x_direct_sp_word(sp, dma);
    const uint16_t expected = (uint16_t)((uint32_t)sp_value + dma_value);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.first_word, expected);
    TI_C54X_TEST_CHECK_U64(result.effect.operand.second_word, UINT16_C(0));
    TI_C54X_TEST_CHECK(result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_DIRECT_SP);
    TI_C54X_TEST_CHECK_U64(result.effect.source.value.direct_sp.sp16,
                           sp_value);
    TI_C54X_TEST_CHECK_U64(result.effect.source.value.direct_sp.dma7,
                           dma_value);
    TI_C54X_TEST_CHECK(result.effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL1);
    TI_C54X_TEST_CHECK(result.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(result.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
    TI_C54X_TEST_CHECK_U64(result.effect.update.ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_before, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(result.effect.update.value_after, UINT16_C(0));
}

static void
test_complete_safe_domain(void)
{
    uint32_t sp_value;

    for (sp_value = UINT32_C(0); sp_value <= UINT32_C(0xffff); ++sp_value) {
        uint32_t dma_value;

        for (dma_value = UINT32_C(0); dma_value <= UINT32_C(0x7f);
             ++dma_value) {
            if (sp_value + dma_value <= UINT32_C(0xffff)) {
                check_safe_pair((uint16_t)sp_value, (uint8_t)dma_value);
            }
        }
    }
}

int
main(void)
{
    test_complete_safe_domain();
    puts("direct_sp: ok");
    return 0;
}
