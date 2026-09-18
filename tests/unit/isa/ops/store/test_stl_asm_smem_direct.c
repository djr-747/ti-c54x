/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the row-less STL src,ASM,Smem
 * direct-slice WORD16 store effect (first word 0x8400 under mask
 * 0xFE80, S selects A/B, I pinned 0), including the ASM shift core
 * (TI Example 2 shape plus SXM fill and ASM boundaries), the
 * DIRECT_WORD16 dma/value staging, null completion, no delay flag,
 * checked/trusted/dispatch parity, select/indirect/knownness/value
 * gating, and transactional rejection. Does not own: catalog rows,
 * operand-layout descriptors, accumulator/status state, input
 * collection, address resolution, buses, CPU/state mutation,
 * sequencing, timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/store/op_stl_asm_smem_direct.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t stl_asm_direct_word(uint8_t select, uint8_t dma)
{
    uint16_t word = UINT16_C(0x8400);

    word |= (uint16_t)((uint16_t)(select & UINT8_C(1)) << 8U);
    word |= (uint16_t)(dma & UINT8_C(0x7F));
    return word;
}

static ti_c54x_decoded_instruction decode(uint8_t select, uint8_t dma)
{
    const uint16_t word = stl_asm_direct_word(select, dma);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_STL_ASM_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id ==
                       TI_C54X_OPCODE_ID_STL_ASM_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_ASM);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    return instruction;
}

static ti_c54x_op_input_snapshot input_for(uint64_t a_raw40, bool a_known,
                                           uint64_t b_raw40, bool b_known,
                                           uint8_t asm_value, bool asm_known,
                                           uint8_t sxm, bool sxm_known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = a_raw40;
    input.accumulator_a_known = a_known;
    input.accumulator_b_raw40 = b_raw40;
    input.accumulator_b_known = b_known;
    input.status_asm = asm_value;
    input.status_asm_known = asm_known;
    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t select,
                                                       uint8_t indirect,
                                                       uint8_t dma)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = select;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = indirect;
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = dma;
    operands.values[2].width_bits = UINT8_C(7);
    return operands;
}

static void assert_memory_write(const ti_c54x_op_effect *effect, uint8_t dma,
                                uint16_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
                           &effect->payload.memory));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.direct_dma7 ==
                       dma);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == value);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
    TI_C54X_TEST_CHECK(!effect->payload.memory.write_smem_delay);
    TI_C54X_TEST_CHECK(!effect->payload.memory.load_t_with_read);
    TI_C54X_TEST_CHECK(effect->payload.memory.rmw_operation ==
                       (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE);
}

typedef struct stl_asm_vector {
    uint8_t select;
    uint64_t a_raw40;
    uint64_t b_raw40;
    uint8_t asm_value;
    uint8_t sxm;
    uint16_t expected;
} stl_asm_vector;

static const stl_asm_vector shift_vectors[] = {
    /* TI Example 2 shape: B right-shifted 8 (asm 24) stores 0x2112. */
    {UINT8_C(1), UINT64_C(0xAAAAAAAAAA), UINT64_C(0xFF84211234),
     UINT8_C(24), UINT8_C(0), UINT16_C(0x2112)},
    /* Same shift under SXM=1 sign fill stores the same low word. */
    {UINT8_C(1), UINT64_C(0xAAAAAAAAAA), UINT64_C(0xFF84211234),
     UINT8_C(24), UINT8_C(1), UINT16_C(0x2112)},
    /* S=0 selects A even when B differs. */
    {UINT8_C(0), UINT64_C(0xFF84211234), UINT64_C(0xAAAAAAAAAA),
     UINT8_C(24), UINT8_C(0), UINT16_C(0x2112)},
    /* asm=0 identity stores the accumulator low word. */
    {UINT8_C(0), UINT64_C(0x123456789A), UINT64_C(0), UINT8_C(0),
     UINT8_C(0), UINT16_C(0x789A)},
    /* asm=15 maximum left shift. */
    {UINT8_C(0), UINT64_C(0x0000000001), UINT64_C(0), UINT8_C(15),
     UINT8_C(0), UINT16_C(0x8000)},
    /* asm=16 maximum right shift, logical fill. */
    {UINT8_C(0), UINT64_C(0xFF12345678), UINT64_C(0), UINT8_C(16),
     UINT8_C(0), UINT16_C(0x1234)},
    /* asm=16 arithmetic fill of a negative accumulator. */
    {UINT8_C(1), UINT64_C(0), UINT64_C(0x8000000000), UINT8_C(16),
     UINT8_C(1), UINT16_C(0x0000)},
    /* asm=31 minimum right shift. */
    {UINT8_C(0), UINT64_C(0x0000000003), UINT64_C(0), UINT8_C(31),
     UINT8_C(0), UINT16_C(0x0001)},
};

