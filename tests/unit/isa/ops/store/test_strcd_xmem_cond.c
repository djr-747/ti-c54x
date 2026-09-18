/*
 * Module contract
 *
 * Owns: executable RED/GREEN proofs for the STRCD conditional-store
 * leaf: the TI SPRU172C 0x9C76-style *AR5-,AGT vector with T source
 * (cond-true stages OVERWRITE with the T word), cond-false stages
 * WRITEBACK with an ignored zero word, always-read DUAL_INDIRECT
 * X_ONLY addressing with Y self-aliased, consumed-input-only
 * knownness (unknown T and unknown selected accumulator reject;
 * unknown unselected accumulator is ignored), unassigned cond
 * nibbles rejecting, checked/trusted equivalence, and
 * transactional rejection of malformed and forged inputs.
 * Does not own: the leaf, the cond evaluator, catalog rows,
 * operand-layout descriptors, address resolution, buses,
 * CPU/state mutation, sequencing, timing, observers, or
 * diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/store/op_strcd_xmem_cond.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define COND_AGT UINT8_C(0x6)
#define COND_BEQ UINT8_C(0xD)
#define COND_BGT UINT8_C(0xE)

static uint16_t
strcd_word(uint8_t select, uint8_t xmod, uint8_t xar_field, uint8_t cond)
{
    return (uint16_t)(UINT16_C(0x9C00) |
                      ((uint16_t)(select & UINT8_C(1)) << 8U) |
                      ((uint16_t)(xmod & UINT8_C(3)) << 6U) |
                      ((uint16_t)(xar_field & UINT8_C(3)) << 4U) |
                      (uint16_t)(cond & UINT8_C(0x0F)));
}

static ti_c54x_decoded_instruction
decode(uint8_t select, uint8_t xmod, uint8_t xar_field, uint8_t cond)
{
    const uint16_t word = strcd_word(select, xmod, xar_field, cond);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_STRCD_XMEM_COND);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SELECT_DUAL_XMOD_XAR_COND4);
    TI_C54X_TEST_CHECK(
        instruction.input_resources ==
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_T_REGISTER);
    return instruction;
}

static ti_c54x_op_input_snapshot
make_input(uint64_t acc_a, bool acc_a_known, uint64_t acc_b,
           bool acc_b_known, uint16_t t_word, bool t_known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = acc_a;
    input.accumulator_a_known = acc_a_known;
    input.accumulator_b_raw40 = acc_b;
    input.accumulator_b_known = acc_b_known;
    input.t_register_raw16 = t_word;
    input.t_register_known = t_known;
    return input;
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

static void
assert_rmw_read(const ti_c54x_op_effect *effect, uint8_t xmod, uint8_t xar,
                uint8_t rmw_operation, uint16_t value_word)
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
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.dual.reserved_word0 ==
        UINT32_C(0));
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.dual.reserved_word1 ==
        UINT32_C(0));
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.dual.reserved_word2 ==
        UINT32_C(0));
    TI_C54X_TEST_CHECK(
        effect->payload.memory.address_spec.dual.reserved_word3 ==
        UINT32_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(!effect->payload.memory.load_t_with_read);
    TI_C54X_TEST_CHECK(!effect->payload.memory.write_smem_delay);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == value_word);
    TI_C54X_TEST_CHECK(effect->payload.memory.rmw_operation ==
                       rmw_operation);
    TI_C54X_TEST_CHECK(effect->payload.memory.transfer_reserved ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void
check_ti_strcd_ar5_postdec_agt_taken(void)
{
    /* TI SPRU172C STRCD worked-example shape: STRCD *AR5-,AGT with
     * A 00_70FF_FFFF (AGT true), T 4321. The leaf stages an
     * always-read RMW with OVERWRITE carrying the T word; the root
     * reads Xmem, ignores it, writes 4321, and post-decrements AR5. */
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(1), UINT8_C(3), COND_AGT);
    /* The builder must reproduce the TI 0x9C76 first-word spelling:
     * S=0, XMOD=01, XAR=11 (AR5), COND=0110 (AGT). */
    ti_c54x_op_input_snapshot input =
        make_input(UINT64_C(0x0070FFFFFF), true, UINT64_C(0), false,
                   UINT16_C(0x4321), true);
    const ti_c54x_trusted_operand_values operands = trusted_operands(
        UINT8_C(0), UINT8_C(1), UINT8_C(3), COND_AGT);
    ti_c54x_op_effect direct = {0};
    ti_c54x_op_effect trusted = {0};

    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       UINT16_C(0x9C76));
    TI_C54X_TEST_CHECK(ti_c54x_op_strcd_xmem_cond(&instruction, &input,
                                                 &direct) ==
                       TI_C54X_OP_STATUS_OK);
    assert_rmw_read(&direct, UINT8_C(1), UINT8_C(5),
                    (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE,
                    UINT16_C(0x4321));
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_strcd_xmem_cond(
                           &instruction, &operands, &input, &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) == 0);
}

