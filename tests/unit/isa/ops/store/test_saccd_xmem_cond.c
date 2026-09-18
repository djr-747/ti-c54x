/*
 * Module contract
 *
 * Owns: exhaustive RED proof of pure dual-X Xmem WORD16 SACCD
 * conditional-store memory effects, including S/Xmod/Xar/COND
 * selection over all 2048 encodings, the (ASM-16) SXM-gated shift
 * with low-16 extraction (TI p. 4-153: 0xFC00), cond-true OVERWRITE
 * versus cond-false WRITEBACK RMW selection, trusted equivalence,
 * and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, address
 * resolution, buses, CPU/state mutation, sequencing, timing,
 * observers, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/store/op_saccd_xmem_cond.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define COND_AGEQ UINT8_C(0x2)
#define COND_ALT UINT8_C(0x3)
#define COND_ANEQ UINT8_C(0x4)
#define COND_AEQ UINT8_C(0x5)
#define COND_AGT UINT8_C(0x6)
#define COND_ALEQ UINT8_C(0x7)
#define COND_BGEQ UINT8_C(0xA)
#define COND_BLT UINT8_C(0xB)
#define COND_BNEQ UINT8_C(0xC)
#define COND_BEQ UINT8_C(0xD)
#define COND_BGT UINT8_C(0xE)
#define COND_BLEQ UINT8_C(0xF)

static uint16_t
saccd_word(uint8_t source, uint8_t xmod, uint8_t xar_field, uint8_t cond)
{
    return (uint16_t)(UINT16_C(0x9E00) |
                      ((uint16_t)(source & UINT8_C(1)) << 8U) |
                      ((uint16_t)(xmod & UINT8_C(3)) << 6U) |
                      ((uint16_t)(xar_field & UINT8_C(3)) << 4U) |
                      (uint16_t)(cond & UINT8_C(0x0F)));
}

static ti_c54x_decoded_instruction
decode(uint8_t source, uint8_t xmod, uint8_t xar_field, uint8_t cond)
{
    const uint16_t word =
        saccd_word(source, xmod, xar_field, cond);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_SACCD_XMEM_COND);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SELECT_DUAL_XMOD_XAR_COND4);
    TI_C54X_TEST_CHECK(
        instruction.input_resources ==
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_SXM_ASM);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint64_t a, uint64_t b, bool a_known, bool b_known, uint8_t asm_value,
          bool asm_known, uint8_t sxm, bool sxm_known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = a;
    input.accumulator_b_raw40 = b;
    input.accumulator_a_known = a_known;
    input.accumulator_b_known = b_known;
    input.status_asm = asm_value;
    input.status_asm_known = asm_known;
    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    return input;
}

static void
assert_memory_rmw(const ti_c54x_op_effect *effect, uint8_t xmod, uint8_t xar,
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
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.dual.reserved_word0 ==
                       UINT32_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.dual.reserved_word1 ==
                       UINT32_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.dual.reserved_word2 ==
                       UINT32_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.dual.reserved_word3 ==
                       UINT32_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == value);
    TI_C54X_TEST_CHECK(effect->payload.memory.rmw_operation == rmw_operation);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t source, uint8_t xmod, uint8_t xar_field,
                 uint8_t cond)
{
    ti_c54x_trusted_operand_values operands = {0};
    operands.count = UINT8_C(4);
    operands.values[0].raw_value = source;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = xmod;
    operands.values[1].width_bits = UINT8_C(2);
    operands.values[2].raw_value = xar_field;
    operands.values[2].width_bits = UINT8_C(2);
    operands.values[3].raw_value = cond;
    operands.values[3].width_bits = UINT8_C(4);
    return operands;
}

typedef struct shift_vector {
    uint64_t accumulator;
    uint8_t asm_value;
    uint8_t sxm;
    uint16_t expected;
    uint8_t cond;
} shift_vector;

/* (ASM-16)-shift-then-extract vectors: the stored word is the low 16
 * bits of the 40-bit source shifted right by (16-ASM) with arithmetic
 * fill under SXM=1 and zero fill under SXM=0. ASM is the signed 5-bit
 * count (raw <= 15 non-negative, else raw - 32), so the shift is
 * always a right shift of 17 to 32. Every expectation below is
 * cross-checked against an independent Python bit oracle; the final
 * row is the TI SPRU172C p. 4-153 worked example (A FF_FE00_4321,
 * ASM=01, ALT taken, stages 0xFC00). */
