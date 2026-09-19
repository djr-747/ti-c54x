/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the DP-relative direct WORD16 LDU
 * memory effect, including D/dma selection, DP-knownness gating,
 * zero-extending completion, checked/trusted/dispatch parity, and
 * transactional rejection. Does not own: catalog rows,
 * operand-layout descriptors, address resolution, buses, CPU/state mutation,
 * sequencing, timing, observers, diagnostics, or production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/load/op_ldu_direct.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* TI SPRU172C LDU p. 4-78: word 0 pins 0x1200 with the D
 * destination bit at bit 8 (A=0/B=1), I pinned 0 at bit 7,
 * and the 7-bit DP-relative direct cookie at bits 6:0. */
static uint16_t ldu_word(uint8_t destination, uint8_t dma)
{
    return (uint16_t)(UINT16_C(0x1200) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      (uint16_t)(dma & UINT8_C(0x7F)));
}

static ti_c54x_decoded_instruction decode(uint8_t destination, uint8_t dma)
{
    const uint16_t word = ldu_word(destination, dma);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LDU_DIRECT);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_DP);
    return instruction;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t destination,
                                                      uint8_t dma)
{
    ti_c54x_trusted_operand_values operands = {0};
    operands.count = UINT8_C(3);
    operands.values[0].raw_value = destination;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT8_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = dma;
    operands.values[2].width_bits = UINT8_C(7);
    return operands;
}

static ti_c54x_op_input_snapshot dp_input(bool known)
{
    ti_c54x_op_input_snapshot input = {0};
    input.status_dp = UINT16_C(0x1A);
    input.status_dp_known = known;
    return input;
}

static void assert_memory_read(const ti_c54x_op_effect *effect, uint8_t dma)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_DP_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.long_offset_word ==
                       UINT16_C(0x1A));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.direct_dma7 ==
                       dma);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
}

static void assert_completed_write(const ti_c54x_op_effect *effect,
                                   uint8_t destination, uint16_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK_U64(effect->payload.accumulator_write.value_raw40,
                           (uint64_t)value);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
}

static void check_all_forms_and_completion(void)
{
    static const uint16_t reads[] = {UINT16_C(0x0000), UINT16_C(0x7FFF),
                                     UINT16_C(0x8000), UINT16_C(0xFFFF)};
    static const uint8_t sxm_values[] = {UINT8_C(0), UINT8_C(1), UINT8_C(2),
                                         UINT8_MAX};

    for (uint8_t destination = 0U; destination <= UINT8_C(1); ++destination) {
        for (uint16_t dma = 0U; dma <= UINT16_C(0x7F); ++dma) {
            const ti_c54x_decoded_instruction instruction =
                decode(destination, (uint8_t)dma);
            const ti_c54x_trusted_opcode_binding *binding =
                ti_c54x_trusted_opcode_binding_for_id(
                    TI_C54X_OPCODE_ID_LDU_DIRECT);
            const ti_c54x_instruction_word_bundle bundle = {
                .program_words = {instruction.bundle.program_words[0],
                                  UINT16_C(0)},
                .program_word_count = UINT8_C(1)};
            ti_c54x_trusted_execution_packet packet;

            TI_C54X_TEST_CHECK(binding != NULL);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                   binding, &bundle, &packet) ==
                               TI_C54X_TRUSTED_EXECUTION_PACKET_OK);

            for (size_t sxm_index = 0U;
                 sxm_index < sizeof(sxm_values) / sizeof(sxm_values[0]);
                 ++sxm_index) {
                ti_c54x_op_input_snapshot input = dp_input(true);
                ti_c54x_op_effect direct = {
                    .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                    .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
                ti_c54x_op_effect checked = direct;
                ti_c54x_op_effect trusted = direct;

                /* SXM is neither observed nor interpreted. */
                input.status_sxm = sxm_values[sxm_index];
                input.status_sxm_known = true;
                TI_C54X_TEST_CHECK(ti_c54x_op_ldu_direct(
                                       &instruction, &input, &direct) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(
                    ti_c54x_dispatch_decoded_with_input_snapshot(
                        &instruction, &input, &checked) ==
                    TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                       &packet, &input, &trusted) ==
                                   TI_C54X_OP_STATUS_OK);
                assert_memory_read(&direct, (uint8_t)dma);
                TI_C54X_TEST_CHECK(
                    memcmp(&direct, &checked, sizeof(direct)) == 0);
                TI_C54X_TEST_CHECK(
                    memcmp(&direct, &trusted, sizeof(direct)) == 0);

                for (size_t index = 0U;
                     index < sizeof(reads) / sizeof(reads[0]); ++index) {
                    ti_c54x_op_effect completed = {
                        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};

                    TI_C54X_TEST_CHECK(direct.payload.memory.complete(
                                           &instruction, &input, reads[index],
                                           &completed) ==
                                       TI_C54X_OP_STATUS_OK);
                    assert_completed_write(&completed, destination,
                                           reads[index]);
                }
            }
        }
    }
}

