/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for two-word LD #lk,16,dst effects,
 * including D selection over both encodings, exhaustive lk coverage,
 * SXM-controlled sign/zero extension and logical left shift, C/TC/OV
 * preservation with no OVM input, unknown-SXM rejection,
 * checked/trusted/dispatch parity, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, buses, memory,
 * CPU/state mutation, sequencing, timing, observers, diagnostics, or the
 * production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/load/op_ld_lk16.h"
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
make_word0(uint8_t destination)
{
    return (uint16_t)(UINT16_C(0xF062) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U));
}

static ti_c54x_decoded_instruction
decode(uint8_t destination, uint16_t immediate)
{
    const uint16_t word = make_word0(destination);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, immediate}, .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_LK_16);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_LK_16);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint8_t sxm, bool sxm_known, uint8_t seed)
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
    input.status_ovm = (uint8_t)((seed >> 2U) & UINT8_C(1));
    input.status_ovm_known = (seed & UINT8_C(16)) != 0U;
    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t destination, uint16_t immediate)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = destination;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = immediate;
    operands.values[1].width_bits = UINT8_C(16);
    return operands;
}

/* Independent oracle mirroring the ops contract: SXM widening of the
 * copied lk followed by logical left shift 16 in 40-bit space. */
static uint64_t
expected_value(uint16_t immediate, uint8_t sxm)
{
    uint64_t value = (uint64_t)immediate;

    if (sxm != UINT8_C(0) &&
        (immediate & UINT16_C(0x8000)) != UINT16_C(0)) {
        value |= (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF));
    }
    return (value << 16U) & TI_C54X_U40_MASK;
}

static void
assert_accumulator_write(const ti_c54x_op_effect *effect,
                         uint8_t destination, uint64_t value)
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
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static void
check_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_LK_16);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LD_LK_16);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xF062));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFEFF));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM);
}

static void
check_effects_and_parity(void)
{
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_LD_LK_16);

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_ld_lk16);

    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        const ti_c54x_decoded_instruction instruction =
            decode(destination, UINT16_C(0));
        for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
            for (uint32_t raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
                const uint16_t immediate = (uint16_t)raw;
                ti_c54x_decoded_instruction worded = instruction;
                ti_c54x_trusted_execution_packet packet;
                const ti_c54x_op_input_snapshot clean =
                    input_for(sxm, true, UINT8_C(0));
                const ti_c54x_op_input_snapshot hostile =
                    input_for(sxm, true, UINT8_C(0x1D));
                ti_c54x_trusted_operand_values operands =
                    trusted_operands(destination, immediate);
                ti_c54x_op_effect checked = {0};
                ti_c54x_op_effect checked_hostile = {0};
                ti_c54x_op_effect trusted = {0};
                ti_c54x_op_effect via_dispatch = {0};
                ti_c54x_op_effect via_trusted = {0};
                const uint64_t expected = expected_value(immediate, sxm);

                /* The extension word is a verbatim copy: varying it
                 * exercises the leaf transform without re-decoding. */
                worded.bundle.program_words[1] = immediate;
                TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                       binding, &worded.bundle, &packet) ==
                                   TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
                TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(
                                       &worded, &clean, &checked) ==
                                   TI_C54X_OP_STATUS_OK);
                assert_accumulator_write(&checked, destination, expected);
                TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(
                                       &worded, &hostile,
                                       &checked_hostile) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(memcmp(&checked_hostile, &checked,
                                          sizeof(checked)) == 0);
                TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk16(
                                       &worded, &operands, &clean,
                                       &trusted) == TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(memcmp(&trusted, &checked,
                                          sizeof(checked)) == 0);
                TI_C54X_TEST_CHECK(
                    ti_c54x_dispatch_decoded_with_input_snapshot(
                        &worded, &clean, &via_dispatch) ==
                    TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(memcmp(&via_dispatch, &checked,
                                          sizeof(checked)) == 0);
                TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                       &packet, &clean, &via_trusted) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(memcmp(&via_trusted, &checked,
                                          sizeof(checked)) == 0);
            }
        }
    }
}

static void
check_argument_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT16_C(1));
    const ti_c54x_op_input_snapshot valid = input_for(1U, true, 0U);
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT16_C(1));
    ti_c54x_op_effect effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(&instruction, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk16(
                           &instruction, NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk16(
                           &instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk16(
                           &instruction, &operands, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void
check_input_and_decode_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(1), UINT16_C(0xFEDC));
    const ti_c54x_op_input_snapshot valid = input_for(1U, true, UINT8_C(0xA5));
    const ti_c54x_op_input_snapshot unknown = input_for(0U, false, 0U);
    const ti_c54x_op_input_snapshot malformed = input_for(2U, true, 0U);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(&instruction, &unknown, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(&instruction, &malformed,
                                          &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.repeatability = TI_C54X_REPEATABILITY_INELIGIBLE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_word_count = UINT8_C(1);
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0001);
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_lk16(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void
check_trusted_operand_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(1), UINT16_C(0xFEDC));
    const ti_c54x_op_input_snapshot input = input_for(0U, true, 0U);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(1), UINT16_C(0xFEDC));

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk16(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT16_C(0xFEDC));
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk16(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT16_C(0xFEDC));
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk16(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT16_C(0xFEDC));
    operands.values[1].width_bits = UINT8_C(15);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_lk16(
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
