/*
 * Module contract
 *
 * Owns: executable structural proofs for generic SINGLE_INDIRECT_WORD16
 * memory effects across MOD0 through MOD3, including access-specific shape
 * rules. Does not own: opcode identity, address planning, architectural
 * state, bus execution, sequencing, timing, observers, or diagnostics.
 */

#include "../../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../../support/test_harness.h"

#include <stdint.h>

static ti_c54x_op_status
complete_effect(const ti_c54x_decoded_instruction *instruction,
                const ti_c54x_op_input_snapshot *input,
                uint16_t read_value_word,
                ti_c54x_op_effect *next_effect)
{
    (void)instruction;
    (void)input;
    (void)read_value_word;
    (void)next_effect;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_memory_effect
effect_for(ti_c54x_indirect_single_mode mode,
           ti_c54x_bus_word16_request_access access)
{
    ti_c54x_op_memory_effect effect = {0};

    effect.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
    effect.address_spec.mode = mode;
    effect.address_spec.arf = UINT8_C(3);
    effect.access = access;
    effect.value_word = UINT16_C(0xBEEF);
    effect.complete = complete_effect;
    return effect;
}

static void
check_read_modes(void)
{
    for (ti_c54x_indirect_single_mode mode = TI_C54X_INDIRECT_SINGLE_MOD0;
         mode <= TI_C54X_INDIRECT_SINGLE_MOD2; ++mode) {
        ti_c54x_op_memory_effect effect = effect_for(
            mode, TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

        effect.value_word = UINT16_C(0);
        TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));
    }

    {
        ti_c54x_op_memory_effect effect = effect_for(
            TI_C54X_INDIRECT_SINGLE_MOD3,
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

        effect.value_word = UINT16_C(0);
        TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    }
}

static void
check_write_modes(void)
{
    for (ti_c54x_indirect_single_mode mode = TI_C54X_INDIRECT_SINGLE_MOD0;
         mode <= TI_C54X_INDIRECT_SINGLE_MOD3; ++mode) {
        ti_c54x_op_memory_effect effect = effect_for(
            mode, TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);

        effect.complete = NULL;
        TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));
    }
}

static void
check_shape_rejections(void)
{
    ti_c54x_op_memory_effect effect = effect_for(
        TI_C54X_INDIRECT_SINGLE_MOD2,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

    effect.value_word = UINT16_C(0);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(NULL));

    effect.address_spec.kind = TI_C54X_OP_MEMORY_ADDRESS_INVALID;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
    effect.address_spec.arf = UINT8_C(8);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect.address_spec.arf = UINT8_C(3);

    effect.address_spec.mode =
        (ti_c54x_indirect_single_mode)(TI_C54X_INDIRECT_SINGLE_MOD3 + 1);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD2;

    effect.value_word = UINT16_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect.value_word = UINT16_C(0);
    effect.complete = NULL;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));

    /* WRITE with no RMW transform may carry a completion that stages
     * at most one ordinary follow-on after the write resolves
     * (CMPS TRN/TC precedent). */
    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    effect.complete = complete_effect;
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));
    effect.complete = NULL;
    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
}

int
main(void)
{
    check_read_modes();
    check_write_modes();
    check_shape_rejections();
    return 0;
}
