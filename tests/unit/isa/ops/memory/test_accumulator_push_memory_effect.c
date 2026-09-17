/*
 * Module contract
 *
 * Owns: executable structural proofs for the accumulator-targeted
 * STACK_PUSH_ACCUMULATOR WORD16 MEMORY effect shapes, including the
 * immediate and delayed kinds with the A/B accumulator select, WRITE
 * access, and transactional rejection of malformed descriptors.
 * Does not own: opcode identity, accumulator values, address
 * arithmetic, architectural state, bus execution, sequencing,
 * timing, observers, or diagnostics.
 */

#include "isa/ops/memory/memory_effect.h"
#include "../../../../support/test_harness.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

static ti_c54x_op_memory_effect
push_effect(ti_c54x_op_memory_address_kind kind, uint8_t select)
{
    ti_c54x_op_memory_effect effect = {0};

    effect.address_spec.kind = kind;
    effect.address_spec.arf = select;
    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    effect.value_word = UINT16_C(0);
    effect.rmw_operation =
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE;
    effect.transfer_reserved = UINT8_C(0);
    effect.complete = NULL;
    return effect;
}

static void
check_valid_selects(void)
{
    static const ti_c54x_op_memory_address_kind kinds[] = {
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16,
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16};
    size_t kind_index;
    uint8_t select;

    for (kind_index = 0U; kind_index < sizeof(kinds) / sizeof(kinds[0]);
         ++kind_index) {
        for (select = UINT8_C(0); select <= UINT8_C(1); ++select) {
            ti_c54x_op_memory_effect effect =
                push_effect(kinds[kind_index], select);

            TI_C54X_TEST_CHECK(
                ti_c54x_op_memory_effect_is_valid(&effect));
        }
    }
}

static void
check_shape_rejections_are_transactional(void)
{
    const ti_c54x_op_memory_effect seed = push_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16,
        UINT8_C(0));
    ti_c54x_op_memory_effect effect = seed;

    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(NULL));

    effect.address_spec.kind = TI_C54X_OP_MEMORY_ADDRESS_INVALID;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.address_spec.arf = UINT8_C(2);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD3;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.address_spec.long_offset_word = UINT16_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.rmw_operation =
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_AND;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.transfer_reserved = UINT8_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));

    /* The delayed kind shares the shape exactly. */
    effect = push_effect(
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16,
        UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));
    effect.address_spec.arf = UINT8_C(7);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));

    /* Validation is read-only: no malformed candidate repair or mutation. */
    effect = seed;
    {
        const ti_c54x_op_memory_effect before = effect;
        (void)ti_c54x_op_memory_effect_is_valid(&effect);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int
main(void)
{
    check_valid_selects();
    check_shape_rejections_are_transactional();
    return 0;
}
