/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT executable proof of the S1 MMR access
 * plan: non-MMR and declined addresses stay endpoint-bound,
 * serviceable MMR writes route without probing, known MMR reads
 * route, unknown MMR reads reject, guard-word writes above 0xFF
 * reject, and NULL/invalid inputs reject. Planning never
 * touches state.
 * Does not own: register storage, request issue, endpoint
 * traffic, opcode semantics, CPU lifecycle, or timing.
 */

#include "sequencer/memory/mmr_access_plan.h"
#include "state/accumulator/accumulator.h"
#include "state/arch_state.h"
#include "state/temporary/t_register.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

typedef struct plan_fixture {
    ti_c54x_arch_state state;
    ti_c54x_accumulator_state accumulators;
    ti_c54x_t_register_state t_register;
} plan_fixture;

static void
fixture_init(plan_fixture *fixture)
{
    memset(fixture, 0, sizeof(*fixture));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&fixture->state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&fixture->accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&fixture->t_register) ==
                       TI_C54X_T_REGISTER_OK);
}

static ti_c54x_sequencer_mmr_access
plan(plan_fixture *fixture,
     ti_c54x_bus_word16_request_space space, uint32_t address_word,
     ti_c54x_bus_word16_request_access access, uint16_t write_value_word)
{
    return ti_c54x_sequencer_plan_mmr_access(
        &fixture->state, &fixture->accumulators, &fixture->t_register,
        space, address_word, access, write_value_word);
}

static void
test_endpoint_addresses_stay_endpoint_bound(void)
{
    plan_fixture fixture;

    fixture_init(&fixture);
    TI_C54X_TEST_CHECK(plan(&fixture,
                            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                            UINT32_C(0x0020),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                            UINT16_C(0)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_ENDPOINT);
    TI_C54X_TEST_CHECK(plan(&fixture,
                            TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM,
                            UINT32_C(0x000E),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                            UINT16_C(0)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_ENDPOINT);
    /* Reserved testing words and the deferred XPC word decline. */
    TI_C54X_TEST_CHECK(plan(&fixture,
                            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                            UINT32_C(0x0004),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                            UINT16_C(0x1234)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_ENDPOINT);
    TI_C54X_TEST_CHECK(plan(&fixture,
                            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                            UINT32_C(0x001E),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                            UINT16_C(0)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_ENDPOINT);
    TI_C54X_TEST_CHECK(plan(&fixture,
                            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                            UINT32_C(0x001F),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                            UINT16_C(0x1234)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_ENDPOINT);
}

static void
test_writes_route_without_probing(void)
{
    plan_fixture fixture;

    /* T resets unknown, yet the write still routes: writes set
     * knownness instead of requiring it. */
    fixture_init(&fixture);
    TI_C54X_TEST_CHECK(plan(&fixture,
                            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                            UINT32_C(0x000E),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                            UINT16_C(0x2468)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED);
    /* Guard words route within range and reject above 0xFF. */
    TI_C54X_TEST_CHECK(plan(&fixture,
                            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                            UINT32_C(0x000A),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                            UINT16_C(0x00FF)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED);
    TI_C54X_TEST_CHECK(plan(&fixture,
                            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                            UINT32_C(0x000A),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                            UINT16_C(0x0100)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_INVALID_ARGUMENT);
}

static void
test_reads_route_when_known_and_reject_when_unknown(void)
{
    plan_fixture fixture;
    uint16_t value = UINT16_C(0);

    fixture_init(&fixture);
    /* Unknown T reads reject. */
    TI_C54X_TEST_CHECK(plan(&fixture,
                            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                            UINT32_C(0x000E),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                            UINT16_C(0)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_UNKNOWN);
    /* Planning never touches state: T stays unknown. */
    TI_C54X_TEST_CHECK(ti_c54x_t_register_read(&fixture.t_register,
                                               &value) ==
                       TI_C54X_T_REGISTER_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_write(&fixture.t_register,
                                                UINT16_C(0x2468)) ==
                       TI_C54X_T_REGISTER_OK);
    TI_C54X_TEST_CHECK(plan(&fixture,
                            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                            UINT32_C(0x000E),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                            UINT16_C(0)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED);
    /* ST0 resets fully known and routes. */
    TI_C54X_TEST_CHECK(plan(&fixture,
                            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                            UINT32_C(0x0006),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                            UINT16_C(0)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED);
}

static void
test_null_and_invalid_reject(void)
{
    plan_fixture fixture;

    fixture_init(&fixture);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_plan_mmr_access(
                           NULL, &fixture.accumulators,
                           &fixture.t_register,
                           TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                           UINT32_C(0x000E),
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_plan_mmr_access(
                           &fixture.state, NULL, &fixture.t_register,
                           TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                           UINT32_C(0x000E),
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_plan_mmr_access(
                           &fixture.state, &fixture.accumulators, NULL,
                           TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                           UINT32_C(0x000E),
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                           UINT16_C(0)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(plan(&fixture,
                            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                            UINT32_C(0x000E),
                            (ti_c54x_bus_word16_request_access)99,
                            UINT16_C(0)) ==
                       TI_C54X_SEQUENCER_MMR_ACCESS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_endpoint_addresses_stay_endpoint_bound();
    test_reads_route_when_known_and_reject_when_unknown();
    test_writes_route_without_probing();
    test_null_and_invalid_reject();
    return 0;
}
