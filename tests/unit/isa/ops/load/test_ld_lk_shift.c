/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for two-word LD #lk[,SHFT],dst effects,
 * including D selection, all 16 unsigned shifts, exhaustive lk
 * coverage, SXM-controlled extension with logical left shift, OVM
 * saturation with sticky destination overflow, C/TC preservation,
 * unknown-SXM/OVM rejection, checked/trusted/dispatch parity, and
 * transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, buses, memory,
 * CPU/state mutation, sequencing, timing, observers, diagnostics, or the
 * production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/load/op_ld_lk_shift.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
make_word0(uint8_t destination, uint8_t shift)
{
    return (uint16_t)(UINT16_C(0xF020) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (shift & UINT8_C(0x0F)));
}

static ti_c54x_decoded_instruction
decode(uint8_t destination, uint8_t shift, uint16_t immediate)
{
    const uint16_t word = make_word0(destination, shift);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, immediate}, .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_LK_SHIFT);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_LK_SHIFT);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_SHIFT4_LK16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM_OVM);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint8_t sxm, bool sxm_known, uint8_t ovm, bool ovm_known,
          uint8_t seed)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_b_raw40 = UINT64_C(0x2ABCDEF012);
    input.accumulator_a_known = (seed & UINT8_C(1)) != 0U;
    input.accumulator_b_known = (seed & UINT8_C(2)) != 0U;
    input.status_c = (uint8_t)(seed & UINT8_C(1));
    input.status_c_known = (seed & UINT8_C(4)) != 0U;
    input.status_tc = (uint8_t)((seed >> 1U) & UINT8_C(1));
    input.status_tc_known = (seed & UINT8_C(8)) != 0U;
    input.status_ovm = ovm;
    input.status_ovm_known = ovm_known;
    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t destination, uint8_t shift, uint16_t immediate)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = destination;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = shift;
    operands.values[1].width_bits = UINT8_C(4);
    operands.values[2].raw_value = immediate;
    operands.values[2].width_bits = UINT8_C(16);
    return operands;
}

/* Independent oracle mirroring the ops contract: SXM widening of the
 * copied lk, logical left shift by the unsigned SHFT, guard-bit
 * overflow with OVM saturation, and sticky destination overflow. */
static uint64_t
expected_value(uint16_t immediate, uint8_t sxm, uint8_t shift, uint8_t ovm,
               bool *overflow)
{
    uint64_t widened = (uint64_t)immediate;
    uint64_t value;
    uint64_t guard;
    uint8_t bit31;
    bool negative;

    if (sxm != UINT8_C(0) &&
        (immediate & UINT16_C(0x8000)) != UINT16_C(0)) {
        widened |= (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF));
    }
    negative = (widened & TI_C54X_U40_SIGN_BIT) != UINT64_C(0);
    value = (widened << shift) & TI_C54X_U40_MASK;
    guard = (value >> UINT64_C(32)) & UINT64_C(0xFF);
    bit31 = (uint8_t)((value >> UINT64_C(31)) & UINT64_C(1));
    *overflow = guard != (bit31 != UINT8_C(0) ? UINT64_C(0xFF)
                                              : UINT64_C(0));
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
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_LK_SHIFT);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LD_LK_SHIFT);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xF020));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFEF0));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_SHIFT4_LK16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM_OVM);
}