static const shift_vector VECTORS[] = {
    {UINT64_C(0x0012345678), UINT8_C(16), UINT8_C(0), UINT16_C(0x0000),
     COND_ANEQ},
    {UINT64_C(0x0012345678), UINT8_C(16), UINT8_C(1), UINT16_C(0x0000),
     COND_ANEQ},
    {UINT64_C(0xFF87654321), UINT8_C(16), UINT8_C(0), UINT16_C(0x00FF),
     COND_ANEQ},
    {UINT64_C(0x0012345678), UINT8_C(0), UINT8_C(0), UINT16_C(0x1234),
     COND_ANEQ},
    {UINT64_C(0x0012345678), UINT8_C(0), UINT8_C(1), UINT16_C(0x1234),
     COND_ANEQ},
    /* Right 16 keeps the window inside the source, so SXM fill is
     * out of view and both policies stage the same word. */
    {UINT64_C(0xFF87654321), UINT8_C(0), UINT8_C(0), UINT16_C(0x8765),
     COND_ANEQ},
    {UINT64_C(0xFF87654321), UINT8_C(0), UINT8_C(1), UINT16_C(0x8765),
     COND_ANEQ},
    /* TI SRCCD worked-example accumulator shape: A = 00_70FF_FFFF. */
    {UINT64_C(0x0070FFFFFF), UINT8_C(16), UINT8_C(0), UINT16_C(0x0000),
     COND_ANEQ},
    {UINT64_C(0x0070FFFFFF), UINT8_C(15), UINT8_C(0), UINT16_C(0xFFFF),
     COND_ANEQ},
    {UINT64_C(0x0070FFFFFF), UINT8_C(15), UINT8_C(1), UINT16_C(0xFFFF),
     COND_ANEQ},
    {UINT64_C(0xFF80000000), UINT8_C(8), UINT8_C(0), UINT16_C(0x0000),
     COND_ANEQ},
    {UINT64_C(0xFF80000000), UINT8_C(8), UINT8_C(1), UINT16_C(0x0000),
     COND_ANEQ},
    /* SXM split: shifts of 25 or more expose the fill in the staged
     * low 16 (zero fill stages 0x00FF/0x7FC0, sign fill 0xFFFF/0xFFC0). */
    {UINT64_C(0xFF87654321), UINT8_C(16), UINT8_C(1), UINT16_C(0xFFFF),
     COND_ANEQ},
    {UINT64_C(0xFF80000000), UINT8_C(7), UINT8_C(0), UINT16_C(0x0000),
     COND_ANEQ},
    {UINT64_C(0xFF80000000), UINT8_C(7), UINT8_C(1), UINT16_C(0x0000),
     COND_ANEQ},
    {UINT64_C(0xFF80000000), UINT8_C(23), UINT8_C(0), UINT16_C(0x7FC0),
     COND_ANEQ},
    {UINT64_C(0xFF80000000), UINT8_C(23), UINT8_C(1), UINT16_C(0xFFC0),
     COND_ANEQ},
    {UINT64_C(0x0000000002), UINT8_C(31), UINT8_C(0), UINT16_C(0x0000),
     COND_ANEQ},
    {UINT64_C(0x0000000001), UINT8_C(31), UINT8_C(1), UINT16_C(0x0000),
     COND_ANEQ},
    {UINT64_C(0x0070FFFFFF), UINT8_C(20), UINT8_C(0), UINT16_C(0x0007),
     COND_ANEQ},
    {UINT64_C(0x0100000000), UINT8_C(8), UINT8_C(0), UINT16_C(0x0000),
     COND_ANEQ},
    {UINT64_C(0x0100000000), UINT8_C(16), UINT8_C(0), UINT16_C(0x0001),
     COND_ANEQ},
    {UINT64_C(0x0000000000), UINT8_C(16), UINT8_C(0), UINT16_C(0x0000),
     COND_AEQ},
    {UINT64_C(0x0000000000), UINT8_C(0), UINT8_C(1), UINT16_C(0x0000),
     COND_AEQ},
    /* TI SACCD worked example (SPRU172C p. 4-153): A FF_FE00_4321
     * with ASM=01 under ALT stages 0xFC00. */
    {UINT64_C(0xFFFE004321), UINT8_C(1), UINT8_C(1), UINT16_C(0xFC00),
     COND_ALT},
};

