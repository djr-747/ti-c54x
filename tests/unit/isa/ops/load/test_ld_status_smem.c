/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for MOD2 WORD16 loads into the DP and
 * ASM status fields, including ARF selection, masked completion, checked/
 * trusted/dispatch parity, and transactional rejection. Does not own:
 * catalog rows, operand-layout descriptors, address resolution, buses,
 * CPU/state mutation, sequencing, timing, observers, diagnostics, or the
 * production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/load/op_ld_status_smem.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct ld_status_case {
    ti_c54x_opcode_id id;
    uint16_t first_word_value;
    uint16_t first_word_mask;
    ti_c54x_repeatability_class repeatability;
    ti_c54x_op_status_field_target target;
    uint16_t completion_mask;
    ti_c54x_op_handler checked_handler;
    ti_c54x_trusted_op_handler trusted_handler;
} ld_status_case;

static const ld_status_case cases[] = {
    {TI_C54X_OPCODE_ID_LD_DP_SMEM, UINT16_C(0x4680), UINT16_C(0xFFE0),
     TI_C54X_REPEATABILITY_INELIGIBLE,
     TI_C54X_OP_STATUS_FIELD_TARGET_DP, UINT16_C(0x01FF),
     ti_c54x_op_ld_dp_smem, ti_c54x_trusted_op_ld_dp_smem},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM, UINT16_C(0x3280), UINT16_C(0xFFE0),
     TI_C54X_REPEATABILITY_UNRESOLVED,
     TI_C54X_OP_STATUS_FIELD_TARGET_ASM, UINT16_C(0x001F),
     ti_c54x_op_ld_asm_smem, ti_c54x_trusted_op_ld_asm_smem},
};

static uint16_t word_for(const ld_status_case *test_case, uint8_t arf)
{
    return (uint16_t)(test_case->first_word_value | UINT16_C(0x0010) |
                      (uint16_t)(arf & 7U));
}

static ti_c54x_decoded_instruction decode_case(const ld_status_case *test_case,
                                               uint8_t arf)
{
    const uint16_t word = word_for(test_case, arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == test_case->id);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == test_case->id);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability == test_case->repeatability);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    return instruction;
}

static ti_c54x_op_input_snapshot input_variant(uint8_t seed, bool known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_b_raw40 = UINT64_C(0x2ABCDEF012);
    input.accumulator_a_known = !known;
    input.accumulator_b_known = known;
    input.status_c = (uint8_t)(seed & 1U);
    input.status_c_known = known;
    input.status_tc = (uint8_t)((seed >> 1U) & 1U);
    input.status_tc_known = !known;
    input.status_sxm = seed;
    input.status_sxm_known = known;
    for (uint8_t index = 0U; index < UINT8_C(8); ++index) {
        input.auxiliary_registers_raw16[index] =
            (uint16_t)(UINT16_C(0xA500) | index);
    }
    input.auxiliary_registers_known_mask = known ? UINT8_C(0xFF) : UINT8_C(0);
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

static void assert_status_write(const ti_c54x_op_effect *effect,
                                const ld_status_case *test_case,
                                uint16_t read_value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.status_field_write.target ==
                       test_case->target);
    TI_C54X_TEST_CHECK(effect->payload.status_field_write.raw_value ==
                       (uint16_t)(read_value & test_case->completion_mask));
}

static void check_metadata_and_exact_forms(const ld_status_case *test_case)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(test_case->id);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == test_case->id);
    TI_C54X_TEST_CHECK(metadata->first_word_value ==
                       test_case->first_word_value);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == test_case->first_word_mask);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout == TI_C54X_OPERAND_LAYOUT_ARF_MOD);
    TI_C54X_TEST_CHECK(metadata->repeatability == test_case->repeatability);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);

    for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
        const ti_c54x_decoded_instruction instruction =
            decode_case(test_case, arf);
        TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                           word_for(test_case, arf));
    }
}

static void check_success_and_parity(const ld_status_case *test_case)
{
    static const uint8_t seeds[] = {UINT8_C(0), UINT8_C(1), UINT8_C(2),
                                    UINT8_C(0xFF)};
    static const uint16_t reads[] = {UINT16_C(0x0000), UINT16_C(0x001F),
                                     UINT16_C(0x01FF), UINT16_C(0x1FFF),
                                     UINT16_C(0xFFFF)};

    for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
        const ti_c54x_decoded_instruction instruction =
            decode_case(test_case, arf);
        const ti_c54x_trusted_opcode_binding *binding =
            ti_c54x_trusted_opcode_binding_for_id(test_case->id);
        const ti_c54x_instruction_word_bundle bundle = instruction.bundle;
        ti_c54x_trusted_execution_packet packet;

        TI_C54X_TEST_CHECK(binding != NULL);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                               binding, &bundle, &packet) ==
                           TI_C54X_TRUSTED_EXECUTION_PACKET_OK);

        for (size_t seed_index = 0U;
             seed_index < sizeof(seeds) / sizeof(seeds[0]); ++seed_index) {
            for (uint8_t known = 0U; known <= UINT8_C(1); ++known) {
                const ti_c54x_op_input_snapshot input =
                    input_variant(seeds[seed_index], known != UINT8_C(0));
                const ti_c54x_trusted_operand_values operands =
                    trusted_operands(arf);
                const ti_c54x_op_effect before = {
                    .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                    .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
                ti_c54x_op_effect direct = before;
                ti_c54x_op_effect checked = before;
                ti_c54x_op_effect trusted_dispatch = before;
                ti_c54x_op_effect trusted_direct = before;

                TI_C54X_TEST_CHECK(test_case->checked_handler(
                                       &instruction, &input, &direct) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(
                    ti_c54x_dispatch_decoded_with_input_snapshot(
                        &instruction, &input, &checked) ==
                    TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                       &packet, &input, &trusted_dispatch) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(test_case->trusted_handler(
                                       &packet.instruction, &operands, &input,
                                       &trusted_direct) ==
                                   TI_C54X_OP_STATUS_OK);
                assert_memory_read(&direct, arf);
                TI_C54X_TEST_CHECK(memcmp(&direct, &checked, sizeof(direct)) ==
                                   0);
                TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_dispatch,
                                          sizeof(direct)) == 0);
                TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_direct,
                                          sizeof(direct)) == 0);

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
                    assert_status_write(&completed, test_case,
                                        reads[read_index]);
                }
            }
        }
    }
}

