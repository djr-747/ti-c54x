/*
 * Module contract
 *
 * Owns: RED proofs for the MAC Smem,#lk,src[,dst] syntax-4 leaf
 * (unrounded only): layout plus 0x6400 first-word gating, S/D/lk
 * selection through the Smem cookie, direct versus
 * standard-indirect memory-read staging with the read-phase T
 * load, the read-value ADD completion of src plus Smem times
 * signed lk into dst through the multiply core, FRCT doubling,
 * OVM saturation with sticky OVdst, C/TC preservation, T-input
 * independence, trusted parity, and transactional rejection. All
 * vectors are derived from the execution rules (dst = src +
 * Smem x lk, (Smem) -> T); the manual gives no worked example
 * for syntax 4.
 * Does not own: catalog rows, operand-layout descriptors,
 * T/accumulator/status mutation, CPU sequencing, timing,
 * observers, diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/decode/operand_layout.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/multiply/op_mac_smem_lk.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define LAYOUT_MAC_SMEM_LK \
    TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7_EXTENSION16

static ti_c54x_decoded_instruction make_instruction(uint16_t first_word,
                                                    uint16_t second_word)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_MAC_SMEM_LK;
    instruction.operand_layout = LAYOUT_MAC_SMEM_LK;
    instruction.repeatability = TI_C54X_REPEATABILITY_ELIGIBLE;
    instruction.input_resources =
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_FRCT_OVM;
    instruction.bundle.program_words[0] = first_word;
    instruction.bundle.program_words[1] = second_word;
    instruction.bundle.program_word_count = UINT8_C(2);
    return instruction;
}

static ti_c54x_op_input_snapshot known_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    /* T is written, never read; the S-selected source accumulator,
     * FRCT, and OVM gate execution. */
    input.accumulator_a_raw40 = UINT64_C(0x0000000100);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x0000000100);
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0x1234);
    input.t_register_known = false;
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

    TI_C54X_TEST_CHECK(ti_c54x_op_mac_smem_lk(instruction, input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect.payload.memory.load_t_with_read == true);
    TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
    return effect;
}

static ti_c54x_op_effect run_completion(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    const ti_c54x_op_effect *staged, uint16_t read_value_word)
{
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(staged->payload.memory.complete(
                           instruction, input, read_value_word, &next) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    return next;
}

/* Direct page-0 word 0x6415 (S=0, D=0, I=0, addr 0x15), lk=3,
 * A=0x100, Smem=2: A = 0x100 + 6 = 0x106. */
static void test_checked_basic_dst_eq_src(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6415), UINT16_C(0x0003));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = run_checked(&instruction, &input);
    ti_c54x_op_effect next =
        run_completion(&instruction, &input, &staged, UINT16_C(0x0002));

    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x106));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

/* Indirect word 0x6594 (S=0, D=1, I=1, MOD2 AR4), lk=3, B=0x100,
 * Smem=2: B = 0x106. */
static void test_checked_dst_differs_from_src(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6594), UINT16_C(0x0003));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = run_checked(&instruction, &input);
    ti_c54x_op_effect next =
        run_completion(&instruction, &input, &staged, UINT16_C(0x0002));

    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD2);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.arf ==
                       UINT8_C(4));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x106));
}

/* Negative lk: A=0x100, Smem=2, lk=-1 (0xFFFF): A = 0xFE. */
static void test_checked_negative_lk(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6415), UINT16_C(0xFFFF));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = run_checked(&instruction, &input);
    ti_c54x_op_effect next =
        run_completion(&instruction, &input, &staged, UINT16_C(0x0002));

    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFE));
}

/* FRCT=1 doubles the product: A=0, Smem=1, lk=1 -> 2. */
static void test_checked_frct_doubles_product(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6415), UINT16_C(0x0001));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0);
    input.status_frct = UINT8_C(1);
    staged = run_checked(&instruction, &input);
    next = run_completion(&instruction, &input, &staged, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(2));
}

/* OVM=1 positive saturation: 0x7FFFFFFF + 1 stays 0x007FFFFFFF
 * with OVdst set. */
