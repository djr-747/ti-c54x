/*
 * Module contract
 *
 * Owns: executable proof that neutral standard-indirect store handlers decode
 * MOD0--MOD3 through their checked and trusted entry points, preserve the
 * existing operand indices, and stage exact WORD16 write effects.
 * Does not own: catalog generation, operand-layout implementation, address planning,
 * buses, CPU or architectural state, sequencing, timing, observers,
 * diagnostics, or production dispatch implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/operand_layout.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/store/op_st_immediate_smem.h"
#include "../../../../../src/isa/ops/store/op_st_t_smem.h"
#include "../../../../../src/isa/ops/store/op_stl_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*checked_store_handler)(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

typedef ti_c54x_op_status (*trusted_store_handler)(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

typedef struct neutral_store_case {
    ti_c54x_opcode_id id;
    uint16_t first_word_value;
    uint16_t first_word_mask;
    uint8_t program_word_count;
    ti_c54x_operand_layout_id operand_layout;
    ti_c54x_repeatability_class repeatability;
    ti_c54x_input_resource_mask input_resources;
    bool has_source;
    bool has_extension;
    checked_store_handler checked;
    trusted_store_handler trusted;
} neutral_store_case;

static const neutral_store_case cases[] = {
    {TI_C54X_OPCODE_ID_STL_SMEM, UINT16_C(0x8080), UINT16_C(0xFEE0),
     UINT8_C(1), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD,
     TI_C54X_REPEATABILITY_UNRESOLVED, TI_C54X_INPUT_RESOURCES_ACCUMULATORS,
     true, false, ti_c54x_op_stl_smem, ti_c54x_trusted_op_stl_smem},
    {TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM, UINT16_C(0x7680), UINT16_C(0xFFE0),
     UINT8_C(2), TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD,
     TI_C54X_REPEATABILITY_UNRESOLVED, TI_C54X_INPUT_RESOURCES_NONE, false,
     true, ti_c54x_op_st_immediate_smem,
     ti_c54x_trusted_op_st_immediate_smem},
    {TI_C54X_OPCODE_ID_ST_T_SMEM, UINT16_C(0x8C80), UINT16_C(0xFFE0),
     UINT8_C(1), TI_C54X_OPERAND_LAYOUT_ARF_MOD,
     TI_C54X_REPEATABILITY_UNRESOLVED,
     TI_C54X_INPUT_RESOURCES_T_REGISTER, false, false, ti_c54x_op_st_t_smem,
     ti_c54x_trusted_op_st_t_smem},
};

static ti_c54x_decoded_instruction
decode_case(const neutral_store_case *test_case, uint8_t source, uint8_t arf,
            uint8_t mode, uint16_t extension)
{
    const uint16_t first_word = (uint16_t)(
        test_case->first_word_value | (uint16_t)(mode << 3U) | arf |
        (test_case->has_source ? (uint16_t)(source << 8U) : UINT16_C(0)));
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, extension},
        .program_word_count = test_case->program_word_count};
    ti_c54x_decoded_instruction instruction = {0};

    TI_C54X_TEST_CHECK((uint16_t)(first_word & test_case->first_word_mask) ==
                       test_case->first_word_value);
    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == test_case->id);
    TI_C54X_TEST_CHECK(first.program_word_count ==
                       test_case->program_word_count);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == test_case->id);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       test_case->operand_layout);
    TI_C54X_TEST_CHECK(instruction.repeatability == test_case->repeatability);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       test_case->input_resources);
    return instruction;
}

static ti_c54x_trusted_operand_values
trusted_operands(const neutral_store_case *test_case,
                 const ti_c54x_decoded_instruction *instruction)
{
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_operand_layout_set_view view;
    const ti_c54x_program_words_view words = {
        instruction->bundle.program_words,
        instruction->bundle.program_word_count};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           test_case->operand_layout,
                           instruction->bundle.program_word_count, &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    operands.count = view.descriptor->operand_count;
    TI_C54X_TEST_CHECK(operands.count <= TI_C54X_TRUSTED_OPERAND_CAPACITY);
    for (uint8_t index = 0U; index < operands.count; ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, index, &operands.values[index]) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    }

    /* Existing indices remain source/ARF, ARF/extension, and ARF. */
    if (test_case->has_source) {
        TI_C54X_TEST_CHECK(operands.values[0].raw_value <= UINT64_C(1));
        TI_C54X_TEST_CHECK(operands.values[1].raw_value <= UINT64_C(7));
        TI_C54X_TEST_CHECK(operands.values[2].raw_value <= UINT64_C(3));
    } else if (test_case->has_extension) {
        TI_C54X_TEST_CHECK(operands.values[0].raw_value <= UINT64_C(7));
        TI_C54X_TEST_CHECK(operands.values[1].width_bits == UINT8_C(16));
        TI_C54X_TEST_CHECK(operands.values[2].raw_value <= UINT64_C(3));
    } else {
        TI_C54X_TEST_CHECK(operands.values[0].raw_value <= UINT64_C(7));
        TI_C54X_TEST_CHECK(operands.values[1].raw_value <= UINT64_C(3));
    }
    return operands;
}

