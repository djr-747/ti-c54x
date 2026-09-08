/* Module contract
 * Owns: pure ADD_ACC_SHIFT/SUB_ACC_SHIFT checked/trusted semantic coverage,
 * including raw40 shifts, C/no-borrow, OVM saturation, sticky overflow,
 * aliasing, and transactional malformed-input rejection.
 * Does not own: catalog generation, CPU/state mutation, timing, or sequencing.
 */

#include "../../../../../src/foundation/arithmetic40_addsub.h"
#include "../../../../../src/foundation/arithmetic40.h"
#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/arithmetic/op_arithmetic.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t make_word(uint16_t base, uint8_t source, uint8_t destination,
                          uint8_t shift)
{
    return (uint16_t)(base | ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U) | shift);
}

static int signed_shift(uint8_t raw)
{
    return (raw & 0x10U) != 0U ? (int)raw - 32 : (int)raw;
}

static uint64_t shifted_raw40(uint64_t raw, int shift, uint8_t sxm)
{
    raw = ti_c54x_u40_mask(raw);
    if (shift > 0) {
        return ti_c54x_u40_mask(raw << (unsigned)shift);
    }
    if (shift < 0) {
        const unsigned count = (unsigned)(-shift);
        return sxm == 0U ? raw >> count
                         : ti_c54x_u40_arithmetic_right_shift(raw, count);
    }
    return raw;
}

static ti_c54x_decoded_instruction decode(uint16_t raw)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(raw);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {raw, 0U}, .program_word_count = 1U};
    ti_c54x_decoded_instruction output;
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &output) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return output;
}

static void check_one(uint16_t base, bool subtract, uint8_t source,
                      uint8_t destination, uint8_t raw_shift, uint8_t sxm,
                      uint8_t ovm, uint64_t a, uint64_t b)
{
    const ti_c54x_decoded_instruction instruction =
        decode(make_word(base, source, destination, raw_shift));
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_sxm = sxm,
        .status_sxm_known = true,
        .status_ovm = ovm,
        .status_ovm_known = true};
    ti_c54x_op_effect direct = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_trusted_operand_values operands = {0};
    const uint64_t source_value = source == 0U ? a : b;
    const uint64_t destination_value = destination == 0U ? a : b;
    const uint64_t shifted = shifted_raw40(source_value,
                                           signed_shift(raw_shift), sxm);
    ti_c54x_u40_add_result add_result = {0};
    ti_c54x_u40_sub_result sub_result = {0};
    uint64_t expected;
    bool expected_c;
    ti_c54x_signed32_overflow_direction overflow;
    const ti_c54x_op_status direct_status = subtract
        ? ti_c54x_op_sub_acc_shift(&instruction, &input, &direct)
        : ti_c54x_op_add_acc_shift(&instruction, &input, &direct);

    operands.values[0] = (ti_c54x_operand_field_value){source, 1U};
    operands.values[1] = (ti_c54x_operand_field_value){destination, 1U};
    operands.values[2] = (ti_c54x_operand_field_value){raw_shift, 5U};
    operands.count = 3U;
    if (subtract) {
        TI_C54X_TEST_CHECK(ti_c54x_u40_sub_checked(destination_value, shifted,
                                                  &sub_result) ==
                           TI_C54X_ARITHMETIC40_ADDSUB_OK);
        expected = sub_result.value;
        expected_c = sub_result.low32_no_borrow;
        overflow = sub_result.signed32_overflow;
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sub_acc_shift(
                               &instruction, &operands, &input, &trusted) ==
                           TI_C54X_OP_STATUS_OK);
    } else {
        TI_C54X_TEST_CHECK(ti_c54x_u40_add_checked(destination_value, shifted,
                                                  &add_result) ==
                           TI_C54X_ARITHMETIC40_ADDSUB_OK);
        expected = add_result.value;
        expected_c = add_result.low32_carry;
        overflow = add_result.signed32_overflow;
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_add_acc_shift(
                               &instruction, &operands, &input, &trusted) ==
                           TI_C54X_OP_STATUS_OK);
    }
    if (ovm != 0U) {
        if (overflow == TI_C54X_SIGNED32_OVERFLOW_POSITIVE) {
            expected = UINT64_C(0x007fffffff);
        } else if (overflow == TI_C54X_SIGNED32_OVERFLOW_NEGATIVE) {
            expected = UINT64_C(0xff80000000);
        }
    }
    TI_C54X_TEST_CHECK(direct_status == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) == 0);
    TI_C54X_TEST_CHECK(direct.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(direct.payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK(direct.payload.accumulator_write.value_raw40 == expected);
    TI_C54X_TEST_CHECK(
        direct.payload.accumulator_write.status_delta.c_action ==
        (expected_c ? TI_C54X_OP_STATUS_BIT_WRITE_1
                    : TI_C54X_OP_STATUS_BIT_WRITE_0));
    TI_C54X_TEST_CHECK(
        direct.payload.accumulator_write.status_delta.tc_action ==
        TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        direct.payload.accumulator_write.status_delta.destination_overflow_action ==
        (overflow == TI_C54X_SIGNED32_OVERFLOW_NONE
             ? TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE
             : TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1));
}

static void check_transactional_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(make_word(UINT16_C(0xF400), 0U, 1U, 0U));
    ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = UINT64_C(0x100000000),
        .accumulator_b_raw40 = UINT64_C(0x0000000001),
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_sxm = 0U,
        .status_sxm_known = false,
        .status_ovm = 0U,
        .status_ovm_known = true};
    ti_c54x_op_effect effect;
    ti_c54x_op_effect before;
    ti_c54x_decoded_instruction forged = instruction;

    memset(&effect, 0xa5, sizeof(effect));
    before = effect;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_acc_shift(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    input.status_sxm_known = true;
    input.status_ovm_known = false;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_acc_shift(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    input.status_ovm_known = true;
    input.accumulator_a_known = false;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_acc_shift(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    input.accumulator_a_known = true;

    forged.id = TI_C54X_OPCODE_ID_SUB_ACC_SHIFT;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_add_acc_shift(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

}

static void check_oracle(uint16_t base, bool subtract, uint8_t source,
                         uint8_t destination, uint8_t raw_shift, uint8_t sxm,
                         uint8_t ovm, uint64_t a, uint64_t b,
                         uint64_t expected, uint8_t expected_c,
                         ti_c54x_op_destination_overflow_action overflow_action)
{
    const ti_c54x_decoded_instruction instruction =
        decode(make_word(base, source, destination, raw_shift));
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_sxm = sxm,
        .status_sxm_known = true,
        .status_ovm = ovm,
        .status_ovm_known = true};
    ti_c54x_op_effect effect = {0};
    const ti_c54x_op_status status = subtract
        ? ti_c54x_op_sub_acc_shift(&instruction, &input, &effect)
        : ti_c54x_op_add_acc_shift(&instruction, &input, &effect);

    TI_C54X_TEST_CHECK(status == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 == expected);
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta.c_action ==
        (expected_c == 0U ? TI_C54X_OP_STATUS_BIT_WRITE_0
                          : TI_C54X_OP_STATUS_BIT_WRITE_1));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta.destination_overflow_action ==
        overflow_action);
}

