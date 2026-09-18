/*
 * Module contract
 *
 * Owns: RED proofs for the row-less NORM src[,dst] leaf: layout
 * and fixed-word validation under the bound NORM_ACC catalog identity,
 * TI SPRU172C NORM vectors (left shift by positive TS, SXM
 * arithmetic right shift by negative TS), OVM saturation of a
 * signed-32 overflowing left shift with sticky OVdst, SXM==0
 * logical right shift, T read-only preservation, C/TC
 * preservation, write to the D-selected accumulator only,
 * checked/trusted parity, input gates, and transactional
 * rejection.
 * Does not own: catalog rows, operand-layout descriptors,
 * accumulator/status mutation, CPU sequencing, timing, observers,
 * diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_norm_acc.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t norm_first_word(uint8_t source, uint8_t destination)
{
    return (uint16_t)(UINT16_C(0xF48F) |
                      (uint16_t)((uint16_t)(source & UINT8_C(1))
                                 << UINT8_C(9)) |
                      (uint16_t)((uint16_t)(destination & UINT8_C(1))
                                 << UINT8_C(8)));
}

static ti_c54x_decoded_instruction make_instruction(uint8_t source,
                                                    uint8_t destination)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_NORM_ACC;
    instruction.operand_layout =
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION;
    instruction.bundle.program_words[0] =
        norm_first_word(source, destination);
    instruction.bundle.program_words[1] = UINT16_C(0);
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
}

static ti_c54x_op_input_snapshot base_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0xFFFFFFF001);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x210A0A0A0A);
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0x0013);
    input.t_register_known = true;
    input.status_sxm = UINT8_C(1);
    input.status_sxm_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t source,
                                                       uint8_t destination)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = (uint64_t)source;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = (uint64_t)destination;
    operands.values[1].width_bits = UINT8_C(1);
    return operands;
}

static void assert_acc_write(const ti_c54x_op_effect *effect,
                             uint8_t destination, uint64_t value,
                             ti_c54x_op_destination_overflow_action ov)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.value_raw40 ==
                       value);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        effect->payload.accumulator_write.status_delta
            .destination_overflow_action == ov);
}

static ti_c54x_op_effect run_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input)
{
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(instruction, input,
                                          &effect) ==
                       TI_C54X_OP_STATUS_OK);
    return effect;
}

/* TI NORM vector: NORM A with A = 0xFF_FFFF_F001 and T = 0x0013
 * (TS = +19) gives A = 0xFF_8008_0000, fitting signed-32 with
 * OV preserved. T is read-only: the single accumulator-write
 * effect carries no T update. */
