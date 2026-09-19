/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT executable proof of the S55 single-word
 * Lmem LONG32 read seam: direct-DP legs plan canonical LONG32
 * operands with a DIRECT_DP source snapshot and no update;
 * standard-indirect MOD0-MOD3 legs plan through the landed
 * two-word-delta resolver with CMPT0/ARP0 policy; MOD3 reads
 * decline; malformed arms, wrong kinds, and bad accesses reject
 * transactionally. Planning never touches state.
 * Does not own: the pair-transaction commit root, opcode
 * semantics, CPU lifecycle, request issue, or timing.
 */

#include "sequencer/memory/long32_access_plan.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_op_memory_address_spec
long32_spec(uint8_t indirect, uint8_t mode, uint8_t arf, uint8_t dma7,
            uint16_t dp_page)
{
    ti_c54x_op_memory_address_spec spec = {0};

    spec.kind = TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE;
    spec.long32_single.indirect = indirect;
    spec.long32_single.mode = mode;
    spec.long32_single.arf = arf;
    spec.long32_single.dma7 = dma7;
    spec.long32_single.dp_page = dp_page;
    return spec;
}

static ti_c54x_long32_single_access
plan_direct(uint8_t dma7, uint16_t dp_page)
{
    const ti_c54x_op_memory_address_spec spec =
        long32_spec(UINT8_C(0), UINT8_C(0), UINT8_C(0), dma7, dp_page);

    return ti_c54x_sequencer_plan_long32_single_access(
        &spec, UINT16_C(0),
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
}

static void
check_direct_even(void)
{
    /* DP 0x004, dma 0x0A: MSW at 0x020A, LSW at 0x020B. */
    const ti_c54x_long32_single_access plan =
        plan_direct(UINT8_C(0x0A), UINT16_C(0x004));

    TI_C54X_TEST_CHECK(plan.status == TI_C54X_LONG32_SINGLE_ACCESS_OK);
    TI_C54X_TEST_CHECK(plan.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_LONG32);
    TI_C54X_TEST_CHECK(plan.effect.operand.first_word == UINT16_C(0x020A));
    TI_C54X_TEST_CHECK(plan.effect.operand.second_word == UINT16_C(0x020B));
    TI_C54X_TEST_CHECK(plan.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_DIRECT_DP);
    TI_C54X_TEST_CHECK(plan.effect.source.value.direct_dp.dp9 ==
                       UINT16_C(0x004));
    TI_C54X_TEST_CHECK(plan.effect.source.value.direct_dp.dma7 ==
                       UINT8_C(0x0A));
    TI_C54X_TEST_CHECK(plan.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
}

static void
check_direct_odd_pairs_down(void)
{
    /* Odd MSW canonical-pairs down per the LONG32 operand rule. */
    const ti_c54x_long32_single_access plan =
        plan_direct(UINT8_C(0x0B), UINT16_C(0x000));

    TI_C54X_TEST_CHECK(plan.status == TI_C54X_LONG32_SINGLE_ACCESS_OK);
    TI_C54X_TEST_CHECK(plan.effect.operand.first_word == UINT16_C(0x000B));
    TI_C54X_TEST_CHECK(plan.effect.operand.second_word == UINT16_C(0x000A));
    TI_C54X_TEST_CHECK(plan.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
}

static void
check_indirect_mod0(void)
{
    const ti_c54x_op_memory_address_spec spec =
        long32_spec(UINT8_C(1), UINT8_C(0), UINT8_C(3), UINT8_C(0),
                    UINT16_C(0));
    const ti_c54x_long32_single_access plan =
        ti_c54x_sequencer_plan_long32_single_access(
            &spec, UINT16_C(0x1000),
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

    TI_C54X_TEST_CHECK(plan.status == TI_C54X_LONG32_SINGLE_ACCESS_OK);
    TI_C54X_TEST_CHECK(plan.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_LONG32);
    TI_C54X_TEST_CHECK(plan.effect.operand.first_word == UINT16_C(0x1000));
    TI_C54X_TEST_CHECK(plan.effect.operand.second_word == UINT16_C(0x1001));
    TI_C54X_TEST_CHECK(plan.effect.source.kind == TI_C54X_ADDRESS_SOURCE_AR);
    TI_C54X_TEST_CHECK(plan.effect.source.value.ar.index == UINT8_C(3));
    TI_C54X_TEST_CHECK(plan.effect.source.value.ar.value_before ==
                       UINT16_C(0x1000));
    TI_C54X_TEST_CHECK(plan.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(plan.effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0);
}

static void
check_indirect_mod1_mod2(void)
{
    const ti_c54x_op_memory_address_spec mod1 =
        long32_spec(UINT8_C(1), UINT8_C(1), UINT8_C(2), UINT8_C(0),
                    UINT16_C(0));
    const ti_c54x_op_memory_address_spec mod2 =
        long32_spec(UINT8_C(1), UINT8_C(2), UINT8_C(2), UINT8_C(0),
                    UINT16_C(0));
    const ti_c54x_long32_single_access minus =
        ti_c54x_sequencer_plan_long32_single_access(
            &mod1, UINT16_C(0x1000),
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    const ti_c54x_long32_single_access plus =
        ti_c54x_sequencer_plan_long32_single_access(
            &mod2, UINT16_C(0x1000),
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

    TI_C54X_TEST_CHECK(minus.status == TI_C54X_LONG32_SINGLE_ACCESS_OK);
    TI_C54X_TEST_CHECK(minus.effect.operand.first_word == UINT16_C(0x1000));
    TI_C54X_TEST_CHECK(minus.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(minus.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
    TI_C54X_TEST_CHECK(minus.effect.update.value_after == UINT16_C(0x0FFE));
    TI_C54X_TEST_CHECK(plus.status == TI_C54X_LONG32_SINGLE_ACCESS_OK);
    TI_C54X_TEST_CHECK(plus.effect.operand.first_word == UINT16_C(0x1000));
    TI_C54X_TEST_CHECK(plus.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(plus.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
    TI_C54X_TEST_CHECK(plus.effect.update.value_after == UINT16_C(0x1002));
}

static void
check_indirect_mod3(void)
{
    const ti_c54x_op_memory_address_spec spec =
        long32_spec(UINT8_C(1), UINT8_C(3), UINT8_C(5), UINT8_C(0),
                    UINT16_C(0));
    const ti_c54x_long32_single_access write =
        ti_c54x_sequencer_plan_long32_single_access(
            &spec, UINT16_C(0x1000),
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    const ti_c54x_long32_single_access read =
        ti_c54x_sequencer_plan_long32_single_access(
            &spec, UINT16_C(0x1000),
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

    /* MOD3 pre-updates by two words: the WRITE address is AR+2. */
    TI_C54X_TEST_CHECK(write.status == TI_C54X_LONG32_SINGLE_ACCESS_OK);
    TI_C54X_TEST_CHECK(write.effect.operand.first_word == UINT16_C(0x1002));
    TI_C54X_TEST_CHECK(write.effect.operand.second_word == UINT16_C(0x1003));
    TI_C54X_TEST_CHECK(write.effect.update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(write.effect.update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS);
    TI_C54X_TEST_CHECK(write.effect.update.value_after == UINT16_C(0x1002));
    /* MOD3 stays write-only on the single-leg read path. */
    TI_C54X_TEST_CHECK(read.status != TI_C54X_LONG32_SINGLE_ACCESS_OK);
    TI_C54X_TEST_CHECK(!ti_c54x_data_operand_is_valid(&read.effect.operand));
}

static void
check_malformed_rejects(void)
{
    static const ti_c54x_op_memory_address_spec CASES[] = {
        /* indirect flag out of range */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE,
         .long32_single = {.indirect = 2}},
        /* mode out of range */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE,
         .long32_single = {.indirect = 1, .mode = 4}},
        /* arf out of range */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE,
         .long32_single = {.indirect = 1, .arf = 8}},
        /* direct leg with indirect fields set */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE,
         .long32_single = {.dma7 = 0x0A, .mode = 1}},
        /* indirect leg with direct offset set */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE,
         .long32_single = {.indirect = 1, .dma7 = 0x0A}},
        /* dma out of range */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE,
         .long32_single = {.dma7 = 0x80}},
        /* dp page out of range */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE,
         .long32_single = {.dp_page = 512}},
        /* reserved word nonzero */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE,
         .long32_single = {.reserved_word0 = 1}},
        /* wrong kind */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16,
         .long32_single = {.dma7 = 0x0A}},
    };
    size_t index;

    for (index = 0U; index < sizeof(CASES) / sizeof(CASES[0]); ++index) {
        const ti_c54x_long32_single_access plan =
            ti_c54x_sequencer_plan_long32_single_access(
                &CASES[index], UINT16_C(0x1000),
                TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

        TI_C54X_TEST_CHECK(plan.status != TI_C54X_LONG32_SINGLE_ACCESS_OK);
        TI_C54X_TEST_CHECK(!ti_c54x_data_operand_is_valid(
            &plan.effect.operand));
    }
}

static void
check_bad_arguments_reject(void)
{
    const ti_c54x_op_memory_address_spec spec =
        long32_spec(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0x0A),
                    UINT16_C(0x004));
    const ti_c54x_long32_single_access null_spec =
        ti_c54x_sequencer_plan_long32_single_access(
            NULL, UINT16_C(0x1000),
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    const ti_c54x_long32_single_access bad_access =
        ti_c54x_sequencer_plan_long32_single_access(
            &spec, UINT16_C(0x1000),
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID);

    TI_C54X_TEST_CHECK(null_spec.status ==
                       TI_C54X_LONG32_SINGLE_ACCESS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(!ti_c54x_data_operand_is_valid(
        &null_spec.effect.operand));
    TI_C54X_TEST_CHECK(bad_access.status !=
                       TI_C54X_LONG32_SINGLE_ACCESS_OK);
    TI_C54X_TEST_CHECK(!ti_c54x_data_operand_is_valid(
        &bad_access.effect.operand));
}

int
main(void)
{
    check_direct_even();
    check_direct_odd_pairs_down();
    check_indirect_mod0();
    check_indirect_mod1_mod2();
    check_indirect_mod3();
    check_malformed_rejects();
    check_bad_arguments_reject();
    return 0;
}
