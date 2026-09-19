/*
 * Module contract
 *
 * Owns: proof that the generic data-memory sequencer resolves
 * absolute-DATA WORD16 specs with no live state observation:
 * page-0 and high-page addresses plan canonical WORD16
 * operands under a fully unknown architectural state, WRITE
 * access and malformed specs reject transactionally.
 * Does not own: opcode identity or handlers, pair-transaction
 * issue, endpoint callbacks, timing, CPU lifecycle, diagnostics,
 * or public ABI.
 */

#include "isa/ops/memory/memory_effect.h"
#include "sequencer/memory/address_resolution.h"
#include "state/arch_state.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

static ti_c54x_op_memory_address_spec
absolute_spec(uint16_t address)
{
    ti_c54x_op_memory_address_spec spec = {0};

    spec.kind = TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16;
    spec.absolute_data.address = address;
    return spec;
}

static void
check_resolves_without_state(uint16_t address)
{
    /* Raw-zero state: every register and mode bit is unknown.
     * Absolute resolution must not observe any of it. */
    const ti_c54x_op_memory_address_spec spec = absolute_spec(address);
    const ti_c54x_arch_state state = {0};
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
    TI_C54X_TEST_CHECK(resolution.request.address_word == address);
    TI_C54X_TEST_CHECK(resolution.address_result.status ==
                       TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK(
        resolution.address_result.effect.operand.first_word == address);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_ABSOLUTE);
    TI_C54X_TEST_CHECK(
        resolution.address_result.effect.source.value.absolute.address ==
        address);
    TI_C54X_TEST_CHECK(resolution.address_result.effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_NONE);
    TI_C54X_TEST_CHECK(resolution.address_commit.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(resolution.ar0_shadow == UINT16_C(0));
    TI_C54X_TEST_CHECK(resolution.bk_shadow == UINT16_C(0));
}

static void
test_page0_and_high_page(void)
{
    check_resolves_without_state(UINT16_C(0x0000));
    check_resolves_without_state(UINT16_C(0x0020));
    check_resolves_without_state(UINT16_C(0xABCD));
    check_resolves_without_state(UINT16_C(0xFFFF));
}

static void
test_write_rejects(void)
{
    const ti_c54x_op_memory_address_spec spec =
        absolute_spec(UINT16_C(0x0020));
    const ti_c54x_arch_state state = {0};
    ti_c54x_memory_address_resolution resolution = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                           UINT16_C(0x1234),
                           &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ACCESS);
}

static void
test_malformed_spec_rejects(void)
{
    const ti_c54x_arch_state state = {0};
    ti_c54x_memory_address_resolution resolution = {0};

    {
        ti_c54x_op_memory_address_spec spec =
            absolute_spec(UINT16_C(0x0020));

        spec.absolute_data.reserved_word2 = UINT32_C(1);
        TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                               &state, &spec,
                               TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                               UINT16_C(0),
                               &resolution) ==
                           TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_SPEC);
    }
    {
        ti_c54x_op_memory_address_spec spec = {0};

        spec.kind = TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16;
        TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                               &state, &spec,
                               TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                               UINT16_C(0),
                               &resolution) ==
                           TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK);
    }
}

static void
test_null_rejects(void)
{
    const ti_c54x_op_memory_address_spec spec =
        absolute_spec(UINT16_C(0x0020));
    const ti_c54x_arch_state state = {0};
    ti_c54x_memory_address_resolution resolution = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           NULL, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0),
                           &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, NULL,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0),
                           &resolution) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_resolve_memory_address(
                           &state, &spec,
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0),
                           NULL) ==
                       TI_C54X_MEMORY_ADDRESS_RESOLUTION_INVALID_ARGUMENT);
}

int
main(void)
{
    test_page0_and_high_page();
    test_write_rejects();
    test_malformed_spec_rejects();
    test_null_rejects();
    return 0;
}