static void check_metadata_and_all_forms(void)
{
    const ti_c54x_opcode_metadata *metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_STL_ASM_SMEM_DIRECT);
    static const uint8_t dmas[] = {UINT8_C(0x00), UINT8_C(0x01),
                                   UINT8_C(0x3F), UINT8_C(0x7E),
                                   UINT8_C(0x7F)};

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id ==
                       TI_C54X_OPCODE_ID_STL_ASM_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0x8400));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFE80));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_ASM);

    for (uint16_t select = 0U; select <= UINT16_C(1); ++select) {
        for (size_t dma_index = 0U;
             dma_index < sizeof(dmas) / sizeof(dmas[0]); ++dma_index) {
            const uint8_t dma = dmas[dma_index];
            const ti_c54x_decoded_instruction instruction =
                decode((uint8_t)select, dma);

            TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                               stl_asm_direct_word((uint8_t)select, dma));
        }
    }
}

static void check_effects_and_parity(void)
{
    static const uint8_t dmas[] = {UINT8_C(0x00), UINT8_C(0x01),
                                   UINT8_C(0x3F), UINT8_C(0x7E),
                                   UINT8_C(0x7F)};

    for (size_t vector_index = 0U;
         vector_index < sizeof(shift_vectors) / sizeof(shift_vectors[0]);
         ++vector_index) {
        const stl_asm_vector *vector = &shift_vectors[vector_index];

        for (size_t dma_index = 0U;
             dma_index < sizeof(dmas) / sizeof(dmas[0]); ++dma_index) {
            const uint8_t dma = dmas[dma_index];
            const ti_c54x_decoded_instruction instruction =
                decode(vector->select, dma);
            const ti_c54x_trusted_opcode_binding *binding =
                ti_c54x_trusted_opcode_binding_for_id(
                    TI_C54X_OPCODE_ID_STL_ASM_SMEM_DIRECT);
            ti_c54x_trusted_execution_packet packet;
            const ti_c54x_op_input_snapshot input = input_for(
                vector->a_raw40, true, vector->b_raw40, true,
                vector->asm_value, true, vector->sxm, true);
            const ti_c54x_op_input_snapshot input_before = input;
            const ti_c54x_trusted_operand_values operands =
                trusted_operands(vector->select, UINT8_C(0), dma);
            const ti_c54x_op_effect before = {
                .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
            ti_c54x_op_effect direct = before;
            ti_c54x_op_effect checked = before;
            ti_c54x_op_effect trusted_dispatch = before;
            ti_c54x_op_effect trusted_leaf = before;

            TI_C54X_TEST_CHECK(binding != NULL);
            TI_C54X_TEST_CHECK(binding->handler ==
                               ti_c54x_trusted_op_stl_asm_smem_direct);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                   binding, &instruction.bundle, &packet) ==
                               TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
            TI_C54X_TEST_CHECK(packet.instruction.id ==
                               TI_C54X_OPCODE_ID_STL_ASM_SMEM_DIRECT);
            TI_C54X_TEST_CHECK(packet.operands.count == UINT8_C(3));
            TI_C54X_TEST_CHECK(packet.operands.values[0].raw_value ==
                               vector->select);
            TI_C54X_TEST_CHECK(packet.operands.values[0].width_bits ==
                               UINT8_C(1));
            TI_C54X_TEST_CHECK(packet.operands.values[1].raw_value ==
                               UINT64_C(0));
            TI_C54X_TEST_CHECK(packet.operands.values[1].width_bits ==
                               UINT8_C(1));
            TI_C54X_TEST_CHECK(packet.operands.values[2].raw_value == dma);
            TI_C54X_TEST_CHECK(packet.operands.values[2].width_bits ==
                               UINT8_C(7));

            TI_C54X_TEST_CHECK(ti_c54x_op_stl_asm_smem_direct(
                                   &instruction, &input, &direct) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                                   &instruction, &input, &checked) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                   &packet, &input, &trusted_dispatch) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stl_asm_smem_direct(
                                   &packet.instruction, &operands, &input,
                                   &trusted_leaf) == TI_C54X_OP_STATUS_OK);
            assert_memory_write(&direct, dma, vector->expected);
            assert_memory_write(&checked, dma, vector->expected);
            assert_memory_write(&trusted_dispatch, dma, vector->expected);
            assert_memory_write(&trusted_leaf, dma, vector->expected);
            TI_C54X_TEST_CHECK(memcmp(&direct, &checked, sizeof(direct)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_dispatch,
                                      sizeof(direct)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&direct, &trusted_leaf,
                                      sizeof(direct)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&input, &input_before, sizeof(input)) ==
                               0);
        }
    }
}

