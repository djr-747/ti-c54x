/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for single-word LD #K,dst effects,
 * including D/K8 selection over all 512 encodings, zero extension,
 * status-input independence (SXM/accumulator knownness ignored),
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
#include "../../../../../src/isa/ops/load/op_ld_k.h"
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
make_word(uint8_t destination, uint8_t immediate)
{
    return (uint16_t)(UINT16_C(0xE800) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (uint16_t)immediate);
}

static ti_c54x_decoded_instruction
decode(uint8_t destination, uint8_t immediate)
{
    const uint16_t word = make_word(destination, immediate);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_K);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_K);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_K8);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    return instruction;
}

/* Hostile snapshot: unknown accumulators and arbitrary status bits must
 * be ignored because the row declares no input resources. */
static ti_c54x_op_input_snapshot
hostile_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0xFFFFFFFFFF);
    input.accumulator_a_known = false;
    input.accumulator_b_raw40 = UINT64_C(0xFFFFFFFFFF);
    input.accumulator_b_known = false;
    input.status_sxm = UINT8_C(1);
    input.status_sxm_known = false;
    input.status_ovm = UINT8_C(1);
    input.status_ovm_known = false;
    input.status_c = UINT8_C(1);
    input.status_c_known = true;
    input.status_tc = UINT8_C(1);
    input.status_tc_known = true;
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t destination, uint8_t immediate)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = destination;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = immediate;
    operands.values[1].width_bits = UINT8_C(8);
    return operands;
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
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_K);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LD_K);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xE800));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFE00));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_K8);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
}

static void
check_effects_and_parity(void)
{
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_LD_K);
    const ti_c54x_op_input_snapshot hostile = hostile_input();

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);

    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        for (uint32_t raw = 0U; raw <= UINT32_C(0xFF); ++raw) {
            const uint8_t immediate = (uint8_t)raw;
            const ti_c54x_decoded_instruction instruction =
                decode(destination, immediate);
            const ti_c54x_op_input_snapshot clean = {0};
            ti_c54x_trusted_execution_packet packet;
            ti_c54x_trusted_operand_values operands =
                trusted_operands(destination, immediate);
            ti_c54x_op_effect checked = {0};
            ti_c54x_op_effect checked_hostile = {0};
            ti_c54x_op_effect trusted = {0};
            ti_c54x_op_effect via_dispatch = {0};
            ti_c54x_op_effect via_trusted = {0};

            TI_C54X_TEST_CHECK(ti_c54x_op_ld_k(&instruction, &clean,
                                              &checked) ==
                               TI_C54X_OP_STATUS_OK);
            assert_accumulator_write(&checked, destination,
                                     (uint64_t)immediate);
            TI_C54X_TEST_CHECK(ti_c54x_op_ld_k(&instruction, &hostile,
                                              &checked_hostile) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(memcmp(&checked_hostile, &checked,
                                      sizeof(checked)) == 0);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_k(
                                   &instruction, &operands, &clean,
                                   &trusted) == TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(memcmp(&trusted, &checked,
                                      sizeof(checked)) == 0);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                                   &instruction, &clean, &via_dispatch) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(memcmp(&via_dispatch, &checked,
                                      sizeof(checked)) == 0);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                   binding, &instruction.bundle,
                                   &packet) ==
                               TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                   &packet, &clean, &via_trusted) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(memcmp(&via_trusted, &checked,
                                      sizeof(checked)) == 0);
        }
    }
}

static void
check_argument_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(1));
    const ti_c54x_op_input_snapshot valid = {0};
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT8_C(1));
    ti_c54x_op_effect effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_k(NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_k(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_k(&instruction, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_k(
                           &instruction, NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_k(
                           &instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_k(
                           &instruction, &operands, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void
check_decode_rejection(void)
{
    const ti_c54x_decoded_instruction valid =
        decode(UINT8_C(1), UINT8_C(0xF8));
    const ti_c54x_op_input_snapshot input = {0};
    ti_c54x_decoded_instruction forged = valid;
    ti_c54x_op_effect effect;

    forged.id = TI_C54X_OPCODE_ID_LD_SRC_SHIFT;
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_k(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    forged = valid;
    forged.operand_layout =
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5;
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_k(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    forged = valid;
    forged.input_resources = TI_C54X_INPUT_RESOURCES_STATUS_SXM;
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_k(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    forged = valid;
    forged.bundle.program_words[0] ^= UINT16_C(0x0200);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_k(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static void
check_trusted_operand_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(0xF8));
    const ti_c54x_op_input_snapshot input = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT8_C(0xF8));

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_k(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(0xF8));
    operands.values[1].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_k(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(0xF8));
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_k(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(0xF8));
    operands.values[1].raw_value = UINT64_C(0x100);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_k(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_metadata();
    check_effects_and_parity();
    check_argument_rejection();
    check_decode_rejection();
    check_trusted_operand_rejection();
    return 0;
}
