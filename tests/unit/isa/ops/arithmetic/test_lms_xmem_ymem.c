/* Module contract
 * Owns: proofs for the LMS Xmem,Ymem leaf (TI SPRU172C Sec4 LMS:
 * A accumulates (X<<16)+2^15 as one 40-bit add while B
 * accumulates the signed X*Y product, FRCT-doubled): the TI
 * worked example, dual-read staging with both sides' modes,
 * pair-completion values with OVA/OVB/C dispositions, OVM
 * saturation, FRCT doubling, SXM widening, the trusted
 * four-operand entry, field/range rejection, forged completion
 * rejection, and checked-entry invalid-decode rejection.
 * Does not own: catalog rows, operand-layout descriptors, decode,
 * dispatch, memory addressing or buses, status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/ops/arithmetic/op_lms_dual.h"
#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/dispatch/trusted_handler.h"
#include "../../../../../src/isa/ops/memory/memory_effect.h"
#include "../../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
lms_word(uint8_t xmod, uint8_t xar_raw, uint8_t ymod, uint8_t yar_raw)
{
    return (uint16_t)(UINT16_C(0xC100) |
                      (uint16_t)(((uint16_t)(xmod & UINT8_C(3))) << 6U) |
                      (uint16_t)(((uint16_t)(xar_raw & UINT8_C(3)))
                                 << 4U) |
                      (uint16_t)(((uint16_t)(ymod & UINT8_C(3))) << 2U) |
                      (uint16_t)(yar_raw & UINT8_C(3)));
}

static ti_c54x_decoded_instruction make_instruction(uint16_t first_word)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    return instruction;
}

static ti_c54x_op_input_snapshot blank_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.auxiliary_registers_known_mask = UINT8_C(0xFF);
    input.accumulator_a_known = true;
    input.accumulator_b_known = true;
    input.status_sxm_known = true;
    input.status_frct_known = true;
    input.status_ovm_known = true;
    return input;
}

static void check_pair_writes(
    const ti_c54x_op_effect *effect, uint64_t expected_a,
    uint64_t expected_b, bool expect_ova, bool expect_ovb,
    bool expect_a_c_write, bool expect_b_c_write)
{
    const ti_c54x_op_accumulator_write_effect *first;
    const ti_c54x_op_accumulator_write_effect *second;

    TI_C54X_TEST_CHECK(effect->kind ==
                       TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE_PAIR);
    first = &effect->payload.accumulator_write_pair.first;
    second = &effect->payload.accumulator_write_pair.second;
    TI_C54X_TEST_CHECK(first->destination == UINT8_C(0));
    TI_C54X_TEST_CHECK(first->value_raw40 == expected_a);
    TI_C54X_TEST_CHECK(first->status_delta.c_action ==
                       (expect_a_c_write
                            ? TI_C54X_OP_STATUS_BIT_WRITE_1
                            : TI_C54X_OP_STATUS_BIT_PRESERVE));
    TI_C54X_TEST_CHECK(first->status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        first->status_delta.destination_overflow_action ==
        (expect_ova ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                    : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE));
    TI_C54X_TEST_CHECK(second->destination == UINT8_C(1));
    TI_C54X_TEST_CHECK(second->value_raw40 == expected_b);
    TI_C54X_TEST_CHECK(second->status_delta.c_action ==
                       (expect_b_c_write
                            ? TI_C54X_OP_STATUS_BIT_WRITE_1
                            : TI_C54X_OP_STATUS_BIT_PRESERVE));
    TI_C54X_TEST_CHECK(second->status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        second->status_delta.destination_overflow_action ==
        (expect_ovb ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                    : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE));
}

/* TI worked example: LMS *AR3+,*AR4+ with X=0x0055 Y=0x00AA,
 * A=00 7777 8888 B=00 0000 0100 FRCT=0 SXM=1 OVM=0 assembles
 * to 0xC19A and completes A=00 77CD 0888 B=00 0000 3972. */
