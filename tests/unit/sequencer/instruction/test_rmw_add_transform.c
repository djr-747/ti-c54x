/*
 * Module contract
 *
 * Owns: executable RED/GREEN proofs for the value-owned RMW-ADD 16-bit
 * reducer: wrapped sum, bit-16 carry, SXM-selected overflow (signed rule
 * under SXM=1, carry under SXM=0), and OVM saturation (0x7FFF/0x8000
 * signed, 0xFFFF unsigned). The TI ADDM Example 2 vector (mem 0x8007 +
 * lk 0xFFF8 under OVM=1/SXM=1 saturating to 0x8000) is pinned here.
 * Does not own: the carrier, the sequencer commit that consumes the
 * staged flags, opcode identity, or status register storage.
 */

#include "../../../../src/sequencer/instruction/memory_instruction_rmw.h"
#include "../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct rmw_add_vector {
    uint16_t read_value_word;
    uint16_t addend_word;
    bool overflow_mode;
    bool sign_extension_mode;
    uint16_t expected_write_word;
    bool expected_carry;
    bool expected_overflow;
} rmw_add_vector;

/* TI SPRU172C ADDM Example 1 analog: 0x0004 + 0x123B = 0x123F, no
 * carry, no overflow under either SXM policy. */
static const rmw_add_vector plain_vectors[] = {
    {UINT16_C(0x0004), UINT16_C(0x123B), false, false, UINT16_C(0x123F),
     false, false},
    {UINT16_C(0x0004), UINT16_C(0x123B), false, true, UINT16_C(0x123F),
     false, false},
    {UINT16_C(0x0004), UINT16_C(0x123B), true, false, UINT16_C(0x123F),
     false, false},
    {UINT16_C(0x0004), UINT16_C(0x123B), true, true, UINT16_C(0x123F),
     false, false},
    /* Mixed-sign signed add cannot overflow: 0x8000 + 1 = 0x8001. */
    {UINT16_C(0x8000), UINT16_C(0x0001), false, true, UINT16_C(0x8001),
     false, false},
    /* Negative plus negative without overflow still carries bit 16:
     * -8 + -8 = -16 (0xFFF0). */
    {UINT16_C(0xFFF8), UINT16_C(0xFFF8), false, true, UINT16_C(0xFFF0),
     true, false},
};

/* TI SPRU172C ADDM Example 2: mem 0x8007 + lk 0xFFF8 under OVM=1 and
 * SXM=1 saturates to 0x8000. The raw 17-bit sum is 0x17FFF, so carry
 * is set and the signed addition overflows. */
static const rmw_add_vector example2_vectors[] = {
    {UINT16_C(0x8007), UINT16_C(0xFFF8), true, true, UINT16_C(0x8000),
     true, true},
    {UINT16_C(0x8007), UINT16_C(0xFFF8), false, true, UINT16_C(0x7FFF),
     true, true},
};

/* Positive signed overflow: 0x7FFF + 1 saturates to 0x7FFF under
 * OVM=1 and wraps to 0x8000 under OVM=0; no bit-16 carry either way. */
static const rmw_add_vector positive_overflow_vectors[] = {
    {UINT16_C(0x7FFF), UINT16_C(0x0001), true, true, UINT16_C(0x7FFF),
     false, true},
    {UINT16_C(0x7FFF), UINT16_C(0x0001), false, true, UINT16_C(0x8000),
     false, true},
};

/* Negative signed overflow without saturation: -32768 + -1 wraps to
 * 0x7FFF with carry set. */
static const rmw_add_vector negative_wrap_vectors[] = {
    {UINT16_C(0x8000), UINT16_C(0xFFFF), false, true, UINT16_C(0x7FFF),
     true, true},
};

/* Unsigned policy (SXM=0): overflow is the bit-16 carry, and OVM=1
 * saturates a carried sum to 0xFFFF. */
static const rmw_add_vector unsigned_vectors[] = {
    {UINT16_C(0xFFFF), UINT16_C(0x0001), false, false, UINT16_C(0x0000),
     true, true},
    {UINT16_C(0xFFFF), UINT16_C(0x0001), true, false, UINT16_C(0xFFFF),
     true, true},
    {UINT16_C(0xFFFE), UINT16_C(0x0001), true, false, UINT16_C(0xFFFF),
     false, false},
};

