/* Module contract
 * Owns: focused checked/trusted proofs for the fourteen MOD3-involving
 * syntax-6 dual-memory ADD/SUB rows, including per-row identity, staged
 * X/Y mode carriage, checked/trusted parity, destination routing, and
 * transactional wrong-identity rejection. Shared fixed-shift arithmetic is
 * owned by the MOD0 proofs and probed here only by destination/value spots.
 * Does not own: catalog generation, address resolution, bus execution, CPU
 * mutation, sequencing, timing, or repeat eligibility.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/arithmetic/op_dual_arithmetic.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*checked_fn)(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
typedef ti_c54x_op_status (*trusted_fn)(
    const ti_c54x_decoded_instruction *,
    const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);

typedef struct dual_mod3_case {
    uint16_t base;
    uint8_t x_mode;
    uint8_t y_mode;
    bool is_sub;
    checked_fn checked;
    trusted_fn trusted;
} dual_mod3_case;

static const dual_mod3_case cases[] = {
    {UINT16_C(0xA00C), UINT8_C(0), UINT8_C(3), false,
     ti_c54x_op_add_dual_m0_m3, ti_c54x_trusted_op_add_dual_m0_m3},
    {UINT16_C(0xA04C), UINT8_C(1), UINT8_C(3), false,
     ti_c54x_op_add_dual_m1_m3, ti_c54x_trusted_op_add_dual_m1_m3},
    {UINT16_C(0xA08C), UINT8_C(2), UINT8_C(3), false,
     ti_c54x_op_add_dual_m2_m3, ti_c54x_trusted_op_add_dual_m2_m3},
    {UINT16_C(0xA0C0), UINT8_C(3), UINT8_C(0), false,
     ti_c54x_op_add_dual_m3_m0, ti_c54x_trusted_op_add_dual_m3_m0},
    {UINT16_C(0xA0C4), UINT8_C(3), UINT8_C(1), false,
     ti_c54x_op_add_dual_m3_m1, ti_c54x_trusted_op_add_dual_m3_m1},
    {UINT16_C(0xA0C8), UINT8_C(3), UINT8_C(2), false,
     ti_c54x_op_add_dual_m3_m2, ti_c54x_trusted_op_add_dual_m3_m2},
    {UINT16_C(0xA0CC), UINT8_C(3), UINT8_C(3), false,
     ti_c54x_op_add_dual_m3_m3, ti_c54x_trusted_op_add_dual_m3_m3},
    {UINT16_C(0xA20C), UINT8_C(0), UINT8_C(3), true,
     ti_c54x_op_sub_dual_m0_m3, ti_c54x_trusted_op_sub_dual_m0_m3},
    {UINT16_C(0xA24C), UINT8_C(1), UINT8_C(3), true,
     ti_c54x_op_sub_dual_m1_m3, ti_c54x_trusted_op_sub_dual_m1_m3},
    {UINT16_C(0xA28C), UINT8_C(2), UINT8_C(3), true,
     ti_c54x_op_sub_dual_m2_m3, ti_c54x_trusted_op_sub_dual_m2_m3},
    {UINT16_C(0xA2C0), UINT8_C(3), UINT8_C(0), true,
     ti_c54x_op_sub_dual_m3_m0, ti_c54x_trusted_op_sub_dual_m3_m0},
    {UINT16_C(0xA2C4), UINT8_C(3), UINT8_C(1), true,
     ti_c54x_op_sub_dual_m3_m1, ti_c54x_trusted_op_sub_dual_m3_m1},
    {UINT16_C(0xA2C8), UINT8_C(3), UINT8_C(2), true,
     ti_c54x_op_sub_dual_m3_m2, ti_c54x_trusted_op_sub_dual_m3_m2},
    {UINT16_C(0xA2CC), UINT8_C(3), UINT8_C(3), true,
     ti_c54x_op_sub_dual_m3_m3, ti_c54x_trusted_op_sub_dual_m3_m3},
};

static ti_c54x_decoded_instruction decode(uint16_t word)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, 0U}, .program_word_count = 1U};
    ti_c54x_decoded_instruction output;

    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &output) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return output;
}

static ti_c54x_op_input_snapshot input_snapshot(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.auxiliary_registers_known_mask = UINT8_C(0xff);
    input.status_c = UINT8_C(1);
    input.status_c_known = true;
    input.status_sxm = UINT8_C(0);
    input.status_sxm_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(void)
{
    ti_c54x_trusted_operand_values output = {0};

    output.values[0] = (ti_c54x_operand_field_value){0U, 1U};
    output.values[1] = (ti_c54x_operand_field_value){0U, 2U};
    output.values[2] = (ti_c54x_operand_field_value){1U, 2U};
    output.count = 3U;
    return output;
}

static void check_mode_carriage_and_parity(const dual_mod3_case *row)
{
    /* D=0, Xar=0 (AR2), Yar=1 (AR3). */
    const uint16_t word = (uint16_t)(row->base | UINT16_C(0x0001));
    const ti_c54x_decoded_instruction instruction = decode(word);
    const ti_c54x_op_input_snapshot input = input_snapshot();
    const ti_c54x_trusted_operand_values trusted = trusted_operands();
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted_effect = {0};
    ti_c54x_op_effect completed = {0};
    const uint16_t x_word = row->is_sub ? UINT16_C(5) : UINT16_C(1);
    const uint16_t y_word = UINT16_C(2);

    TI_C54X_TEST_CHECK(row->checked(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(row->trusted(&instruction, &trusted, &input,
                                    &trusted_effect) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(checked.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.dual.x_mode ==
                       row->x_mode);
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.dual.y_mode ==
                       row->y_mode);
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(checked.payload.memory.address_spec.dual.y_ar ==
                       UINT8_C(3));
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
        &checked.payload.memory));
    TI_C54X_TEST_CHECK(memcmp(&checked.payload.memory,
                              &trusted_effect.payload.memory,
                              sizeof(checked.payload.memory)) == 0);
    TI_C54X_TEST_CHECK(checked.payload.memory.complete_dual(
                           &instruction, &input, x_word, y_word,
                           &completed) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(completed.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    /* D=0 routes to accumulator A, which is destination index 0. */
    TI_C54X_TEST_CHECK(completed.payload.accumulator_write.destination ==
                       UINT8_C(0));
    /* (1+2)<<16 and (5-2)<<16 share the 0x00030000 fixed-shift spot value. */
    TI_C54X_TEST_CHECK(completed.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x00030000));
}

static void check_wrong_identity_rejects(const dual_mod3_case *row)
{
    /* The MOD0 sibling decodes to a different row and must be rejected. */
    const uint16_t mod0_word = (uint16_t)(
        (row->is_sub ? UINT16_C(0xA200) : UINT16_C(0xA000)) |
        UINT16_C(0x0001));
    const ti_c54x_decoded_instruction mod0_instruction = decode(mod0_word);
    const ti_c54x_op_input_snapshot input = input_snapshot();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(row->checked(&mod0_instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_NONE);
}

int main(void)
{
    size_t index;

    for (index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        check_mode_carriage_and_parity(&cases[index]);
        check_wrong_identity_rejects(&cases[index]);
    }
    return 0;
}
