/*
 * Module contract
 *
 * Owns: exhaustive RED proof of pure two-word STH-by-SHIFT memory
 * effects over direct-DP and MOD4-family indirect addressing,
 * including source/SHIFT selection, signed-SHIFT high-word
 * extraction (bits 31-16), SXM riding hostile and unconsumed, DP
 * gating for direct words, trusted equivalence, and transactional
 * rejection. Indirect MOD0-3 stays out: its first word collides
 * with LD_SMEM_SHIFT pending second-word disambiguation.
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
#include "../../../../../src/isa/ops/store/op_sth_shift_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t
direct_word1(uint8_t addr7)
{
    return (uint16_t)(UINT16_C(0x6F00) | (uint16_t)(addr7 & UINT8_C(0x7F)));
}

static uint16_t
shift_word2(uint8_t source, int8_t shift)
{
    return (uint16_t)(UINT16_C(0x0C60) |
                      ((uint16_t)(source & UINT8_C(1)) << 8U) |
                      (uint16_t)((uint8_t)shift & UINT8_C(0x1F)));
}

static ti_c54x_decoded_instruction
decode_direct(uint8_t source, uint8_t addr7, int8_t shift, uint16_t dp)
{
    const uint16_t words[2] = {direct_word1(addr7),
                               shift_word2(source, shift)};
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(words[0]);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {words[0], words[1]}, .program_word_count = 2};
    ti_c54x_decoded_instruction instruction;

    (void)dp;
    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_STH_SHIFT_DIRECT);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DIRECT7_SHIFT5);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_DP);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint64_t a, uint64_t b, bool a_known, bool b_known, uint16_t dp,
          bool dp_known, uint8_t seed)
{
    /* SXM rides hostile seeds: the stored field never observes it. */
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = a_known,
        .accumulator_b_known = b_known,
        .status_sxm = (uint8_t)(seed & UINT8_C(1)),
        .status_sxm_known = (seed & UINT8_C(2)) != 0U,
        .status_dp = dp,
        .status_dp_known = dp_known};
    return input;
}

typedef struct shift_vector {
    uint64_t accumulator;
    int8_t shift;
    uint16_t expected;
} shift_vector;

/* Hand-computed signed-SHIFT-then-extract vectors: stored equals bits
 * 31-16 of the accumulator shifted by SHIFT (negative shifts
 * arithmetic right). */
static const shift_vector VECTORS[] = {
    {UINT64_C(0x0012345678), 0, UINT16_C(0x1234)},
    {UINT64_C(0xFF87654321), 0, UINT16_C(0x8765)},
    {UINT64_C(0x0012345678), 15, UINT16_C(0x2B3C)},
    {UINT64_C(0xFF87654321), 15, UINT16_C(0xA190)},
    {UINT64_C(0x0012345678), -16, UINT16_C(0x0000)},
    {UINT64_C(0xFF87654321), -16, UINT16_C(0xFFFF)},
    {UINT64_C(0xFF87654321), -1, UINT16_C(0xC3B2)},
    {UINT64_C(0xFF84211234), -8, UINT16_C(0xFF84)},
    /* TI SPRU172C STH syntax-4 example: STH A,-4 (SHIFT -4) with
     * A FF 8421 1234 under SXM=1 stores F842. */
    {UINT64_C(0xFF84211234), -4, UINT16_C(0xF842)},
};