static ti_c54x_op_input_snapshot
input_for(const neutral_store_case *test_case)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x0000001234);
    input.accumulator_b_raw40 = UINT64_C(0x000000BEEF);
    input.accumulator_a_known = true;
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0xCAFE);
    input.t_register_known = true;
    input.status_sxm = UINT8_C(0);
    input.status_sxm_known = true;
    (void)test_case;
    return input;
}

static void
assert_memory_write(const ti_c54x_op_effect *effect, uint8_t mode, uint8_t arf,
                    uint16_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode == mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == value);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void
check_one_form(const neutral_store_case *test_case, uint8_t source, uint8_t arf,
               uint8_t mode)
{
    const uint16_t extension = UINT16_C(0xA55A);
    const ti_c54x_decoded_instruction instruction =
        decode_case(test_case, source, arf, mode, extension);
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(test_case, &instruction);
    const ti_c54x_op_input_snapshot input = input_for(test_case);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(test_case->id);
    ti_c54x_trusted_execution_packet packet;
    ti_c54x_op_effect direct = {0};
    ti_c54x_op_effect checked_dispatch = {0};
    ti_c54x_op_effect trusted_dispatch = {0};
    ti_c54x_op_effect trusted_leaf = {0};
    const uint16_t expected = test_case->has_source
                                  ? (source == UINT8_C(0) ? UINT16_C(0x1234)
                                                         : UINT16_C(0xBEEF))
                                  : (test_case->has_extension
                                         ? extension
                                         : UINT16_C(0xCAFE));

    TI_C54X_TEST_CHECK(test_case->checked(&instruction, &input, &direct) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &checked_dispatch) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler == test_case->trusted);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                           &packet, &input, &trusted_dispatch) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(test_case->trusted(&packet.instruction, &operands, &input,
                                          &trusted_leaf) ==
                       TI_C54X_OP_STATUS_OK);

    assert_memory_write(&direct, mode, arf, expected);
    assert_memory_write(&checked_dispatch, mode, arf, expected);
    assert_memory_write(&trusted_dispatch, mode, arf, expected);
    assert_memory_write(&trusted_leaf, mode, arf, expected);
    TI_C54X_TEST_CHECK(memcmp(&direct, &checked_dispatch, sizeof(direct)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_dispatch, sizeof(direct)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_leaf, sizeof(direct)) == 0);
}

static void
check_all_modes_and_operand_indices(const neutral_store_case *test_case)
{
    for (uint8_t mode = UINT8_C(0); mode <= UINT8_C(3); ++mode) {
        for (uint8_t arf = UINT8_C(0); arf < UINT8_C(8); ++arf) {
            if (test_case->has_source) {
                check_one_form(test_case, UINT8_C(0), arf, mode);
                check_one_form(test_case, UINT8_C(1), arf, mode);
            } else {
                check_one_form(test_case, UINT8_C(0), arf, mode);
            }
        }
    }
}

static void
check_checked_rejection_is_transactional(const neutral_store_case *test_case)
{
    const ti_c54x_decoded_instruction instruction =
        decode_case(test_case, UINT8_C(0), UINT8_C(3), UINT8_C(0),
                    UINT16_C(0xA55A));
    const ti_c54x_op_input_snapshot input = input_for(test_case);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged = instruction;

    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    TI_C54X_TEST_CHECK(test_case->checked(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    effect = before;
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        check_all_modes_and_operand_indices(&cases[index]);
        check_checked_rejection_is_transactional(&cases[index]);
    }
    return 0;
}
