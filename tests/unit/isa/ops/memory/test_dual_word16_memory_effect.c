/*
 * Module contract
 *
 * Owns: executable structural proofs for the private dual-indirect WORD16
 * memory effect carrier, including its same-size address arm,
 * MOD0/MOD1/MOD2/AR2..AR5 preconditions, pair completion callback, and
 * transactional rejection of malformed fields. MOD3 remains deferred.
 * Does not own: opcode identity, address arithmetic,
 * endpoint I/O, transaction sequencing, architectural state, timing,
 * observers, or diagnostics.
 */

#include "../../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(ti_c54x_op_memory_address_spec) == 24,
               "dual address arm changed the LP64 address ABI");
_Static_assert(sizeof(ti_c54x_op_memory_effect) == 40,
               "dual callback changed the LP64 memory-effect ABI");
_Static_assert(sizeof(ti_c54x_op_effect) == 48,
               "dual callback changed the LP64 effect ABI");
#elif UINTPTR_MAX == UINT32_MAX
_Static_assert(sizeof(ti_c54x_op_memory_address_spec) == 24,
               "dual address arm changed the ILP32 address ABI");
_Static_assert(sizeof(ti_c54x_op_memory_effect) == 36,
               "dual callback changed the ILP32 memory-effect ABI");
_Static_assert(sizeof(ti_c54x_op_effect) == 40,
               "dual callback changed the ILP32 effect ABI");
#endif

static ti_c54x_op_status
complete_dual(const ti_c54x_decoded_instruction *instruction,
              const ti_c54x_op_input_snapshot *input,
              uint16_t x_word,
              uint16_t y_word,
              ti_c54x_op_effect *next_effect)
{
    TI_C54X_TEST_CHECK(instruction == NULL);
    TI_C54X_TEST_CHECK(input == NULL);
    TI_C54X_TEST_CHECK(x_word == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(y_word == UINT16_C(0x5678));
    TI_C54X_TEST_CHECK(next_effect != NULL);
    *next_effect = (ti_c54x_op_effect){0};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_memory_effect
valid_dual_read(void)
{
    ti_c54x_op_memory_effect effect = {0};

    effect.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16;
    effect.address_spec.dual.x_mode = UINT8_C(0);
    effect.address_spec.dual.x_ar = UINT8_C(2);
    effect.address_spec.dual.y_mode = UINT8_C(0);
    effect.address_spec.dual.y_ar = UINT8_C(5);
    effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    effect.value_word = UINT16_C(0);
    effect.rmw_operation = TI_C54X_OP_MEMORY_RMW_OPERATION_NONE;
    effect.transfer_reserved = UINT8_C(0);
    effect.complete_dual = complete_dual;
    return effect;
}

static void
check_valid_shape_and_callback(void)
{
    ti_c54x_op_memory_effect effect = valid_dual_read();
    ti_c54x_op_effect next_effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));
    TI_C54X_TEST_CHECK(effect.complete_dual(
                           NULL, NULL, UINT16_C(0x1234), UINT16_C(0x5678),
                           &next_effect) == TI_C54X_OP_STATUS_OK);
}

static void
check_post_access_modes_are_valid(void)
{
    ti_c54x_op_memory_effect effect = valid_dual_read();

    effect.address_spec.dual.x_mode = UINT8_C(1);
    effect.address_spec.dual.y_mode = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(&effect));
}

static void
check_rejections_are_transactional(void)
{
    const ti_c54x_op_memory_effect baseline = valid_dual_read();
    ti_c54x_op_memory_effect effect;

#define CHECK_REJECT(member, value)                                             \
    do {                                                                         \
        effect = baseline;                                                       \
        effect.member = (value);                                                 \
        const ti_c54x_op_memory_effect before = effect;                          \
        TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));        \
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);       \
    } while (0)

    CHECK_REJECT(address_spec.dual.x_mode, UINT8_C(4));
    CHECK_REJECT(address_spec.dual.x_ar, UINT8_C(1));
    CHECK_REJECT(address_spec.dual.x_ar, UINT8_C(6));
    CHECK_REJECT(address_spec.dual.y_mode, UINT8_C(4));
    CHECK_REJECT(address_spec.dual.y_ar, UINT8_C(1));
    CHECK_REJECT(address_spec.dual.y_ar, UINT8_C(6));
    CHECK_REJECT(address_spec.dual.reserved_word0, UINT32_C(1));
    CHECK_REJECT(address_spec.dual.reserved_word1, UINT32_C(1));
    CHECK_REJECT(address_spec.dual.reserved_word2, UINT32_C(1));
    CHECK_REJECT(address_spec.dual.reserved_word3, UINT32_C(1));
    CHECK_REJECT(value_word, UINT16_C(1));
    CHECK_REJECT(rmw_operation, TI_C54X_OP_MEMORY_RMW_OPERATION_AND);
    CHECK_REJECT(transfer_reserved, UINT8_C(1));
    CHECK_REJECT(access, TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);

    effect = baseline;
    effect.complete_dual = NULL;
    {
        const ti_c54x_op_memory_effect before = effect;
        TI_C54X_TEST_CHECK(!ti_c54x_op_memory_effect_is_valid(&effect));
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }

#undef CHECK_REJECT
}

int
main(void)
{
    check_valid_shape_and_callback();
    check_post_access_modes_are_valid();
    check_rejections_are_transactional();
    return 0;
}
