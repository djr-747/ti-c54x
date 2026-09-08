/*
 * Module contract
 *
 * Owns: executable proof for typed OVM, OVA, and OVB accessors, including
 * knownness, masks, and transactional invalid/null handling.
 * Does not own: accumulator policy, opcode semantics, collection, or timing.
 */

#include "state/register_layout.h"
#include "state/status/registers.h"
#include "../../../support/test_harness.h"

#include <string.h>

static void check_field(ti_c54x_status_register_state *state,
                        ti_c54x_status_register_field_result (*read)(
                            const ti_c54x_status_register_state *),
                        ti_c54x_status_register_status (*write)(
                            ti_c54x_status_register_state *, uint8_t),
                        uint16_t mask,
                        bool st1_field)
{
    ti_c54x_status_register_state before;
    ti_c54x_status_register_field_result result;

    state->st0 = UINT16_C(0x1357);
    state->st1 = UINT16_C(0x2468);
    state->st0_known_mask = UINT16_C(0x5a5a);
    state->st1_known_mask = UINT16_C(0xa5a5);
    before = *state;
    TI_C54X_TEST_CHECK(write(state, 2U) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&before, state, sizeof(before)) == 0);

    TI_C54X_TEST_CHECK(write(state, 1U) == TI_C54X_STATUS_REGISTER_OK);
    result = read(state);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(result.raw_value == 1U);
    TI_C54X_TEST_CHECK(result.known);
    if (st1_field) {
        TI_C54X_TEST_CHECK(state->st0 == before.st0);
        TI_C54X_TEST_CHECK(state->st0_known_mask == before.st0_known_mask);
        TI_C54X_TEST_CHECK((state->st0 & mask) == (before.st0 & mask));
        TI_C54X_TEST_CHECK((state->st1_known_mask & mask) != 0U);
        TI_C54X_TEST_CHECK((state->st1 & (uint16_t)~mask) ==
                           (before.st1 & (uint16_t)~mask));
        TI_C54X_TEST_CHECK((state->st1_known_mask & (uint16_t)~mask) ==
                           (before.st1_known_mask & (uint16_t)~mask));
    } else {
        TI_C54X_TEST_CHECK(state->st1 == before.st1);
        TI_C54X_TEST_CHECK(state->st1_known_mask == before.st1_known_mask);
        TI_C54X_TEST_CHECK((state->st0_known_mask & mask) != 0U);
        TI_C54X_TEST_CHECK((state->st0 & (uint16_t)~mask) ==
                           (before.st0 & (uint16_t)~mask));
        TI_C54X_TEST_CHECK((state->st0_known_mask & (uint16_t)~mask) ==
                           (before.st0_known_mask & (uint16_t)~mask));
    }
    TI_C54X_TEST_CHECK(write(state, 0U) == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(read(state).raw_value == 0U);
}

int main(void)
{
    ti_c54x_status_register_state state;
    ti_c54x_status_register_state before;
    ti_c54x_status_register_field_result result;

    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&state) ==
                       TI_C54X_STATUS_REGISTER_OK);
    result = ti_c54x_status_register_read_ovm(&state);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(result.raw_value == 0U);
    TI_C54X_TEST_CHECK(result.known);
    check_field(&state, ti_c54x_status_register_read_ova,
                ti_c54x_status_register_write_ova, TI_C54X_ST0_OVA_MASK,
                false);
    check_field(&state, ti_c54x_status_register_read_ovb,
                ti_c54x_status_register_write_ovb, TI_C54X_ST0_OVB_MASK,
                false);
    check_field(&state, ti_c54x_status_register_read_ovm,
                ti_c54x_status_register_write_ovm, TI_C54X_ST1_OVM_MASK,
                true);

    state.st1_known_mask = (uint16_t)(state.st1_known_mask &
                                      (uint16_t)~TI_C54X_ST1_OVM_MASK);
    before = state;
    result = ti_c54x_status_register_read_ovm(&state);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_STATUS_REGISTER_UNKNOWN);
    TI_C54X_TEST_CHECK(!result.known && result.raw_value == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_ovm(NULL).status ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_ovm(NULL, 0U) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_ova(NULL).status ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_ovb(NULL).status ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_ova(NULL, 0U) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_ovb(NULL, 0U) ==
                       TI_C54X_STATUS_REGISTER_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&before, &state, sizeof(state)) == 0);

    state.st0_known_mask = (uint16_t)(state.st0_known_mask &
                                      (uint16_t)~TI_C54X_ST0_OVA_MASK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_ova(&state).status ==
                       TI_C54X_STATUS_REGISTER_UNKNOWN);
    state.st0_known_mask = (uint16_t)(state.st0_known_mask &
                                      (uint16_t)~TI_C54X_ST0_OVB_MASK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_ovb(&state).status ==
                       TI_C54X_STATUS_REGISTER_UNKNOWN);
    return 0;
}
