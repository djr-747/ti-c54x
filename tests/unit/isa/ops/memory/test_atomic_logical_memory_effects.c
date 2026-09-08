/*
 * Module contract
 *
 * Owns: executable RED/GREEN proofs for the checked, trusted-leaf, and
 * dispatch paths of the two-word ANDM/ORM/XORM standard-indirect WORD16
 * semantic effects. It covers only operand extraction and effect staging.
 * Does not own: catalog generation, decode tables, address resolution, bus or
 * CPU state, sequencing, timing, observers, diagnostics, or the transaction
 * that consumes the MEMORY carrier.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/operand_layout.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/memory/op_atomic_logical_memory.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*checked_handler)(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
typedef ti_c54x_op_status (*trusted_handler)(
    const ti_c54x_decoded_instruction *, const ti_c54x_trusted_operand_values *,
    const ti_c54x_op_input_snapshot *, ti_c54x_op_effect *);

typedef struct atomic_logical_case {
    ti_c54x_opcode_id id;
    uint16_t first_word_value;
    checked_handler checked;
    trusted_handler trusted;
    uint8_t operation;
} atomic_logical_case;

static const atomic_logical_case cases[] = {
    {TI_C54X_OPCODE_ID_ANDM_SMEM_MOD0_3, UINT16_C(0x6880),
     ti_c54x_op_andm_smem_mod0_3, ti_c54x_trusted_op_andm_smem_mod0_3,
     TI_C54X_OP_MEMORY_RMW_OPERATION_AND},
    {TI_C54X_OPCODE_ID_ORM_SMEM_MOD0_3, UINT16_C(0x6980),
     ti_c54x_op_orm_smem_mod0_3, ti_c54x_trusted_op_orm_smem_mod0_3,
     TI_C54X_OP_MEMORY_RMW_OPERATION_OR},
    {TI_C54X_OPCODE_ID_XORM_SMEM_MOD0_3, UINT16_C(0x6A80),
     ti_c54x_op_xorm_smem_mod0_3, ti_c54x_trusted_op_xorm_smem_mod0_3,
     TI_C54X_OP_MEMORY_RMW_OPERATION_XOR},
};

static ti_c54x_decoded_instruction decode_case(const atomic_logical_case *item,
                                                uint8_t arf, uint8_t mode,
                                                uint16_t extension)
{
    const uint16_t first_word = (uint16_t)(
        item->first_word_value | (uint16_t)(mode << 3U) | arf);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, extension}, .program_word_count = 2U};
    ti_c54x_decoded_instruction instruction = {0};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == item->id);
    TI_C54X_TEST_CHECK(first.program_word_count == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == item->id);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    return instruction;
}

static ti_c54x_trusted_operand_values trusted_operands(
    const ti_c54x_decoded_instruction *instruction)
{
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_operand_layout_set_view view;
    const ti_c54x_program_words_view words = {
        instruction->bundle.program_words,
        instruction->bundle.program_word_count};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction->operand_layout,
                           instruction->bundle.program_word_count, &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    operands.count = view.descriptor->operand_count;
    TI_C54X_TEST_CHECK(operands.count == 3U);
    for (uint8_t index = 0U; index < operands.count; ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, index, &operands.values[index]) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    }
    return operands;
}

static ti_c54x_op_input_snapshot ignored_input(uint16_t salt)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x123456789A) ^ salt;
    input.accumulator_b_raw40 = UINT64_C(0x0FEDCBA987) ^ salt;
    input.accumulator_a_known = false;
    input.accumulator_b_known = true;
    input.status_c = (uint8_t)salt;
    input.status_c_known = false;
    input.status_tc = (uint8_t)(salt >> 8U);
    input.status_tc_known = true;
    input.status_sxm = (uint8_t)(salt & 1U);
    input.status_sxm_known = false;
    return input;
}

static void assert_memory_effect(const ti_c54x_op_effect *effect, uint8_t arf,
                                 uint8_t mode, uint16_t extension,
                                 uint8_t operation)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode == mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == extension);
    TI_C54X_TEST_CHECK(effect->payload.memory.rmw_operation == operation);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void check_metadata(const atomic_logical_case *item)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(item->id);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == item->id);
    TI_C54X_TEST_CHECK(metadata->first_word_value == item->first_word_value);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFFE0));
    TI_C54X_TEST_CHECK(metadata->program_word_count == 2U);
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
}

static void check_valid_forms(const atomic_logical_case *item)
{
    const uint16_t extensions[] = {UINT16_C(0), UINT16_C(0x55AA),
                                   UINT16_C(0xFFFF)};
    const ti_c54x_op_input_snapshot input = ignored_input(UINT16_C(0xA55A));

    for (uint8_t arf = 0U; arf < 8U; ++arf) {
        for (uint8_t mode = 0U; mode <= 2U; ++mode) {
            for (size_t extension_index = 0U;
                 extension_index < sizeof(extensions) / sizeof(extensions[0]);
                 ++extension_index) {
                const uint16_t extension = extensions[extension_index];
                const ti_c54x_decoded_instruction instruction =
                    decode_case(item, arf, mode, extension);
                const ti_c54x_trusted_operand_values operands =
                    trusted_operands(&instruction);
                const ti_c54x_trusted_opcode_binding *binding =
                    ti_c54x_trusted_opcode_binding_for_id(item->id);
                const ti_c54x_instruction_word_bundle bundle =
                    instruction.bundle;
                ti_c54x_trusted_execution_packet packet;
                const ti_c54x_op_effect sentinel = {
                    .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                    .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
                ti_c54x_op_effect checked = sentinel;
                ti_c54x_op_effect trusted = sentinel;
                ti_c54x_op_effect trusted_dispatch = sentinel;
                ti_c54x_op_effect dispatched = sentinel;
                ti_c54x_decoded_instruction mutated_instruction = instruction;
                ti_c54x_op_effect trusted_from_copied_values = sentinel;

                TI_C54X_TEST_CHECK(binding != NULL);
                TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                       binding, &bundle, &packet) ==
                                   TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
                TI_C54X_TEST_CHECK(item->checked(&instruction, &input,
                                                  &checked) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(item->trusted(
                                       &instruction, &operands, &input,
                                       &trusted) == TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                                       &instruction, &input, &dispatched) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                       &packet, &input, &trusted_dispatch) ==
                                   TI_C54X_OP_STATUS_OK);
                assert_memory_effect(&checked, arf, mode, extension,
                                     item->operation);
                assert_memory_effect(&trusted, arf, mode, extension,
                                     item->operation);
                TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                                          sizeof(checked)) == 0);
                TI_C54X_TEST_CHECK(memcmp(&checked, &dispatched,
                                          sizeof(checked)) == 0);
                TI_C54X_TEST_CHECK(memcmp(&checked, &trusted_dispatch,
                                          sizeof(checked)) == 0);
                /* Trusted execution consumes the packet's copied operands;
                 * changing the source bundle after extraction must not force
                 * a second layout decode or alter the staged extension. */
                mutated_instruction.bundle.program_words[1] ^= UINT16_C(0xFFFF);
                TI_C54X_TEST_CHECK(item->trusted(
                                       &mutated_instruction, &operands, &input,
                                       &trusted_from_copied_values) ==
                                   TI_C54X_OP_STATUS_OK);
                assert_memory_effect(&trusted_from_copied_values, arf, mode,
                                     extension, item->operation);
            }
        }
    }
}