static void test_checked_ovm_saturates_positive(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6415), UINT16_C(0x0001));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x007FFFFFFF);
    input.status_ovm = UINT8_C(1);
    staged = run_checked(&instruction, &input);
    next = run_completion(&instruction, &input, &staged, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x007FFFFFFF));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* OVM=0 wraps: 0x7FFFFFFF + 1 -> 0x0080000000 with OVdst set. */
static void test_checked_ovm_clear_wraps(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6415), UINT16_C(0x0001));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged = run_checked(&instruction, &input);
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0x007FFFFFFF);
    staged = run_checked(&instruction, &input);
    next = run_completion(&instruction, &input, &staged, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0080000000));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

/* OVM=1 negative saturation: -2^31 + (-1) stays 0xFF80000000. */
static void test_checked_ovm_saturates_negative(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6415), UINT16_C(0xFFFF));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect staged;
    ti_c54x_op_effect next;

    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    input.status_ovm = UINT8_C(1);
    staged = run_checked(&instruction, &input);
    next = run_completion(&instruction, &input, &staged, UINT16_C(0x0001));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(
        next.payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1);
}

static void test_checked_unknown_inputs_stop(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6415), UINT16_C(0x0003));
    ti_c54x_decoded_instruction split =
        make_instruction(UINT16_C(0x6515), UINT16_C(0x0003));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_smem_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    /* dst==src needs only the shared accumulator; dst!=src needs
     * the destination accumulator too. */
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_smem_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_smem_lk(&split, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_frct_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_smem_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    input = known_input();

    input.status_ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_smem_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void test_checked_exotic_mode_rejects(void)
{
    /* Cookie 0xA2: I=1 with MOD4 is outside MOD0--MOD3. */
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x65A2), UINT16_C(0x0003));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mac_smem_lk(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_NONE);
}

static void test_trusted_parity(void)
{
    /* Word 0x6594: S=0, D=1, cookie 0x94 (I=1, MOD2 AR4); lk=3;
     * B=0x100, Smem=2 -> B=0x106. */
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6594), UINT16_C(0x0003));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect next = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = UINT64_C(0);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT64_C(0x94);
    operands.values[2].width_bits = UINT8_C(8);
    operands.values[3].raw_value = UINT64_C(0x0003);
    operands.values[3].width_bits = UINT8_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mac_smem_lk(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.arf ==
                       UINT8_C(4));
    TI_C54X_TEST_CHECK(effect.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x0002), &next) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(1));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x106));
}

static void test_checked_decode_rejects(void)
{
    ti_c54x_decoded_instruction neighbor =
        make_instruction(UINT16_C(0x6000), UINT16_C(0x0003));
    ti_c54x_decoded_instruction wrong_layout = make_instruction(
        UINT16_C(0x6415), UINT16_C(0x0003));
    ti_c54x_decoded_instruction short_bundle = {0};
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    /* Fixed-bits neighbor outside the 0x6400/0xFC00 slot. */
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_smem_lk(&neighbor, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    /* MAC word under the MPY sibling layout is not this form. */
    wrong_layout.operand_layout =
        TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7_EXTENSION16;
    TI_C54X_TEST_CHECK(
        ti_c54x_op_mac_smem_lk(&wrong_layout, &input, &effect) ==
        TI_C54X_OP_STATUS_INVALID_DECODE);

    short_bundle.id = TI_C54X_OPCODE_ID_MAC_SMEM_LK;
    short_bundle.operand_layout = LAYOUT_MAC_SMEM_LK;
    short_bundle.bundle.program_words[0] = UINT16_C(0x6415);
    short_bundle.bundle.program_word_count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_smem_lk(&short_bundle, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static void test_null_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x6415), UINT16_C(0x0003));
    ti_c54x_op_input_snapshot input = known_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mac_smem_lk(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_smem_lk(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mac_smem_lk(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mac_smem_lk(NULL, NULL, &input,
                                                      &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int main(void)
{
    test_checked_basic_dst_eq_src();
    test_checked_dst_differs_from_src();
    test_checked_negative_lk();
    test_checked_frct_doubles_product();
    test_checked_ovm_saturates_positive();
    test_checked_ovm_clear_wraps();
    test_checked_ovm_saturates_negative();
    test_checked_unknown_inputs_stop();
    test_checked_exotic_mode_rejects();
    test_trusted_parity();
    test_checked_decode_rejects();
    test_null_rejects();
    return 0;
}
