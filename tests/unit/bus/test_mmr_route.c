/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT executable proof of the S1 MMR route
 * (SPRU131G Table 3-3 §3.3.4): DATA-space WORD16 traffic at
 * 0x00–0x1F routes to register state — owned registers through
 * checked whole-word access, IMR/TRN/BRC/RSA/REA through the
 * private MMR file — while reserved 0x02–0x05, XPC 0x1E, 0x1F,
 * and every other space or address decline untouched with no
 * endpoint traffic. Unknown reads report UNKNOWN without
 * touching state; NULL and invalid requests reject.
 * Does not own: MMR storage, bus transfers, endpoint callbacks,
 * arbitration, opcode semantics, CPU lifecycle, or timing.
 */

#include "../../../src/bus/mmr/mmr_route.h"
#include "../../../src/state/accumulator/accumulator.h"
#include "../../../src/state/arch_state.h"
#include "../../../src/state/temporary/t_register.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

typedef struct mmr_fixture {
    ti_c54x_arch_state state;
    ti_c54x_accumulator_state accumulators;
    ti_c54x_t_register_state t_register;
} mmr_fixture;

static void
fixture_init(mmr_fixture *fixture)
{
    memset(fixture, 0, sizeof(*fixture));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&fixture->state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&fixture->accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&fixture->t_register) ==
                       TI_C54X_T_REGISTER_OK);
}

static ti_c54x_bus_word16_request
make_request(ti_c54x_bus_word16_request_space space,
             ti_c54x_bus_word16_request_access access,
             uint32_t address_word, uint16_t write_value_word)
{
    ti_c54x_bus_word16_request request;

    request.space = space;
    request.access = access;
    request.address_word = address_word;
    request.write_value_word = write_value_word;
    return request;
}

static ti_c54x_bus_mmr_route_result
route(mmr_fixture *fixture,
      ti_c54x_bus_word16_request_space space,
      ti_c54x_bus_word16_request_access access,
      uint32_t address_word, uint16_t write_value_word)
{
    ti_c54x_bus_word16_request request =
        make_request(space, access, address_word, write_value_word);

    return ti_c54x_bus_mmr_route(&request, &fixture->state,
                                 &fixture->accumulators,
                                 &fixture->t_register);
}

static void
test_table_addresses_are_documented(void)
{
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_IMR, UINT16_C(0x0000));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_IFR, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_ST0, UINT16_C(0x0006));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_ST1, UINT16_C(0x0007));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_AL, UINT16_C(0x0008));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_AH, UINT16_C(0x0009));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_AG, UINT16_C(0x000A));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_BL, UINT16_C(0x000B));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_BH, UINT16_C(0x000C));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_BG, UINT16_C(0x000D));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_T, UINT16_C(0x000E));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_TRN, UINT16_C(0x000F));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_AR0, UINT16_C(0x0010));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_AR7, UINT16_C(0x0017));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_SP, UINT16_C(0x0018));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_BK, UINT16_C(0x0019));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_BRC, UINT16_C(0x001A));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_RSA, UINT16_C(0x001B));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_REA, UINT16_C(0x001C));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_PMST, UINT16_C(0x001D));
    TI_C54X_TEST_CHECK_U64(TI_C54X_BUS_MMR_ADDRESS_MAX, UINT16_C(0x001F));
}

static void
test_non_mmr_traffic_declines_untouched(void)
{
    mmr_fixture fixture;
    mmr_fixture before;
    ti_c54x_bus_mmr_route_result result;

    fixture_init(&fixture);
    before = fixture;
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                   UINT32_C(0x0020), UINT16_C(0));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_DECLINED);
    TI_C54X_TEST_CHECK(
        memcmp(&fixture, &before, sizeof(fixture)) == 0);
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                   UINT32_C(0x000E), UINT16_C(0));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_DECLINED);
    TI_C54X_TEST_CHECK(
        memcmp(&fixture, &before, sizeof(fixture)) == 0);
    /* Reserved testing words and the deferred XPC word decline. */
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT32_C(0x0003), UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_DECLINED);
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                   UINT32_C(0x001E), UINT16_C(0));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_DECLINED);
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT32_C(0x001F), UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_DECLINED);
    TI_C54X_TEST_CHECK(
        memcmp(&fixture, &before, sizeof(fixture)) == 0);
}

