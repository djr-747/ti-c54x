/*
 * Module contract
 *
 * Owns: executable proof for all three indexed standard-indirect WORD16 store
 * families. Every MOD5/MOD6 checked and trusted leaf must stage an
 * INDEXED_AR0_WORD16 write with the exact source payload and no completion
 * callback. Does not own: catalog generation, decode/layout implementation,
 * address planning, buses, CPU state, sequencing, timing, observers, or
 * diagnostics.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/operand_layout.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef ti_c54x_op_status (*checked_handler)(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);

/* Keep the wrapper declarations local so this aggregate exercises the exact
 * indexed family surface without coupling to unrelated family headers. */
extern ti_c54x_op_status ti_c54x_op_stl_smem_mod5(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_stl_smem_mod6(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_st_immediate_smem_mod5(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_st_immediate_smem_mod6(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_st_t_smem_mod5(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);
extern ti_c54x_op_status ti_c54x_op_st_t_smem_mod6(
    const ti_c54x_decoded_instruction *, const ti_c54x_op_input_snapshot *,
    ti_c54x_op_effect *);

typedef enum store_payload {
    STORE_LOW_ACCUMULATOR = 0,
    STORE_IMMEDIATE = 1,
    STORE_T_REGISTER = 2
} store_payload;

typedef struct store_case {
    ti_c54x_opcode_id id5;
    ti_c54x_opcode_id id6;
    uint16_t base5;
    uint16_t base6;
    uint8_t words;
    ti_c54x_operand_layout_id layout;
    ti_c54x_input_resource_mask resources;
    store_payload payload;
    checked_handler checked5;
    checked_handler checked6;
    ti_c54x_trusted_op_handler trusted5;
    ti_c54x_trusted_op_handler trusted6;
} store_case;

static const store_case cases[] = {
    {TI_C54X_OPCODE_ID_STL_SMEM_MOD5, TI_C54X_OPCODE_ID_STL_SMEM_MOD6,
     UINT16_C(0x80A8), UINT16_C(0x80B0), 1U,
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_ACCUMULATORS, STORE_LOW_ACCUMULATOR,
     ti_c54x_op_stl_smem_mod5, ti_c54x_op_stl_smem_mod6,
     ti_c54x_trusted_op_stl_smem_mod5, ti_c54x_trusted_op_stl_smem_mod6},
    {TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD5,
     TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD6, UINT16_C(0x76A8),
     UINT16_C(0x76B0), 2U, TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, STORE_IMMEDIATE,
     ti_c54x_op_st_immediate_smem_mod5,
     ti_c54x_op_st_immediate_smem_mod6,
     ti_c54x_trusted_op_st_immediate_smem_mod5,
     ti_c54x_trusted_op_st_immediate_smem_mod6},
    {TI_C54X_OPCODE_ID_ST_T_SMEM_MOD5, TI_C54X_OPCODE_ID_ST_T_SMEM_MOD6,
     UINT16_C(0x8CA8), UINT16_C(0x8CB0), 1U,
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_T_REGISTER,
     STORE_T_REGISTER, ti_c54x_op_st_t_smem_mod5, ti_c54x_op_st_t_smem_mod6,
     ti_c54x_trusted_op_st_t_smem_mod5, ti_c54x_trusted_op_st_t_smem_mod6},
};

static ti_c54x_decoded_instruction
decode_case(const store_case *test_case, uint8_t mode)
{
    const uint8_t source = UINT8_C(1);
    const uint8_t arf = UINT8_C(5);
    const uint16_t base = mode == UINT8_C(5) ? test_case->base5
                                             : test_case->base6;
    const uint16_t word = (uint16_t)(base | arf |
                                     (test_case->payload == STORE_LOW_ACCUMULATOR
                                          ? (uint16_t)(source << 8U)
                                          : UINT16_C(0)));
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0xC53A)},
        .program_word_count = test_case->words};
    ti_c54x_decoded_instruction instruction = {0};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id ==
                       (mode == UINT8_C(5) ? test_case->id5 : test_case->id6));
    TI_C54X_TEST_CHECK(first.program_word_count == test_case->words);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == first.id);
    TI_C54X_TEST_CHECK(instruction.operand_layout == test_case->layout);
    TI_C54X_TEST_CHECK(instruction.input_resources == test_case->resources);
    return instruction;
}

static ti_c54x_trusted_operand_values
trusted_operands(const store_case *test_case,
                 const ti_c54x_decoded_instruction *instruction)
{
    ti_c54x_trusted_operand_values operands = {0};
    ti_c54x_operand_layout_set_view view;
    const ti_c54x_program_words_view words = {
        instruction->bundle.program_words, instruction->bundle.program_word_count};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           test_case->layout, test_case->words, &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    operands.count = view.descriptor->operand_count;
    TI_C54X_TEST_CHECK(operands.count <= TI_C54X_TRUSTED_OPERAND_CAPACITY);
    for (uint8_t index = 0U; index < operands.count; ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &words, index, &operands.values[index]) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    }
    return operands;
}

static ti_c54x_op_input_snapshot
valid_input(void)
{
    ti_c54x_op_input_snapshot input = {0};
    input.accumulator_a_raw40 = UINT64_C(0x0000001234);
    input.accumulator_b_raw40 = UINT64_C(0x000000BEEF);
    input.accumulator_a_known = true;
    input.accumulator_b_known = true;
    input.t_register_raw16 = UINT16_C(0xCAFE);
    input.t_register_known = true;
    return input;
}

static void
assert_indexed_write(const ti_c54x_op_effect *effect, uint8_t mode,
                     uint16_t expected_value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode == 0);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.indexed_mode ==
                       (ti_c54x_indirect_indexed_mode)mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == UINT8_C(5));
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.write_value_word ==
                       expected_value);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void
check_case(const store_case *test_case, uint8_t mode)
{
    const ti_c54x_decoded_instruction instruction = decode_case(test_case, mode);
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(test_case, &instruction);
    const ti_c54x_op_input_snapshot input = valid_input();
    checked_handler checked = mode == UINT8_C(5) ? test_case->checked5
                                                 : test_case->checked6;
    ti_c54x_trusted_op_handler trusted = mode == UINT8_C(5)
                                             ? test_case->trusted5
                                             : test_case->trusted6;
    const uint16_t expected =
        test_case->payload == STORE_LOW_ACCUMULATOR ? UINT16_C(0xBEEF)
        : test_case->payload == STORE_IMMEDIATE ? UINT16_C(0xC53A)
                                                : UINT16_C(0xCAFE);
    ti_c54x_op_effect checked_effect = {0};
    ti_c54x_op_effect trusted_effect = {0};

    TI_C54X_TEST_CHECK(checked(&instruction, &input, &checked_effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(trusted(&instruction, &operands, &input,
                               &trusted_effect) == TI_C54X_OP_STATUS_OK);
    assert_indexed_write(&checked_effect, mode, expected);
    assert_indexed_write(&trusted_effect, mode, expected);
}

int
main(void)
{
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        check_case(&cases[index], UINT8_C(5));
        check_case(&cases[index], UINT8_C(6));
    }
    return 0;
}
