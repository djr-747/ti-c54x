/* Module contract
 * Owns: structural validation and atomic accumulator-write-pair
 * effect tests, including both-writes-validate-before-commit.
 * Does not own: opcode semantics, CPU lifecycle, or timing.
 */

#include "../../../../src/isa/ops/op_effect.h"
#include "../../../../src/sequencer/accumulator/effect_apply.h"
#include "../../../../src/state/accumulator/accumulator.h"
#include "../../../../src/state/status/registers.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static void reset_all(ti_c54x_accumulator_state *accumulators,
                      ti_c54x_status_register_state *status)
{
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(status) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           accumulators, TI_C54X_ACCUMULATOR_A, 4U) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           accumulators, TI_C54X_ACCUMULATOR_B, 9U) ==
                       TI_C54X_ACCUMULATOR_OK);
}

static ti_c54x_op_effect make_pair(void)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE_PAIR;
    effect.payload.accumulator_write_pair.first.destination =
        TI_C54X_ACCUMULATOR_A;
    effect.payload.accumulator_write_pair.first.value_raw40 = 9U;
    effect.payload.accumulator_write_pair.first.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    effect.payload.accumulator_write_pair.first.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    effect.payload.accumulator_write_pair.first.status_delta
        .destination_overflow_action =
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1;
    effect.payload.accumulator_write_pair.second.destination =
        TI_C54X_ACCUMULATOR_B;
    effect.payload.accumulator_write_pair.second.value_raw40 = 4U;
    effect.payload.accumulator_write_pair.second.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_WRITE_1;
    effect.payload.accumulator_write_pair.second.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_WRITE_0;
    effect.payload.accumulator_write_pair.second.status_delta
        .destination_overflow_action =
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_0;
    return effect;
}

int main(void)
{
    ti_c54x_accumulator_state accumulators;
    ti_c54x_status_register_state status;
    ti_c54x_op_effect effect = make_pair();
    unsigned char before_a[sizeof(accumulators)];
    unsigned char before_s[sizeof(status)];

    reset_all(&accumulators, &status);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply_pair(
                           &effect, &accumulators,
                           &status) == TI_C54X_ACCUMULATOR_EFFECT_OK);
    {
        uint64_t value = 0U;
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                               &accumulators, TI_C54X_ACCUMULATOR_A,
                               &value) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK_U64(value, 9U);
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_read(
                               &accumulators, TI_C54X_ACCUMULATOR_B,
                               &value) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK_U64(value, 4U);
        TI_C54X_TEST_CHECK(ti_c54x_status_register_read_c(&status)
                               .raw_value == 1U);
        TI_C54X_TEST_CHECK(ti_c54x_status_register_read_tc(&status)
                               .raw_value == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_status_register_read_ova(&status)
                               .raw_value == 1U);
        TI_C54X_TEST_CHECK(ti_c54x_status_register_read_ovb(&status)
                               .raw_value == 0U);
    }

    /* An invalid second write rejects with neither write
     * committing. */
    reset_all(&accumulators, &status);
    (void)memcpy(before_a, &accumulators, sizeof before_a);
    (void)memcpy(before_s, &status, sizeof before_s);
    effect.payload.accumulator_write_pair.second.destination =
        TI_C54X_ACCUMULATOR_INVALID;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply_pair(
                           &effect, &accumulators,
                           &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(before_a, &accumulators,
                              sizeof before_a) == 0);
    TI_C54X_TEST_CHECK(memcmp(before_s, &status, sizeof before_s) == 0);

    /* A foreign kind is unsupported, not a pair failure. */
    effect.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply_pair(
                           &effect, &accumulators,
                           &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_UNSUPPORTED);

    /* NULL borrowers reject. */
    effect = make_pair();
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply_pair(
                           NULL, &accumulators,
                           &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply_pair(
                           &effect, NULL,
                           &status) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_effect_apply_pair(
                           &effect, &accumulators,
                           NULL) ==
                       TI_C54X_ACCUMULATOR_EFFECT_INVALID_ARGUMENT);
    return 0;
}
