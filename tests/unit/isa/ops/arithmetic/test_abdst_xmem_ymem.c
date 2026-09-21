/* Module contract
 * Owns: proofs for the ABDST Xmem,Ymem leaf (TI SPRU172C Sec4
 * ABDST: B accumulates |A(32-16)| with FRCT doubling, A = (X-Y)<<16):
 * the TI worked example, dual-read staging with both sides'
 * modes, pair-completion values with OVA/OVB/C dispositions,
 * OVM saturation on both channels, the FRCT abs-doubling and
 * -32768 edge, SXM widening, the trusted four-operand entry,
 * field/range rejection, forged completion rejection, and
 * checked-entry invalid-decode rejection.
 * Does not own: catalog rows, operand-layout descriptors, decode,
 * dispatch, memory addressing or buses, status mutation, CPU
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/ops/arithmetic/op_distance_dual.h"
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
abdst_word(uint8_t xmod, uint8_t xar_raw, uint8_t ymod, uint8_t yar_raw)
{
    return (uint16_t)(UINT16_C(0xE300) |
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
    bool expect_c_write)
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
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(first->status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        first->status_delta.destination_overflow_action ==
        (expect_ova ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                    : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE));
    TI_C54X_TEST_CHECK(second->destination == UINT8_C(1));
    TI_C54X_TEST_CHECK(second->value_raw40 == expected_b);
    TI_C54X_TEST_CHECK(second->status_delta.c_action ==
                       (expect_c_write
                            ? TI_C54X_OP_STATUS_BIT_WRITE_1
                            : TI_C54X_OP_STATUS_BIT_PRESERVE));
    TI_C54X_TEST_CHECK(second->status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        second->status_delta.destination_overflow_action ==
        (expect_ovb ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                    : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE));
}

/* TI worked example: ABDST *AR3+,*AR4+ with X=0x0055 Y=0x00AA,
 * A=FF ABCD 0000 B=0 FRCT=0 SXM=1 assembles to 0xE39A and
 * completes A=FF FFAB 0000 B=00 1BB1 8229: (-21555)^2. */
static void test_ti_worked_example(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(2), UINT8_C(1), UINT8_C(2),
                                    UINT8_C(2)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       UINT16_C(0xE39A));
    input.accumulator_a_raw40 = UINT64_C(0xFFABCD0000);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
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
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x0055),
                           UINT16_C(0x00AA),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0xFFFFAB0000),
                      UINT64_C(0x5433), false, false, false);
}

/* FRCT doubles the absolute term: B += 2*|-21555|. */
static void test_frct_doubles_abs(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                    UINT8_C(0)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0xFFABCD0000);
    input.accumulator_b_raw40 = UINT64_C(0x100);
    input.status_sxm = UINT8_C(1);
    input.status_frct = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x0055),
                           UINT16_C(0x00AA),
                           &next) == TI_C54X_OP_STATUS_OK);
    /* 2 * 21555 + 0x100 = 0xA866 + 0x100. */
    check_pair_writes(&next, UINT64_C(0xFFFFAB0000),
                      UINT64_C(0xA966), false, false, false);
}

/* ABS -32768 edge: |0x8000| stays 0x8000 without OVM (ABS
 * precedent) and clamps to 0x7FFF with OVM. */
static void test_abs_negative_edge(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                    UINT8_C(0)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0xFF80000000);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0),
                           UINT16_C(0), &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0), UINT64_C(0x8000),
                      false, false, false);

    input.status_ovm = UINT8_C(1);
    (void)memset(&next, 0, sizeof next);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0),
                           UINT16_C(0), &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0), UINT64_C(0x7FFF),
                      false, false, false);
}

/* A-channel overflow: X=0x7FFF Y=0x8000 diffs to 65535, whose
 * <<16 carries guard 0x00 against bit31 1: OVA with OVM
 * saturation, wrapped without. */
static void test_a_channel_overflow(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                    UINT8_C(0)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    input.status_ovm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x7FFF),
                           UINT16_C(0x8000),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0x007FFFFFFF), UINT64_C(0),
                      true, false, false);

    input.status_ovm = UINT8_C(0);
    (void)memset(&next, 0, sizeof next);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x7FFF),
                           UINT16_C(0x8000),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0x00FFFF0000), UINT64_C(0),
                      true, false, false);
}