static void
check_effects_and_parity(void)
{
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_LD_LK_SHIFT);

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_ld_lk_shift);

    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        for (uint8_t shift = 0U; shift <= UINT8_C(15); ++shift) {
            const ti_c54x_decoded_instruction instruction =
                decode(destination, shift, UINT16_C(0));
            for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
                for (uint8_t ovm = 0U; ovm <= UINT8_C(1); ++ovm) {
                    for (uint32_t raw = 0U; raw <= UINT32_C(0xFFFF);
                         ++raw) {
                        const uint16_t immediate = (uint16_t)raw;
                        ti_c54x_decoded_instruction worded = instruction;
                        ti_c54x_trusted_execution_packet packet;
                        const ti_c54x_op_input_snapshot clean =
                            input_for(sxm, true, ovm, true, UINT8_C(0));
                        const ti_c54x_op_input_snapshot hostile =
                            input_for(sxm, true, ovm, true, UINT8_C(0x1D));
                        ti_c54x_trusted_operand_values operands =
                            trusted_operands(destination, shift, immediate);
                        ti_c54x_op_effect checked = {0};
                        ti_c54x_op_effect checked_hostile = {0};
                        ti_c54x_op_effect trusted = {0};
                        ti_c54x_op_effect via_dispatch = {0};
                        ti_c54x_op_effect via_trusted = {0};
                        bool overflow = false;
                        const uint64_t expected =
                            expected_value(immediate, sxm, shift, ovm,
                                           &overflow);

                        /* The extension word is a verbatim copy:
                         * varying it exercises the leaf transform
                         * without re-decoding. */
                        worded.bundle.program_words[1] = immediate;
                        TI_C54X_TEST_CHECK(
                            ti_c54x_trusted_execution_packet_prepare(
                                binding, &worded.bundle, &packet) ==
                            TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
                        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(
                                               &worded, &clean, &checked) ==
                                           TI_C54X_OP_STATUS_OK);
                        assert_accumulator_write(&checked, destination,
                                                 expected, overflow);
                        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(
                                               &worded, &hostile,
                                               &checked_hostile) ==
                                           TI_C54X_OP_STATUS_OK);
                        TI_C54X_TEST_CHECK(memcmp(&checked_hostile, &checked,
                                                  sizeof(checked)) == 0);
                        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk_shift(
                                               &worded, &operands, &clean,
                                               &trusted) ==
                                           TI_C54X_OP_STATUS_OK);
                        TI_C54X_TEST_CHECK(memcmp(&trusted, &checked,
                                                  sizeof(checked)) == 0);
                        TI_C54X_TEST_CHECK(
                            ti_c54x_dispatch_decoded_with_input_snapshot(
                                &worded, &clean, &via_dispatch) ==
                            TI_C54X_OP_STATUS_OK);
                        TI_C54X_TEST_CHECK(memcmp(&via_dispatch, &checked,
                                                  sizeof(checked)) == 0);
                        TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                               &packet, &clean,
                                               &via_trusted) ==
                                           TI_C54X_OP_STATUS_OK);
                        TI_C54X_TEST_CHECK(memcmp(&via_trusted, &checked,
                                                  sizeof(checked)) == 0);
                    }
                }
            }
        }
    }
}

static void
check_argument_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(3), UINT16_C(1));
    const ti_c54x_op_input_snapshot valid =
        input_for(1U, true, 0U, true, 0U);
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT8_C(3), UINT16_C(1));
    ti_c54x_op_effect effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(&instruction, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk_shift(
                           &instruction, NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk_shift(
                           &instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk_shift(
                           &instruction, &operands, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void
check_input_and_decode_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(1), UINT8_C(15), UINT16_C(0xFEDC));
    const ti_c54x_op_input_snapshot valid =
        input_for(1U, true, 1U, true, UINT8_C(0xA5));
    const ti_c54x_op_input_snapshot unknown_sxm =
        input_for(0U, false, 1U, true, 0U);
    const ti_c54x_op_input_snapshot unknown_ovm =
        input_for(1U, true, 0U, false, 0U);
    const ti_c54x_op_input_snapshot malformed_sxm =
        input_for(2U, true, 1U, true, 0U);
    const ti_c54x_op_input_snapshot malformed_ovm =
        input_for(1U, true, 2U, true, 0U);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(&instruction, &unknown_sxm,
                                              &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(&instruction, &unknown_ovm,
                                              &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(&instruction, &malformed_sxm,
                                              &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(&instruction, &malformed_ovm,
                                              &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(&forged, &valid,
                                                  &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(&forged, &valid,
                                                  &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.repeatability = TI_C54X_REPEATABILITY_INELIGIBLE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(&forged, &valid,
                                                  &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(&forged, &valid,
                                                  &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_word_count = UINT8_C(1);
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(&forged, &valid,
                                                  &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0020);
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk_shift(&forged, &valid,
                                                  &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void
check_trusted_operand_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(1), UINT8_C(15), UINT16_C(0xFEDC));
    const ti_c54x_op_input_snapshot input =
        input_for(0U, true, 0U, true, 0U);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(1), UINT8_C(15), UINT16_C(0xFEDC));

    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk_shift(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(15), UINT16_C(0xFEDC));
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk_shift(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(15), UINT16_C(0xFEDC));
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk_shift(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(15), UINT16_C(0xFEDC));
    operands.values[1].width_bits = UINT8_C(5);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk_shift(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(15), UINT16_C(0xFEDC));
    operands.values[1].raw_value = UINT64_C(16);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk_shift(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(15), UINT16_C(0xFEDC));
    operands.values[2].width_bits = UINT8_C(15);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk_shift(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_metadata();
    check_effects_and_parity();
    check_argument_rejection();
    check_input_and_decode_rejection();
    check_trusted_operand_rejection();
    return 0;
}