static void
check_shift_vectors(void)
{
    for (uint8_t source = 0U; source <= UINT8_C(1); ++source) {
        for (uint8_t xmod = 0U; xmod <= UINT8_C(3); ++xmod) {
            for (uint8_t xar_field = 0U; xar_field <= UINT8_C(3);
                 ++xar_field) {
                const uint8_t xar = (uint8_t)(xar_field + UINT8_C(2));
                for (size_t index = 0U;
                     index < sizeof(VECTORS) / sizeof(VECTORS[0]); ++index) {
                    /* The sweep conds are A-codes, so the A filler is a
                     * consumed condition input: known, 40-bit, and
                     * taken-shaped (nonzero for ANEQ, zero for AEQ).
                     * The B filler is unconsumed here. */
                    const uint64_t filler_a =
                        VECTORS[index].cond == COND_AEQ
                            ? UINT64_C(0)
                            : UINT64_C(0xAAAAAAAAAA);
                    const ti_c54x_decoded_instruction instruction = decode(
                        source, xmod, xar_field, VECTORS[index].cond);
                    const ti_c54x_op_input_snapshot input = input_for(
                        source == 0U ? VECTORS[index].accumulator : filler_a,
                        source == 1U ? VECTORS[index].accumulator
                                     : UINT64_C(0x5555555555),
                        true, true, VECTORS[index].asm_value, true,
                        VECTORS[index].sxm, true);
                    const ti_c54x_trusted_operand_values operands =
                        trusted_operands(source, xmod, xar_field,
                                         VECTORS[index].cond);
                    ti_c54x_op_effect direct = {0};
                    ti_c54x_op_effect trusted = {0};

                    TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(
                                           &instruction, &input,
                                           &direct) == TI_C54X_OP_STATUS_OK);
                    assert_memory_rmw(
                        &direct, xmod, xar, VECTORS[index].expected,
                        (uint8_t)
                            TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE);
                    TI_C54X_TEST_CHECK(
                        ti_c54x_trusted_op_saccd_xmem_cond(
                            &instruction, &operands, &input,
                            &trusted) == TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted,
                                              sizeof(direct)) == 0);
                }
            }
        }
    }
}

typedef struct cond_vector {
    uint8_t cond4;
    uint64_t acc_raw40;
    bool expected_taken;
} cond_vector;

/* Every TI code in both outcomes; the source accumulator carries the
 * condition value so the staged word is its low 16 after the ASM=0
 * right-16 shift. */
static const cond_vector COND_VECTORS[] = {
    {COND_AEQ, UINT64_C(0x0000000000), true},
    {COND_AEQ, UINT64_C(0x0070FFFFFF), false},
    {COND_ANEQ, UINT64_C(0x0070FFFFFF), true},
    {COND_ANEQ, UINT64_C(0x0000000000), false},
    {COND_AGT, UINT64_C(0x0070FFFFFF), true},
    {COND_AGT, UINT64_C(0xFF80000000), false},
    {COND_AGEQ, UINT64_C(0x0000000000), true},
    {COND_AGEQ, UINT64_C(0xFF80000000), false},
    {COND_ALT, UINT64_C(0xFF80000000), true},
    {COND_ALT, UINT64_C(0x0000000000), false},
    {COND_ALEQ, UINT64_C(0xFF80000000), true},
    {COND_ALEQ, UINT64_C(0x0070FFFFFF), false},
    {COND_BEQ, UINT64_C(0x0000000000), true},
    {COND_BEQ, UINT64_C(0x0070FFFFFF), false},
    {COND_BNEQ, UINT64_C(0x0070FFFFFF), true},
    {COND_BNEQ, UINT64_C(0x0000000000), false},
    {COND_BGT, UINT64_C(0x0070FFFFFF), true},
    {COND_BGT, UINT64_C(0xFF80000000), false},
    {COND_BGEQ, UINT64_C(0x0000000000), true},
    {COND_BGEQ, UINT64_C(0xFF80000000), false},
    {COND_BLT, UINT64_C(0xFF80000000), true},
    {COND_BLT, UINT64_C(0x0000000000), false},
    {COND_BLEQ, UINT64_C(0xFF80000000), true},
    {COND_BLEQ, UINT64_C(0x0070FFFFFF), false},
};