static void
test_owned_registers_round_trip(void)
{
    mmr_fixture fixture;
    ti_c54x_bus_mmr_route_result result;
    uint64_t canonical;

    fixture_init(&fixture);
    /* T routes through the temporary owner. */
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT32_C(0x000E), UINT16_C(0x2468));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                   UINT32_C(0x000E), UINT16_C(0));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    TI_C54X_TEST_CHECK_U64(result.read_value_word, UINT16_C(0x2468));
    /* AR3 routes through the arch owner and becomes known. */
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT32_C(0x0013), UINT16_C(0x0300));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                   UINT32_C(0x0013), UINT16_C(0));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    TI_C54X_TEST_CHECK_U64(result.read_value_word, UINT16_C(0x0300));
    /* SP and BK behave the same way. */
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT32_C(0x0018), UINT16_C(0x0400));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT32_C(0x0019), UINT16_C(0x0040));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                   UINT32_C(0x0019), UINT16_C(0));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    TI_C54X_TEST_CHECK_U64(result.read_value_word, UINT16_C(0x0040));
    /* IFR routes through the arch owner. */
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT32_C(0x0001), UINT16_C(0x0010));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                   UINT32_C(0x0001), UINT16_C(0));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    TI_C54X_TEST_CHECK_U64(result.read_value_word, UINT16_C(0x0010));
    /* ST0 routes through the status bank. */
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT32_C(0x0006), UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                   UINT32_C(0x0006), UINT16_C(0));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    TI_C54X_TEST_CHECK_U64(result.read_value_word, UINT16_C(0x1234));
    /* AL/AH compose through the accumulator owner. */
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT32_C(0x0008), UINT16_C(0x4321));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                   UINT32_C(0x0009), UINT16_C(0x8765));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_SERVICED);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(&fixture.accumulators,
                                                TI_C54X_ACCUMULATOR_A,
                                                &canonical) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK_U64(canonical, UINT64_C(0x000087654321));
}

static void
test_private_file_round_trip(void)
{
    mmr_fixture fixture;
    ti_c54x_bus_mmr_route_result result;
    static const uint16_t addresses[] = {UINT16_C(0x0000),
                                         UINT16_C(0x000F),
                                         UINT16_C(0x001A),
                                         UINT16_C(0x001B),
                                         UINT16_C(0x001C)};
    size_t index;

    fixture_init(&fixture);
    for (index = 0U; index < sizeof(addresses) / sizeof(addresses[0]);
         ++index) {
        /* Unknown file reads report UNKNOWN with canonical zero. */
        result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                       addresses[index], UINT16_C(0));
        TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_UNKNOWN);
        TI_C54X_TEST_CHECK_U64(result.read_value_word, UINT16_C(0));
        /* Writes claim the cell and set it known. */
        result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                       addresses[index], UINT16_C(0x5A5A));
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_BUS_MMR_STATUS_SERVICED);
        result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                       addresses[index], UINT16_C(0));
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_BUS_MMR_STATUS_SERVICED);
        TI_C54X_TEST_CHECK_U64(result.read_value_word, UINT16_C(0x5A5A));
    }
}

static void
test_unknown_owned_reads_report_unknown(void)
{
    mmr_fixture fixture;
    ti_c54x_bus_mmr_route_result result;

    /* Accumulators reset unknown: AL reads UNKNOWN here. */
    fixture_init(&fixture);
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                   UINT32_C(0x0008), UINT16_C(0));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_UNKNOWN);
    TI_C54X_TEST_CHECK_U64(result.read_value_word, UINT16_C(0));
    /* T resets unknown as well. */
    result = route(&fixture, TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                   UINT32_C(0x000E), UINT16_C(0));
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_MMR_STATUS_UNKNOWN);
}

static void
test_null_and_invalid_reject(void)
{
    mmr_fixture fixture;
    ti_c54x_bus_word16_request request =
        make_request(TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                     TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                     UINT32_C(0x000E), UINT16_C(0));
    ti_c54x_bus_mmr_route_result result;

    fixture_init(&fixture);
    result = ti_c54x_bus_mmr_route(NULL, &fixture.state,
                                   &fixture.accumulators,
                                   &fixture.t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_MMR_STATUS_INVALID_ARGUMENT);
    result = ti_c54x_bus_mmr_route(&request, NULL, &fixture.accumulators,
                                   &fixture.t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_MMR_STATUS_INVALID_ARGUMENT);
    result = ti_c54x_bus_mmr_route(&request, &fixture.state, NULL,
                                   &fixture.t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_MMR_STATUS_INVALID_ARGUMENT);
    result = ti_c54x_bus_mmr_route(&request, &fixture.state,
                                   &fixture.accumulators, NULL);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_MMR_STATUS_INVALID_ARGUMENT);
    request.access = (ti_c54x_bus_word16_request_access)99;
    result = ti_c54x_bus_mmr_route(&request, &fixture.state,
                                   &fixture.accumulators,
                                   &fixture.t_register);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_MMR_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_table_addresses_are_documented();
    test_non_mmr_traffic_declines_untouched();
    test_owned_registers_round_trip();
    test_private_file_round_trip();
    test_unknown_owned_reads_report_unknown();
    test_null_and_invalid_reject();
    return 0;
}