int main(void)
{
    uint8_t shift;

    for (shift = 0U; shift < 32U; ++shift) {
        check_one(UINT16_C(0xF400), false, 0U, 1U, shift, 0U, 0U,
                  UINT64_C(0x0000000001), UINT64_C(0x0000000001));
        check_one(UINT16_C(0xF420), true, 1U, 0U, shift, 1U, 1U,
                  UINT64_C(0x8000000000), UINT64_C(0x7fffffff));
    }
    /* Guard-only sources prove syntax-9 shifts consume canonical raw40. */
    check_one(UINT16_C(0xF400), false, 0U, 1U, 0x1fU, 0U,
              0U, UINT64_C(0x0000001000), UINT64_C(0x0000000000));
    check_one(UINT16_C(0xF400), false, 0U, 1U, 0x1fU, 1U,
              0U, UINT64_C(0x8000000000), UINT64_C(0x0000000000));
    check_one(UINT16_C(0xF400), false, 0U, 1U, 0x1fU, 0U,
              1U, UINT64_C(0x0000001000), UINT64_C(0x0000000000));
    /* Alias uses original source and destination snapshots. */
    check_one(UINT16_C(0xF400), false, 1U, 1U, 1U, 0U, 0U,
              UINT64_C(0x0000000000), UINT64_C(0x0000000003));
    check_oracle(UINT16_C(0xF400), false, 0U, 0U, 0U, 0U, 0U, 1U, 9U,
                 UINT64_C(2), 0U, TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    check_oracle(UINT16_C(0xF420), true, 0U, 0U, 0U, 0U, 0U, 1U, 9U,
                 UINT64_C(0), 1U, TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    check_oracle(UINT16_C(0xF400), false, 1U, 1U, 0U, 0U, 0U, 9U, 2U,
                 UINT64_C(4), 0U, TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    check_oracle(UINT16_C(0xF420), true, 1U, 1U, 0U, 0U, 0U, 9U, 2U,
                 UINT64_C(0), 1U, TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    /* -16 uses the full raw40 source for both SXM modes. */
    check_oracle(UINT16_C(0xF400), false, 0U, 1U, 0x10U, 0U, 0U, 1U, 0U,
                 UINT64_C(0), 0U, TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    check_oracle(UINT16_C(0xF400), false, 0U, 1U, 0x10U, 1U, 0U,
                 UINT64_C(0xff80000000), 0U, UINT64_C(0xffffff8000), 0U,
                 TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    /* +15 truncates a guard-only bit modulo 40. */
    check_oracle(UINT16_C(0xF400), false, 0U, 1U, 15U, 0U, 0U,
                 UINT64_C(0x100000000), 0U, UINT64_C(0), 0U,
                 TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    /* OVM=0 exposes raw modulo40 overflowing results. */
    check_oracle(UINT16_C(0xF400), false, 0U, 1U, 0U, 0U, 0U,
                 UINT64_C(0x7fffffff), UINT64_C(0x8000000000),
                 UINT64_C(0x807fffffff), 0U,
                 TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    check_oracle(UINT16_C(0xF420), true, 1U, 0U, 0U, 0U, 0U,
                 UINT64_C(0x7fffffff), UINT64_C(0xff80000000),
                 UINT64_C(0xffffffff), 0U,
                 TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    /* OVM clamps both positive and negative overflow directions. */
    check_oracle(UINT16_C(0xF400), false, 1U, 0U, 0U, 0U, 1U,
                 UINT64_C(0x7fffffff), 1U, UINT64_C(0x7fffffff), 0U,
                 TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    check_oracle(UINT16_C(0xF420), true, 1U, 0U, 0U, 0U, 1U,
                 UINT64_C(0xff80000000), 1U, UINT64_C(0xff80000000), 1U,
                 TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    check_oracle(UINT16_C(0xF400), false, 1U, 1U, 0U, 0U, 1U,
                 UINT64_C(0), UINT64_C(0xff80000000),
                 UINT64_C(0xff80000000), 1U,
                 TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    check_transactional_rejection();
    return 0;
}