static void check_transactional_checked_rejection(const ld_status_case *test_case)
{
    const ti_c54x_decoded_instruction instruction = decode_case(test_case, 0U);
    const ti_c54x_op_input_snapshot input = input_variant(UINT8_C(0xA5), false);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(test_case->checked_handler(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(test_case->checked_handler(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(test_case->checked_handler(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;
        const struct {
            ti_c54x_opcode_id id;
            ti_c54x_operand_layout_id layout;
            ti_c54x_repeatability_class repeatability;
            ti_c54x_input_resource_mask resources;
            uint8_t program_word_count;
            uint16_t word_xor;
        } invalid[] = {
            {TI_C54X_OPCODE_ID_NOP, instruction.operand_layout,
             instruction.repeatability, instruction.input_resources, 1U, 0U},
            {instruction.id, TI_C54X_OPERAND_LAYOUT_NONE,
             instruction.repeatability, instruction.input_resources, 1U, 0U},
            {instruction.id, instruction.operand_layout,
             instruction.repeatability == TI_C54X_REPEATABILITY_INELIGIBLE
                 ? TI_C54X_REPEATABILITY_UNRESOLVED
                 : TI_C54X_REPEATABILITY_INELIGIBLE,
             instruction.input_resources, 1U, 0U},
            {instruction.id, instruction.operand_layout, instruction.repeatability,
             TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U, 0U},
            {instruction.id, instruction.operand_layout, instruction.repeatability,
             instruction.input_resources, 2U, 0U},
            {instruction.id, instruction.operand_layout, instruction.repeatability,
             instruction.input_resources, 1U, UINT16_C(0x0020)},
            {instruction.id, instruction.operand_layout, instruction.repeatability,
             instruction.input_resources, 1U, UINT16_C(0x0100)},
        };

        for (size_t index = 0U; index < sizeof(invalid) / sizeof(invalid[0]);
             ++index) {
            forged = instruction;
            forged.id = invalid[index].id;
            forged.operand_layout = invalid[index].layout;
            forged.repeatability = invalid[index].repeatability;
            forged.input_resources = invalid[index].resources;
            forged.bundle.program_word_count = invalid[index].program_word_count;
            forged.bundle.program_words[0] ^= invalid[index].word_xor;
            effect = before;
            TI_C54X_TEST_CHECK(test_case->checked_handler(
                                   &forged, &input, &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        }
    }
}

static void check_trusted_operand_rejection(const ld_status_case *test_case)
{
    const ti_c54x_decoded_instruction instruction = decode_case(test_case, 7U);
    const ti_c54x_op_input_snapshot input = input_variant(UINT8_C(0x5A), true);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(test_case->id);
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands = trusted_operands(7U);

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(test_case->trusted_handler(
                           &packet.instruction, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(test_case->trusted_handler(
                           &packet.instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(test_case->trusted_handler(
                           &packet.instruction, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(0);
    TI_C54X_TEST_CHECK(test_case->trusted_handler(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(7U);
    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(test_case->trusted_handler(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(7U);
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(test_case->trusted_handler(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(7U);
    operands.values[0].raw_value = UINT64_C(8);
    TI_C54X_TEST_CHECK(test_case->trusted_handler(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void check_completion_rejection(const ld_status_case *test_case)
{
    const ti_c54x_decoded_instruction instruction = decode_case(test_case, 7U);
    const ti_c54x_op_input_snapshot input = input_variant(UINT8_C(0xFF), false);
    ti_c54x_op_effect read_effect = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x1234)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(test_case->checked_handler(&instruction, &input,
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
        const uint16_t invalid_words[] = {UINT16_C(0x0020), UINT16_C(0x0100)};
        for (size_t index = 0U;
             index < sizeof(invalid_words) / sizeof(invalid_words[0]); ++index) {
            forged = instruction;
            forged.bundle.program_words[0] ^= invalid_words[index];
            effect = before;
            TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                                   &forged, &input, UINT16_C(0xFFFF), &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        }
    }
}

static void check_distinct_trusted_handlers(void)
{
    const ti_c54x_trusted_opcode_binding *dp =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_LD_DP_SMEM);
    const ti_c54x_trusted_opcode_binding *as =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_LD_ASM_SMEM);

    TI_C54X_TEST_CHECK(dp != NULL);
    TI_C54X_TEST_CHECK(as != NULL);
    TI_C54X_TEST_CHECK(dp->handler != NULL);
    TI_C54X_TEST_CHECK(as->handler != NULL);
    TI_C54X_TEST_CHECK(dp->handler != as->handler);
}

int main(void)
{
    check_distinct_trusted_handlers();
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        check_metadata_and_exact_forms(&cases[index]);
        check_success_and_parity(&cases[index]);
        check_transactional_checked_rejection(&cases[index]);
        check_trusted_operand_rejection(&cases[index]);
        check_completion_rejection(&cases[index]);
    }
    puts("ld_status_smem_ops: ok");
    return 0;
}
