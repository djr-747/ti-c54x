/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for one-word FRET/FRETD effects,
 * including STACK_POP_FAR carriage, READ-only TOS shape with a
 * zero staged value and null completion, SP-knownness gating
 * with transactional unknown-SP rejection, checked/trusted/
 * dispatch parity, and transactional rejection (including
 * crossed FRET/FRETD rows).
 * Does not own: catalog rows, operand-layout descriptors, buses,
 * memory, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or the production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/branch/op_fret.h"
#include "../../../../../src/isa/ops/branch/op_fretd.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef ti_c54x_op_status (*fret_checked_fn)(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
typedef ti_c54x_op_status (*fret_trusted_fn)(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

typedef struct fret_leaf {
    ti_c54x_opcode_id id;
    uint16_t word;
    ti_c54x_op_memory_address_kind kind;
    fret_checked_fn checked;
    fret_trusted_fn trusted;
} fret_leaf;

static const fret_leaf LEAVES[] = {
    {TI_C54X_OPCODE_ID_FRET, UINT16_C(0xF4E4),
     TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_FAR_WORD16, ti_c54x_op_fret,
     ti_c54x_trusted_op_fret},
    {TI_C54X_OPCODE_ID_FRETD, UINT16_C(0xF6E4),
     TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_FAR_DELAYED_WORD16,
     ti_c54x_op_fretd, ti_c54x_trusted_op_fretd},
};

static ti_c54x_decoded_instruction
decode(uint16_t word, ti_c54x_opcode_id id)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == id);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == id);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint16_t sp_value, bool sp_known, uint8_t seed)
{
    /* STACK_POINTER resources: the leaf must ignore every non-SP input
     * field, so hostile non-SP seeds must produce identical effects
     * while SP knownness gates success. */
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
    input.status_sxm = (uint8_t)((seed >> 3U) & UINT8_C(1));
    input.status_sxm_known = (seed & UINT8_C(32)) != 0U;
    input.stack_pointer_raw16 = sp_value;
    input.stack_pointer_known = sp_known;
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(void)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(0);
    return operands;
}

static void
assert_far_pop(const ti_c54x_op_effect *effect,
               ti_c54x_op_memory_address_kind kind)
{
    const ti_c54x_op_memory_effect *memory;

    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    memory = &effect->payload.memory;
    TI_C54X_TEST_CHECK(memory->address_spec.kind == kind);
    TI_C54X_TEST_CHECK(memory->address_spec.mode == 0);
    TI_C54X_TEST_CHECK(memory->address_spec.indexed_mode == 0);
    TI_C54X_TEST_CHECK(memory->address_spec.circular_mode == 0);
    TI_C54X_TEST_CHECK(memory->address_spec.reverse_carry_mode == 0);
    TI_C54X_TEST_CHECK(memory->address_spec.arf == UINT8_C(0));
    TI_C54X_TEST_CHECK(memory->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(memory->value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(memory->rmw_operation ==
                       (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE);
    TI_C54X_TEST_CHECK(memory->transfer_reserved == UINT8_C(0));
    TI_C54X_TEST_CHECK(memory->complete == NULL);
}

static void
check_metadata(void)
{
    size_t index;

    for (index = 0U;
         index < sizeof(LEAVES) / sizeof(LEAVES[0]); ++index) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(LEAVES[index].id);

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == LEAVES[index].id);
        TI_C54X_TEST_CHECK(metadata->first_word_value ==
                           LEAVES[index].word);
        TI_C54X_TEST_CHECK(metadata->first_word_mask ==
                           UINT16_C(0xFFFF));
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
        TI_C54X_TEST_CHECK(metadata->operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_NONE);
        TI_C54X_TEST_CHECK(metadata->repeatability ==
                           TI_C54X_REPEATABILITY_INELIGIBLE);
        TI_C54X_TEST_CHECK(metadata->input_resources ==
                           TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    }
}

static void
check_effects_and_parity(void)
{
    size_t index;

    for (index = 0U;
         index < sizeof(LEAVES) / sizeof(LEAVES[0]); ++index) {
        const ti_c54x_trusted_opcode_binding *binding =
            ti_c54x_trusted_opcode_binding_for_id(LEAVES[index].id);
        ti_c54x_decoded_instruction instruction =
            decode(LEAVES[index].word, LEAVES[index].id);
        ti_c54x_trusted_execution_packet packet;
        const ti_c54x_op_input_snapshot clean =
            input_for(UINT16_C(0x0300), true, UINT8_C(0));
        const ti_c54x_op_input_snapshot hostile =
            input_for(UINT16_C(0x0400), true, UINT8_C(0x1D));
        ti_c54x_trusted_operand_values operands = trusted_operands();
        ti_c54x_op_effect checked = {0};
        ti_c54x_op_effect checked_hostile = {0};
        ti_c54x_op_effect trusted = {0};
        ti_c54x_op_effect via_dispatch = {0};
        ti_c54x_op_effect via_trusted = {0};

        (void)instruction;
        TI_C54X_TEST_CHECK(binding != NULL);
        TI_C54X_TEST_CHECK(binding->handler != NULL);
        TI_C54X_TEST_CHECK(binding->handler == LEAVES[index].trusted);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                               binding, &instruction.bundle, &packet) ==
                           TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
        TI_C54X_TEST_CHECK(packet.operands.count == UINT8_C(0));
        TI_C54X_TEST_CHECK(LEAVES[index].checked(
                               &instruction, &clean, &checked) ==
                           TI_C54X_OP_STATUS_OK);
        assert_far_pop(&checked, LEAVES[index].kind);
        TI_C54X_TEST_CHECK(LEAVES[index].checked(
                               &instruction, &hostile,
                               &checked_hostile) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&checked_hostile, &checked,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(LEAVES[index].trusted(
                               &instruction, &operands, &clean,
                               &trusted) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&trusted, &checked,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                               &instruction, &clean,
                               &via_dispatch) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&via_dispatch, &checked,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                               &packet, &clean,
                               &via_trusted) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&via_trusted, &checked,
                                  sizeof(checked)) == 0);
    }
}