static void
check_cond_select(void)
{
    for (size_t index = 0U;
         index < sizeof(COND_VECTORS) / sizeof(COND_VECTORS[0]); ++index) {
        const bool use_b =
            COND_VECTORS[index].cond4 >= UINT8_C(0xA);
        /* Alternate the S source independently of the condition side so
         * both same-side and cross-side shapes are proven. */
        const uint8_t source =
            (uint8_t)((index & 1U) == 0U ? (use_b ? 1U : 0U)
                                         : (use_b ? 0U : 1U));
        const bool source_is_cond_side =
            (source == UINT8_C(1)) == use_b;
        /* Source value is fixed nonzero; the condition value rides the
         * condition side. At ASM=0 the staged word is the low 16 of
         * the source value shifted right 16. */
        const uint64_t source_value = UINT64_C(0x0012345678);
        const uint64_t cond_value = COND_VECTORS[index].acc_raw40;
        ti_c54x_op_input_snapshot input;
        ti_c54x_decoded_instruction instruction;
        ti_c54x_op_effect effect = {0};
        const uint16_t expected_value = UINT16_C(0x1234);
        const uint8_t expected_rmw =
            COND_VECTORS[index].expected_taken
                ? (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE
                : (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK;

        if (source_is_cond_side) {
            input = input_for(cond_value, cond_value, true, true,
                              UINT8_C(0), true, UINT8_C(0), true);
        } else if (use_b) {
            input = input_for(source_value, cond_value, true, true,
                              UINT8_C(0), true, UINT8_C(0), true);
        } else {
            input = input_for(cond_value, source_value, true, true,
                              UINT8_C(0), true, UINT8_C(0), true);
        }
        instruction =
            decode(source, UINT8_C(2), UINT8_C(1), COND_VECTORS[index].cond4);
        TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(
                               &instruction, &input,
                               &effect) == TI_C54X_OP_STATUS_OK);
        assert_memory_rmw(&effect, UINT8_C(2), UINT8_C(3),
                          source_is_cond_side
                              ? (uint16_t)((cond_value >> 16U) &
                                           UINT64_C(0xFFFF))
                              : expected_value,
                          expected_rmw);
    }
}

static void
check_b_code_sweeps_all_modes(void)
{
    /* One B-code (BGT, TI *AR5-,AGT worked-example shape) swept over
     * every Xmod/Xar encoding in both outcomes, with the source on A
     * so the B condition side is independent of S. */
    for (uint8_t xmod = 0U; xmod <= UINT8_C(3); ++xmod) {
        for (uint8_t xar_field = 0U; xar_field <= UINT8_C(3); ++xar_field) {
            const uint8_t xar = (uint8_t)(xar_field + UINT8_C(2));
            const ti_c54x_decoded_instruction taken_ins =
                decode(UINT8_C(0), xmod, xar_field, COND_BGT);
            const ti_c54x_decoded_instruction untaken_ins =
                decode(UINT8_C(0), xmod, xar_field, COND_BEQ);
            const ti_c54x_op_input_snapshot taken_input = input_for(
                UINT64_C(0x0012345678), UINT64_C(0x0070FFFFFF), true, true,
                UINT8_C(0), true, UINT8_C(0), true);
            const ti_c54x_op_input_snapshot untaken_input = input_for(
                UINT64_C(0x0012345678), UINT64_C(0x0070FFFFFF), true, true,
                UINT8_C(0), true, UINT8_C(0), true);
            ti_c54x_op_effect taken_effect = {0};
            ti_c54x_op_effect untaken_effect = {0};

            TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(
                                   &taken_ins, &taken_input,
                                   &taken_effect) == TI_C54X_OP_STATUS_OK);
            assert_memory_rmw(&taken_effect, xmod, xar, UINT16_C(0x1234),
                              (uint8_t)
                                  TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE);
            /* BGT on positive B is true, so BEQ on the same B is
             * false and must select WRITEBACK. */
            TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(
                                   &untaken_ins, &untaken_input,
                                   &untaken_effect) ==
                               TI_C54X_OP_STATUS_OK);
            assert_memory_rmw(&untaken_effect, xmod, xar,
                              UINT16_C(0x1234),
                              (uint8_t)
                                  TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK);
        }
    }
}

