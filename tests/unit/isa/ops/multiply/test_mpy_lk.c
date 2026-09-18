/*
 * Module contract
 *
 * Owns: RED proofs for the MPY #lk leaf (syntax 4, UNROUNDED):
 * layout plus first-word gating over ACCUMULATOR_IMMEDIATE_LK16,
 * (T) times signed lk16 STORE completions into both accumulators,
 * negative-lk and negative-T vectors, FRCT shift with FRCT 0/1,
 * OVM saturation with sticky OVdst, input gates, unknown
 * destination tolerance, trusted parity, and transactional
 * rejection. Vectors are derived from the execution rule
 * (T) x lk -> dst; the manual gives no worked example for
 * syntax 4. The R (rounding) suffix form is asserted to defer
 * as INVALID_DECODE.
 * Does not own: catalog rows, operand-layout descriptors,
 * T/accumulator/status mutation, CPU sequencing, timing,
 * observers, diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/multiply/op_mpy_lk.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define LAYOUT_LK16 TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16

static ti_c54x_decoded_instruction make_instruction(
    uint16_t first_word, uint16_t second_word)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.operand_layout = LAYOUT_LK16;
    instruction.bundle.program_words[0] = first_word;
    instruction.bundle.program_words[1] = second_word;
    instruction.bundle.program_word_count = UINT8_C(2);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0xDEAD);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0xBEEF);
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0x0400);
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

    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(instruction, input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    return effect;
}

/* T=0x0400 (1024) times lk=0x1234 (4660) = 4771840 = 0x48D000
 * into A. The stale A/B inputs are overwritten, C/TC kept. */
static void test_basic_store_a(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF066), UINT16_C(0x1234));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x48D000));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* T=2 times lk=0xFFFF (-1) = -2 = 0xFFFFFFFFFE (40-bit) into B. */
static void test_negative_lk_store_b(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF166), UINT16_C(0xFFFF));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.t_register_raw16 = UINT16_C(0x0002);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFFFFFE));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* T=0xFFFF (-1) times lk=0x8000 (-32768) = 32768 = 0x8000. */
static void test_negative_t_times_negative_lk(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF066), UINT16_C(0x8000));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.t_register_raw16 = UINT16_C(0xFFFF);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x8000));
}

/* FRCT=1 doubles the product: T=6 times lk=7 = 42, shifted to 84. */
static void test_frct_shifts_product(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF066), UINT16_C(0x0007));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.t_register_raw16 = UINT16_C(6);
    input.status_frct = UINT8_C(1);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(84));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* FRCT=1 with a negative product: T=2 times lk=0xFFFD (-3) = -6,
 * doubled to -12 = 0xFFFFFFFFF4 (40-bit) into B. */
static void test_frct_negative_product(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF166), UINT16_C(0xFFFD));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.t_register_raw16 = UINT16_C(0x0002);
    input.status_frct = UINT8_C(1);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFFFFF4));
}

/* OVM saturation: T=0x8000 (-32768) times lk=0x8000 (-32768) =
 * 1073741824; FRCT=1 doubles to 2147483648, one past the signed
 * 32-bit maximum, so OVdst sets in both modes: OVM=0 keeps the raw
 * 0x80000000 while OVM=1 pins at 0x007FFFFFFF. The same inputs
 * with FRCT=0 give 0x40000000 with OVdst kept. */
static void test_ovm_saturation(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF066), UINT16_C(0x8000));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect;

    input.t_register_raw16 = UINT16_C(0x8000);
    input.status_frct = UINT8_C(1);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x80000000));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.status_ovm = UINT8_C(1);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.status_frct = UINT8_C(0);
    input.status_ovm = UINT8_C(0);
    effect = run_checked(&instruction, &input);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x40000000));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Wrong layout, wrong word count, the LD #lk sibling base 0xF062,
 * the MPYR #lk rounded base 0xF067, and a bad fixed-bits word all
 * reject as INVALID_DECODE with the effect untouched. */
static void test_decode_rejects(void)
{
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_decoded_instruction wrong_layout =
        make_instruction(UINT16_C(0xF066), UINT16_C(0x1234));
    ti_c54x_decoded_instruction one_word =
        make_instruction(UINT16_C(0xF066), UINT16_C(0x1234));
    ti_c54x_decoded_instruction ld_sibling =
        make_instruction(UINT16_C(0xF062), UINT16_C(0x1234));
    ti_c54x_decoded_instruction rounded =
        make_instruction(UINT16_C(0xF067), UINT16_C(0x1234));
    ti_c54x_decoded_instruction bad_base =
        make_instruction(UINT16_C(0xE066), UINT16_C(0x1234));
    ti_c54x_op_effect before = {0};
    ti_c54x_op_effect effect = before;

    wrong_layout.operand_layout = TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7;
    one_word.bundle.program_word_count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&wrong_layout, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&one_word, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&ld_sibling, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&rounded, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&bad_base, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

/* Unknown T/FRCT/OVM stops; out-of-range FRCT/OVM values reject. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF066), UINT16_C(0x1234));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.t_register_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_frct_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_frct = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* MPY stores the product: destination knownness is not required. */
static void test_checked_unknown_accumulator_tolerated(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF066), UINT16_C(0x1234));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x48D000));
}

/* Trusted [D=1, lk=0x0003] with T=4 stores 12 into B. */
static void test_trusted_completion(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF066), UINT16_C(0x1234));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.t_register_raw16 = UINT16_C(0x0004);
    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0x0003);
    operands.values[1].width_bits = UINT8_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mpy_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(effect.payload.accumulator_write.value_raw40 ==
                       UINT64_C(12));
    TI_C54X_TEST_CHECK(
        effect.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Malformed trusted operands (bad count, destination out of range
 * or wrong width, lk wrong width or out of range) reject as
 * INVALID_OPERAND. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF066), UINT16_C(0x1234));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(1);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0x0003);
    operands.values[1].width_bits = UINT8_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mpy_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(3);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mpy_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(2);
    operands.values[0].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mpy_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[0].raw_value = UINT64_C(1);
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mpy_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].width_bits = UINT8_C(8);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mpy_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[1].raw_value = UINT64_C(0x10000);
    operands.values[1].width_bits = UINT8_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mpy_lk(&instruction, &operands,
                                                &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0xF066), UINT16_C(0x1234));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mpy_lk(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mpy_lk(&instruction, NULL, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mpy_lk(&instruction, &operands,
                                                NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mpy_lk(&instruction, &operands,
                                                &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_basic_store_a();
    test_negative_lk_store_b();
    test_negative_t_times_negative_lk();
    test_frct_shifts_product();
    test_frct_negative_product();
    test_ovm_saturation();
    test_decode_rejects();
    test_checked_input_gates();
    test_checked_unknown_accumulator_tolerated();
    test_trusted_completion();
    test_trusted_shape_rejects();
    test_null_rejects();
    return 0;
}
