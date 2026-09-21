/* Module contract
 * Owns: proofs for the FIRS Xmem,Ymem,pmad leaf (TI SPRU172C
 * Sec4 FIRS: A stages (X+Y)<<16 while B accumulates the
 * signed old-A(32-16) times the Pmem(pmad) coefficient,
 * FRCT-doubled): the dual-plus-coefficient staging with the
 * pmad frozen into the triple arm, triple completion values
 * with OVA/OVB/C dispositions, OVM saturation, FRCT
 * doubling, SXM widening of both addends, the trusted
 * two-operand entry, field/range rejection, forged completion
 * rejection, and checked-entry invalid-decode rejection.
 * Does not own: catalog rows, operand-layout descriptors, decode,
 * dispatch, memory addressing or buses, status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/ops/arithmetic/op_firs_dual.h"
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
firs_word(uint8_t xmod, uint8_t xar_raw, uint8_t ymod, uint8_t yar_raw)
{
    return (uint16_t)(UINT16_C(0xE000) |
                      (uint16_t)(((uint16_t)(xmod & UINT8_C(3))) << 6U) |
                      (uint16_t)(((uint16_t)(xar_raw & UINT8_C(3)))
                                 << 4U) |
                      (uint16_t)(((uint16_t)(ymod & UINT8_C(3))) << 2U) |
                      (uint16_t)(yar_raw & UINT8_C(3)));
}

static ti_c54x_decoded_instruction make_instruction(uint16_t first_word,
                                                    uint16_t pmad)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, pmad},
        .program_word_count = UINT8_C(2)};
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

/* Basic step: X=3 Y=5 stages A=(3+5)<<16 while B adds the old
 * A(32-16)=4 times coef=3 to B=7. The pmad freezes into the
 * triple arm beside both sides' modes. */
static void test_basic_step(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(firs_word(UINT8_C(2), UINT8_C(1), UINT8_C(2),
                                   UINT8_C(2)),
                         UINT16_C(0x1000));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       UINT16_C(0xE09A));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] ==
                       UINT16_C(0x1000));
    input.accumulator_a_raw40 = UINT64_C(0x00040000);
    input.accumulator_b_raw40 = UINT64_C(7);
    input.status_sxm = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_op_firs_xmem_ymem_pmad(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(staged.payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_PROGRAM_TRIPLE_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual_program
                           .x_mode == UINT8_C(2));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual_program
                           .x_ar == UINT8_C(3));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual_program
                           .y_mode == UINT8_C(2));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual_program
                           .y_ar == UINT8_C(4));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual_program
                           .program_pmad == UINT16_C(0x1000));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_triple(
                           &instruction, &input, UINT16_C(3),
                           UINT16_C(5), UINT16_C(3),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0x00080000), UINT64_C(19),
                      false, false, false, false);
}

static void test_ovm_saturation_and_frct(void)
{
    /* OVM=1: X=Y=0x7FFF stages (65534<<16)=0xFFFE0000 with a
     * guard mismatch (positive overflow); FRCT=1 doubles the B
     * product (2*4*3=24). */
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(firs_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                   UINT8_C(0)),
                         UINT16_C(0x2000));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x00040000);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    input.status_frct = UINT8_C(1);
    input.status_ovm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_firs_xmem_ymem_pmad(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_triple(
                           &instruction, &input, UINT16_C(0x7FFF),
                           UINT16_C(0x7FFF), UINT16_C(0x0003),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0x007FFFFFFF), UINT64_C(24),
                      true, false, false, false);
}

static void test_sxm_widening(void)
{
    /* X=0x8000: SXM=1 treats it as -32768 (A stages
     * 0xFF80000000 with no overflow), SXM=0 as +32768
     * (A stages 0x0080000000, positive overflow per the
     * shared 40-bit contract). */
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(firs_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                   UINT8_C(0)),
                         UINT16_C(0x3000));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_firs_xmem_ymem_pmad(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_triple(
                           &instruction, &input, UINT16_C(0x8000),
                           UINT16_C(0x0000), UINT16_C(0x0000),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0xFF80000000), UINT64_C(0),
                      false, false, false, false);

    input.status_sxm = UINT8_C(0);
    next = (ti_c54x_op_effect){0};
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_triple(
                           &instruction, &input, UINT16_C(0x8000),
                           UINT16_C(0x0000), UINT16_C(0x0000),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0x0080000000), UINT64_C(0),
                      true, false, false, false);
}

static void test_b_carry_and_overflow(void)
{
    /* B=00 FFFFFFFF plus old-A(32-16)=1 times coef=0x3872 is
     * 0x0100003871 with a bit-31 carry; the 40-bit sum exceeds
     * INT32_MAX, so OVB sets per the shared contract. */
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(firs_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                   UINT8_C(0)),
                         UINT16_C(0x4000));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0x00010000);
    input.accumulator_b_raw40 = UINT64_C(0x00FFFFFFFF);
    input.status_sxm = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_op_firs_xmem_ymem_pmad(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_triple(
                           &instruction, &input, UINT16_C(0x0000),
                           UINT16_C(0x0000), UINT16_C(0x3872),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0), UINT64_C(0x0100003871),
                      false, true, false, true);
}

static void test_trusted_entry(void)
{
    /* The trusted two-operand entry (packed dual byte plus
     * pmad) stages the same triple shape as the checked path. */
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(firs_word(UINT8_C(1), UINT8_C(0), UINT8_C(2),
                                   UINT8_C(1)),
                         UINT16_C(0x5000));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    operands.count = UINT8_C(2);
    operands.values[0].width_bits = UINT8_C(8);
    operands.values[0].raw_value = UINT64_C(0x49);
    operands.values[1].width_bits = UINT8_C(16);
    operands.values[1].raw_value = UINT64_C(0x5000);
    input.accumulator_a_raw40 = UINT64_C(0);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_firs_xmem_ymem_pmad(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual_program
                           .x_mode == UINT8_C(1));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual_program
                           .x_ar == UINT8_C(2));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual_program
                           .y_mode == UINT8_C(2));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual_program
                           .y_ar == UINT8_C(3));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual_program
                           .program_pmad == UINT16_C(0x5000));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_triple(
                           &instruction, &input, UINT16_C(2),
                           UINT16_C(4), UINT16_C(5),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0x00060000), UINT64_C(0),
                      false, false, false, false);
}

static void test_rejection_is_transactional(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(firs_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                   UINT8_C(0)),
                         UINT16_C(0x6000));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;
    ti_c54x_trusted_operand_values operands = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_firs_xmem_ymem_pmad(NULL, &input,
                                                     &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_firs_xmem_ymem_pmad(&instruction, NULL,
                                                     &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_firs_xmem_ymem_pmad(&instruction, &input,
                                                     NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.id = TI_C54X_OPCODE_ID_NOP;
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_op_firs_xmem_ymem_pmad(&forged, &input,
                                                     &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(1);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_firs_xmem_ymem_pmad(
                           &instruction, &operands, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    test_basic_step();
    test_ovm_saturation_and_frct();
    test_sxm_widening();
    test_b_carry_and_overflow();
    test_trusted_entry();
    test_rejection_is_transactional();
    return 0;
}