static void
check_unassigned_and_unknown_cond(void)
{
    static const uint8_t unassigned[] = {UINT8_C(0x0), UINT8_C(0x1),
                                         UINT8_C(0x8), UINT8_C(0x9)};
    const ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(0x0070FFFFFF), UINT64_C(0x0070FFFFFF), true, true,
                  UINT8_C(16), true, UINT8_C(0), true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    size_t index;

    for (index = 0U;
         index < sizeof(unassigned) / sizeof(unassigned[0]); ++index) {
        const ti_c54x_decoded_instruction instruction =
            decode(UINT8_C(0), UINT8_C(0), UINT8_C(0), unassigned[index]);
        ti_c54x_op_effect effect = before;
        ti_c54x_trusted_operand_values operands =
            trusted_operands(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                             unassigned[index]);
        ti_c54x_op_effect trusted = before;

        TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(
                               &instruction, &input,
                               &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_saccd_xmem_cond(
                               &instruction, &operands, &input,
                               &trusted) == TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&trusted, &before, sizeof(trusted)) == 0);
    }

    /* Unknown selected condition accumulator rejects; the
     * unselected side may stay unknown. */
    {
        const ti_c54x_decoded_instruction instruction =
            decode(UINT8_C(0), UINT8_C(0), UINT8_C(0), COND_AGT);
        const ti_c54x_op_input_snapshot unknown_a = input_for(
            UINT64_C(0x0070FFFFFF), UINT64_C(0), false, false, UINT8_C(16),
            true, UINT8_C(0), true);
        const ti_c54x_op_input_snapshot unknown_b_only = input_for(
            UINT64_C(0x0070FFFFFF), UINT64_C(0), true, false, UINT8_C(0),
            true, UINT8_C(0), true);
        ti_c54x_op_effect effect = before;

        TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(
                               &instruction, &unknown_a,
                               &effect) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(
                               &instruction, &unknown_b_only,
                               &effect) == TI_C54X_OP_STATUS_OK);
        assert_memory_rmw(&effect, UINT8_C(0), UINT8_C(2),
                          UINT16_C(0x70FF),
                          (uint8_t)
                              TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE);
    }
}

static void
check_unknown_and_malformed_input_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(0), UINT8_C(0), COND_AGT);
    const ti_c54x_op_input_snapshot selected_unknown = input_for(
        UINT64_C(1), UINT64_C(2), false, true, UINT8_C(16), true,
        UINT8_C(0), true);
    const ti_c54x_op_input_snapshot asm_unknown = input_for(
        UINT64_C(0x0070FFFFFF), UINT64_C(0x0070FFFFFF), true, true,
        UINT8_C(16), false, UINT8_C(0), true);
    const ti_c54x_op_input_snapshot sxm_unknown = input_for(
        UINT64_C(0x0070FFFFFF), UINT64_C(0x0070FFFFFF), true, true,
        UINT8_C(16), true, UINT8_C(0), false);
    const ti_c54x_op_input_snapshot bad_asm = input_for(
        UINT64_C(0x0070FFFFFF), UINT64_C(0x0070FFFFFF), true, true,
        UINT8_C(32), true, UINT8_C(0), true);
    const ti_c54x_op_input_snapshot bad_sxm = input_for(
        UINT64_C(0x0070FFFFFF), UINT64_C(0x0070FFFFFF), true, true,
        UINT8_C(16), true, UINT8_C(2), true);
    const ti_c54x_op_input_snapshot malformed_acc = input_for(
        TI_C54X_U40_MASK + UINT64_C(1), UINT64_C(2), true, true,
        UINT8_C(16), true, UINT8_C(0), true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(&instruction, NULL,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(NULL, &selected_unknown,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(
                           &instruction, &selected_unknown,
                           &effect) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(&instruction, &asm_unknown,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(&instruction, &sxm_unknown,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(&instruction, &bad_asm,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(&instruction, &bad_sxm,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(&instruction, &malformed_acc,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void
check_trusted_malformed_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(0), UINT8_C(0), COND_AGT);
    const ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(0x0070FFFFFF), UINT64_C(0x0070FFFFFF), true, true,
                  UINT8_C(16), true, UINT8_C(0), true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT8_C(0), UINT8_C(0), COND_AGT);
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_saccd_xmem_cond(
                           &instruction, NULL, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_saccd_xmem_cond(
                           &instruction, &operands, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(0), UINT8_C(0), COND_AGT);
    operands.count = UINT8_C(3);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_saccd_xmem_cond(
                           &instruction, &operands, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(0), UINT8_C(0), COND_AGT);
    operands.values[3].raw_value = UINT64_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_saccd_xmem_cond(
                           &instruction, &operands, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void
check_forged_instruction_is_transactional(void)
{
    const ti_c54x_decoded_instruction valid =
        decode(UINT8_C(1), UINT8_C(2), UINT8_C(3), COND_BGT);
    const ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(0x0012345678), UINT64_C(0x0070FFFFFF), true, true,
                  UINT8_C(16), true, UINT8_C(0), true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_decoded_instruction forged = valid;
    ti_c54x_op_effect effect = before;

    forged.bundle.program_words[0] ^= UINT16_C(0x0200);
    TI_C54X_TEST_CHECK(ti_c54x_op_saccd_xmem_cond(&forged, &input,
                                                 &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    check_shift_vectors();
    check_cond_select();
    check_b_code_sweeps_all_modes();
    check_unassigned_and_unknown_cond();
    check_unknown_and_malformed_input_is_transactional();
    check_trusted_malformed_is_transactional();
    check_forged_instruction_is_transactional();
    return 0;
}
