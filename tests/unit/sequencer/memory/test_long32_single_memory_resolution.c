/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT_ONLY proof that the generic data-memory
 * sequencer resolves LONG32_SINGLE legs from copied architectural
 * state: page-0 direct legs plan canonical LONG32 operands with
 * no state observation; standard-indirect MOD0-MOD3 legs resolve
 * through the two-word-delta path under CMPT=0/ARP=0 with AR
 * commits; mode/state violations reject transactionally.
 * Does not own: opcode identity or handlers, pair-transaction
 * issue, endpoint callbacks, timing, CPU lifecycle, diagnostics,
 * or public ABI.
 */

#include "isa/ops/memory/memory_effect.h"
#include "sequencer/memory/address_resolution.h"
#include "state/arch_state.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

static ti_c54x_arch_state
mode_state(uint8_t arf, uint16_t ar_value)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, arf, ar_value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_memory_address_spec
direct_spec(uint8_t dma7)
{
    ti_c54x_op_memory_address_spec spec = {0};

    spec.kind = TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE;
    spec.long32_single.dma7 = dma7;
    return spec;
}

static ti_c54x_op_memory_address_spec
indirect_spec(uint8_t mode, uint8_t arf)
{
    ti_c54x_op_memory_address_spec spec = {0};

    spec.kind = TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE;
    spec.long32_single.indirect = UINT8_C(1);
    spec.long32_single.mode = mode;
    spec.long32_single.arf = arf;
    return spec;
}

static void
check_direct(void)
{
    const ti_c54x_op_memory_address_spec spec = direct_spec(UINT8_C(0x0A));
    ti_c54x_arch_state state = mode_state(UINT8_C(2), UINT16_C(0x1000));
    ti_c54x_memory_address_resolution resolution = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0),
                           &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(resolution.request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(resolution.request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(resolution.request.address_word == UINT16_C(0x000A));
    TI_C54X_TEST_CHECK(resolution.address_result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_LONG32);
    TI_C54X_TEST_CHECK(
        resolution.address_result.effect.operand.first_word ==
        UINT16_C(0x000A));
    TI_C54X_TEST_CHECK(
        resolution.address_result.effect.operand.second_word ==
        UINT16_C(0x000B));
    TI_C54X_TEST_CHECK(resolution.address_result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_DIRECT_PAGE0);
    TI_C54X_TEST_CHECK(resolution.address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
}

static void
check_indirect_mod1(void)
{
    const ti_c54x_op_memory_address_spec spec =
        indirect_spec(UINT8_C(1), UINT8_C(2));
    ti_c54x_arch_state state = mode_state(UINT8_C(2), UINT16_C(0x1000));
    ti_c54x_memory_address_resolution resolution = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0),
                           &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(resolution.request.address_word == UINT16_C(0x1000));
    TI_C54X_TEST_CHECK(
        resolution.address_result.effect.operand.first_word ==
        UINT16_C(0x1000));
    TI_C54X_TEST_CHECK(
        resolution.address_result.effect.operand.second_word ==
        UINT16_C(0x1001));
    TI_C54X_TEST_CHECK(resolution.address_result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR);
    TI_C54X_TEST_CHECK(resolution.address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(resolution.address_commit.ar_index == UINT8_C(2));
    TI_C54X_TEST_CHECK(resolution.address_commit.value_after ==
                       UINT16_C(0x0FFE));
}

static void
check_indirect_mod3_write(void)
{
    const ti_c54x_op_memory_address_spec spec =
        indirect_spec(UINT8_C(3), UINT8_C(4));
    ti_c54x_arch_state state = mode_state(UINT8_C(4), UINT16_C(0x2000));
    ti_c54x_memory_address_resolution resolution = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                           UINT16_C(0x1234),
                           &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    TI_C54X_TEST_CHECK(resolution.request.address_word == UINT16_C(0x2002));
    TI_C54X_TEST_CHECK(resolution.request.write_value_word ==
                       UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(
        resolution.address_result.effect.operand.first_word ==
        UINT16_C(0x2002));
    TI_C54X_TEST_CHECK(resolution.address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(resolution.address_commit.value_after ==
                       UINT16_C(0x2002));
}

static void
check_rejects(void)
{
    const ti_c54x_op_memory_address_spec mod3_read =
        indirect_spec(UINT8_C(3), UINT8_C(2));
    const ti_c54x_op_memory_address_spec bad_arm = indirect_spec(
        UINT8_C(4), UINT8_C(2));
    ti_c54x_arch_state state = mode_state(UINT8_C(2), UINT16_C(0x1000));
    ti_c54x_memory_address_resolution resolution = {0};
    ti_c54x_memory_address_resolution_status status;

    /* MOD3 stays write-only on the read path. */
    status = ti_c54x_sequencer_resolve_memory_address(
        &state, &mod3_read, TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        UINT16_C(0), &resolution);
    TI_C54X_TEST_CHECK(status != TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);

    /* Malformed arm. */
    status = ti_c54x_sequencer_resolve_memory_address(
        &state, &bad_arm, TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        UINT16_C(0), &resolution);
    TI_C54X_TEST_CHECK(status ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);

    /* CMPT set. */
    {
        const ti_c54x_op_memory_address_spec mod0 =
            indirect_spec(UINT8_C(0), UINT8_C(2));

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state,
                                                        UINT8_C(1)) ==
                           TI_C54X_ARCH_STATE_OK);
        status = ti_c54x_sequencer_resolve_memory_address(
            &state, &mod0, TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            UINT16_C(0), &resolution);
    }
    TI_C54X_TEST_CHECK(status ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_UNKNOWN_MODE);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);

    /* NULL arguments. */
    status = ti_c54x_sequencer_resolve_memory_address(
        NULL, &mod3_read, TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        UINT16_C(0), &resolution);
    TI_C54X_TEST_CHECK(status ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ARGUMENT);
}

int
main(void)
{
    check_direct();
    check_indirect_mod1();
    check_indirect_mod3_write();
    check_rejects();
    return 0;
}