/* B-channel carry and overflow: B=00 FFFF FFFF plus a nonzero
 * term carries C and overflows OVB with OVM saturation. */
static void test_b_channel_carry_and_overflow(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                    UINT8_C(0)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    /* |A_hi| = 1; B = 00 FFFF FFFF + 1 carries. */
    input.accumulator_a_raw40 = UINT64_C(0x00010000);
    input.accumulator_b_raw40 = UINT64_C(0x00FFFFFFFF);
    input.status_sxm = UINT8_C(1);
    input.status_ovm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0),
                           UINT16_C(0), &next) == TI_C54X_OP_STATUS_OK);
    /* Sum 0x100000000: low32 carry, 32-bit overflow, saturated. */
    check_pair_writes(&next, UINT64_C(0), UINT64_C(0x007FFFFFFF),
                      false, true, true);
}

/* SXM=0 zero-extends: X=0x8000 reads +32768, so A stages
 * 00 8000 0000 with OVA; SXM=1 reads -32768 for FF 8000 0000. */
static void test_sxm_widening(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                    UINT8_C(0)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x8000),
                           UINT16_C(0), &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0x0080000000), UINT64_C(0),
                      true, false, false);

    input.status_sxm = UINT8_C(1);
    (void)memset(&next, 0, sizeof next);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x8000),
                           UINT16_C(0), &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0xFF80000000), UINT64_C(0),
                      false, false, false);
}

/* MOD3 stages on both sides with the resolver owning commits. */
static void test_mod3_stages_both_sides(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(3), UINT8_C(3), UINT8_C(3),
                                    UINT8_C(0)));
    ti_c54x_op_effect staged = {0};

    input.accumulator_a_raw40 = UINT64_C(0);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_mode ==
                       UINT8_C(3));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.x_ar ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.y_mode ==
                       UINT8_C(3));
    TI_C54X_TEST_CHECK(staged.payload.memory.address_spec.dual.y_ar ==
                       UINT8_C(2));
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
}

/* Unknown inputs gate: unknown A/B, unknown SXM/FRCT/OVM, and
 * unknown selected ARs all reject without touching the effect. */
static void test_unknown_inputs_reject(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(1), UINT8_C(0), UINT8_C(1),
                                    UINT8_C(1)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect before = {0};
    ti_c54x_op_input_snapshot input;

    input = blank_input();
    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);

    input = blank_input();
    input.accumulator_b_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);

    input = blank_input();
    input.status_frct_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);

    input = blank_input();
    input.auxiliary_registers_known_mask = UINT8_C(0xF3);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);
}

/* Out-of-range status values reject as invalid operands. */
static void test_bad_status_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                    UINT8_C(0)));
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_op_effect staged = {0};

    input.status_ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
}

/* NULL borrowers reject on every entry and on the completion. */
static void test_null_arguments_reject(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(2), UINT8_C(1), UINT8_C(2),
                                    UINT8_C(2)));
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};
    ti_c54x_trusted_operand_values operands = {0};

    input.accumulator_a_raw40 = UINT64_C(0xFFABCD0000);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           NULL, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, NULL,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           NULL, &input, UINT16_C(0x0055),
                           UINT16_C(0x00AA),
                           &next) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, NULL, UINT16_C(0x0055),
                           UINT16_C(0x00AA),
                           &next) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x0055),
                           UINT16_C(0x00AA),
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abdst_xmem_ymem(
                           &instruction, NULL, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abdst_xmem_ymem(
                           &instruction, &operands, NULL,
                           &staged) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abdst_xmem_ymem(
                           &instruction, &operands, &input,
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

/* Checked entry rejects a foreign layout, a two-word bundle, and
 * foreign first-word prefixes without touching the effect. */
static void test_checked_entry_rejects_bad_decode(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(2), UINT8_C(1), UINT8_C(2),
                                    UINT8_C(2)));
    ti_c54x_decoded_instruction forged;
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect before = {0};

    input.accumulator_a_raw40 = UINT64_C(0xFFABCD0000);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);

    forged = instruction;
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(&forged, &input,
                                                  &staged) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);

    forged = instruction;
    forged.bundle.program_word_count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(&forged, &input,
                                                  &staged) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);

    forged = instruction;
    forged.bundle.program_words[0] = UINT16_C(0xE2AA);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(&forged, &input,
                                                  &staged) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&before, &staged, sizeof staged) == 0);
}