static void check_mod3_rejection(const atomic_logical_case *item)
{
    const ti_c54x_decoded_instruction instruction =
        decode_case(item, UINT8_C(3), UINT8_C(3), UINT16_C(0x1234));
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(&instruction);
    const ti_c54x_op_input_snapshot input = ignored_input(UINT16_C(0x5AA5));
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(item->id);
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect sentinel = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect checked = sentinel;
    ti_c54x_op_effect trusted = sentinel;
    ti_c54x_op_effect dispatched = sentinel;
    ti_c54x_op_effect trusted_dispatch = sentinel;

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(item->checked(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(item->trusted(&instruction, &operands, &input, &trusted) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                           &packet, &input, &trusted_dispatch) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&checked, &sentinel, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &sentinel, sizeof(trusted)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&dispatched, &sentinel, sizeof(dispatched)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&trusted_dispatch, &sentinel,
                              sizeof(trusted_dispatch)) == 0);
}

static void check_malformed_and_trusted_mismatch(const atomic_logical_case *item)
{
    const ti_c54x_decoded_instruction instruction =
        decode_case(item, UINT8_C(4), UINT8_C(1), UINT16_C(0xABCD));
    const ti_c54x_op_input_snapshot input = ignored_input(UINT16_C(0x1234));
    ti_c54x_trusted_operand_values operands = trusted_operands(&instruction);
    const ti_c54x_op_effect sentinel = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = sentinel;
    ti_c54x_decoded_instruction malformed = instruction;

    malformed.operand_layout = TI_C54X_OPERAND_LAYOUT_ARF_MOD;
    TI_C54X_TEST_CHECK(item->checked(&malformed, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    effect = sentinel;
    operands.values[1].width_bits = UINT8_C(15);
    TI_C54X_TEST_CHECK(item->trusted(&instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    effect = sentinel;
    operands = trusted_operands(&instruction);
    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(item->trusted(&instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    effect = sentinel;
    operands = trusted_operands(&instruction);
    operands.values[0].raw_value = UINT64_C(8);
    TI_C54X_TEST_CHECK(item->trusted(&instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    effect = sentinel;
    operands = trusted_operands(&instruction);
    operands.values[2].raw_value = UINT64_C(4);
    TI_C54X_TEST_CHECK(item->trusted(&instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
}

static void test_atomic_logical_memory_effects(void)
{
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        check_metadata(&cases[index]);
        check_valid_forms(&cases[index]);
        check_mod3_rejection(&cases[index]);
        check_malformed_and_trusted_mismatch(&cases[index]);
    }
}

int main(void)
{
    test_atomic_logical_memory_effects();
    return EXIT_SUCCESS;
}
