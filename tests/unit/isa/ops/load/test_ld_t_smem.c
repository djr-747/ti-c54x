/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the MOD2 WORD16 LD-to-T memory
 * effect, including ARF selection, raw WORD16 T completion, checked/trusted/
 * dispatch parity, ignored SXM/status input, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, address resolution,
 * buses, CPU/state mutation, sequencing, timing, observers, diagnostics, or
 * production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/load/op_ld_t_smem.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t ld_t_word(uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x3090) | (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_decoded_instruction decode(uint8_t arf)
{
    const uint16_t word = ld_t_word(arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_T_SMEM);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_T_SMEM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    return instruction;
}

static ti_c54x_op_input_snapshot input_for(uint8_t sxm, bool sxm_known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    input.status_c = UINT8_C(1);
    input.status_c_known = false;
    input.status_tc = UINT8_C(1);
    input.status_tc_known = false;
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t arf)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = arf;
    operands.values[0].width_bits = UINT8_C(3);
    operands.values[1].raw_value = TI_C54X_INDIRECT_SINGLE_MOD2;
    operands.values[1].width_bits = UINT8_C(2);
    return operands;
}

static void assert_memory_read(const ti_c54x_op_effect *effect, uint8_t arf)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode ==
                       TI_C54X_INDIRECT_SINGLE_MOD2);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.write_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
}

static void assert_t_write(const ti_c54x_op_effect *effect, uint16_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_T_REGISTER_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.t_register_write.raw_value == value);
}

static void check_all_forms_and_completion(void)
{
    static const uint8_t sxm_values[] = {
        UINT8_C(0), UINT8_C(1), UINT8_C(2), UINT8_MAX};
    static const uint16_t reads[] = {UINT16_C(0x0000), UINT16_C(0x7FFF),
                                     UINT16_C(0x8000), UINT16_C(0xFFFF)};

    for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
        const ti_c54x_decoded_instruction instruction = decode(arf);
        const ti_c54x_trusted_opcode_binding *binding =
            ti_c54x_trusted_opcode_binding_for_id(
                TI_C54X_OPCODE_ID_LD_T_SMEM);
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
            for (uint8_t known = 0U; known <= UINT8_C(1); ++known) {
                const ti_c54x_op_input_snapshot input =
                    input_for(sxm_values[sxm_index], known != UINT8_C(0));
                ti_c54x_op_effect direct = {
                    .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                    .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
                ti_c54x_op_effect checked = direct;
                ti_c54x_op_effect trusted = direct;

                TI_C54X_TEST_CHECK(ti_c54x_op_ld_t_smem(
                                       &instruction, &input, &direct) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(
                    ti_c54x_dispatch_decoded_with_input_snapshot(
                        &instruction, &input, &checked) ==
                    TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                       &packet, &input, &trusted) ==
                                   TI_C54X_OP_STATUS_OK);
                assert_memory_read(&direct, arf);
                TI_C54X_TEST_CHECK(memcmp(&direct, &checked, sizeof(direct)) ==
                                   0);
                TI_C54X_TEST_CHECK(memcmp(&direct, &trusted, sizeof(direct)) ==
                                   0);

                for (size_t read_index = 0U;
                     read_index < sizeof(reads) / sizeof(reads[0]);
                     ++read_index) {
                    ti_c54x_op_effect completed = {
                        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};

                    TI_C54X_TEST_CHECK(direct.payload.memory.complete(
                                           &instruction, &input, reads[read_index],
                                           &completed) ==
                                       TI_C54X_OP_STATUS_OK);
                    assert_t_write(&completed, reads[read_index]);
                }
            }
        }
    }
}

static void check_transactional_checked_rejection(void)
{
    const ti_c54x_decoded_instruction instruction = decode(UINT8_C(0));
    const ti_c54x_op_input_snapshot input = input_for(UINT8_C(2), false);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_t_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_t_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_t_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_t_smem(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_t_smem(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_STATUS_SXM;
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_t_smem(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0020);
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_t_smem(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void check_trusted_operand_rejection(void)
{
    const ti_c54x_op_input_snapshot input = input_for(UINT8_C(3), false);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_LD_T_SMEM);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {ld_t_word(UINT8_C(7)), UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands = trusted_operands(UINT8_C(7));

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_t_smem(
                           &packet.instruction, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_t_smem(
                           &packet.instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_t_smem(
                           &packet.instruction, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_t_smem(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(7));
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_t_smem(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(7));
    operands.values[0].raw_value = UINT64_C(8);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_t_smem(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void check_completion_rejection(void)
{
    const ti_c54x_decoded_instruction instruction = decode(UINT8_C(7));
    const ti_c54x_op_input_snapshot input = input_for(UINT8_C(1), false);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCC33)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_op_effect read_effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_t_smem(&instruction, &input,
                                            &read_effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           NULL, &input, UINT16_C(0x8000), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, NULL, UINT16_C(0x8000), &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                           &instruction, &input, UINT16_C(0x8000), NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                               &forged, &input, UINT16_C(0x8000), &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0020);
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                               &forged, &input, UINT16_C(0x8000), &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int main(void)
{
    check_all_forms_and_completion();
    check_transactional_checked_rejection();
    check_trusted_operand_rejection();
    check_completion_rejection();
    return 0;
}
