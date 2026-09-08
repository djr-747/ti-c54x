/*
 * Module contract
 *
 * Owns: executable structural proofs for the private REVERSE_CARRY_WORD16
 * MEMORY effect shape, including typed MOD4/MOD7 READ and WRITE access,
 * ARF bounds, canonical unrelated fields, and validation immutability.
 * Does not own: opcode identity, reverse-carry arithmetic, address resolution,
 * architectural state, bus execution, sequencing, timing, observers, or
 * diagnostics.
 */

#include "isa/ops/memory/memory_effect.h"
#include "address/indirect_reverse_carry.h"
#include "address/indirect_circular.h"
#include "address/indirect_indexed.h"
#include "address/indirect_single.h"
#include "../../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
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
reverse_effect(ti_c54x_indirect_reverse_carry_mode mode,
               uint8_t arf,
               ti_c54x_bus_word16_request_access access)
{
    ti_c54x_op_memory_effect effect = {0};

    effect.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16;
    effect.address_spec.reverse_carry_mode = mode;
    effect.address_spec.arf = arf;
    effect.access = access;
    effect.write_value_word = UINT16_C(0xBEEF);
    effect.complete = complete_effect;
    return effect;
}

static void
check_read_modes_and_registers(void)
{
    const ti_c54x_indirect_reverse_carry_mode modes[] = {
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7};

    for (size_t mode_index = 0;
         mode_index < sizeof(modes) / sizeof(modes[0]); ++mode_index) {
        for (uint8_t arf = UINT8_C(1); arf <= UINT8_C(7); ++arf) {
            ti_c54x_op_memory_effect effect = reverse_effect(
                modes[mode_index], arf,
                TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

            effect.write_value_word = UINT16_C(0);
            TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));
        }
    }
}

static void
check_write_modes_and_registers(void)
{
    const ti_c54x_indirect_reverse_carry_mode modes[] = {
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7};

    for (size_t mode_index = 0;
         mode_index < sizeof(modes) / sizeof(modes[0]); ++mode_index) {
        for (uint8_t arf = UINT8_C(1); arf <= UINT8_C(7); ++arf) {
            ti_c54x_op_memory_effect effect = reverse_effect(
                modes[mode_index], arf,
                TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);

            effect.complete = NULL;
            TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));
        }
    }
}

static void
check_invalid_kind_and_fields(void)
{
    ti_c54x_op_memory_effect seed = reverse_effect(
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD4, UINT8_C(1),
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    ti_c54x_op_memory_effect effect = seed;

    effect.write_value_word = UINT16_C(0);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(NULL));

    effect.address_spec.kind = TI_C54X_OP_MEMORY_ADDRESS_INVALID;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));

    effect = seed;
    effect.address_spec.reverse_carry_mode =
        (ti_c54x_indirect_reverse_carry_mode)UINT8_C(0);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.address_spec.reverse_carry_mode =
        (ti_c54x_indirect_reverse_carry_mode)UINT8_C(5);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));

    effect = seed;
    effect.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD1;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.address_spec.indexed_mode = TI_C54X_INDIRECT_INDEXED_MOD5;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.address_spec.circular_mode = TI_C54X_INDIRECT_CIRCULAR_MOD8;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));

    effect = seed;
    effect.address_spec.arf = UINT8_C(0);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.address_spec.arf = UINT8_C(8);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
}

static void
check_access_shapes_and_nonmutation(void)
{
    ti_c54x_op_memory_effect effect = reverse_effect(
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, UINT8_C(7),
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    const ti_c54x_op_memory_effect seed = effect;

    effect.write_value_word = UINT16_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = seed;
    effect.complete = NULL;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));

    effect = reverse_effect(TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, UINT8_C(7),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    effect.complete = complete_effect;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    effect = reverse_effect(TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, UINT8_C(7),
                            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));

    effect = seed;
    {
        const ti_c54x_op_memory_effect before = effect;
        (void)ti_c54x_op_memory_effect_is_valid(&effect);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static ti_c54x_op_memory_effect
existing_effect(ti_c54x_op_memory_address_kind kind)
{
    ti_c54x_op_memory_effect effect = {0};

    effect.address_spec.kind = kind;
    effect.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD2;
    effect.address_spec.indexed_mode = TI_C54X_INDIRECT_INDEXED_MOD5;
    effect.address_spec.circular_mode = TI_C54X_INDIRECT_CIRCULAR_MOD8;
    effect.address_spec.arf = UINT8_C(3);
    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    effect.write_value_word = UINT16_C(0x1234);
    return effect;
}

static void
check_existing_kinds_keep_reverse_mode_canonical(void)
{
    const ti_c54x_op_memory_address_kind kinds[] = {
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16,
        TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16,
        TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16};

    for (size_t i = 0; i < sizeof(kinds) / sizeof(kinds[0]); ++i) {
        ti_c54x_op_memory_effect effect = existing_effect(kinds[i]);

        effect.address_spec.mode = 0;
        effect.address_spec.indexed_mode = 0;
        effect.address_spec.circular_mode = 0;
        if (kinds[i] == TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16) {
            effect.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD2;
        } else if (kinds[i] == TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16) {
            effect.address_spec.indexed_mode = TI_C54X_INDIRECT_INDEXED_MOD5;
        } else {
            effect.address_spec.circular_mode = TI_C54X_INDIRECT_CIRCULAR_MOD8;
            effect.address_spec.arf = UINT8_C(0);
        }
        TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));

        effect.address_spec.reverse_carry_mode =
            TI_C54X_INDIRECT_REVERSE_CARRY_MOD4;
        TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
    }
}

int
main(void)
{
    check_read_modes_and_registers();
    check_write_modes_and_registers();
    check_invalid_kind_and_fields();
    check_access_shapes_and_nonmutation();
    check_existing_kinds_keep_reverse_mode_canonical();
    return 0;
}