static void check_dp_unknown_rejects(void)
{
    const ti_c54x_decoded_instruction instruction = decode(0U, 0U);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_LDU_DIRECT);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {instruction.bundle.program_words[0], UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_trusted_execution_packet packet;
    ti_c54x_op_input_snapshot input = dp_input(false);
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    const ti_c54x_op_effect before = effect;

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_ldu_direct(&instruction, &input,
                                             &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(&packet, &input,
                                                &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    /* A 10-bit DP value is not a 9-bit page. */
    input = dp_input(true);
    input.status_dp = UINT16_C(0x200);
    TI_C54X_TEST_CHECK(ti_c54x_op_ldu_direct(&instruction, &input,
                                             &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void check_neighbor_rows_reject(void)
{
    /* The I=1 indirect half routes to the S34 leaves; the shared
     * staging core rejects it here so a misroute fails
     * transactional. The 0x11FF below-neighbor stays undecoded. */
    static const uint16_t indirect[] = {
        UINT16_C(0x1280), UINT16_C(0x129F), UINT16_C(0x1380),
        UINT16_C(0x139F)};

    for (size_t index = 0U;
         index < sizeof(indirect) / sizeof(indirect[0]); ++index) {
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(indirect[index]);

        TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(first.id != TI_C54X_OPCODE_ID_LDU_DIRECT);
    }
    {
        const ti_c54x_decode_result below =
            ti_c54x_decode_first_word(UINT16_C(0x11FF));

        TI_C54X_TEST_CHECK(below.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(below.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }
    /* The S34 indirect neighbors still claim their words. */
    TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x1290)).id ==
                       TI_C54X_OPCODE_ID_LDU_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x1390)).id ==
                       TI_C54X_OPCODE_ID_LDU_SMEM);
}

static void check_transactional_checked_and_dispatch_rejection(void)
{
    const ti_c54x_decoded_instruction instruction = decode(0U, 0U);
    const ti_c54x_op_input_snapshot input = dp_input(true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_ldu_direct(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ldu_direct(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ldu_direct(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_ldu_direct(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                               &forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ldu_direct(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ldu_direct(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        /* I=1 over either select rejects as INVALID_OPERAND: the
         * indirect half stays decodable in the shared extractor so
         * a misroute fails transactional (the S34 leaves own it). */
        forged = instruction;
        forged.bundle.program_words[0] |= UINT16_C(0x0080);
        TI_C54X_TEST_CHECK(ti_c54x_op_ldu_direct(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void check_trusted_operand_rejection(void)
{
    const ti_c54x_op_input_snapshot input = dp_input(true);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_LDU_DIRECT);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {ldu_word(1U, 7U), UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_trusted_execution_packet packet;
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    const ti_c54x_op_effect before = effect;
    ti_c54x_trusted_operand_values operands = trusted_operands(1U, 7U);

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldu_direct(
                           &packet.instruction, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldu_direct(
                           &packet.instruction, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldu_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(1U, 7U);
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldu_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(1U, 7U);
    operands.values[1].raw_value = UINT64_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldu_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(1U, 7U);
    operands.values[2].raw_value = UINT64_C(0x80);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldu_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void check_completion_rejection(void)
{
    const ti_c54x_decoded_instruction instruction = decode(1U, 7U);
    const ti_c54x_op_input_snapshot input = dp_input(true);
    ti_c54x_op_effect read_effect = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x1234)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_ldu_direct(&instruction, &input,
                                             &read_effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           NULL, &input, UINT16_C(0xFFFF), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, NULL, UINT16_C(0xFFFF), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, &input, UINT16_C(0xFFFF), NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.bundle.program_words[0] |= UINT16_C(0x0080);
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                               &forged, &input, UINT16_C(0xFFFF), &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int main(void)
{
    check_all_forms_and_completion();
    check_dp_unknown_rejects();
    check_neighbor_rows_reject();
    check_transactional_checked_and_dispatch_rejection();
    check_trusted_operand_rejection();
    check_completion_rejection();
    return 0;
}
