/* Module contract
 * Owns: structural validation and atomic MAX/MIN effect tests.
 * Does not own: compare semantics, CPU lifecycle, or timing.
 */

#include "../../../../src/isa/ops/op_effect.h"
#include "../../../../src/sequencer/accumulator/effect_apply.h"
#include "../../../../src/state/accumulator/accumulator.h"
#include "../../../../src/state/status/registers.h"
#include "../../../support/test_harness.h"

#include <string.h>

int main(void)
{
    ti_c54x_accumulator_state accumulators;
    ti_c54x_status_register_state status;
    ti_c54x_op_effect effect = {0};
    unsigned char before_a[sizeof(accumulators)];
    unsigned char before_s[sizeof(status)];

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&status) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &accumulators, TI_C54X_ACCUMULATOR_A, 4U) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &accumulators, TI_C54X_ACCUMULATOR_B, 9U) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_cpl(&status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    effect.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_SELECT;
    effect.payload.accumulator_select.destination = TI_C54X_ACCUMULATOR_A;
    effect.payload.accumulator_select.selected_raw40 = 9U;
    effect.payload.accumulator_select.c_value = 1U;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    {
        uint64_t value = 0U;
        uint8_t c = 0U;
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                               &accumulators, TI_C54X_ACCUMULATOR_A,
                               &value) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK_U64(value, 9U);
        {
            const ti_c54x_status_register_field_result c_result =
                ti_c54x_status_register_read_c(&status);
            TI_C54X_TEST_CHECK(c_result.status ==
                               TI_C54X_STATUS_REGISTER_OK);
            c = (uint8_t)c_result.raw_value;
        }
        TI_C54X_TEST_CHECK(c == 1U);
    }

    effect.payload.accumulator_select.destination = TI_C54X_ACCUMULATOR_B;
    effect.payload.accumulator_select.selected_raw40 = 4U;
    effect.payload.accumulator_select.c_value = 0U;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    {
        uint64_t value = 0U;
        const ti_c54x_status_register_field_result c_result =
            ti_c54x_status_register_read_c(&status);
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                               &accumulators, TI_C54X_ACCUMULATOR_B,
                               &value) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK_U64(value, 4U);
        TI_C54X_TEST_CHECK(c_result.status == TI_C54X_STATUS_REGISTER_OK);
        TI_C54X_TEST_CHECK(c_result.raw_value == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_status_register_read_cpl(&status).raw_value ==
                           1U);
    }

    memcpy(before_a, &accumulators, sizeof(before_a));
    memcpy(before_s, &status, sizeof(before_s));
    effect.payload.accumulator_select.destination = TI_C54X_ACCUMULATOR_INVALID;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(before_a, &accumulators, sizeof(before_a)) == 0);
    TI_C54X_TEST_CHECK(memcmp(before_s, &status, sizeof(before_s)) == 0);

    effect.kind = TI_C54X_OP_EFFECT_NONE;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_OK);
    effect.kind = TI_C54X_OP_EFFECT_REPEAT_START;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_UNSUPPORTED);

    effect.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_SELECT;
    effect.payload.accumulator_select.destination = TI_C54X_ACCUMULATOR_A;
    effect.payload.accumulator_select.selected_raw40 = 12U;
    effect.payload.accumulator_select.c_value = 2U;
    memcpy(before_a, &accumulators, sizeof(before_a));
    memcpy(before_s, &status, sizeof(before_s));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(before_a, &accumulators, sizeof(before_a)) == 0);
    TI_C54X_TEST_CHECK(memcmp(before_s, &status, sizeof(before_s)) == 0);
    effect.payload.accumulator_select.destination = TI_C54X_ACCUMULATOR_A;
    effect.payload.accumulator_select.selected_raw40 = UINT64_C(0x10000000000);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(before_a, &accumulators, sizeof(before_a)) == 0);
    TI_C54X_TEST_CHECK(memcmp(before_s, &status, sizeof(before_s)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           NULL, &accumulators, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, NULL, &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply(
                           &effect, &accumulators, NULL) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT);
    return 0;
}
