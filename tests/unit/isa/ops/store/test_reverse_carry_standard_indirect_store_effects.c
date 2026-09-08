/*
 * Module contract
 *
 * Owns: RED and executable proofs for the fixed reverse-carry MOD4/MOD7
 * WORD16 store effects of STL, ST #K, and ST T. It checks exact catalog/decode
 * contracts, ARF selection, source payloads, trusted/checked parity, and
 * transactional ARF0/input/decode rejection. Does not own: catalog
 * generation, address resolution, endpoint mutation, CPU state, sequencing,
 * timing, devices, observers, diagnostics, or production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/store/op_st_immediate_smem.h"
#include "../../../../../src/isa/ops/store/op_st_t_smem.h"
#include "../../../../../src/isa/ops/store/op_stl_smem.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum store_family {
    STORE_FAMILY_STL = 0,
    STORE_FAMILY_IMMEDIATE = 1,
    STORE_FAMILY_T = 2
} store_family;

typedef struct reverse_store_form {
    store_family family;
    ti_c54x_opcode_id id;
    uint16_t first_word_value;
    uint16_t first_word_mask;
    ti_c54x_operand_layout_id layout;
    ti_c54x_input_resource_mask resources;
    uint8_t program_word_count;
    ti_c54x_indirect_reverse_carry_mode mode;
} reverse_store_form;

static const reverse_store_form forms[] = {
    {STORE_FAMILY_STL, TI_C54X_OPCODE_ID_STL_SMEM_MOD4, UINT16_C(0x80A0),
     UINT16_C(0xFEF8), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_ACCUMULATORS, UINT8_C(1),
     TI_C54X_INDIRECT_REVERSE_CARRY_MOD4},
    {STORE_FAMILY_STL, TI_C54X_OPCODE_ID_STL_SMEM_MOD7, UINT16_C(0x80B8),
     UINT16_C(0xFEF8), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_ACCUMULATORS, UINT8_C(1),
     TI_C54X_INDIRECT_REVERSE_CARRY_MOD7},
    {STORE_FAMILY_IMMEDIATE,
     TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD4, UINT16_C(0x76A0),
     UINT16_C(0xFFF8), TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, UINT8_C(2),
     TI_C54X_INDIRECT_REVERSE_CARRY_MOD4},
    {STORE_FAMILY_IMMEDIATE,
     TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD7, UINT16_C(0x76B8),
     UINT16_C(0xFFF8), TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, UINT8_C(2),
     TI_C54X_INDIRECT_REVERSE_CARRY_MOD7},
    {STORE_FAMILY_T, TI_C54X_OPCODE_ID_ST_T_SMEM_MOD4, UINT16_C(0x8CA0),
     UINT16_C(0xFFF8), TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_T_REGISTER, UINT8_C(1),
     TI_C54X_INDIRECT_REVERSE_CARRY_MOD4},
    {STORE_FAMILY_T, TI_C54X_OPCODE_ID_ST_T_SMEM_MOD7, UINT16_C(0x8CB8),
     UINT16_C(0xFFF8), TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_T_REGISTER, UINT8_C(1),
     TI_C54X_INDIRECT_REVERSE_CARRY_MOD7},
};

static uint16_t reverse_store_word(const reverse_store_form *form,
                                   uint8_t source, uint8_t arf)
{
    uint16_t word = form->first_word_value;

    if (form->family == STORE_FAMILY_STL) {
        word = (uint16_t)(word | ((uint16_t)(source & UINT8_C(1)) << 8U));
    }
    return (uint16_t)(word | (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_decoded_instruction decode_form(const reverse_store_form *form,
                                               uint8_t source, uint8_t arf)
{
    const uint16_t word = reverse_store_word(form, source, arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0xBEEF)},
        .program_word_count = form->program_word_count};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == form->id);
    TI_C54X_TEST_CHECK(first.program_word_count == form->program_word_count);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == form->id);
    TI_C54X_TEST_CHECK(instruction.operand_layout == form->layout);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources == form->resources);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count ==
                       form->program_word_count);
    return instruction;
}

static ti_c54x_op_input_snapshot input_for(uint8_t source, uint8_t arf)
{
    ti_c54x_op_input_snapshot input = {0};

    (void)arf;
    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_b_raw40 = UINT64_C(0xFEDCBA9876);
    input.accumulator_a_known = source == UINT8_C(0);
    input.accumulator_b_known = source == UINT8_C(1);
    input.t_register_raw16 = UINT16_C(0xA55A);
    input.t_register_known = true;
    return input;
}

static ti_c54x_op_status checked_store(
    const reverse_store_form *form,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    switch (form->family) {
    case STORE_FAMILY_STL:
        return form->mode == TI_C54X_INDIRECT_REVERSE_CARRY_MOD4
                   ? ti_c54x_op_stl_smem_mod4(instruction, input, effect)
                   : ti_c54x_op_stl_smem_mod7(instruction, input, effect);
    case STORE_FAMILY_IMMEDIATE:
        return form->mode == TI_C54X_INDIRECT_REVERSE_CARRY_MOD4
                   ? ti_c54x_op_st_immediate_smem_mod4(instruction, input,
                                                        effect)
                   : ti_c54x_op_st_immediate_smem_mod7(instruction, input,
                                                        effect);
    case STORE_FAMILY_T:
        return form->mode == TI_C54X_INDIRECT_REVERSE_CARRY_MOD4
                   ? ti_c54x_op_st_t_smem_mod4(instruction, input, effect)
                   : ti_c54x_op_st_t_smem_mod7(instruction, input, effect);
    default:
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
}

static ti_c54x_trusted_operand_values trusted_operands(
    const reverse_store_form *form, uint8_t source, uint8_t arf)
{
    ti_c54x_trusted_operand_values operands = {0};

    if (form->family == STORE_FAMILY_STL) {
        operands.count = UINT8_C(3);
        operands.values[0].raw_value = source;
        operands.values[0].width_bits = UINT8_C(1);
        operands.values[1].raw_value = arf;
        operands.values[1].width_bits = UINT8_C(3);
        operands.values[2].raw_value = form->mode;
        operands.values[2].width_bits = UINT8_C(4);
    } else if (form->family == STORE_FAMILY_IMMEDIATE) {
        operands.count = UINT8_C(3);
        operands.values[0].raw_value = arf;
        operands.values[0].width_bits = UINT8_C(3);
        operands.values[1].raw_value = UINT16_C(0xBEEF);
        operands.values[1].width_bits = UINT8_C(16);
        operands.values[2].raw_value = form->mode;
        operands.values[2].width_bits = UINT8_C(4);
    } else {
        operands.count = UINT8_C(2);
        operands.values[0].raw_value = arf;
        operands.values[0].width_bits = UINT8_C(3);
        operands.values[1].raw_value = form->mode;
        operands.values[1].width_bits = UINT8_C(4);
    }
    return operands;
}

static ti_c54x_op_status trusted_store(
    const reverse_store_form *form,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    switch (form->family) {
    case STORE_FAMILY_STL:
        return form->mode == TI_C54X_INDIRECT_REVERSE_CARRY_MOD4
                   ? ti_c54x_trusted_op_stl_smem_mod4(
                         instruction, operands, input, effect)
                   : ti_c54x_trusted_op_stl_smem_mod7(
                         instruction, operands, input, effect);
    case STORE_FAMILY_IMMEDIATE:
        return form->mode == TI_C54X_INDIRECT_REVERSE_CARRY_MOD4
                   ? ti_c54x_trusted_op_st_immediate_smem_mod4(
                         instruction, operands, input, effect)
                   : ti_c54x_trusted_op_st_immediate_smem_mod7(
                         instruction, operands, input, effect);
    case STORE_FAMILY_T:
        return form->mode == TI_C54X_INDIRECT_REVERSE_CARRY_MOD4
                   ? ti_c54x_trusted_op_st_t_smem_mod4(
                         instruction, operands, input, effect)
                   : ti_c54x_trusted_op_st_t_smem_mod7(
                         instruction, operands, input, effect);
    default:
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
}

static void assert_write_effect(const reverse_store_form *form,
                                const ti_c54x_op_effect *effect, uint8_t arf,
                                uint16_t expected_value)
{
    const ti_c54x_op_memory_address_spec *address =
        &effect->payload.memory.address_spec;

    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(address->kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16);
    TI_C54X_TEST_CHECK(address->mode == TI_C54X_INDIRECT_SINGLE_MOD0);
    TI_C54X_TEST_CHECK(address->indexed_mode == 0);
    TI_C54X_TEST_CHECK(address->circular_mode == 0);
    TI_C54X_TEST_CHECK(address->reverse_carry_mode == form->mode);
    TI_C54X_TEST_CHECK(address->arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.write_value_word ==
                       expected_value);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
                           &effect->payload.memory));
}

static void check_valid_effects_and_parity(void)
{
    for (size_t form_index = 0U;
         form_index < sizeof(forms) / sizeof(forms[0]); ++form_index) {
        const reverse_store_form *form = &forms[form_index];
        const uint8_t source_count = form->family == STORE_FAMILY_STL ? 2U : 1U;

        for (uint8_t source = 0U; source < source_count; ++source) {
            for (uint8_t arf = UINT8_C(1); arf <= UINT8_C(7);
                 arf = arf == UINT8_C(1) ? UINT8_C(7) : UINT8_C(8)) {
                const ti_c54x_decoded_instruction instruction =
                    decode_form(form, source, arf);
                const ti_c54x_op_input_snapshot input = input_for(source, arf);
                const ti_c54x_trusted_opcode_binding *binding =
                    ti_c54x_trusted_opcode_binding_for_id(form->id);
                ti_c54x_trusted_execution_packet packet;
                const ti_c54x_trusted_operand_values operands =
                    trusted_operands(form, source, arf);
                ti_c54x_op_effect direct = {
                    .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                    .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
                ti_c54x_op_effect checked_dispatch = direct;
                ti_c54x_op_effect trusted_direct = direct;
                ti_c54x_op_effect trusted_dispatch = direct;
                const uint16_t expected =
                    form->family == STORE_FAMILY_STL
                        ? (uint16_t)(source == UINT8_C(0)
                                         ? UINT16_C(0x789A)
                                         : UINT16_C(0x9876))
                        : form->family == STORE_FAMILY_IMMEDIATE
                              ? UINT16_C(0xBEEF)
                              : UINT16_C(0xA55A);

                TI_C54X_TEST_CHECK(binding != NULL);
                TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                       binding, &instruction.bundle, &packet) ==
                                   TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
                TI_C54X_TEST_CHECK(checked_store(form, &instruction, &input,
                                                 &direct) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(trusted_store(form, &instruction, &operands,
                                                 &input, &trusted_direct) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(
                    ti_c54x_dispatch_decoded_with_input_snapshot(
                        &instruction, &input, &checked_dispatch) ==
                    TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                       &packet, &input, &trusted_dispatch) ==
                                   TI_C54X_OP_STATUS_OK);
                assert_write_effect(form, &direct, arf, expected);
                TI_C54X_TEST_CHECK(memcmp(&direct, &checked_dispatch,
                                          sizeof(direct)) == 0);
                TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_direct,
                                          sizeof(direct)) == 0);
                TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_dispatch,
                                          sizeof(direct)) == 0);
            }
        }
    }
}

static void check_arf0_rejection_before_effect(void)
{
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};

    for (size_t form_index = 0U;
         form_index < sizeof(forms) / sizeof(forms[0]); ++form_index) {
        const reverse_store_form *form = &forms[form_index];
        const ti_c54x_decoded_instruction instruction = decode_form(form, 0U, 0U);
        const ti_c54x_op_input_snapshot input = input_for(0U, 0U);
        ti_c54x_op_effect effect = before;

        TI_C54X_TEST_CHECK(checked_store(form, &instruction, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void check_trusted_rejection_is_transactional(void)
{
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x6A6A)}};

    for (size_t form_index = 0U;
         form_index < sizeof(forms) / sizeof(forms[0]); ++form_index) {
        const reverse_store_form *form = &forms[form_index];
        const ti_c54x_decoded_instruction ar0_instruction =
            decode_form(form, UINT8_C(0), UINT8_C(0));
        const ti_c54x_op_input_snapshot input = input_for(UINT8_C(0), UINT8_C(0));
        ti_c54x_trusted_operand_values operands =
            trusted_operands(form, UINT8_C(0), UINT8_C(0));
        ti_c54x_op_effect effect = before;

        TI_C54X_TEST_CHECK(trusted_store(form, &ar0_instruction, &operands,
                                         &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        operands = trusted_operands(form, UINT8_C(0), UINT8_C(1));
        if (form->family == STORE_FAMILY_STL) {
            operands.values[1].width_bits = UINT8_C(2);
        } else {
            operands.values[0].width_bits = UINT8_C(2);
        }
        effect = before;
        {
            const ti_c54x_decoded_instruction valid_instruction =
                decode_form(form, UINT8_C(0), UINT8_C(1));
            TI_C54X_TEST_CHECK(trusted_store(form, &valid_instruction, &operands,
                                             &input, &effect) ==
                               TI_C54X_OP_STATUS_INVALID_OPERAND);
        }
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void check_source_input_rejection(void)
{
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x1234)}};

    for (size_t form_index = 0U;
         form_index < sizeof(forms) / sizeof(forms[0]); ++form_index) {
        const reverse_store_form *form = &forms[form_index];
        const ti_c54x_decoded_instruction instruction = decode_form(form, 0U, 1U);
        ti_c54x_op_input_snapshot input = input_for(0U, 1U);
        ti_c54x_op_effect effect = before;

        if (form->family == STORE_FAMILY_STL) {
            input.accumulator_a_known = false;
            TI_C54X_TEST_CHECK(checked_store(form, &instruction, &input,
                                             &effect) ==
                               TI_C54X_OP_STATUS_UNKNOWN_INPUT);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
            input = input_for(0U, 1U);
            input.accumulator_a_raw40 = UINT64_C(0x10000000000);
            TI_C54X_TEST_CHECK(checked_store(form, &instruction, &input,
                                             &effect) ==
                               TI_C54X_OP_STATUS_INVALID_OPERAND);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        } else if (form->family == STORE_FAMILY_T) {
            input.t_register_known = false;
            TI_C54X_TEST_CHECK(checked_store(form, &instruction, &input,
                                             &effect) ==
                               TI_C54X_OP_STATUS_UNKNOWN_INPUT);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        }
    }
}

static void check_decode_rejection_is_transactional(void)
{
    const reverse_store_form *form = &forms[0];
    const ti_c54x_decoded_instruction instruction = decode_form(form, 0U, 1U);
    const ti_c54x_op_input_snapshot input = input_for(0U, 1U);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged = instruction;

    forged.id = TI_C54X_OPCODE_ID_NOP;
    TI_C54X_TEST_CHECK(checked_store(form, &forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    forged = instruction;
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    TI_C54X_TEST_CHECK(checked_store(form, &forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    forged = instruction;
    forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
    TI_C54X_TEST_CHECK(checked_store(form, &forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    forged = instruction;
    forged.bundle.program_words[0] ^= UINT16_C(0x0008);
    TI_C54X_TEST_CHECK(checked_store(form, &forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_valid_effects_and_parity();
    check_arf0_rejection_before_effect();
    check_trusted_rejection_is_transactional();
    check_source_input_rejection();
    check_decode_rejection_is_transactional();
    return 0;
}
