/*
 * Module contract
 *
 * Owns: executable proof that every neutral standard-indirect load handler stages the
 * decoded MOD0--MOD2 address mode and rejects MOD3 transactionally through
 * both checked and trusted entry points. It covers only effect staging and
 * the handler/catalog contract. Does not own: catalog generation, operand
 * layout implementation, address resolution, buses, CPU/state mutation,
 * sequencing, timing, observers, or diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/operand_layout.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/load/op_ld_smem.h"
#include "../../../../../src/isa/ops/load/op_ld_status_smem.h"
#include "../../../../../src/isa/ops/load/op_ld_t_smem.h"
#include "../../../../../src/isa/ops/load/op_ldu_smem.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*checked_load_handler)(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

typedef ti_c54x_op_status (*trusted_load_handler)(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

typedef struct neutral_load_case {
    ti_c54x_opcode_id id;
    uint16_t first_word_value;
    uint16_t first_word_mask;
    ti_c54x_operand_layout_id operand_layout;
    ti_c54x_repeatability_class repeatability;
    ti_c54x_input_resource_mask input_resources;
    bool has_destination;
    checked_load_handler checked;
    trusted_load_handler trusted;
} neutral_load_case;

static const neutral_load_case cases[] = {
    {TI_C54X_OPCODE_ID_LD_SMEM, UINT16_C(0x1080), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD,
     TI_C54X_REPEATABILITY_UNRESOLVED, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
     true, ti_c54x_op_ld_smem, ti_c54x_trusted_op_ld_smem},
    {TI_C54X_OPCODE_ID_LD_SMEM_TS, UINT16_C(0x1480), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD,
     TI_C54X_REPEATABILITY_UNRESOLVED,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, true,
     ti_c54x_op_ld_smem_ts, ti_c54x_trusted_op_ld_smem_ts},
    {TI_C54X_OPCODE_ID_LDU_SMEM, UINT16_C(0x1280), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD,
     TI_C54X_REPEATABILITY_UNRESOLVED, TI_C54X_INPUT_RESOURCES_NONE, true,
     ti_c54x_op_ldu_smem, ti_c54x_trusted_op_ldu_smem},
    {TI_C54X_OPCODE_ID_LD_T_SMEM, UINT16_C(0x3080), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD, TI_C54X_REPEATABILITY_UNRESOLVED,
     TI_C54X_INPUT_RESOURCES_NONE, false, ti_c54x_op_ld_t_smem,
     ti_c54x_trusted_op_ld_t_smem},
    {TI_C54X_OPCODE_ID_LD_DP_SMEM, UINT16_C(0x4680), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD, TI_C54X_REPEATABILITY_INELIGIBLE,
     TI_C54X_INPUT_RESOURCES_NONE, false, ti_c54x_op_ld_dp_smem,
     ti_c54x_trusted_op_ld_dp_smem},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM, UINT16_C(0x3280), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD, TI_C54X_REPEATABILITY_UNRESOLVED,
     TI_C54X_INPUT_RESOURCES_NONE, false, ti_c54x_op_ld_asm_smem,
     ti_c54x_trusted_op_ld_asm_smem},
    {TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16, UINT16_C(0x4480), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD,
     TI_C54X_REPEATABILITY_UNRESOLVED, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
     true, ti_c54x_op_ld_smem_shift16, ti_c54x_trusted_op_ld_smem_shift16},
};

static ti_c54x_op_input_snapshot
valid_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.status_sxm_known = true;
    input.status_sxm = UINT8_C(0);
    input.t_register_known = true;
    input.t_register_raw16 = UINT16_C(0);
    return input;
}

static ti_c54x_decoded_instruction
decode_case(const neutral_load_case *test_case, uint8_t mode)
{
    const uint8_t arf = UINT8_C(5);
    const uint8_t destination = UINT8_C(1);
    const uint16_t first_word = (uint16_t)(
        test_case->first_word_value | (uint16_t)(mode << 3U) | arf |
        (test_case->has_destination ? (uint16_t)(destination << 8U)
                                    : UINT16_C(0)));
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction = {0};

    TI_C54X_TEST_CHECK((uint16_t)(first_word & test_case->first_word_mask) ==
                       test_case->first_word_value);
    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == test_case->id);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == test_case->id);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       test_case->operand_layout);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       test_case->repeatability);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       test_case->input_resources);
    return instruction;
}

static ti_c54x_trusted_operand_values
trusted_operands(const neutral_load_case *test_case,
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
    return operands;
}

static void
assert_memory_mode(const ti_c54x_op_effect *effect, uint8_t mode)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode == mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf ==
                       UINT8_C(5));
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
}

static void
check_mod0_to_mod2(const neutral_load_case *test_case)
{
    const ti_c54x_op_input_snapshot input = valid_input();

    for (uint8_t mode = UINT8_C(0); mode <= UINT8_C(2); ++mode) {
        const ti_c54x_decoded_instruction instruction =
            decode_case(test_case, mode);
        const ti_c54x_trusted_operand_values operands =
            trusted_operands(test_case, &instruction);
        ti_c54x_op_effect checked = {0};
        ti_c54x_op_effect trusted = {0};

        TI_C54X_TEST_CHECK(test_case->checked(&instruction, &input, &checked) ==
                           TI_C54X_OP_STATUS_OK);
        assert_memory_mode(&checked, mode);
        TI_C54X_TEST_CHECK(test_case->trusted(
                               &instruction, &operands, &input, &trusted) ==
                           TI_C54X_OP_STATUS_OK);
        assert_memory_mode(&trusted, mode);
    }
}

static void
check_mod3_is_transactional(const neutral_load_case *test_case)
{
    const ti_c54x_decoded_instruction instruction = decode_case(test_case, 3U);
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(test_case, &instruction);
    const ti_c54x_op_input_snapshot input = valid_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    ti_c54x_op_effect checked = before;
    ti_c54x_op_effect trusted = before;

    TI_C54X_TEST_CHECK(test_case->checked(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&checked, &before, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(test_case->trusted(
                           &instruction, &operands, &input, &trusted) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &before, sizeof(trusted)) == 0);
}

static void
test_all_neutral_loads(void)
{
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        check_mod0_to_mod2(&cases[index]);
        check_mod3_is_transactional(&cases[index]);
    }
}

int
main(void)
{
    test_all_neutral_loads();
    return 0;
}