static ti_c54x_op_memory_effect
add_effect(uint16_t addend_word)
{
    ti_c54x_op_memory_effect effect = {0};

    effect.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
    effect.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD2;
    effect.address_spec.arf = UINT8_C(2);
    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    effect.value_word = addend_word;
    effect.rmw_operation =
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_ADD;
    effect.complete = NULL;
    return effect;
}

static void
check_vectors(const rmw_add_vector *vectors, size_t count)
{
    size_t index;

    for (index = 0U; index < count; ++index) {
        const rmw_add_vector *vector = &vectors[index];
        const ti_c54x_op_memory_effect effect =
            add_effect(vector->addend_word);
        uint16_t write_word = UINT16_C(0xBEEF);
        bool carry = false;
        bool overflow = false;

        TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_rmw_transform_add(
                               &effect, vector->read_value_word,
                               vector->overflow_mode,
                               vector->sign_extension_mode, &write_word,
                               &carry, &overflow));
        TI_C54X_TEST_CHECK(write_word == vector->expected_write_word);
        TI_C54X_TEST_CHECK(carry == vector->expected_carry);
        TI_C54X_TEST_CHECK(overflow == vector->expected_overflow);
    }
}

static void
check_rejects_non_add_carrier(void)
{
    const ti_c54x_op_memory_rmw_operation operations[] = {
        TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
        TI_C54X_OP_MEMORY_RMW_OPERATION_AND,
        TI_C54X_OP_MEMORY_RMW_OPERATION_OR,
        TI_C54X_OP_MEMORY_RMW_OPERATION_XOR,
        TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE,
        TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK};
    size_t index;
    uint16_t write_word;
    bool carry;
    bool overflow;

    for (index = 0U;
         index < sizeof(operations) / sizeof(operations[0]); ++index) {
        ti_c54x_op_memory_effect effect = add_effect(UINT16_C(0x0001));

        effect.rmw_operation = (uint8_t)operations[index];
        write_word = UINT16_C(0xBEEF);
        carry = true;
        overflow = true;
        TI_C54X_TEST_CHECK(!ti_c54x_memory_instruction_rmw_transform_add(
            &effect, UINT16_C(0x0002), false, false, &write_word,
            &carry, &overflow));
        TI_C54X_TEST_CHECK(write_word == UINT16_C(0xBEEF));
        TI_C54X_TEST_CHECK(carry);
        TI_C54X_TEST_CHECK(overflow);
    }
}

static void
check_rejects_null_arguments(void)
{
    const ti_c54x_op_memory_effect effect =
        add_effect(UINT16_C(0x0001));
    uint16_t write_word;
    bool flag;

    TI_C54X_TEST_CHECK(!ti_c54x_memory_instruction_rmw_transform_add(
        NULL, UINT16_C(0x0002), false, false, &write_word, &flag,
        &flag));
    TI_C54X_TEST_CHECK(!ti_c54x_memory_instruction_rmw_transform_add(
        &effect, UINT16_C(0x0002), false, false, NULL, &flag, &flag));
    TI_C54X_TEST_CHECK(!ti_c54x_memory_instruction_rmw_transform_add(
        &effect, UINT16_C(0x0002), false, false, &write_word, NULL,
        &flag));
    TI_C54X_TEST_CHECK(!ti_c54x_memory_instruction_rmw_transform_add(
        &effect, UINT16_C(0x0002), false, false, &write_word, &flag,
        NULL));
}

int
main(void)
{
    check_vectors(plain_vectors,
                  sizeof(plain_vectors) / sizeof(plain_vectors[0]));
    check_vectors(example2_vectors,
                  sizeof(example2_vectors) / sizeof(example2_vectors[0]));
    check_vectors(positive_overflow_vectors,
                  sizeof(positive_overflow_vectors) /
                      sizeof(positive_overflow_vectors[0]));
    check_vectors(negative_wrap_vectors,
                  sizeof(negative_wrap_vectors) /
                      sizeof(negative_wrap_vectors[0]));
    check_vectors(unsigned_vectors,
                  sizeof(unsigned_vectors) / sizeof(unsigned_vectors[0]));
    check_rejects_non_add_carrier();
    check_rejects_null_arguments();
    return 0;
}