static void test_ti_left_vector(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT8_C(0));
    ti_c54x_op_input_snapshot input = base_input();
    ti_c54x_op_effect next = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       UINT16_C(0xF48F));
    assert_acc_write(&next, UINT8_C(0), UINT64_C(0xFF80080000),
                     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* TI NORM vector: NORM B,A with B = 0x21_0A0A_0A0A and
 * T = 0x0FF9 (TS = -7) gives A = 0x00_4214_1414 under SXM==1.
 * The source B selects the shift input while D selects the
 * A write target. */
static void test_ti_right_vector(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(1), UINT8_C(0));
    ti_c54x_op_input_snapshot input = base_input();
    ti_c54x_op_effect next;

    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       UINT16_C(0xF68F));
    input.t_register_raw16 = UINT16_C(0x0FF9);
    next = run_checked(&instruction, &input);
    assert_acc_write(&next, UINT8_C(0), UINT64_C(0x0042141414),
                     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Left-shift signed-32 overflow sticky-sets OVdst: OVM==1
 * saturates (positive to 0x00_7FFF_FFFF, negative-overflow to
 * 0xFF_8000_0000) while OVM==0 holds the wrapped value. */
static void test_ovm_saturation(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT8_C(0));
    ti_c54x_op_input_snapshot input = base_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x0040000000);
    input.t_register_raw16 = UINT16_C(0x0002);
    input.status_ovm = UINT8_C(1);
    next = run_checked(&instruction, &input);
    assert_acc_write(&next, UINT8_C(0), UINT64_C(0x007FFFFFFF),
                     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
    input.status_ovm = UINT8_C(0);
    next = run_checked(&instruction, &input);
    assert_acc_write(&next, UINT8_C(0), UINT64_C(0x0100000000),
                     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);

    /* Negative source overflowing positive: A = 0xFF_8000_0000
     * (INT_MIN, fitting) shifted left once wraps to
     * 0xFF_0000_0000, whose bit 31 is clear above guard 0xFF. */
    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    input.t_register_raw16 = UINT16_C(0x0001);
    input.status_ovm = UINT8_C(1);
    next = run_checked(&instruction, &input);
    assert_acc_write(&next, UINT8_C(0), UINT64_C(0x007FFFFFFF),
                     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);

    /* Negative overflow under a zero shift: A = 0x00_C000_0000
     * has bit 31 set above guard 0x00. */
    input.accumulator_a_raw40 = UINT64_C(0x00C0000000);
    input.t_register_raw16 = UINT16_C(0x0000);
    next = run_checked(&instruction, &input);
    assert_acc_write(&next, UINT8_C(0), UINT64_C(0xFF80000000),
                     TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Right shifts never overflow: SXM==0 zero-fills while SXM==1
 * sign-extends. A = 0xFF_8000_0001 shifted right once gives
 * 0x7F_C000_0000 logical versus 0xFF_C000_0000 arithmetic. */
static void test_sxm_right_shift(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT8_C(0));
    ti_c54x_op_input_snapshot input = base_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFF80000001);
    input.t_register_raw16 = UINT16_C(0xFFFF);
    input.status_sxm = UINT8_C(0);
    next = run_checked(&instruction, &input);
    assert_acc_write(&next, UINT8_C(0), UINT64_C(0x7FC0000000),
                     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
    input.status_sxm = UINT8_C(1);
    next = run_checked(&instruction, &input);
    assert_acc_write(&next, UINT8_C(0), UINT64_C(0xFFC0000000),
                     TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Unknown source, OVM, SXM, or T stops; out-of-range src raw40
 * and status values reject. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction src_a =
        make_instruction(UINT8_C(0), UINT8_C(0));
    ti_c54x_decoded_instruction src_b =
        make_instruction(UINT8_C(1), UINT8_C(0));
    ti_c54x_op_input_snapshot input = base_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = base_input();
    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(&src_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    input = base_input();
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(&src_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = base_input();
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = base_input();
    input.status_sxm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = base_input();
    input.t_register_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = base_input();
    input.t_register_known = false;
    input.status_sxm = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = base_input();
    input.accumulator_a_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = base_input();
    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = base_input();
    input.status_sxm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Checked and trusted entries agree on every field combination
 * over representative inputs, leaving inputs untouched. */
static void test_checked_trusted_parity(void)
{
    static const uint64_t sources[] = {UINT64_C(0xFFFFFFF001),
                                       UINT64_C(0x210A0A0A0A),
                                       UINT64_C(0x0040000000),
                                       UINT64_C(0xFF80000001)};
    static const uint16_t counts[] = {UINT16_C(0x0013), UINT16_C(0x0FF9),
                                      UINT16_C(0x0002), UINT16_C(0xFFFF)};
    static const uint8_t sxms[] = {UINT8_C(0), UINT8_C(1)};
    static const uint8_t ovms[] = {UINT8_C(0), UINT8_C(1)};

    for (size_t s = 0U; s < sizeof(sources) / sizeof(sources[0]); ++s) {
        for (size_t t = 0U; t < sizeof(counts) / sizeof(counts[0]); ++t) {
            for (size_t x = 0U; x < sizeof(sxms) / sizeof(sxms[0]); ++x) {
                for (size_t o = 0U; o < sizeof(ovms) / sizeof(ovms[0]); ++o) {
                    for (uint8_t src = UINT8_C(0); src <= UINT8_C(1);
                         ++src) {
                        for (uint8_t dst = UINT8_C(0); dst <= UINT8_C(1);
                             ++dst) {
                            const ti_c54x_decoded_instruction instruction =
                                make_instruction(src, dst);
                            const ti_c54x_trusted_operand_values operands =
                                trusted_operands(src, dst);
                            ti_c54x_op_input_snapshot input = base_input();
                            ti_c54x_op_input_snapshot before;
                            const ti_c54x_op_effect poison = {
                                .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                                .payload.repeat_start = {
                                    .operand = UINT16_C(0xA55A)}};
                            ti_c54x_op_effect checked = poison;
                            ti_c54x_op_effect trusted = poison;
                            ti_c54x_op_status checked_status;
                            ti_c54x_op_status trusted_status;

                            if (src == UINT8_C(0)) {
                                input.accumulator_a_raw40 = sources[s];
                            } else {
                                input.accumulator_b_raw40 = sources[s];
                            }
                            input.t_register_raw16 = counts[t];
                            input.status_sxm = sxms[x];
                            input.status_ovm = ovms[o];
                            before = input;
                            checked_status = ti_c54x_op_norm_acc(
                                &instruction, &input, &checked);
                            trusted_status = ti_c54x_trusted_op_norm_acc(
                                &instruction, &operands, &input, &trusted);
                            TI_C54X_TEST_CHECK(checked_status ==
                                               TI_C54X_OP_STATUS_OK);
                            TI_C54X_TEST_CHECK(trusted_status ==
                                               TI_C54X_OP_STATUS_OK);
                            TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                                                      sizeof(checked)) == 0);
                            TI_C54X_TEST_CHECK(
                                checked.payload.accumulator_write
                                    .destination == dst);
                            TI_C54X_TEST_CHECK(memcmp(&input, &before,
                                                      sizeof(input)) == 0);
                        }
                    }
                }
            }
        }
    }
}

/* Wrong id, layout, word count, or fixed opcode word rejects as
 * INVALID_DECODE; nulls reject; the effect stays untouched. */
static void test_layout_rejects(void)
{
    ti_c54x_decoded_instruction wrong_id =
        make_instruction(UINT8_C(0), UINT8_C(0));
    ti_c54x_decoded_instruction wrong_layout =
        make_instruction(UINT8_C(0), UINT8_C(0));
    ti_c54x_decoded_instruction two_words =
        make_instruction(UINT8_C(0), UINT8_C(0));
    ti_c54x_decoded_instruction rnd_word =
        make_instruction(UINT8_C(0), UINT8_C(0));
    ti_c54x_op_input_snapshot input = base_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT8_C(0));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};

    wrong_id.id = TI_C54X_OPCODE_ID_NOP;
    wrong_layout.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    two_words.bundle.program_word_count = UINT8_C(2);
    rnd_word.bundle.program_words[0] = UINT16_C(0xF49F);

#define CHECK_INVALID_DECODE(candidate)                                         \
    do {                                                                        \
        effect = before;                                                        \
        TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(                                  \
                               &(candidate), &input, &effect) ==                \
                           TI_C54X_OP_STATUS_INVALID_DECODE);                  \
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);      \
    } while (0)

    CHECK_INVALID_DECODE(wrong_id);
    CHECK_INVALID_DECODE(wrong_layout);
    CHECK_INVALID_DECODE(two_words);
    CHECK_INVALID_DECODE(rnd_word);
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(&two_words, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_norm_acc(&two_words, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_norm_acc(&two_words, NULL, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_norm_acc(&two_words, &operands,
                                                  NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_norm_acc(&two_words, &operands,
                                                  &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

#undef CHECK_INVALID_DECODE
}

/* Malformed trusted operands (bad count, out-of-range value, bad
 * width) reject as INVALID_OPERAND. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(1), UINT8_C(0));
    ti_c54x_op_input_snapshot input = base_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(1), UINT8_C(0));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};

    effect = before;
    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_norm_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0));
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_norm_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0));
    operands.values[1].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_norm_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    test_ti_left_vector();
    test_ti_right_vector();
    test_ovm_saturation();
    test_sxm_right_shift();
    test_checked_input_gates();
    test_checked_trusted_parity();
    test_layout_rejects();
    test_trusted_shape_rejects();
    return 0;
}