static void check_unknown_case(uint8_t select,
                               const ti_c54x_op_input_snapshot *input)
{
    const ti_c54x_decoded_instruction instruction =
        decode(select, UINT8_C(0x7F));
    const ti_c54x_op_input_snapshot input_before = *input;
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_STL_ASM_SMEM_DIRECT);
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(select, UINT8_C(0), UINT8_C(0x7F));
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_stl_asm_smem_direct(&instruction, input,
                                                     &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(&packet, input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stl_asm_smem_direct(
                           &packet.instruction, &operands, input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(memcmp(input, &input_before, sizeof(*input)) == 0);
}

static void check_unknown_input_is_transactional(void)
{
    const ti_c54x_op_input_snapshot unknown_src =
        input_for(UINT64_C(0xFF84211234), true, UINT64_C(0xFF84211234),
                  false, UINT8_C(24), true, UINT8_C(0), true);
    const ti_c54x_op_input_snapshot unknown_asm =
        input_for(UINT64_C(0xFF84211234), true, UINT64_C(0xFF84211234),
                  true, UINT8_C(24), false, UINT8_C(0), true);
    const ti_c54x_op_input_snapshot unknown_sxm =
        input_for(UINT64_C(0xFF84211234), true, UINT64_C(0xFF84211234),
                  true, UINT8_C(24), true, UINT8_C(0), false);
    const ti_c54x_op_input_snapshot unknown_a_selected =
        input_for(UINT64_C(0x123456789A), false, UINT64_C(0xFF84211234),
                  true, UINT8_C(0), true, UINT8_C(0), true);

    check_unknown_case(UINT8_C(1), &unknown_src);
    check_unknown_case(UINT8_C(1), &unknown_asm);
    check_unknown_case(UINT8_C(1), &unknown_sxm);
    check_unknown_case(UINT8_C(0), &unknown_a_selected);

    /* An unknown unselected accumulator stays unconsumed: S=1 with A
     * unknown but B known still stores the shifted B low word. */
    {
        const ti_c54x_decoded_instruction instruction =
            decode(UINT8_C(1), UINT8_C(0x10));
        const ti_c54x_op_input_snapshot selected =
            input_for(UINT64_C(0xAAAAAAAAAA), false,
                      UINT64_C(0xFF84211234), true, UINT8_C(24), true,
                      UINT8_C(0), true);
        ti_c54x_op_effect effect = {0};

        TI_C54X_TEST_CHECK(ti_c54x_op_stl_asm_smem_direct(
                               &instruction, &selected, &effect) ==
                           TI_C54X_OP_STATUS_OK);
        assert_memory_write(&effect, UINT8_C(0x10), UINT16_C(0x2112));
    }
}

static void check_invalid_operand_gating_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(1), UINT8_C(0x03));
    const ti_c54x_op_input_snapshot input_before_base = input_for(
        UINT64_C(0x123456789A), true, UINT64_C(0xFF84211234), true,
        UINT8_C(24), true, UINT8_C(0), true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    static const uint64_t wide_raw40 = UINT64_C(0x10000000000);
    ti_c54x_op_effect effect = before;

    {
        const ti_c54x_op_input_snapshot wide =
            input_for(input_before_base.accumulator_a_raw40, true,
                      wide_raw40, true, UINT8_C(24), true, UINT8_C(0),
                      true);
        TI_C54X_TEST_CHECK(ti_c54x_op_stl_asm_smem_direct(
                               &instruction, &wide, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        const ti_c54x_op_input_snapshot bad_asm =
            input_for(input_before_base.accumulator_a_raw40, true,
                      input_before_base.accumulator_b_raw40, true,
                      UINT8_C(32), true, UINT8_C(0), true);
        TI_C54X_TEST_CHECK(ti_c54x_op_stl_asm_smem_direct(
                               &instruction, &bad_asm, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        const ti_c54x_op_input_snapshot bad_sxm =
            input_for(input_before_base.accumulator_a_raw40, true,
                      input_before_base.accumulator_b_raw40, true,
                      UINT8_C(24), true, UINT8_C(2), true);
        TI_C54X_TEST_CHECK(ti_c54x_op_stl_asm_smem_direct(
                               &instruction, &bad_sxm, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
    {
        ti_c54x_trusted_operand_values operands =
            trusted_operands(UINT8_C(1), UINT8_C(0), UINT8_C(0x03));

        operands.values[0].raw_value = UINT64_C(2);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stl_asm_smem_direct(
                               &instruction, &operands,
                               &input_before_base, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void check_indirect_gating_is_invalid_operand(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(0x03));
    const ti_c54x_op_input_snapshot input = input_for(
        UINT64_C(0x123456789A), true, UINT64_C(0xFF84211234), true,
        UINT8_C(24), true, UINT8_C(0), true);
    const ti_c54x_op_input_snapshot input_before = input;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;
    ti_c54x_trusted_operand_values operands;

    forged = instruction;
    forged.bundle.program_words[0] |= UINT16_C(0x0080);
    TI_C54X_TEST_CHECK(ti_c54x_op_stl_asm_smem_direct(&forged, &input,
                                                     &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.bundle.program_words[0] = UINT16_C(0x8583);
    TI_C54X_TEST_CHECK(ti_c54x_op_stl_asm_smem_direct(&forged, &input,
                                                     &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands = trusted_operands(UINT8_C(0), UINT8_C(0), UINT8_C(0x03));
    operands.values[1].raw_value = UINT64_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stl_asm_smem_direct(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&input, &input_before, sizeof(input)) == 0);
}

static void check_checked_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(0x03));
    const ti_c54x_op_input_snapshot input = input_for(
        UINT64_C(0x123456789A), true, UINT64_C(0xFF84211234), true,
        UINT8_C(24), true, UINT8_C(0), true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;

#define CHECK_INVALID_DECODE(candidate)                                         \
    do {                                                                        \
        effect = before;                                                        \
        TI_C54X_TEST_CHECK(ti_c54x_op_stl_asm_smem_direct(                      \
                               &(candidate), &input, &effect) ==                \
                           TI_C54X_OP_STATUS_INVALID_DECODE);                  \
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);      \
    } while (0)

    TI_C54X_TEST_CHECK(ti_c54x_op_stl_asm_smem_direct(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_stl_asm_smem_direct(&instruction, NULL,
                                                     &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_stl_asm_smem_direct(&instruction, &input,
                                                     NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.id = TI_C54X_OPCODE_ID_NOP;
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.repeatability = TI_C54X_REPEATABILITY_INELIGIBLE;
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.bundle.program_word_count = UINT8_C(2);
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.bundle.program_words[0] ^= UINT16_C(0x0200);
    CHECK_INVALID_DECODE(forged);

#undef CHECK_INVALID_DECODE
}

static void check_trusted_operand_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(1), UINT8_C(0x7F));
    const ti_c54x_op_input_snapshot input = input_for(
        UINT64_C(0x123456789A), true, UINT64_C(0xFF84211234), true,
        UINT8_C(24), true, UINT8_C(0), true);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_STL_ASM_SMEM_DIRECT);
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(1), UINT8_C(0), UINT8_C(0x7F));

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stl_asm_smem_direct(
                           &packet.instruction, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stl_asm_smem_direct(
                           &packet.instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stl_asm_smem_direct(
                           &packet.instruction, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stl_asm_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0), UINT8_C(0x7F));
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stl_asm_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0), UINT8_C(0x7F));
    operands.values[1].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stl_asm_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0), UINT8_C(0x7F));
    operands.values[2].raw_value = UINT64_C(0x80);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_stl_asm_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_metadata_and_all_forms();
    check_effects_and_parity();
    check_unknown_input_is_transactional();
    check_invalid_operand_gating_is_transactional();
    check_indirect_gating_is_invalid_operand();
    check_checked_rejection_is_transactional();
    check_trusted_operand_rejection_is_transactional();
    return 0;
}