/* Forged completion instructions reject without touching the
 * effect: flipped prefix and foreign layout. */
static void test_completion_forged_decode_rejects(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(2), UINT8_C(1), UINT8_C(2),
                                    UINT8_C(2)));
    ti_c54x_decoded_instruction forged;
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next;
    ti_c54x_op_effect before;

    input.accumulator_a_raw40 = UINT64_C(0xFFABCD0000);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_abdst_xmem_ymem(
                           &instruction, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);

    forged = instruction;
    forged.bundle.program_words[0] ^= UINT16_C(0x0100);
    (void)memset(&next, 0xA5, sizeof next);
    before = next;
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &forged, &input, UINT16_C(0x0055),
                           UINT16_C(0x00AA),
                           &next) == TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&next, &before, sizeof next) == 0);

    forged = instruction;
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    (void)memset(&next, 0xA5, sizeof next);
    before = next;
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &forged, &input, UINT16_C(0x0055),
                           UINT16_C(0x00AA),
                           &next) == TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&next, &before, sizeof next) == 0);
}

/* Trusted entry stages the same read pair from (XMOD, raw XAR,
 * YMOD, raw YAR) and completes the worked-example pair. */
static void test_trusted_stages_read(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(2), UINT8_C(1), UINT8_C(2),
                                    UINT8_C(2)));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_effect next = {0};

    input.accumulator_a_raw40 = UINT64_C(0xFFABCD0000);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    operands.count = UINT8_C(4);
    operands.values[0].width_bits = UINT8_C(2);
    operands.values[0].raw_value = UINT64_C(2);
    operands.values[1].width_bits = UINT8_C(2);
    operands.values[1].raw_value = UINT64_C(1);
    operands.values[2].width_bits = UINT8_C(2);
    operands.values[2].raw_value = UINT64_C(2);
    operands.values[3].width_bits = UINT8_C(2);
    operands.values[3].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abdst_xmem_ymem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(staged.kind == TI_C54X_OP_EFFECT_MEMORY);
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
    TI_C54X_TEST_CHECK(
        ti_c54x_op_memory_effect_is_valid(&staged.payload.memory));
    TI_C54X_TEST_CHECK(staged.payload.memory.complete_dual(
                           &instruction, &input, UINT16_C(0x0055),
                           UINT16_C(0x00AA),
                           &next) == TI_C54X_OP_STATUS_OK);
    check_pair_writes(&next, UINT64_C(0xFFFFAB0000),
                      UINT64_C(0x5433), false, false, false);
}

/* Trusted entry rejects bad arity, bad widths, and out-of-range
 * values. */
static void test_trusted_shape_rejects(void)
{
    ti_c54x_op_input_snapshot input = blank_input();
    ti_c54x_decoded_instruction instruction =
        make_instruction(abdst_word(UINT8_C(2), UINT8_C(1), UINT8_C(2),
                                    UINT8_C(2)));
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_op_effect staged = {0};

    input.accumulator_a_raw40 = UINT64_C(0xFFABCD0000);
    input.accumulator_b_raw40 = UINT64_C(0);
    input.status_sxm = UINT8_C(1);
    operands.count = UINT8_C(3);
    operands.values[0].width_bits = UINT8_C(2);
    operands.values[1].width_bits = UINT8_C(2);
    operands.values[2].width_bits = UINT8_C(2);
    operands.values[3].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abdst_xmem_ymem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);

    operands.count = UINT8_C(4);
    operands.values[3].width_bits = UINT8_C(3);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abdst_xmem_ymem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);

    operands.values[3].width_bits = UINT8_C(2);
    operands.values[2].raw_value = UINT64_C(4);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_abdst_xmem_ymem(
                           &instruction, &operands, &input,
                           &staged) == TI_C54X_OP_STATUS_INVALID_OPERAND);
}

int main(void)
{
    test_ti_worked_example();
    test_frct_doubles_abs();
    test_abs_negative_edge();
    test_a_channel_overflow();
    test_b_channel_carry_and_overflow();
    test_sxm_widening();
    test_mod3_stages_both_sides();
    test_unknown_inputs_reject();
    test_bad_status_rejects();
    test_null_arguments_reject();
    test_checked_entry_rejects_bad_decode();
    test_completion_forged_decode_rejects();
    test_trusted_stages_read();
    test_trusted_shape_rejects();
    return 0;
}