static void
check_cond_false_stages_writeback(void)
{
    /* Same encoding with A zero: AGT is false, so the leaf stages
     * WRITEBACK with an ignored zero word; the root rewrites the
     * read Xmem word and ignores the staged value. */
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(1), UINT8_C(3), COND_AGT);
    const ti_c54x_op_input_snapshot input =
        make_input(UINT64_C(0), true, UINT64_C(0), true,
                   UINT16_C(0x4321), true);
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_strcd_xmem_cond(&instruction, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_rmw_read(&effect, UINT8_C(1), UINT8_C(5),
                    (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK,
                    UINT16_C(0));
}

static void
check_b_code_reads_accumulator_b(void)
{
    /* BGT over a zero B is false (WRITEBACK); BEQ over a zero B is
     * true (OVERWRITE with T). Both prove the B-code accumulator
     * routing through the leaf. */
    const ti_c54x_decoded_instruction bgt =
        decode(UINT8_C(0), UINT8_C(2), UINT8_C(0), COND_BGT);
    const ti_c54x_decoded_instruction beq =
        decode(UINT8_C(0), UINT8_C(2), UINT8_C(0), COND_BEQ);
    const ti_c54x_op_input_snapshot input =
        make_input(UINT64_C(0x0070FFFFFF), true, UINT64_C(0), true,
                   UINT16_C(0x1111), true);
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_strcd_xmem_cond(&bgt, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_rmw_read(&effect, UINT8_C(2), UINT8_C(2),
                    (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK,
                    UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_op_strcd_xmem_cond(&beq, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_rmw_read(&effect, UINT8_C(2), UINT8_C(2),
                    (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE,
                    UINT16_C(0x1111));
}

/* Expected taken outcome per cond nibble for the sweep input below
 * (A positive nonzero, B zero): the twelve TI codes from the S48
 * seam, with the four unassigned nibbles marked via
 * expected_status. */
typedef struct sweep_expectation {
    bool assigned;
    bool taken;
} sweep_expectation;

static const sweep_expectation SWEEP_EXPECT[16] = {
    {false, false}, {false, false}, /* 0x0, 0x1 unassigned */
    {true, true},                   /* AGEQ over positive A */
    {true, false},                  /* ALT over positive A */
    {true, true},                   /* ANEQ over positive A */
    {true, false},                  /* AEQ over positive A */
    {true, true},                   /* AGT over positive A */
    {true, false},                  /* ALEQ over positive A */
    {false, false}, {false, false}, /* 0x8, 0x9 unassigned */
    {true, true},                   /* BGEQ over zero B */
    {true, false},                  /* BLT over zero B */
    {true, false},                  /* BNEQ over zero B */
    {true, true},                   /* BEQ over zero B */
    {true, false},                  /* BGT over zero B */
    {true, true},                   /* BLEQ over zero B */
};

static void
check_encoding_sweep_with_trusted_equivalence(void)
{
    const ti_c54x_op_input_snapshot input =
        make_input(UINT64_C(0x0070FFFFFF), true, UINT64_C(0), true,
                   UINT16_C(0x4321), true);

    for (uint8_t xmod = UINT8_C(0); xmod <= UINT8_C(3); ++xmod) {
        for (uint8_t xar_field = UINT8_C(0); xar_field <= UINT8_C(3);
             ++xar_field) {
            const uint8_t xar = (uint8_t)(xar_field + UINT8_C(2));
            for (uint8_t cond = UINT8_C(0); cond <= UINT8_C(15);
                 ++cond) {
                const ti_c54x_decoded_instruction instruction =
                    decode(UINT8_C(0), xmod, xar_field, cond);
                const ti_c54x_trusted_operand_values operands =
                    trusted_operands(UINT8_C(0), xmod, xar_field,
                                     cond);
                const ti_c54x_op_effect sentinel = {
                    .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                    .payload.repeat_start = {.operand =
                                                 UINT16_C(0xCAFE)}};
                ti_c54x_op_effect direct = sentinel;
                ti_c54x_op_effect trusted = sentinel;

                if (!SWEEP_EXPECT[cond].assigned) {
                    TI_C54X_TEST_CHECK(
                        ti_c54x_op_strcd_xmem_cond(
                            &instruction, &input, &direct) ==
                        TI_C54X_OP_STATUS_INVALID_OPERAND);
                    TI_C54X_TEST_CHECK(memcmp(&direct, &sentinel,
                                              sizeof(direct)) == 0);
                    TI_C54X_TEST_CHECK(
                        ti_c54x_trusted_op_strcd_xmem_cond(
                            &instruction, &operands, &input,
                            &trusted) ==
                        TI_C54X_OP_STATUS_INVALID_OPERAND);
                    TI_C54X_TEST_CHECK(memcmp(&trusted, &sentinel,
                                              sizeof(trusted)) == 0);
                    continue;
                }
                TI_C54X_TEST_CHECK(ti_c54x_op_strcd_xmem_cond(
                                       &instruction, &input,
                                       &direct) == TI_C54X_OP_STATUS_OK);
                if (SWEEP_EXPECT[cond].taken) {
                    assert_rmw_read(
                        &direct, xmod, xar,
                        (uint8_t)
                            TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE,
                        UINT16_C(0x4321));
                } else {
                    assert_rmw_read(
                        &direct, xmod, xar,
                        (uint8_t)
                            TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK,
                        UINT16_C(0));
                }
                TI_C54X_TEST_CHECK(
                    ti_c54x_trusted_op_strcd_xmem_cond(
                        &instruction, &operands, &input, &trusted) ==
                    TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(memcmp(&direct, &trusted,
                                          sizeof(direct)) == 0);
            }
        }
    }
}

static void
check_unknown_and_malformed_input_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(1), UINT8_C(3), COND_AGT);
    const ti_c54x_op_input_snapshot unknown_t =
        make_input(UINT64_C(0x0070FFFFFF), true, UINT64_C(0), true,
                   UINT16_C(0x4321), false);
    const ti_c54x_op_input_snapshot unknown_selected =
        make_input(UINT64_C(0x0070FFFFFF), false, UINT64_C(0), true,
                   UINT16_C(0x4321), true);
    const ti_c54x_op_input_snapshot malformed_acc = make_input(
        TI_C54X_U40_MASK + UINT64_C(1), true, UINT64_C(0), true,
        UINT16_C(0x4321), true);
    const ti_c54x_op_effect sentinel = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = sentinel;

    TI_C54X_TEST_CHECK(ti_c54x_op_strcd_xmem_cond(&instruction, NULL,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_strcd_xmem_cond(NULL, &unknown_t,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_strcd_xmem_cond(&instruction,
                                                 &unknown_t, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_strcd_xmem_cond(&instruction,
                                                 &unknown_t, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(
        ti_c54x_op_strcd_xmem_cond(&instruction, &unknown_selected,
                                   &effect) ==
        TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(
        ti_c54x_op_strcd_xmem_cond(&instruction, &malformed_acc,
                                   &effect) ==
        TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
}

static void
check_forged_instruction_is_transactional(void)
{
    const ti_c54x_decoded_instruction valid =
        decode(UINT8_C(0), UINT8_C(1), UINT8_C(3), COND_AGT);
    const ti_c54x_op_input_snapshot input =
        make_input(UINT64_C(0x0070FFFFFF), true, UINT64_C(0), true,
                   UINT16_C(0x4321), true);
    const ti_c54x_op_effect sentinel = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_decoded_instruction forged = valid;
    ti_c54x_op_effect effect = sentinel;

    /* Flipping the S bit reaches the SRCCD row shape, which this
     * leaf must not serve. */
    forged.bundle.program_words[0] ^= UINT16_C(0x0100);
    TI_C54X_TEST_CHECK(ti_c54x_op_strcd_xmem_cond(&forged, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    forged = valid;
    forged.bundle.program_words[0] ^= UINT16_C(0x0200);
    TI_C54X_TEST_CHECK(ti_c54x_op_strcd_xmem_cond(&forged, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
}

static void
check_trusted_operand_validation_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(1), UINT8_C(3), COND_AGT);
    const ti_c54x_op_input_snapshot input =
        make_input(UINT64_C(0x0070FFFFFF), true, UINT64_C(0), true,
                   UINT16_C(0x4321), true);
    const ti_c54x_op_effect sentinel = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT8_C(1), UINT8_C(3), COND_AGT);
    ti_c54x_op_effect effect = sentinel;

    /* The trusted entry ignores the borrowed instruction (constructor-
     * validated upstream), mirroring the STH-by-SHFT trusted leaf:
     * only the operands, input, and effect pointers are validated. */
    TI_C54X_TEST_CHECK(
        ti_c54x_trusted_op_strcd_xmem_cond(&instruction, NULL, &input,
                                           &effect) ==
        TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    operands.count = UINT8_C(3);
    TI_C54X_TEST_CHECK(
        ti_c54x_trusted_op_strcd_xmem_cond(&instruction, &operands,
                                           &input, &effect) ==
        TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(1), UINT8_C(3),
                                COND_AGT);
    TI_C54X_TEST_CHECK(
        ti_c54x_trusted_op_strcd_xmem_cond(&instruction, &operands,
                                           &input, &effect) ==
        TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    operands.values[0] =
        (ti_c54x_operand_field_value){UINT64_C(0), UINT8_C(2)};
    TI_C54X_TEST_CHECK(
        ti_c54x_trusted_op_strcd_xmem_cond(&instruction, &operands,
                                           &input, &effect) ==
        TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(4), UINT8_C(3),
                                COND_AGT);
    TI_C54X_TEST_CHECK(
        ti_c54x_trusted_op_strcd_xmem_cond(&instruction, &operands,
                                           &input, &effect) ==
        TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(1), UINT8_C(4),
                                COND_AGT);
    TI_C54X_TEST_CHECK(
        ti_c54x_trusted_op_strcd_xmem_cond(&instruction, &operands,
                                           &input, &effect) ==
        TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(1), UINT8_C(3),
                                UINT8_C(16));
    operands.values[3].width_bits = UINT8_C(5);
    TI_C54X_TEST_CHECK(
        ti_c54x_trusted_op_strcd_xmem_cond(&instruction, &operands,
                                           &input, &effect) ==
        TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
}

int
main(void)
{
    check_ti_strcd_ar5_postdec_agt_taken();
    check_cond_false_stages_writeback();
    check_b_code_reads_accumulator_b();
    check_encoding_sweep_with_trusted_equivalence();
    check_unknown_and_malformed_input_is_transactional();
    check_forged_instruction_is_transactional();
    check_trusted_operand_validation_is_transactional();
    return 0;
}
