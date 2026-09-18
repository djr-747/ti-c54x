/*
 * Module contract
 *
 * Owns: executable RED/GREEN proofs for the SRCCD Xmem conditional-store
 * leaf: the TI 0x9D76 *AR5-,AGT worked-example vector (A 00_70FF_FFFF,
 * BRC 0x4321 stages OVERWRITE), the full 16-nibble condition truth table
 * over the shared accumulator-condition evaluator, cond-false WRITEBACK
 * staging, BRC knownness gating, trusted equivalence, and transactional
 * rejection of unknown/malformed inputs and forged decodes.
 * Does not own: the leaf, catalog rows, operand-layout descriptors,
 * address resolution, buses, CPU/state mutation, sequencing, timing,
 * observers, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/store/op_srccd_xmem_cond.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define COND_AGEQ UINT8_C(0x2)
#define COND_AGT UINT8_C(0x6)
#define COND_BEQ UINT8_C(0xD)
#define COND_BGT UINT8_C(0xE)
#define BRC_VALUE UINT16_C(0x4321)

/* SRCCD first word: 1001_1101_S_XMOD_XAR_COND with S pinned 1. */
static uint16_t
srccd_word(uint8_t xmod, uint8_t xar_field, uint8_t cond)
{
    return (uint16_t)(UINT16_C(0x9D00) |
                      ((uint16_t)(xmod & UINT8_C(3)) << 6U) |
                      ((uint16_t)(xar_field & UINT8_C(3)) << 4U) |
                      (uint16_t)(cond & UINT8_C(0x0F)));
}

static ti_c54x_decoded_instruction
decode(uint8_t xmod, uint8_t xar_field, uint8_t cond)
{
    const uint16_t word = srccd_word(xmod, xar_field, cond);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_SRCCD_XMEM_COND);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SELECT_DUAL_XMOD_XAR_COND4);
    TI_C54X_TEST_CHECK(
        instruction.input_resources ==
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_BRC_REGISTER);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint64_t a, uint64_t b, bool a_known, bool b_known, uint16_t brc,
          bool brc_known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = a;
    input.accumulator_b_raw40 = b;
    input.accumulator_a_known = a_known;
    input.accumulator_b_known = b_known;
    input.brc_register_raw16 = brc;
    input.brc_register_known = brc_known;
    return input;
}

static void
assert_rmw_effect(const ti_c54x_op_effect *effect, uint8_t xmod, uint8_t xar,
                  uint16_t value, uint8_t rmw_operation)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.dual.x_mode ==
                       xmod);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.dual.x_ar == xar);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.dual.y_mode ==
                       xmod);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.dual.y_ar == xar);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(!effect->payload.memory.load_t_with_read);
    TI_C54X_TEST_CHECK(!effect->payload.memory.write_smem_delay);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == value);
    TI_C54X_TEST_CHECK(effect->payload.memory.rmw_operation == rmw_operation);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t select, uint8_t xmod, uint8_t xar_field,
                 uint8_t cond)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = select;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = xmod;
    operands.values[1].width_bits = UINT8_C(2);
    operands.values[2].raw_value = xar_field;
    operands.values[2].width_bits = UINT8_C(2);
    operands.values[3].raw_value = cond;
    operands.values[3].width_bits = UINT8_C(4);
    return operands;
}

/* TI SPRU172C SRCCD worked example: SRCCD *AR5-,AGT with A 00_70FF_FFFF
 * (AGT true) stages the BRC source for an always-read/conditional-value
 * RMW; address 0x0202 holds 0x1234 and AR5 post-decrements elsewhere. */
static void
check_ti_vector(void)
{
    const ti_c54x_decoded_instruction instruction = decode(
        UINT8_C(1), UINT8_C(3), COND_AGT);
    const ti_c54x_op_input_snapshot input = input_for(
        UINT64_C(0x0070FFFFFF), UINT64_C(0), true, true, BRC_VALUE, true);
    const ti_c54x_trusted_operand_values operands = trusted_operands(
        UINT8_C(1), UINT8_C(1), UINT8_C(3), COND_AGT);
    ti_c54x_op_effect direct = {0};
    ti_c54x_op_effect trusted = {0};

    TI_C54X_TEST_CHECK(srccd_word(UINT8_C(1), UINT8_C(3), COND_AGT) ==
                       UINT16_C(0x9D76));
    TI_C54X_TEST_CHECK(ti_c54x_op_srccd_xmem_cond(&instruction, &input,
                                                 &direct) ==
                       TI_C54X_OP_STATUS_OK);
    assert_rmw_effect(&direct, UINT8_C(1), UINT8_C(5), BRC_VALUE,
                      TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_srccd_xmem_cond(
                           &instruction, &operands, &input,
                           &trusted) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) == 0);
}

