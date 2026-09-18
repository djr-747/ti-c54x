/*
 * Module contract
 *
 * Owns: executable structural proofs for the private INDEXED_AR0_WORD16
 * MEMORY effect shape, including MOD5/MOD6 READ and WRITE access and
 * transactional rejection of malformed descriptors. Does not own: opcode
 * identity, address arithmetic, architectural state, bus execution,
 * sequencing, timing, observers, or diagnostics.
 */

#include "isa/ops/memory/memory_effect.h"
#include "address/indirect_indexed.h"
#include "../../../../support/test_harness.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

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
indexed_effect(ti_c54x_indirect_indexed_mode mode,
               ti_c54x_bus_word16_request_access access)
{
    ti_c54x_op_memory_effect effect = {0};

    effect.address_spec.kind = TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16;
    effect.address_spec.indexed_mode = mode;
    effect.address_spec.arf = UINT8_C(3);
    effect.access = access;
    effect.value_word = UINT16_C(0xBEEF);
    effect.complete = complete_effect;
    return effect;
}

static void
check_read_modes(void)
{
    const ti_c54x_indirect_indexed_mode modes[] = {
        TI_C54X_INDIRECT_INDEXED_MOD5, TI_C54X_INDIRECT_INDEXED_MOD6};

    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        ti_c54x_op_memory_effect effect = indexed_effect(
            modes[i], TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

        effect.value_word = UINT16_C(0);
        TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));
    }
}

static void
check_write_modes(void)
{
    const ti_c54x_indirect_indexed_mode modes[] = {
        TI_C54X_INDIRECT_INDEXED_MOD5, TI_C54X_INDIRECT_INDEXED_MOD6};

    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        ti_c54x_op_memory_effect effect = indexed_effect(
            modes[i], TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);

        effect.complete = NULL;
        TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));
    }
}

static void
check_shape_rejections_are_transactional(void)
{
    const ti_c54x_op_memory_effect seed = indexed_effect(
        TI_C54X_INDIRECT_INDEXED_MOD5,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_op_memory_effect effect = seed;

    effect.value_word = UINT16_C(0);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(NULL));

    effect.address_spec.kind = TI_C54X_OP_MEMORY_ADDRESS_INVALID;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.address_spec.indexed_mode =
        (ti_c54x_indirect_indexed_mode)UINT8_C(4);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.address_spec.arf = UINT8_C(0);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.address_spec.arf = UINT8_C(8);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD3;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.value_word = UINT16_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.complete = NULL;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));

    /* WRITE with no RMW transform may carry a follow-on completion
     * (CMPS TRN/TC precedent). */
    effect = indexed_effect(TI_C54X_INDIRECT_INDEXED_MOD5,
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    effect.complete = complete_effect;
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));
    effect = indexed_effect(TI_C54X_INDIRECT_INDEXED_MOD5,
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID;
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
    check_read_modes();
    check_write_modes();
    check_shape_rejections_are_transactional();
    return 0;
}
