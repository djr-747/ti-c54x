/*
 * Module contract
 *
 * Owns: exhaustive RED proof of pure Xmem WORD16 STH-by-SHFT memory
 * effects, including source/Xmod/Xar/SHFT selection over all 1024
 * encodings, (SHFT-16) right-shifted high-word extraction (bits
 * 31-16), SXM riding hostile and unconsumed, trusted equivalence,
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
#include "../../../../../src/isa/ops/store/op_sth_shft_xmem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t
sth_word(uint8_t source, uint8_t xmod, uint8_t xar_field, uint8_t shift)
{
    return (uint16_t)(UINT16_C(0x9A00) |
                      ((uint16_t)(source & UINT8_C(1)) << 8U) |
                      ((uint16_t)(xmod & UINT8_C(3)) << 6U) |
                      ((uint16_t)(xar_field & UINT8_C(3)) << 4U) |
                      (uint16_t)(shift & UINT8_C(0x0F)));
}

static ti_c54x_decoded_instruction
decode(uint8_t source, uint8_t xmod, uint8_t xar_field, uint8_t shift)
{
    const uint16_t word = sth_word(source, xmod, xar_field, shift);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_STH_SHFT_XMEM);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint64_t a, uint64_t b, bool a_known, bool b_known, uint8_t seed)
{
    /* SXM rides hostile seeds: right shifts never exceed 16, so the
     * stored field is pure source bits and SXM is not an op input. */
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = a,
        .accumulator_b_raw40 = b,
        .accumulator_a_known = a_known,
        .accumulator_b_known = b_known,
        .status_sxm = (uint8_t)(seed & UINT8_C(1)),
        .status_sxm_known = (seed & UINT8_C(2)) != 0U};
    return input;
}

static void
assert_memory_write(const ti_c54x_op_effect *effect, uint8_t xmod,
                    uint8_t xar, uint16_t value)
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
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == value);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t source, uint8_t xmod, uint8_t xar_field,
                 uint8_t shift)
{
    ti_c54x_trusted_operand_values operands = {0};
    operands.count = UINT8_C(4);
    operands.values[0].raw_value = source;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = xmod;
    operands.values[1].width_bits = UINT8_C(2);
    operands.values[2].raw_value = xar_field;
    operands.values[2].width_bits = UINT8_C(2);
    operands.values[3].raw_value = shift;
    operands.values[3].width_bits = UINT8_C(4);
    return operands;
}

typedef struct shift_vector {
    uint64_t accumulator;
    uint8_t shift;
    uint16_t expected;
} shift_vector;

/* Hand-computed (SHFT-16)-shift-then-extract vectors: stored equals
 * bits 31-16 of the accumulator shifted right by (16-SHFT). */
static const shift_vector VECTORS[] = {
    {UINT64_C(0x0012345678), UINT8_C(0), UINT16_C(0x0000)},
    {UINT64_C(0xFF87654321), UINT8_C(0), UINT16_C(0xFFFF)},
    {UINT64_C(0x0012345678), UINT8_C(8), UINT16_C(0x0012)},
    {UINT64_C(0xFF87654321), UINT8_C(8), UINT16_C(0xFF87)},
    {UINT64_C(0x0012345678), UINT8_C(15), UINT16_C(0x091A)},
    {UINT64_C(0xFF87654321), UINT8_C(15), UINT16_C(0xC3B2)},
    /* TI SPRU172C STH syntax-3 example: STH B,-8 (SHFT 8) with
     * B FF 8421 1234 stores FF84. */
    {UINT64_C(0xFF84211234), UINT8_C(8), UINT16_C(0xFF84)},
    {UINT64_C(0xFF84211234), UINT8_C(0), UINT16_C(0xFFFF)},
};

static void
check_shift_vectors(void)
{
    for (uint8_t source = 0U; source <= UINT8_C(1); ++source) {
        for (uint8_t xmod = 0U; xmod <= UINT8_C(3); ++xmod) {
            for (uint8_t xar_field = 0U; xar_field <= UINT8_C(3);
                 ++xar_field) {
                const uint8_t xar =
                    (uint8_t)(xar_field + UINT8_C(2));
                for (size_t index = 0U;
                     index < sizeof(VECTORS) / sizeof(VECTORS[0]); ++index) {
                    const ti_c54x_decoded_instruction instruction = decode(
                        source, xmod, xar_field, VECTORS[index].shift);
                    for (uint8_t seed = 0U; seed < UINT8_C(4); ++seed) {
                        const ti_c54x_op_input_snapshot input = input_for(
                            source == 0U ? VECTORS[index].accumulator
                                         : UINT64_C(0xAAAAAAAAAAAAAAAA),
                            source == 1U ? VECTORS[index].accumulator
                                         : UINT64_C(0x5555555555),
                            true, true, seed);
                        const ti_c54x_trusted_operand_values operands =
                            trusted_operands(source, xmod, xar_field,
                                             VECTORS[index].shift);
                        ti_c54x_op_effect direct = {0};
                        ti_c54x_op_effect trusted = {0};

                        TI_C54X_TEST_CHECK(ti_c54x_op_sth_shft_xmem(
                                               &instruction, &input,
                                               &direct) ==
                                           TI_C54X_OP_STATUS_OK);
                        assert_memory_write(&direct, xmod, xar,
                                            VECTORS[index].expected);
                        TI_C54X_TEST_CHECK(
                            ti_c54x_trusted_op_sth_shft_xmem(
                                &instruction, &operands, &input,
                                &trusted) == TI_C54X_OP_STATUS_OK);
                        TI_C54X_TEST_CHECK(memcmp(&direct, &trusted,
                                                  sizeof(direct)) == 0);
                    }
                }
            }
        }
    }
}

static void
check_unknown_and_malformed_input_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(0U, 0U, 0U, 0U);
    const ti_c54x_op_input_snapshot selected_unknown =
        input_for(UINT64_C(1), UINT64_C(2), false, true, UINT8_C(0));
    const ti_c54x_op_input_snapshot malformed_acc =
        input_for(TI_C54X_U40_MASK + UINT64_C(1), UINT64_C(2), true, true,
                  UINT8_C(0));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_sth_shft_xmem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_sth_shft_xmem(&instruction,
                                               &selected_unknown,
                                               &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_sth_shft_xmem(&instruction, &malformed_acc,
                                               &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void
check_forged_instruction_is_transactional(void)
{
    const ti_c54x_decoded_instruction valid = decode(1U, 2U, 3U, 8U);
    const ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(0x0012345678), UINT64_C(0xFF84211234), true, true,
                  UINT8_C(0));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_decoded_instruction forged = valid;
    ti_c54x_op_effect effect = before;

    forged.bundle.program_words[0] ^= UINT16_C(0x0200);
    TI_C54X_TEST_CHECK(ti_c54x_op_sth_shft_xmem(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    check_shift_vectors();
    check_unknown_and_malformed_input_is_transactional();
    check_forged_instruction_is_transactional();
    return 0;
}
