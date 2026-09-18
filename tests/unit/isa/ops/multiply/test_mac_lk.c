/*
 * Module contract
 *
 * Owns: RED proofs for the MAC #lk,src[,dst] syntax-3 leaf
 * (unrounded only): layout plus 0xF067 first-word gating, S/D/lk
 * selection, signed-lk product through the shared multiply core,
 * FRCT doubling, OVM saturation with sticky OVdst, C/TC
 * preservation, T-read input gates, trusted parity, and
 * transactional rejection. All vectors are derived from the
 * execution rules ((T) x lk + (src) -> dst); the manual gives no
 * worked example for syntax 3.
 * Does not own: catalog rows, operand-layout descriptors,
 * T/accumulator/status mutation, CPU sequencing, timing,
 * observers, diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/decode/operand_layout.h"
#include "../../../../../src/isa/ops/multiply/op_mac_lk.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define LAYOUT_MAC_LK \
    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16

static ti_c54x_decoded_instruction make_instruction(
    uint16_t first_word, uint16_t second_word,
    ti_c54x_operand_layout_id layout)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.operand_layout = layout;
    instruction.bundle.program_words[0] = first_word;
    instruction.bundle.program_words[1] = second_word;
    instruction.bundle.program_word_count = UINT8_C(2);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x0000000100);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x0000000100);
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0x0002);
    input.t_register_known = true;
    input.status_frct = UINT8_C(0);
    input.status_frct_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static ti_c54x_op_effect run_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input)
{
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(instruction, input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    return effect;
}

/* MAC #3,A (0xF067, S=0, D=0): A = 0x100 + 2*3 = 0x106, C/TC kept,
 * OVdst kept. */
static void test_checked_basic_dst_eq_src(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF067), UINT16_C(0x0003), LAYOUT_MAC_LK);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x106));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* MAC #3,B,B (0xF367, S=1, D=1): B = 0x100 + 2*3 = 0x106. */
static void test_checked_src_b_dst_b(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF367), UINT16_C(0x0003), LAYOUT_MAC_LK);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x106));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* MAC #3,A,B (0xF167, S=0, D=1): B = A + 2*3 = 0x106. */
static void test_checked_dst_differs_from_src(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF167), UINT16_C(0x0003), LAYOUT_MAC_LK);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x106));
}

/* Negative lk: MAC #-1,A (lk 0xFFFF): A = 0x100 + 4*(-1) = 0xFC. */
static void test_checked_negative_lk(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF067), UINT16_C(0xFFFF), LAYOUT_MAC_LK);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.t_register_raw16 = UINT16_C(0x0004);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFC));
}

/* Negative T times negative lk: T=-2, lk=-3 -> +6; A = 0 + 6. */
static void test_checked_negative_t_times_negative_lk(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF067), UINT16_C(0xFFFD), LAYOUT_MAC_LK);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.accumulator_a_raw40 = UINT64_C(0);
    input.t_register_raw16 = UINT16_C(0xFFFE);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(6));
}

/* FRCT=1 doubles the product: A = 0 + (2*3)*2 = 12. */
static void test_checked_frct_doubles_product(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF067), UINT16_C(0x0003), LAYOUT_MAC_LK);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.accumulator_a_raw40 = UINT64_C(0);
    input.status_frct = UINT8_C(1);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(12));
}

/* FRCT=0 keeps the raw product: A = 0 + 2*3 = 6. */
static void test_checked_frct_clear_keeps_product(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF067), UINT16_C(0x0003), LAYOUT_MAC_LK);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.accumulator_a_raw40 = UINT64_C(0);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(6));
}

/* OVM=1 positive saturation: 0x7FFFFFFF + 1 overflows to
 * 0x007FFFFFFF with OVdst set. */
static void test_checked_ovm_saturates_positive(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF067), UINT16_C(0x0001), LAYOUT_MAC_LK);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.accumulator_a_raw40 = UINT64_C(0x007FFFFFFF);
    input.t_register_raw16 = UINT16_C(0x0001);
    input.status_ovm = UINT8_C(1);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* OVM=0 wraps: 0x7FFFFFFF + 1 -> 0x0080000000 with OVdst still set. */
static void test_checked_ovm_clear_wraps(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF067), UINT16_C(0x0001), LAYOUT_MAC_LK);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.accumulator_a_raw40 = UINT64_C(0x007FFFFFFF);
    input.t_register_raw16 = UINT16_C(0x0001);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0080000000));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* OVM=1 negative saturation: -2^31 + (-1) overflows to
 * 0xFF80000000 with OVdst set. */
static void test_checked_ovm_saturates_negative(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF067), UINT16_C(0xFFFF), LAYOUT_MAC_LK);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    input.t_register_raw16 = UINT16_C(0x0001);
    input.status_ovm = UINT8_C(1);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

static void test_checked_unknown_inputs_stop(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF067), UINT16_C(0x0003), LAYOUT_MAC_LK);
    ti_c54x_decoded_instruction split =
        make_instruction(UINT16_C(0xF167), UINT16_C(0x0003), LAYOUT_MAC_LK);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.t_register_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    /* dst==src needs only the shared accumulator; dst!=src needs
     * the destination accumulator too. */
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(&split, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_frct_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void test_trusted_parity(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF167), UINT16_C(0x0003), LAYOUT_MAC_LK);
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x0003);
    operands.values[2].width_bits = UINT8_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mac_lk(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x106));
}

static void test_checked_decode_rejects(void)
{
    ti_c54x_decoded_instruction add_syn8 =
        make_instruction(UINT16_C(0xF060), UINT16_C(0x0003), LAYOUT_MAC_LK);
    ti_c54x_decoded_instruction wrong_layout = make_instruction(
        UINT16_C(0xF067), UINT16_C(0x0003),
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16);
    ti_c54x_decoded_instruction short_bundle = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    /* Neighboring ADD #lk,16 first word is not a MAC word. */
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(&add_syn8, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    /* MAC word under the shift-4 sibling layout is not this form. */
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(&wrong_layout, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);

    short_bundle.operand_layout = LAYOUT_MAC_LK;
    short_bundle.bundle.program_words[0] = UINT16_C(0xF067);
    short_bundle.bundle.program_word_count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(&short_bundle, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF067), UINT16_C(0x0003), LAYOUT_MAC_LK);
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_lk(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_checked_basic_dst_eq_src();
    test_checked_src_b_dst_b();
    test_checked_dst_differs_from_src();
    test_checked_negative_lk();
    test_checked_negative_t_times_negative_lk();
    test_checked_frct_doubles_product();
    test_checked_frct_clear_keeps_product();
    test_checked_ovm_saturates_positive();
    test_checked_ovm_clear_wraps();
    test_checked_ovm_saturates_negative();
    test_checked_unknown_inputs_stop();
    test_trusted_parity();
    test_checked_decode_rejects();
    test_null_rejects();
    return 0;
}
