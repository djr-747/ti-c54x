/*
 * Module contract
 *
 * Owns: RED proofs for one-word FCALA far-accumulator-call
 * effects, including accumulator-low/page carriage with S source
 * select, STACK_PUSH_FAR_ACCUMULATOR kind with arf select and no
 * status effect, unknown-selected-accumulator rejection,
 * checked/trusted/dispatch parity, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, buses,
 * memory, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or the production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/branch/op_fcala.h"
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

static const uint16_t FCALA_WORD_S0 = UINT16_C(0xF4E7);
static const uint16_t FCALA_WORD_S1 = UINT16_C(0xF5E7);

static ti_c54x_decoded_instruction
decode(uint16_t word0)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(word0);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word0, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_FCALA);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_FCALA);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STACK_POINTER);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint8_t select, bool sp_known, bool selected_known,
          uint64_t selected_value, uint8_t seed)
{
    /* The unselected accumulator plus all status fields ride hostile
     * seeds; SP and the selected accumulator stay pinned. */
    ti_c54x_op_input_snapshot input = {0};

    if (select == UINT8_C(0)) {
        input.accumulator_a_raw40 = selected_value;
        input.accumulator_a_known = selected_known;
        input.accumulator_b_raw40 = UINT64_C(0x2ABCDEF012);
        input.accumulator_b_known = (seed & UINT8_C(2)) != 0U;
    } else {
        input.accumulator_a_raw40 = UINT64_C(0x123456789A);
        input.accumulator_a_known = (seed & UINT8_C(1)) != 0U;
        input.accumulator_b_raw40 = selected_value;
        input.accumulator_b_known = selected_known;
    }
    input.status_c = (uint8_t)(seed & UINT8_C(1));
    input.status_c_known = (seed & UINT8_C(4)) != 0U;
    input.status_tc = (uint8_t)((seed >> 1U) & UINT8_C(1));
    input.status_tc_known = (seed & UINT8_C(8)) != 0U;
    input.stack_pointer_raw16 = UINT16_C(0x1111);
    input.stack_pointer_known = sp_known;
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t select)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(1);
    operands.values[0].raw_value = select;
    operands.values[0].width_bits = UINT8_C(1);
    return operands;
}

static void
assert_far_accumulator_push(const ti_c54x_op_effect *effect,
                            uint8_t select)
{
    const ti_c54x_op_memory_effect *memory;

    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    memory = &effect->payload.memory;
    TI_C54X_TEST_CHECK(
        memory->address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_ACCUMULATOR_WORD16);
    TI_C54X_TEST_CHECK(memory->address_spec.mode == 0);
    TI_C54X_TEST_CHECK(memory->address_spec.indexed_mode == 0);
    TI_C54X_TEST_CHECK(memory->address_spec.circular_mode == 0);
    TI_C54X_TEST_CHECK(memory->address_spec.reverse_carry_mode == 0);
    TI_C54X_TEST_CHECK(memory->address_spec.long_offset_word ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(memory->address_spec.direct_dma7 == UINT8_C(0));
    TI_C54X_TEST_CHECK(memory->address_spec.arf == select);
    TI_C54X_TEST_CHECK(memory->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    /* The pure handler cannot know PC: the root stages the live
     * return address into the frozen copy at prepare and reads
     * the target plus page from the frozen input snapshot. */
    TI_C54X_TEST_CHECK(memory->value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(memory->rmw_operation ==
                       (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE);
    TI_C54X_TEST_CHECK(memory->transfer_reserved == UINT8_C(0));
    TI_C54X_TEST_CHECK(memory->complete == NULL);
}

static void
check_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FCALA);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_FCALA);
    TI_C54X_TEST_CHECK(metadata->first_word_value == FCALA_WORD_S0);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFEFF));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STACK_POINTER);
}

static void
check_ti_vector(void)
{
    /* TI SPRU172C FCALA A with A 00 007F 3000: PC 0025 to 3000,
     * XPC 00 to 7F. The leaf proves the staged kind and select;
     * the CPU test proves the pushed words and target commit. */
    ti_c54x_decoded_instruction instruction = decode(FCALA_WORD_S0);
    const ti_c54x_op_input_snapshot clean =
        input_for(UINT8_C(0), true, true, UINT64_C(0x00007F3000),
                  UINT8_C(0));
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_fcala(&instruction, &clean,
                                        &effect) == TI_C54X_OP_STATUS_OK);
    assert_far_accumulator_push(&effect, UINT8_C(0));
}

