/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for single-word LD src[,SHIFT],dst
 * effects, including S/D/SHIFT5 selection, SXM-dependent right shifts,
 * OVM saturation with sticky destination overflow, unknown and
 * non-canonical accumulator rejection, checked/trusted/dispatch parity,
 * and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, buses, memory,
 * CPU/state mutation, sequencing, timing, observers, diagnostics, or the
 * production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/load/op_ld_src_shift.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SRC_U40_MASK UINT64_C(0xFFFFFFFFFF)

static uint16_t
make_word(uint8_t source, uint8_t destination, uint8_t shift)
{
    return (uint16_t)(UINT16_C(0xF440) |
                      (uint16_t)(((uint16_t)(source & UINT8_C(1))) << 9U) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (uint16_t)(shift & UINT8_C(0x1F)));
}

static ti_c54x_decoded_instruction
decode(uint8_t source, uint8_t destination, uint8_t shift)
{
    const uint16_t word = make_word(source, destination, shift);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_SRC_SHIFT);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_SRC_SHIFT);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint64_t a, bool a_known, uint64_t b, bool b_known, uint8_t sxm,
          bool sxm_known, uint8_t ovm, bool ovm_known, uint8_t seed)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = a;
    input.accumulator_a_known = a_known;
    input.accumulator_b_raw40 = b;
    input.accumulator_b_known = b_known;
    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    input.status_ovm = ovm;
    input.status_ovm_known = ovm_known;
    input.status_c = (uint8_t)(seed & UINT8_C(1));
    input.status_c_known = (seed & UINT8_C(2)) != 0U;
    input.status_tc = (uint8_t)((seed >> 2U) & UINT8_C(1));
    input.status_tc_known = (seed & UINT8_C(8)) != 0U;
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t source, uint8_t destination, uint8_t shift)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = source;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = destination;
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = shift;
    operands.values[2].width_bits = UINT8_C(5);
    return operands;
}

/* Independent oracle: no production helpers. Right shifts fill with the
 * SXM bit; overflow follows the guard rule; saturation follows source
 * sign under OVM. */
static uint64_t
expected_value(uint64_t source40, uint8_t shift_raw, uint8_t sxm,
               uint8_t ovm, bool *overflow)
{
    uint64_t value;
    uint64_t guard;
    uint8_t bit31;
    bool negative =
        (source40 & UINT64_C(0x8000000000)) != UINT64_C(0);

    if (shift_raw <= UINT8_C(15)) {
        value = (source40 << shift_raw) & SRC_U40_MASK;
    } else {
        const unsigned right = 32U - shift_raw;

        if (sxm != UINT8_C(0)) {
            const int64_t signed_source =
                (int64_t)(negative
                              ? (source40 | UINT64_C(0xFFFFFF0000000000))
                              : source40);

            value = ((uint64_t)(signed_source >> right)) & SRC_U40_MASK;
        } else {
            value = (source40 >> right) & SRC_U40_MASK;
        }
    }
    guard = (value >> 32U) & UINT64_C(0xFF);
    bit31 = (uint8_t)((value >> 31U) & UINT64_C(1));
    *overflow = guard != (bit31 != UINT8_C(0) ? UINT64_C(0xFF) : UINT64_C(0));
    if (*overflow && ovm != UINT8_C(0)) {
        value = negative ? UINT64_C(0xFF80000000) : UINT64_C(0x007FFFFFFF);
    }
    return value;
}

static void
assert_accumulator_write(const ti_c54x_op_effect *effect,
                         uint8_t destination, uint64_t value, bool overflow)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK_U64(effect->payload.accumulator_write.value_raw40,
                           value);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        effect->payload.accumulator_write.status_delta
            .destination_overflow_action ==
        (overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                  : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE));
}

static void
check_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_SRC_SHIFT);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LD_SRC_SHIFT);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xF440));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFCE0));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM);
}

