/*
 * Module contract
 *
 * Owns: proofs for the NEG src[,dst] leaf: layout and fixed-word
 * validation, 40-bit two's-complement negation with the TI NEG
 * vectors (NEG A,B with A = 0xFF_FFFF_F228 gives B =
 * 0x00_0000_0DD8; NEG B,A with B = 0x00_8000_0000 gives A =
 * 0xFF_8000_0000), minimum-edge OVM saturation (OVM=0 holds
 * 0x80_0000_0000, OVM=1 pins 0x00_7FFF_FFFF) with sticky OVdst,
 * C written (1 iff src == 0) with TC preserved, single
 * accumulator-write effects, checked/trusted parity, input
 * gates, and transactional rejection. Hand-built decoded
 * instructions carry the bound NEG_ACC catalog identity.
 * Does not own: catalog rows, operand-layout descriptors,
 * accumulator/status mutation, CPU sequencing, timing, observers,
 * diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/arithmetic/op_neg_acc.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t neg_acc_first_word(uint8_t source, uint8_t destination)
{
    return (uint16_t)(UINT16_C(0xF484) |
                      (uint16_t)(((uint16_t)(source & UINT8_C(1)))
                                 << 9) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8));
}

static ti_c54x_decoded_instruction make_instruction(uint8_t source,
                                                    uint8_t destination)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_NEG_ACC;
    instruction.operand_layout =
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION;
    instruction.bundle.program_words[0] =
        neg_acc_first_word(source, destination);
    instruction.bundle.program_words[1] = UINT16_C(0);
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0xFFFFFFF228);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x0080000000);
    input.accumulator_b_known = true;
    input.status_ovm = UINT8_C(0);
    input.status_ovm_known = true;
    return input;
}

static ti_c54x_op_effect run_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input)
{
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(instruction, input,
                                         &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    return effect;
}

static void check_accumulator_write(const ti_c54x_op_effect *effect,
                                    uint8_t destination, uint64_t value,
                                    ti_c54x_op_status_bit_action c_action,
                                    ti_c54x_op_destination_overflow_action ov)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.value_raw40 ==
                       value);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.c_action ==
                       c_action);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        effect->payload.accumulator_write.status_delta
            .destination_overflow_action == ov);
}

/* TI NEG Example 1: NEG A,B with A = 0xFF_FFFF_F228 stores B =
 * 0x00_0000_0DD8. A is untouched (single write effect to B), C
 * clears, TC and OVB preserved. */