static void test_ti_worked_example(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(lms_word(UINT8_C(2), UINT8_C(1), UINT8_C(2),
                                  UINT8_C(2)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       UINT16_C(0xC19A));
    input.accumulator_a_raw40 = UINT64_C(0x0077778888);
    input.accumulator_b_raw40 = UINT64_C(0x100);
    input.status_sxm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_lms_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(3));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.y_mode ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.y_ar ==
                       UINT8_C(4));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x0055),
                           UINT16_C(0x00AA),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0x0077CD0888),
                      UINT64_C(0x3972), false, false, false, false);
}

static void test_a_carry_and_b_channels(void)
{
    /* A=00 FFFFFFFF, X=0x8000 (SXM=0): term 0x80008000 carries
     * out of bit 31, and the 40-bit mathematical sum
     * (+4294967295 + +2147516416) exceeds INT32_MAX, so OVA
     * sets alongside C per the shared 40-bit add contract. */
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(lms_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                  UINT8_C(0)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x00FFFFFFFF);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_op_lms_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x8000),
                           UINT16_C(0x0000),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0x0180007FFF), UINT64_C(0),
                      true, false, true, false);
}

static void test_ovm_saturation_and_frct(void)
{
    /* OVM=1: A=00 7FFFFF00 plus X=0x7FFF saturates positive;
     * FRCT=1 doubles the B product (2*85*170=28900). */
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(lms_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                  UINT8_C(0)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x007FFFFF00);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    input.status_frct = UINT8_C(1);
    input.status_ovm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_lms_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x0055),
                           UINT16_C(0x00AA),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0x007FFFFFFF),
                      UINT64_C(0x70E4), true, false, false, false);
}

static void test_sxm_widening(void)
{
    /* X=0x8000: SXM=1 treats it as -32768 (A falls by
     * 0x7FFF8000), SXM=0 as +32768. */
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(lms_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                  UINT8_C(0)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_lms_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x8000),
                           UINT16_C(0x0000),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0xFF80008000), UINT64_C(0),
                      false, false, false, false);

    /* SXM=0 widens X to +32768, so the round term is the
     * 33-bit quantity +2147516416: adding it to A=0 exceeds
     * INT32_MAX and OVA sets with no saturation (OVM=0). */
    input.status_sxm = UINT8_C(0);
    next = (ti_c54x_op_effect){0};
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x8000),
                           UINT16_C(0x0000),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0x0080008000), UINT64_C(0),
                      true, false, false, false);
}

static void test_b_carry(void)
{
    /* B=00 FFFFFFFF plus 85*170=0x3872 is 0x0100003871 with a
     * bit-31 carry; the 40-bit mathematical sum (+4294967295 +
     * +14450) exceeds INT32_MAX, so OVB sets per the shared
     * 40-bit add contract (add_checked(0xFFFFFFFF, 1) pins
     * POSITIVE in the foundation suite). */
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(lms_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                  UINT8_C(0)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0);
    input.accumulator_b_raw40 = UINT64_C(0x00FFFFFFFF);
    input.status_sxm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_lms_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x0055),
                           UINT16_C(0x00AA),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0x00558000), UINT64_C(0x0100003871),
                      false, true, false, true);
}

static void test_rejection_is_transactional(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(lms_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                  UINT8_C(0)));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;
    ti_c54x_trusted_operand_values operands = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_lms_xmem_ymem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_lms_xmem_ymem(&instruction, NULL,
                                                &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_lms_xmem_ymem(&instruction, &input,
                                                NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.id = TI_C54X_OPCODE_ID_NOP;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_lms_xmem_ymem(&forged, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(3);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_lms_xmem_ymem(
                           &instruction, &operands, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    test_ti_worked_example();
    test_a_carry_and_b_channels();
    test_ovm_saturation_and_frct();
    test_sxm_widening();
    test_b_carry();
    test_rejection_is_transactional();
    return 0;
}