static void
check_effects_and_parity(void)
{
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_FCALA);
    static const uint16_t words[2] = {FCALA_WORD_S0, FCALA_WORD_S1};
    static const uint64_t targets[2] = {UINT64_C(0x00007F3000),
                                        UINT64_C(0x0000012000)};

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_fcala);

    for (uint32_t select = 0U; select <= UINT32_C(1); ++select) {
        ti_c54x_decoded_instruction instruction =
            decode(words[select]);
        ti_c54x_trusted_execution_packet packet;
        const ti_c54x_op_input_snapshot clean =
            input_for((uint8_t)select, true, true, targets[select],
                      UINT8_C(0));
        const ti_c54x_op_input_snapshot hostile =
            input_for((uint8_t)select, true, true, targets[select],
                      UINT8_C(0x1D));
        ti_c54x_trusted_operand_values operands =
            trusted_operands((uint8_t)select);
        ti_c54x_op_effect checked = {0};
        ti_c54x_op_effect checked_hostile = {0};
        ti_c54x_op_effect trusted = {0};
        ti_c54x_op_effect via_dispatch = {0};
        ti_c54x_op_effect via_trusted = {0};

        TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                               binding, &instruction.bundle,
                               &packet) ==
                           TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
        TI_C54X_TEST_CHECK(ti_c54x_op_fcala(&instruction, &clean,
                                            &checked) ==
                           TI_C54X_OP_STATUS_OK);
        assert_far_accumulator_push(&checked, (uint8_t)select);
        TI_C54X_TEST_CHECK(ti_c54x_op_fcala(&instruction, &hostile,
                                            &checked_hostile) ==
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&checked_hostile, &checked,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcala(
                               &instruction, &operands, &clean,
                               &trusted) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&trusted, &checked,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                               &instruction, &clean,
                               &via_dispatch) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&via_dispatch, &checked,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(&packet, &clean,
                                                   &via_trusted) ==
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(memcmp(&via_trusted, &checked,
                                  sizeof(checked)) == 0);
    }
}

static void
check_unknown_selected_rejection(void)
{
    /* Unknown selected accumulator rejects UNKNOWN_INPUT; unknown
     * SP rejects INVALID_OPERAND; both leave the output
     * transactional. */
    static const uint16_t words[2] = {FCALA_WORD_S0, FCALA_WORD_S1};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};

    for (uint32_t select = 0U; select <= UINT32_C(1); ++select) {
        ti_c54x_decoded_instruction instruction =
            decode(words[select]);
        ti_c54x_op_input_snapshot unknown_acc =
            input_for((uint8_t)select, true, false,
                      UINT64_C(0x00007F3000), UINT8_C(0));
        ti_c54x_op_input_snapshot unknown_sp =
            input_for((uint8_t)select, false, true,
                      UINT64_C(0x00007F3000), UINT8_C(0));
        ti_c54x_op_effect effect = before;

        TI_C54X_TEST_CHECK(ti_c54x_op_fcala(&instruction, &unknown_acc,
                                            &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_op_fcala(&instruction, &unknown_sp,
                                            &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void
check_argument_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(FCALA_WORD_S0);
    const ti_c54x_op_input_snapshot valid =
        input_for(UINT8_C(0), true, true, UINT64_C(0x00007F3000),
                  0U);
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0));
    ti_c54x_op_effect effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_fcala(NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_fcala(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_fcala(&instruction, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcala(
                           &instruction, NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcala(
                           &instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcala(
                           &instruction, &operands, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void
check_decode_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(FCALA_WORD_S0);
    const ti_c54x_op_input_snapshot valid =
        input_for(UINT8_C(0), true, true, UINT64_C(0x00007F3000),
                  UINT8_C(0xA5));
    const ti_c54x_op_input_snapshot both_known =
        input_for(UINT8_C(0), true, true, UINT64_C(0x00007F3000),
                  UINT8_C(0xA7));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_fcala(&forged, &valid,
                                            &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_fcala(&forged, &valid,
                                            &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_word_count = UINT8_C(2);
        TI_C54X_TEST_CHECK(ti_c54x_op_fcala(&forged, &valid,
                                            &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        /* Bit 0 distinguishes FCALA from FBACC: flipping it
         * leaves the row. */
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0001);
        TI_C54X_TEST_CHECK(ti_c54x_op_fcala(&forged, &valid,
                                            &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        /* S bit 8 selects the source: flipping it rides inside
         * the same row with the sibling accumulator selected,
         * which must be known. */
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0100);
        TI_C54X_TEST_CHECK(ti_c54x_op_fcala(&forged, &both_known,
                                            &effect) ==
                           TI_C54X_OP_STATUS_OK);
        assert_far_accumulator_push(&effect, UINT8_C(1));
    }
}

static void
check_trusted_operand_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(FCALA_WORD_S0);
    const ti_c54x_op_input_snapshot valid =
        input_for(UINT8_C(0), true, true, UINT64_C(0x00007F3000),
                  0U);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0));

    operands.count = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcala(
                           &instruction, &operands, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0));
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcala(
                           &instruction, &operands, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0));
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcala(
                           &instruction, &operands, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int
main(void)
{
    check_metadata();
    check_ti_vector();
    check_effects_and_parity();
    check_unknown_selected_rejection();
    check_argument_rejection();
    check_decode_rejection();
    check_trusted_operand_rejection();
    puts("fcala: ok");
    return 0;
}