static void test_neg_a_to_b(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(UINT8_C(0),
                                                               UINT8_C(1));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       UINT16_C(0xF584));
    check_accumulator_write(&next, UINT8_C(1), UINT64_C(0xDD8),
                            TI_C54X_OP_STATUS_BIT_WRITE_0,
                            TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* TI NEG Example 2: NEG B,A with B = 0x00_8000_0000 stores A =
 * 0xFF_8000_0000 with C cleared and OVA preserved. */
static void test_neg_b_to_a(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(UINT8_C(1),
                                                               UINT8_C(0));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next = run_checked(&instruction, &input);

    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       UINT16_C(0xF684));
    check_accumulator_write(&next, UINT8_C(0), UINT64_C(0xFF80000000),
                            TI_C54X_OP_STATUS_BIT_WRITE_0,
                            TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Minimum edge with OVM=0: NEG A with A = 0x80_0000_0000 holds A
 * unchanged (plain 40-bit wrap) with OVA sticky-set and C
 * cleared. */
static void test_min_edge_ovm0(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(UINT8_C(0),
                                                               UINT8_C(0));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x8000000000);
    next = run_checked(&instruction, &input);
    check_accumulator_write(&next, UINT8_C(0), UINT64_C(0x8000000000),
                            TI_C54X_OP_STATUS_BIT_WRITE_0,
                            TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Minimum edge with OVM=1: NEG A with A = 0x80_0000_0000 pins A
 * at 0x00_7FFF_FFFF with OVA sticky-set. */
static void test_min_edge_ovm1(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(UINT8_C(0),
                                                               UINT8_C(0));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x8000000000);
    input.status_ovm = UINT8_C(1);
    next = run_checked(&instruction, &input);
    check_accumulator_write(&next, UINT8_C(0), UINT64_C(0x007FFFFFFF),
                            TI_C54X_OP_STATUS_BIT_WRITE_0,
                            TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Positive-overflow edge from the TI NEG prose: NEG A with A =
 * 0xFF_8000_0000 computes 0x00_8000_0000, which overflows
 * signed-32. OVM=0 holds it with OVA sticky-set; OVM=1 pins
 * 0x00_7FFF_FFFF. */
static void test_positive_overflow_edge(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(UINT8_C(0),
                                                               UINT8_C(0));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    next = run_checked(&instruction, &input);
    check_accumulator_write(&next, UINT8_C(0), UINT64_C(0x0080000000),
                            TI_C54X_OP_STATUS_BIT_WRITE_0,
                            TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);

    input.status_ovm = UINT8_C(1);
    next = run_checked(&instruction, &input);
    check_accumulator_write(&next, UINT8_C(0), UINT64_C(0x007FFFFFFF),
                            TI_C54X_OP_STATUS_BIT_WRITE_0,
                            TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* Zero input: NEG A,B with A = 0 stores B = 0 and sets C. */
static void test_zero_sets_carry(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(UINT8_C(0),
                                                               UINT8_C(1));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0);
    next = run_checked(&instruction, &input);
    check_accumulator_write(&next, UINT8_C(1), UINT64_C(0),
                            TI_C54X_OP_STATUS_BIT_WRITE_1,
                            TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Self-negation D == S writes the source accumulator itself:
 * NEG B,B with B = 1 stores 0xFF_FFFF_FFFF. */
static void test_self_negation(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(UINT8_C(1),
                                                               UINT8_C(1));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect next;

    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       UINT16_C(0xF784));
    input.accumulator_b_raw40 = UINT64_C(1);
    next = run_checked(&instruction, &input);
    check_accumulator_write(&next, UINT8_C(1), UINT64_C(0xFFFFFFFFFF),
                            TI_C54X_OP_STATUS_BIT_WRITE_0,
                            TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Trusted [S,D] matches every checked vector above byte for
 * byte. */
static void test_trusted_parity(void)
{
    static const uint64_t a_values[] = {UINT64_C(0xFFFFFFF228),
                                        UINT64_C(0x8000000000),
                                        UINT64_C(0)};
    static const uint64_t b_values[] = {UINT64_C(0x0080000000),
                                        UINT64_C(0x8000000000),
                                        UINT64_C(1)};
    static const uint8_t ovm_values[] = {UINT8_C(0), UINT8_C(1)};

    for (size_t a = 0U; a < sizeof(a_values) / sizeof(a_values[0]); ++a) {
        for (size_t b = 0U; b < sizeof(b_values) / sizeof(b_values[0]); ++b) {
            for (size_t m = 0U;
                 m < sizeof(ovm_values) / sizeof(ovm_values[0]); ++m) {
                for (uint8_t s = UINT8_C(0); s < UINT8_C(2); ++s) {
                    for (uint8_t d = UINT8_C(0); d < UINT8_C(2); ++d) {
                        const ti_c54x_decoded_instruction instruction =
                            make_instruction(s, d);
                        ti_c54x_trusted_operand_values operands = {0};
                        ti_c54x_op_input_snapshot input = known_input();
                        ti_c54x_op_effect checked = {0};
                        ti_c54x_op_effect trusted = {0};

                        input.accumulator_a_raw40 = a_values[a];
                        input.accumulator_b_raw40 = b_values[b];
                        input.status_ovm = ovm_values[m];
                        const ti_c54x_op_input_snapshot input_before = input;
                        operands.count = UINT8_C(2);
                        operands.values[0].raw_value = s;
                        operands.values[0].width_bits = UINT8_C(1);
                        operands.values[1].raw_value = d;
                        operands.values[1].width_bits = UINT8_C(1);
                        TI_C54X_TEST_CHECK(
                            ti_c54x_op_neg_acc(&instruction, &input,
                                               &checked) ==
                            TI_C54X_OP_STATUS_OK);
                        TI_C54X_TEST_CHECK(
                            ti_c54x_trusted_op_neg_acc(&instruction,
                                                       &operands, &input,
                                                       &trusted) ==
                            TI_C54X_OP_STATUS_OK);
                        TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                                                  sizeof(checked)) == 0);
                        TI_C54X_TEST_CHECK(input_before.accumulator_a_raw40 ==
                                           input.accumulator_a_raw40);
                        (void)input_before;
                    }
                }
            }
        }
    }
}

/* Unknown source or unknown OVM stops; out-of-range src raw40
 * and OVM > 1 reject. */
static void test_checked_input_gates(void)
{
    ti_c54x_decoded_instruction src_a = make_instruction(UINT8_C(0),
                                                         UINT8_C(1));
    ti_c54x_decoded_instruction src_b = make_instruction(UINT8_C(1),
                                                         UINT8_C(0));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(&src_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(&src_b, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();
    input.accumulator_a_raw40 = UINT64_C(0x1FFFFFFFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    input = known_input();
    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(&src_a, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Malformed trusted operands (bad count, source out of range,
 * bad width, destination out of range) reject as
 * INVALID_OPERAND. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_decoded_instruction instruction = make_instruction(UINT8_C(0),
                                                               UINT8_C(1));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(1);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_neg_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.count = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_neg_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_neg_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_neg_acc(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* Wrong id, layout, word count, or fixed opcode word rejects as
 * INVALID_DECODE; nulls reject. Every rejection leaves the
 * caller-owned effect untouched. */
static void test_layout_rejects(void)
{
    ti_c54x_decoded_instruction wrong_id = make_instruction(UINT8_C(0),
                                                            UINT8_C(1));
    ti_c54x_decoded_instruction wrong_layout = make_instruction(UINT8_C(0),
                                                                UINT8_C(1));
    ti_c54x_decoded_instruction two_words = make_instruction(UINT8_C(0),
                                                             UINT8_C(1));
    ti_c54x_decoded_instruction rnd_word = make_instruction(UINT8_C(0),
                                                            UINT8_C(1));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands = {0};

    wrong_id.id = TI_C54X_OPCODE_ID_NOP;
    wrong_layout.operand_layout =
        TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7;
    two_words.bundle.program_word_count = UINT8_C(2);
    rnd_word.bundle.program_words[0] = UINT16_C(0xF59F);
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(&wrong_id, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(&wrong_layout, &input,
                                         &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(&two_words, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(&rnd_word, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(&two_words, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_neg_acc(&two_words, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_neg_acc(
                           &two_words, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_neg_acc(
                           &two_words, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_neg_acc(
                           &two_words, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    test_neg_a_to_b();
    test_neg_b_to_a();
    test_min_edge_ovm0();
    test_min_edge_ovm1();
    test_positive_overflow_edge();
    test_zero_sets_carry();
    test_self_negation();
    test_trusted_parity();
    test_checked_input_gates();
    test_trusted_shape_rejects();
    test_layout_rejects();
    return 0;
}
