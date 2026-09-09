/*
 * Module contract
 *
 * Owns: COMMON_TI executable proof of the BK memory-mapped route
 * (SPRU131G Table 3-3 §3.3.4): DATA-space word address 0x0019 reads the
 * known BK value and writes replace BK; unknown BK reads decline; every
 * other space or address declines untouched; NULL and invalid requests
 * reject. This is a mapping record only and claims no full MMR/device
 * routing.
 * Does not own: BK storage or knownness policy, bus transfers, endpoint
 * callbacks, arbitration, opcode semantics, CPU lifecycle, or timing.
 */

#include "../../../src/bus/mmr/bk_route.h"
#include "../../../src/state/arch_state.h"
#include "../../../src/state/block_size_register.h"
#include "../../support/test_harness.h"

#include <stdint.h>

static ti_c54x_bus_word16_request make_request(
    ti_c54x_bus_word16_request_space space,
    ti_c54x_bus_word16_request_access access,
    uint32_t address_word,
    uint16_t write_value_word)
{
    ti_c54x_bus_word16_request request;

    request.space = space;
    request.access = access;
    request.address_word = address_word;
    request.write_value_word = write_value_word;
    return request;
}

static void test_bk_address_is_documented(void)
{
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_BK_DATA_ADDRESS_WORD,
                           UINT16_C(0x0019));
}

static void test_known_bk_reads(void)
{
    ti_c54x_arch_state state;
    ti_c54x_bus_word16_request request =
        make_request(TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                     TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                     UINT32_C(0x0019), UINT16_C(0));
    ti_c54x_bus_mmr_bk_result result;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state,
                                                  UINT16_C(0x0040)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_bus_mmr_bk_route(&request, &state);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_MMR_BK_STATUS_SERVICED);
    TI_C54X_TEST_CHECK_U64(result.read_value_word, UINT16_C(0x0040));
}

static void test_unknown_bk_read_declines(void)
{
    ti_c54x_arch_state state;
    ti_c54x_bus_word16_request request =
        make_request(TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                     TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                     UINT32_C(0x0019), UINT16_C(0));
    ti_c54x_bus_mmr_bk_result result;
    uint16_t probe = UINT16_C(0xFFFF);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_bus_mmr_bk_route(&request, &state);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_BK_STATUS_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, &probe) ==
                       TI_C54X_ARCH_STATE_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(probe, UINT16_C(0xFFFF));
}

static void test_write_replaces_bk(void)
{
    ti_c54x_arch_state state;
    ti_c54x_bus_word16_request request =
        make_request(TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                     TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                     UINT32_C(0x0019), UINT16_C(0x0100));
    ti_c54x_bus_mmr_bk_result result;
    uint16_t value = UINT16_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_bus_mmr_bk_route(&request, &state);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_MMR_BK_STATUS_SERVICED);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, &value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(value, UINT16_C(0x0100));
}

static void test_other_addresses_and_spaces_decline(void)
{
    ti_c54x_arch_state state;
    uint16_t probe = UINT16_C(0xFFFF);
    static const struct {
        ti_c54x_bus_word16_request_space space;
        uint32_t address_word;
    } declined[] = {
        {TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, UINT32_C(0x0018)},
        {TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, UINT32_C(0x001A)},
        {TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM, UINT32_C(0x0019)},
        {TI_C54X_BUS_WORD16_REQUEST_SPACE_IO, UINT32_C(0x0019)},
    };
    size_t index;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state,
                                                  UINT16_C(0x0020)) ==
                       TI_C54X_ARCH_STATE_OK);
    for (index = 0U; index < sizeof(declined) / sizeof(declined[0]);
         ++index) {
        const ti_c54x_bus_word16_request request =
            make_request(declined[index].space,
                         TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                         declined[index].address_word, UINT16_C(0));
        const ti_c54x_bus_mmr_bk_result result =
            ti_c54x_bus_mmr_bk_route(&request, &state);
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_BUS_MMR_BK_STATUS_DECLINED);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(&state, &probe) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(probe, UINT16_C(0x0020));
    }
}

static void test_invalid_inputs_reject(void)
{
    ti_c54x_arch_state state;
    ti_c54x_bus_word16_request request =
        make_request(TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID,
                     TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                     UINT32_C(0x0019), UINT16_C(0));
    ti_c54x_bus_word16_request valid =
        make_request(TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                     TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                     UINT32_C(0x0019), UINT16_C(0));

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_bus_mmr_bk_route(NULL, &state).status ==
                       TI_C54X_BUS_MMR_BK_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_bus_mmr_bk_route(&valid, NULL).status ==
                       TI_C54X_BUS_MMR_BK_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_bus_mmr_bk_route(&request, &state).status ==
                       TI_C54X_BUS_MMR_BK_STATUS_INVALID_ARGUMENT);
}

int main(void)
{
    test_bk_address_is_documented();
    test_known_bk_reads();
    test_unknown_bk_read_declines();
    test_write_replaces_bk();
    test_other_addresses_and_spaces_decline();
    test_invalid_inputs_reject();
    return 0;
}
