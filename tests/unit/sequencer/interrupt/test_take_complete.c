/*
 * Module contract
 *
 * Owns: executable proof of the NMI take completion (P3-S94):
 * the staged push value is ignored and exactly one follow-on
 * effect is staged setting ST1 bit 11 (INTM), mirroring the
 * RETE INTM-clear completion. Does not own: the take itself,
 * arbitration, vectors, CPU flow, timing, buses, allocation,
 * or diagnostics.
 */

#include "sequencer/interrupt/take_complete.h"

#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"

#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

static void
test_stages_intm_set(void)
{
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_take_complete_intm(
                           NULL, NULL, UINT16_C(0x1234), &next) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_STATUS_BIT_WRITE);
    TI_C54X_TEST_CHECK(next.payload.status_bit_write.target_register ==
                       TI_C54X_OP_STATUS_REGISTER_ST1);
    TI_C54X_TEST_CHECK(next.payload.status_bit_write.bit_index ==
                       UINT8_C(11));
    TI_C54X_TEST_CHECK(next.payload.status_bit_write.raw_value ==
                       UINT8_C(1));
}

static void
test_null_next_rejects(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_take_complete_intm(NULL, NULL, UINT16_C(0),
                                                 NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_stages_intm_set();
    test_null_next_rejects();
    return 0;
}