static void
check_argument_rejection(void)
{
    size_t index;

    for (index = 0U;
         index < sizeof(LEAVES) / sizeof(LEAVES[0]); ++index) {
        const ti_c54x_decoded_instruction instruction =
            decode(LEAVES[index].word, LEAVES[index].id);
        const ti_c54x_op_input_snapshot valid =
            input_for(UINT16_C(0x0300), true, 0U);
        ti_c54x_trusted_operand_values operands = trusted_operands();
        ti_c54x_op_effect effect;

        TI_C54X_TEST_CHECK(LEAVES[index].checked(NULL, &valid,
                                                &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(LEAVES[index].checked(&instruction, NULL,
                                                &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(LEAVES[index].checked(&instruction, &valid,
                                                NULL) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(LEAVES[index].trusted(
                               &instruction, NULL, &valid,
                               &effect) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(LEAVES[index].trusted(
                               &instruction, &operands, NULL,
                               &effect) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        TI_C54X_TEST_CHECK(LEAVES[index].trusted(
                               &instruction, &operands, &valid,
                               NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    }
}

static void
check_unknown_sp_rejection(void)
{
    size_t index;

    for (index = 0U;
         index < sizeof(LEAVES) / sizeof(LEAVES[0]); ++index) {
        const ti_c54x_decoded_instruction instruction =
            decode(LEAVES[index].word, LEAVES[index].id);
        const ti_c54x_op_input_snapshot unknown =
            input_for(UINT16_C(0x0300), false, UINT8_C(0xA5));
        ti_c54x_trusted_operand_values operands = trusted_operands();
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
        ti_c54x_op_effect effect = before;

        TI_C54X_TEST_CHECK(LEAVES[index].checked(&instruction, &unknown,
                                                &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        TI_C54X_TEST_CHECK(LEAVES[index].trusted(
                               &instruction, &operands, &unknown,
                               &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void
check_crossed_and_forged_rejection(void)
{
    /* FRET and FRETD differ only in the Z delay bit: each leaf
     * must reject its sibling row. */
    size_t index;

    for (index = 0U;
         index < sizeof(LEAVES) / sizeof(LEAVES[0]); ++index) {
        const size_t sibling =
            (index + 1U) % (sizeof(LEAVES) / sizeof(LEAVES[0]));
        const ti_c54x_decoded_instruction own =
            decode(LEAVES[index].word, LEAVES[index].id);
        const ti_c54x_decoded_instruction crossed =
            decode(LEAVES[sibling].word, LEAVES[sibling].id);
        const ti_c54x_op_input_snapshot valid =
            input_for(UINT16_C(0x0300), true, UINT8_C(0xA5));
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
        ti_c54x_op_effect effect = before;

        TI_C54X_TEST_CHECK(LEAVES[index].checked(&crossed, &valid,
                                                &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        {
            ti_c54x_decoded_instruction forged = own;

            forged.id = TI_C54X_OPCODE_ID_NOP;
            TI_C54X_TEST_CHECK(LEAVES[index].checked(&forged, &valid,
                                                    &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before,
                                      sizeof(effect)) == 0);

            forged = own;
            forged.bundle.program_words[0] ^=
                UINT16_C(0x0001);
            TI_C54X_TEST_CHECK(LEAVES[index].checked(&forged, &valid,
                                                    &effect) ==
                               TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before,
                                      sizeof(effect)) == 0);
        }
    }
}

static void
check_trusted_operand_rejection(void)
{
    size_t index;

    for (index = 0U;
         index < sizeof(LEAVES) / sizeof(LEAVES[0]); ++index) {
        const ti_c54x_decoded_instruction instruction =
            decode(LEAVES[index].word, LEAVES[index].id);
        const ti_c54x_op_input_snapshot input =
            input_for(UINT16_C(0x0300), true, 0U);
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
        ti_c54x_op_effect effect = before;
        ti_c54x_trusted_operand_values operands = trusted_operands();

        /* FRET[D] take no operands: any staged operand rejects. */
        operands.count = UINT8_C(1);
        operands.values[0].raw_value = UINT64_C(0x1000);
        operands.values[0].width_bits = UINT8_C(16);
        TI_C54X_TEST_CHECK(LEAVES[index].trusted(
                               &instruction, &operands, &input,
                               &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int main(void)
{
    check_metadata();
    check_effects_and_parity();
    check_argument_rejection();
    check_unknown_sp_rejection();
    check_crossed_and_forged_rejection();
    check_trusted_operand_rejection();
    puts("fret: ok");
    return 0;
}