/* Same encoding with A zero: AGT is false, so the leaf stages WRITEBACK
 * (the read word is rewritten) while still staging the BRC value word. */
static void
check_false_condition_writeback(void)
{
    const ti_c54x_decoded_instruction instruction = decode(
        UINT8_C(1), UINT8_C(3), COND_AGT);
    const ti_c54x_op_input_snapshot input = input_for(
        UINT64_C(0), UINT64_C(0), true, true, BRC_VALUE, true);
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_srccd_xmem_cond(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_rmw_effect(&effect, UINT8_C(1), UINT8_C(5), BRC_VALUE,
                      TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK);
}

/* B-class condition over accumulator B: BGT with positive B takes. */
static void
check_b_condition(void)
{
    const ti_c54x_decoded_instruction instruction = decode(
        UINT8_C(0), UINT8_C(0), COND_BGT);
    const ti_c54x_op_input_snapshot input = input_for(
        UINT64_C(0), UINT64_C(0x0070FFFFFF), true, true, BRC_VALUE, true);
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_srccd_xmem_cond(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_rmw_effect(&effect, UINT8_C(0), UINT8_C(2), BRC_VALUE,
                      TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE);
}

/* Full 4-bit nibble sweep with A positive-nonzero and B zero: the twelve
 * TI codes stage OVERWRITE/WRITEBACK per the signed-against-zero compare
 * and the four unassigned nibbles reject. */
static void
check_cond_sweep(void)
{
    static const bool taken[16] = {
        false, false, true, false, true, false, true, false,
        false, false, true, false, false, true, false, true};
    static const bool assigned[16] = {
        false, false, true, true, true, true, true, true,
        false, false, true, true, true, true, true, true};

    for (uint8_t xmod = 0U; xmod <= UINT8_C(3); ++xmod) {
        for (uint8_t xar_field = 0U; xar_field <= UINT8_C(3); ++xar_field) {
            const uint8_t xar = (uint8_t)(xar_field + UINT8_C(2));
            for (uint16_t cond = 0U; cond <= UINT16_C(15); ++cond) {
                const uint8_t cond4 = (uint8_t)cond;
                const ti_c54x_decoded_instruction instruction = decode(
                    xmod, xar_field, cond4);
                const ti_c54x_op_input_snapshot input = input_for(
                    UINT64_C(0x0070FFFFFF), UINT64_C(0), true, true,
                    BRC_VALUE, true);
                const ti_c54x_trusted_operand_values operands =
                    trusted_operands(UINT8_C(1), xmod, xar_field, cond4);
                ti_c54x_op_effect direct = {0};
                ti_c54x_op_effect trusted = {0};
                const uint8_t expected =
                    taken[cond] ? TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE
                                : TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK;

                if (!assigned[cond]) {
                    TI_C54X_TEST_CHECK(
                        ti_c54x_op_srccd_xmem_cond(&instruction, &input,
                                                  &direct) ==
                        TI_C54X_OP_STATUS_INVALID_OPERAND);
                    continue;
                }
                TI_C54X_TEST_CHECK(ti_c54x_op_srccd_xmem_cond(
                                       &instruction, &input,
                                       &direct) == TI_C54X_OP_STATUS_OK);
                assert_rmw_effect(&direct, xmod, xar, BRC_VALUE, expected);
                TI_C54X_TEST_CHECK(
                    ti_c54x_trusted_op_srccd_xmem_cond(
                        &instruction, &operands, &input,
                        &trusted) == TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(memcmp(&direct, &trusted,
                                          sizeof(direct)) == 0);
            }
        }
    }
}

static void
check_unknown_and_malformed_input_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(
        UINT8_C(1), UINT8_C(3), COND_AGT);
    const ti_c54x_op_input_snapshot unknown_a = input_for(
        UINT64_C(0x0070FFFFFF), UINT64_C(0), false, true, BRC_VALUE, true);
    const ti_c54x_op_input_snapshot unknown_brc_taken = input_for(
        UINT64_C(0x0070FFFFFF), UINT64_C(0), true, true, UINT16_C(0), false);
    const ti_c54x_op_input_snapshot unknown_brc_untaken = input_for(
        UINT64_C(0), UINT64_C(0), true, true, UINT16_C(0), false);
    const ti_c54x_op_input_snapshot malformed_acc = input_for(
        TI_C54X_U40_MASK + UINT64_C(1), UINT64_C(0), true, true, BRC_VALUE,
        true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_srccd_xmem_cond(&instruction, NULL,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_srccd_xmem_cond(NULL, &unknown_a,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_srccd_xmem_cond(&instruction, &unknown_a,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(
        ti_c54x_op_srccd_xmem_cond(&instruction, &unknown_brc_taken,
                                   &effect) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(
        ti_c54x_op_srccd_xmem_cond(&instruction, &unknown_brc_untaken,
                                   &effect) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_srccd_xmem_cond(&instruction, &malformed_acc,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void
check_unconsumed_unknown_accumulator_ignored(void)
{
    /* Consumed-input-only knownness: an A condition ignores unknown B. */
    const ti_c54x_decoded_instruction instruction = decode(
        UINT8_C(1), UINT8_C(3), COND_AGT);
    const ti_c54x_op_input_snapshot input = input_for(
        UINT64_C(0x0070FFFFFF), UINT64_C(0), true, false, BRC_VALUE, true);
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_srccd_xmem_cond(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_rmw_effect(&effect, UINT8_C(1), UINT8_C(5), BRC_VALUE,
                      TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE);
}

static void
check_unassigned_nibble_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction = decode(
        UINT8_C(1), UINT8_C(3), UINT8_C(0));
    const ti_c54x_op_input_snapshot input = input_for(
        UINT64_C(0x0070FFFFFF), UINT64_C(0), true, true, BRC_VALUE, true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_srccd_xmem_cond(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void
check_trusted_rejects_bad_operands(void)
{
    const ti_c54x_decoded_instruction instruction = decode(
        UINT8_C(1), UINT8_C(3), COND_BEQ);
    const ti_c54x_op_input_snapshot input = input_for(
        UINT64_C(0), UINT64_C(0), true, true, BRC_VALUE, true);
    const ti_c54x_trusted_operand_values cleared_select =
        trusted_operands(UINT8_C(0), UINT8_C(1), UINT8_C(3), COND_BEQ);
    ti_c54x_trusted_operand_values bad_width = trusted_operands(
        UINT8_C(1), UINT8_C(1), UINT8_C(3), COND_BEQ);
    ti_c54x_op_effect effect = {0};

    /* S is pinned 1 by the SRCCD first-word mask; a cleared select has no
     * encoding and rejects. */
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_srccd_xmem_cond(
                           &instruction, &cleared_select, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    bad_width.values[3].width_bits = UINT8_C(3);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_srccd_xmem_cond(
                           &instruction, &bad_width, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_srccd_xmem_cond(
                           &instruction, NULL, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void
check_forged_instruction_is_transactional(void)
{
    const ti_c54x_decoded_instruction valid = decode(UINT8_C(1), UINT8_C(3),
                                                    COND_AGT);
    const ti_c54x_op_input_snapshot input = input_for(
        UINT64_C(0x0070FFFFFF), UINT64_C(0), true, true, BRC_VALUE, true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_decoded_instruction forged = valid;
    ti_c54x_op_effect effect = before;

    /* Flip a masked bit: 0x9D76 -> 0x9F76 leaves the SRCCD row. */
    forged.bundle.program_words[0] ^= UINT16_C(0x0200);
    TI_C54X_TEST_CHECK(ti_c54x_op_srccd_xmem_cond(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    check_ti_vector();
    check_false_condition_writeback();
    check_b_condition();
    check_cond_sweep();
    check_unknown_and_malformed_input_is_transactional();
    check_unconsumed_unknown_accumulator_ignored();
    check_unassigned_nibble_is_transactional();
    check_trusted_rejects_bad_operands();
    check_forged_instruction_is_transactional();
    return 0;
}