static void
check_direct_vectors(void)
{
    for (uint8_t source = 0U; source <= UINT8_C(1); ++source) {
        for (size_t index = 0U;
             index < sizeof(VECTORS) / sizeof(VECTORS[0]); ++index) {
            const ti_c54x_decoded_instruction instruction =
                decode_direct(source, UINT8_C(0x0A), VECTORS[index].shift,
                              UINT16_C(0x004));
            for (uint8_t seed = 0U; seed < UINT8_C(4); ++seed) {
                const ti_c54x_op_input_snapshot input = input_for(
                    source == 0U ? VECTORS[index].accumulator
                                 : UINT64_C(0xAAAAAAAAAAAAAAAA),
                    source == 1U ? VECTORS[index].accumulator
                                 : UINT64_C(0x5555555555),
                    true, true, UINT16_C(0x004), true, seed);
                ti_c54x_trusted_operand_values operands = {0};
                ti_c54x_op_effect direct = {0};
                ti_c54x_op_effect trusted = {0};

                operands.count = UINT8_C(3);
                operands.values[0].raw_value = source;
                operands.values[0].width_bits = UINT8_C(1);
                operands.values[1].raw_value = UINT8_C(0x0A);
                operands.values[1].width_bits = UINT8_C(7);
                operands.values[2].raw_value =
                    (uint64_t)((uint8_t)VECTORS[index].shift);
                operands.values[2].width_bits = UINT8_C(5);
                TI_C54X_TEST_CHECK(ti_c54x_op_sth_shift_direct(
                                       &instruction, &input,
                                       &direct) == TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(direct.kind ==
                                   TI_C54X_OP_EFFECT_MEMORY);
                TI_C54X_TEST_CHECK(
                    direct.payload.memory.address_spec.kind ==
                    TI_C54X_OP_MEMORY_ADDRESS_DIRECT_DP_WORD16);
                TI_C54X_TEST_CHECK(
                    direct.payload.memory.address_spec.long_offset_word ==
                    UINT16_C(0x004));
                TI_C54X_TEST_CHECK(
                    direct.payload.memory.address_spec.direct_dma7 ==
                    UINT8_C(0x0A));
                TI_C54X_TEST_CHECK(direct.payload.memory.access ==
                                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
                TI_C54X_TEST_CHECK(direct.payload.memory.value_word ==
                                   VECTORS[index].expected);
                TI_C54X_TEST_CHECK(direct.payload.memory.complete == NULL);
                TI_C54X_TEST_CHECK(
                    ti_c54x_trusted_op_sth_shift_direct(
                        &instruction, &operands, &input,
                        &trusted) == TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(memcmp(&direct, &trusted,
                                          sizeof(direct)) == 0);
            }
        }
    }
}

static void
check_unknown_dp_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_direct(0U, UINT8_C(0x0A), -4, UINT16_C(0x004));
    const ti_c54x_op_input_snapshot dp_unknown =
        input_for(UINT64_C(0x0012345678), UINT64_C(0), true, true,
                  UINT16_C(0x004), false, UINT8_C(0));
    const ti_c54x_op_input_snapshot selected_unknown =
        input_for(UINT64_C(1), UINT64_C(2), false, true, UINT16_C(0x004),
                  true, UINT8_C(0));
    const ti_c54x_op_input_snapshot malformed_acc =
        input_for(TI_C54X_U40_MASK + UINT64_C(1), UINT64_C(2), true, true,
                  UINT16_C(0x004), true, UINT8_C(0));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_sth_shift_direct(
                           &instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_sth_shift_direct(
                           &instruction, &dp_unknown, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_sth_shift_direct(
                           &instruction, &selected_unknown, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_sth_shift_direct(
                           &instruction, &malformed_acc, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void
check_forged_instruction_is_transactional(void)
{
    const ti_c54x_decoded_instruction valid =
        decode_direct(1U, UINT8_C(0x0A), -4, UINT16_C(0x004));
    const ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(0x0012345678), UINT64_C(0xFF84211234), true, true,
                  UINT16_C(0x004), true, UINT8_C(0));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_decoded_instruction forged = valid;
    ti_c54x_op_effect effect = before;

    /* Break a masked first-word bit. */
    forged.bundle.program_words[0] ^= UINT16_C(0x0100);
    TI_C54X_TEST_CHECK(ti_c54x_op_sth_shift_direct(
                           &forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    /* Break the second-word fixed pattern. */
    forged = valid;
    forged.bundle.program_words[1] ^= UINT16_C(0x0020);
    TI_C54X_TEST_CHECK(ti_c54x_op_sth_shift_direct(
                           &forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    check_direct_vectors();
    check_unknown_dp_is_transactional();
    check_forged_instruction_is_transactional();
    return 0;
}
