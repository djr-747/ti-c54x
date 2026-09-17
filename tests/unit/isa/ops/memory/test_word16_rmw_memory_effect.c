/*
 * Module contract
 *
 * Owns: executable structural proofs for the private WORD16 memory RMW
 * carrier.
 * Does not own: opcode identity, address arithmetic, endpoint I/O, transaction
 * sequencing, architectural state, timing, observers, or diagnostics.
 */

#include "../../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(ti_c54x_op_memory_effect) == 48,
               "WORD16 memory effect size changed on a 64-bit ABI");
_Static_assert(sizeof(ti_c54x_op_effect) == 56,
               "generic operation effect size changed on a 64-bit ABI");
#elif UINTPTR_MAX == UINT32_MAX
_Static_assert(sizeof(ti_c54x_op_memory_effect) == 40,
               "WORD16 memory effect size changed on a 32-bit ABI");
_Static_assert(sizeof(ti_c54x_op_effect) == 44,
               "generic operation effect size changed on a 32-bit ABI");
#endif

static ti_c54x_op_status
noop_complete(const ti_c54x_decoded_instruction *instruction,
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
rmw_effect(ti_c54x_op_memory_rmw_operation operation, uint16_t value_word)
{
    ti_c54x_op_memory_effect effect = {0};

    effect.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
    effect.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD2;
    effect.address_spec.arf = UINT8_C(2);
    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    effect.value_word = value_word;
    effect.rmw_operation = (uint8_t)operation;
    effect.complete = NULL;
    return effect;
}

static ti_c54x_op_memory_effect
ordinary_read_effect(void)
{
    ti_c54x_op_memory_effect effect = rmw_effect(
        TI_C54X_OP_MEMORY_RMW_OPERATION_NONE, UINT16_C(0));

    effect.complete = noop_complete;
    return effect;
}

static ti_c54x_op_memory_effect
ordinary_write_effect(void)
{
    ti_c54x_op_memory_effect effect = rmw_effect(
        TI_C54X_OP_MEMORY_RMW_OPERATION_NONE, UINT16_C(0x00a5));

    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    return effect;
}

static void
check_valid_rmw_operations(void)
{
    const ti_c54x_op_memory_rmw_operation operations[] = {
        TI_C54X_OP_MEMORY_RMW_OPERATION_AND,
        TI_C54X_OP_MEMORY_RMW_OPERATION_OR,
        TI_C54X_OP_MEMORY_RMW_OPERATION_XOR};

    for (size_t index = 0; index < sizeof(operations) / sizeof(operations[0]);
         ++index) {
        ti_c54x_op_memory_effect effect = rmw_effect(
            operations[index], (uint16_t)(UINT16_C(0x1200) + index));

        TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));
    }
}

static void
check_ordinary_shapes_remain_valid(void)
{
    ti_c54x_op_memory_effect read = ordinary_read_effect();
    ti_c54x_op_memory_effect write = ordinary_write_effect();

    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&read));
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&write));
}

static void
check_malformed_combinations_are_rejected(void)
{
    ti_c54x_op_memory_effect effect = rmw_effect(
        TI_C54X_OP_MEMORY_RMW_OPERATION_AND, UINT16_C(0x00f0));

    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(NULL));

    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));

    effect = rmw_effect(TI_C54X_OP_MEMORY_RMW_OPERATION_AND,
                        UINT16_C(0x00f0));
    effect.complete = noop_complete;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));

    effect = rmw_effect(TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
                        UINT16_C(0x00f0));
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));

    effect = rmw_effect(TI_C54X_OP_MEMORY_RMW_OPERATION_AND,
                        UINT16_C(0x00f0));
    effect.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD3;
    TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
}

static void
check_validation_is_non_mutating(void)
{
    const ti_c54x_op_memory_effect before = rmw_effect(
        TI_C54X_OP_MEMORY_RMW_OPERATION_XOR, UINT16_C(0x5a5a));
    ti_c54x_op_memory_effect after = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&after));
    TI_C54X_TEST_CHECK(after.address_spec.kind == before.address_spec.kind);
    TI_C54X_TEST_CHECK(after.address_spec.mode == before.address_spec.mode);
    TI_C54X_TEST_CHECK(after.value_word == before.value_word);
    TI_C54X_TEST_CHECK(after.rmw_operation == before.rmw_operation);
    TI_C54X_TEST_CHECK(after.complete == before.complete);
}

int
main(void)
{
    check_valid_rmw_operations();
    check_ordinary_shapes_remain_valid();
    check_malformed_combinations_are_rejected();
    check_validation_is_non_mutating();
    return 0;
}