static void
check_effects_and_parity(void)
{
    static const uint64_t sources[] = {
        UINT64_C(0x0000000000), UINT64_C(0x007FFFFFFF),
        UINT64_C(0x0080000000), UINT64_C(0xFF80000000),
        UINT64_C(0xFFFFFFFFFF), UINT64_C(0x0000123456)};
    static const uint8_t shifts[] = {UINT8_C(0),  UINT8_C(1),  UINT8_C(8),
                                     UINT8_C(15), UINT8_C(16), UINT8_C(17),
                                     UINT8_C(24), UINT8_C(31)};

    for (uint8_t source = 0U; source <= UINT8_C(1); ++source) {
        for (uint8_t destination = 0U; destination <= UINT8_C(1);
             ++destination) {
            for (size_t shift_index = 0U;
                 shift_index < sizeof(shifts) / sizeof(shifts[0]);
                 ++shift_index) {
                const uint8_t shift = shifts[shift_index];
                const ti_c54x_decoded_instruction instruction =
                    decode(source, destination, shift);
                const ti_c54x_trusted_opcode_binding *binding =
                    ti_c54x_trusted_opcode_binding_for_id(
                        TI_C54X_OPCODE_ID_LD_SRC_SHIFT);
                ti_c54x_trusted_execution_packet packet;

                TI_C54X_TEST_CHECK(binding != NULL);
                TI_C54X_TEST_CHECK(binding->handler ==
                                   ti_c54x_trusted_op_ld_src_shift);
                TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                       binding, &instruction.bundle,
                                       &packet) ==
                                   TI_C54X_TRUSTED_EXECUTION_PACKET_OK);

                for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
                    for (uint8_t ovm = 0U; ovm <= UINT8_C(1); ++ovm) {
                        ti_c54x_op_effect baseline = {0};
                        for (uint8_t seed = 0U; seed < UINT8_C(4); ++seed) {
                            for (size_t value_index = 0U;
                                 value_index <
                                 sizeof(sources) / sizeof(sources[0]);
                                 ++value_index) {
                                const uint64_t source40 =
                                    sources[value_index];
                                const uint64_t other40 =
                                    sources[(value_index + 3U) %
                                            (sizeof(sources) /
                                             sizeof(sources[0]))];
                                const ti_c54x_op_input_snapshot input =
                                    source == UINT8_C(0)
                                        ? input_for(source40, true, other40,
                                                    true, sxm, true, ovm,
                                                    true, seed)
                                        : input_for(other40, true, source40,
                                                    true, sxm, true, ovm,
                                                    true, seed);
                                const ti_c54x_trusted_operand_values
                                    operands = trusted_operands(
                                        source, destination, shift);
                                const ti_c54x_op_effect before = {
                                    .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                                    .payload.repeat_start = {
                                        .operand = UINT16_C(0xA55A)}};
                                ti_c54x_op_effect direct = before;
                                ti_c54x_op_effect checked = before;
                                ti_c54x_op_effect trusted = before;
                                ti_c54x_op_effect trusted_direct = before;
                                bool overflow = false;
                                const uint64_t expected = expected_value(
                                    source40, shift, sxm, ovm, &overflow);

                                TI_C54X_TEST_CHECK(
                                    ti_c54x_op_ld_src_shift(
                                        &instruction, &input, &direct) ==
                                    TI_C54X_OP_STATUS_OK);
                                TI_C54X_TEST_CHECK(
                                    ti_c54x_dispatch_decoded_with_input_snapshot(
                                        &instruction, &input, &checked) ==
                                    TI_C54X_OP_STATUS_OK);
                                TI_C54X_TEST_CHECK(
                                    ti_c54x_dispatch_trusted(
                                        &packet, &input, &trusted) ==
                                    TI_C54X_OP_STATUS_OK);
                                TI_C54X_TEST_CHECK(
                                    ti_c54x_trusted_op_ld_src_shift(
                                        &packet.instruction, &operands,
                                        &input, &trusted_direct) ==
                                    TI_C54X_OP_STATUS_OK);
                                assert_accumulator_write(
                                    &direct, destination, expected,
                                    overflow);
                                if (seed == UINT8_C(0) &&
                                    value_index == 0U) {
                                    baseline = direct;
                                } else if (value_index == 0U) {
                                    TI_C54X_TEST_CHECK(
                                        memcmp(&direct, &baseline,
                                               sizeof(direct)) == 0);
                                }
                                TI_C54X_TEST_CHECK(
                                    memcmp(&direct, &checked,
                                           sizeof(direct)) == 0);
                                TI_C54X_TEST_CHECK(
                                    memcmp(&direct, &trusted,
                                           sizeof(direct)) == 0);
                                TI_C54X_TEST_CHECK(
                                    memcmp(&direct, &trusted_direct,
                                           sizeof(direct)) == 0);
                            }
                        }
                    }
                }
            }
        }
    }
}

static void
check_input_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(1), UINT8_C(7));
    const ti_c54x_op_input_snapshot valid =
        input_for(UINT64_C(0x123456789A), true, UINT64_C(0), true,
                  UINT8_C(1), true, UINT8_C(0), true, UINT8_C(0));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_src_shift(NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_src_shift(&instruction, NULL,
                                                &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_src_shift(&instruction, &valid,
                                                NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        static const ti_c54x_op_input_snapshot rejected[] = {
            /* Unknown SXM. */
            {.accumulator_a_raw40 = UINT64_C(0x123456789A),
             .accumulator_a_known = true,
             .accumulator_b_known = true,
             .status_sxm = UINT8_C(0),
             .status_sxm_known = false,
             .status_ovm_known = true},
            /* Unknown OVM. */
            {.accumulator_a_raw40 = UINT64_C(0x123456789A),
             .accumulator_a_known = true,
             .accumulator_b_known = true,
             .status_sxm = UINT8_C(1),
             .status_sxm_known = true,
             .status_ovm_known = false},
            /* Malformed SXM. */
            {.accumulator_a_raw40 = UINT64_C(0x123456789A),
             .accumulator_a_known = true,
             .accumulator_b_known = true,
             .status_sxm = UINT8_C(2),
             .status_sxm_known = true,
             .status_ovm_known = true},
            /* Unknown source accumulator. */
            {.accumulator_b_known = true,
             .status_sxm = UINT8_C(1),
             .status_sxm_known = true,
             .status_ovm_known = true},
            /* Non-canonical source value. */
            {.accumulator_a_raw40 = UINT64_C(0x10000000000),
             .accumulator_a_known = true,
             .accumulator_b_known = true,
             .status_sxm = UINT8_C(1),
             .status_sxm_known = true,
             .status_ovm_known = true},
        };
        static const ti_c54x_op_status expected[] = {
            TI_C54X_OP_STATUS_UNKNOWN_INPUT, TI_C54X_OP_STATUS_UNKNOWN_INPUT,
            TI_C54X_OP_STATUS_INVALID_OPERAND,
            TI_C54X_OP_STATUS_UNKNOWN_INPUT,
            TI_C54X_OP_STATUS_INVALID_OPERAND};

        for (size_t index = 0U;
             index < sizeof(rejected) / sizeof(rejected[0]); ++index) {
            TI_C54X_TEST_CHECK(ti_c54x_op_ld_src_shift(
                                   &instruction, &rejected[index],
                                   &effect) == expected[index]);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) ==
                               0);
        }
    }

    {
        /* Unknown destination accumulator rejects only when it differs
         * from the source. */
        ti_c54x_op_input_snapshot same =
            input_for(UINT64_C(0x123456789A), true, UINT64_C(0), false,
                      UINT8_C(1), true, UINT8_C(0), true, UINT8_C(0));
        ti_c54x_op_input_snapshot different = same;
        const ti_c54x_decoded_instruction same_dst =
            decode(UINT8_C(0), UINT8_C(0), UINT8_C(3));
        const ti_c54x_decoded_instruction other_dst =
            decode(UINT8_C(0), UINT8_C(1), UINT8_C(3));

        different.accumulator_b_known = false;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_src_shift(&same_dst, &same,
                                                    &effect) ==
                           TI_C54X_OP_STATUS_OK);
        effect = before;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_src_shift(&other_dst, &different,
                                                    &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void
check_decode_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(1), UINT8_C(1), UINT8_C(2));
    const ti_c54x_op_input_snapshot valid =
        input_for(UINT64_C(0), true, UINT64_C(0), true, UINT8_C(1), true,
                  UINT8_C(0), true, UINT8_C(0));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_src_shift(&forged, &valid,
                                                    &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_src_shift(&forged, &valid,
                                                    &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_src_shift(&forged, &valid,
                                                    &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0020);
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_src_shift(&forged, &valid,
                                                    &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void
check_trusted_operand_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(1), UINT8_C(9));
    const ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(1), true, UINT64_C(2), true, UINT8_C(0), true,
                  UINT8_C(0), true, UINT8_C(0));
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_LD_SRC_SHIFT);
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT8_C(1), UINT8_C(9));

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_src_shift(
                           &packet.instruction, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_src_shift(
                           &packet.instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_src_shift(
                           &packet.instruction, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_src_shift(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(1), UINT8_C(9));
    operands.values[2].width_bits = UINT8_C(4);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_src_shift(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(1), UINT8_C(9));
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_src_shift(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_metadata();
    check_effects_and_parity();
    check_input_rejection();
    check_decode_rejection();
    check_trusted_operand_rejection();
    return 0;
}
