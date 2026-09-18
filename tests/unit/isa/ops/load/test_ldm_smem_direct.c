/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the row-less LDM MMR,dst
 * direct-slice WORD16 load effect (first word 0x4800 under mask
 * 0xFE80, D selects A/B, I pinned 0), including the DIRECT_WORD16
 * dma staging, zero-extending completion, checked/trusted/dispatch
 * parity, select/indirect gating with I=1 and D>1 rejected, SXM
 * independence, the TI LDM AR4,A and LDM 060h,B vectors, and
 * transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors,
 * accumulator state, input collection, address resolution, buses,
 * CPU/state mutation, sequencing, timing, observers, diagnostics, or
 * the production semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/load/op_ldm_smem_direct.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t ldm_direct_word(uint8_t destination, uint8_t dma)
{
    uint16_t word = (uint16_t)(UINT16_C(0x4800) | (uint16_t)(dma & UINT8_C(0x7F)));
    if (destination != UINT8_C(0)) {
        word |= UINT16_C(0x0100);
    }
    return word;
}

static ti_c54x_decoded_instruction decode(uint8_t destination, uint8_t dma)
{
    const uint16_t word = ldm_direct_word(destination, dma);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, UINT16_C(0)}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    return instruction;
}

static ti_c54x_op_input_snapshot input_for(uint8_t sxm, bool sxm_known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    input.accumulator_a_raw40 = UINT64_C(0xAAAAAAAAAA);
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x5555555555);
    input.accumulator_b_known = true;
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t destination,
                                                       uint8_t dma)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = destination;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = dma;
    operands.values[2].width_bits = UINT8_C(7);
    return operands;
}

static void assert_memory_read(const ti_c54x_op_effect *effect, uint8_t dma)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
                           &effect->payload.memory));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.direct_dma7 ==
                       dma);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
    TI_C54X_TEST_CHECK(!effect->payload.memory.write_smem_delay);
    TI_C54X_TEST_CHECK(!effect->payload.memory.load_t_with_read);
    TI_C54X_TEST_CHECK(effect->payload.memory.rmw_operation ==
                       (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE);
}

static void assert_completed_write(const ti_c54x_op_effect *effect,
                                   uint8_t destination, uint16_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK_U64(effect->payload.accumulator_write.value_raw40,
                           (uint64_t)value);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
}

static void check_metadata_and_all_forms(void)
{
    const ti_c54x_opcode_metadata *metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0x4800));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFE80));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);

    for (uint16_t destination = 0U; destination < UINT16_C(2);
         ++destination) {
        for (uint16_t dma = 0U; dma < UINT16_C(0x80); ++dma) {
            const ti_c54x_decoded_instruction instruction =
                decode((uint8_t)destination, (uint8_t)dma);

            TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                               ldm_direct_word((uint8_t)destination,
                                               (uint8_t)dma));
        }
    }
}

static void check_ti_vectors(void)
{
    /* TI SPRU172C LDM syntax vectors: LDM 060h,B assembles to 0x4960
     * and zero-extends the 0x1234 MMR word into B=0x0000001234. */
    const ti_c54x_decoded_instruction direct =
        decode(UINT8_C(1), UINT8_C(0x60));
    const ti_c54x_op_input_snapshot input = input_for(UINT8_C(1), true);
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect completed = {0};

    TI_C54X_TEST_CHECK(direct.bundle.program_words[0] == UINT16_C(0x4960));
    TI_C54X_TEST_CHECK(ti_c54x_op_ldm_smem_direct(&direct, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_memory_read(&effect, UINT8_C(0x60));
    TI_C54X_TEST_CHECK(effect.payload.memory.complete(
                           &direct, &input, UINT16_C(0x1234),
                           &completed) == TI_C54X_OP_STATUS_OK);
    assert_completed_write(&completed, UINT8_C(1), UINT16_C(0x1234));

    /* LDM AR4,A is the deferred indirect shape (I=1 over the AR4
     * selector): the leaf rejects it as INVALID_OPERAND and leaves
     * the caller effect unchanged. */
    {
        ti_c54x_decoded_instruction forged = decode(UINT8_C(0),
                                                    UINT8_C(0x04));
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};

        forged.bundle.program_words[0] |= UINT16_C(0x0080);
        TI_C54X_TEST_CHECK(forged.bundle.program_words[0] ==
                           UINT16_C(0x4884));
        effect = before;
        TI_C54X_TEST_CHECK(ti_c54x_op_ldm_smem_direct(&forged, &input,
                                                      &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void check_effects_completion_and_parity(void)
{
    static const uint16_t reads[] = {UINT16_C(0x0000), UINT16_C(0x1234),
                                     UINT16_C(0x8000), UINT16_C(0xFFFF)};
    static const uint8_t dmas[] = {UINT8_C(0x00), UINT8_C(0x01),
                                   UINT8_C(0x0B), UINT8_C(0x3F),
                                   UINT8_C(0x60), UINT8_C(0x7E),
                                   UINT8_C(0x7F)};
    static const uint8_t sxm_values[] = {UINT8_C(0), UINT8_C(1)};

    for (uint8_t destination = 0U; destination < UINT8_C(2);
         ++destination) {
        for (size_t dma_index = 0U;
             dma_index < sizeof(dmas) / sizeof(dmas[0]); ++dma_index) {
            const uint8_t dma = dmas[dma_index];
            const ti_c54x_decoded_instruction instruction =
                decode(destination, dma);
            const ti_c54x_trusted_opcode_binding *binding =
                ti_c54x_trusted_opcode_binding_for_id(
                    TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT);
            ti_c54x_trusted_execution_packet packet;

            TI_C54X_TEST_CHECK(binding != NULL);
            TI_C54X_TEST_CHECK(binding->handler ==
                               ti_c54x_trusted_op_ldm_smem_direct);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                   binding, &instruction.bundle, &packet) ==
                               TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
            TI_C54X_TEST_CHECK(packet.instruction.id ==
                               TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT);
            TI_C54X_TEST_CHECK(packet.operands.count == UINT8_C(3));
            TI_C54X_TEST_CHECK(packet.operands.values[0].raw_value ==
                               destination);
            TI_C54X_TEST_CHECK(packet.operands.values[0].width_bits ==
                               UINT8_C(1));
            TI_C54X_TEST_CHECK(packet.operands.values[1].raw_value ==
                               UINT64_C(0));
            TI_C54X_TEST_CHECK(packet.operands.values[1].width_bits ==
                               UINT8_C(1));
            TI_C54X_TEST_CHECK(packet.operands.values[2].raw_value == dma);
            TI_C54X_TEST_CHECK(packet.operands.values[2].width_bits ==
                               UINT8_C(7));

            for (size_t sxm_index = 0U;
                 sxm_index < sizeof(sxm_values) / sizeof(sxm_values[0]);
                 ++sxm_index) {
                for (uint8_t known = 0U; known <= UINT8_C(1); ++known) {
                    const ti_c54x_op_input_snapshot input = input_for(
                        sxm_values[sxm_index], known != UINT8_C(0));
                    const ti_c54x_op_input_snapshot input_before = input;
                    const ti_c54x_trusted_operand_values operands =
                        trusted_operands(destination, dma);
                    const ti_c54x_op_effect before = {
                        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                        .payload.repeat_start = {.operand =
                                                     UINT16_C(0xA55A)}};
                    ti_c54x_op_effect direct = before;
                    ti_c54x_op_effect checked = before;
                    ti_c54x_op_effect trusted_dispatch = before;
                    ti_c54x_op_effect trusted_leaf = before;

                    TI_C54X_TEST_CHECK(ti_c54x_op_ldm_smem_direct(
                                           &instruction, &input,
                                           &direct) ==
                                       TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(
                        ti_c54x_dispatch_decoded_with_input_snapshot(
                            &instruction, &input, &checked) ==
                        TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                           &packet, &input,
                                           &trusted_dispatch) ==
                                       TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldm_smem_direct(
                                           &packet.instruction, &operands,
                                           &input, &trusted_leaf) ==
                                       TI_C54X_OP_STATUS_OK);
                    assert_memory_read(&direct, dma);
                    assert_memory_read(&checked, dma);
                    assert_memory_read(&trusted_dispatch, dma);
                    assert_memory_read(&trusted_leaf, dma);
                    TI_C54X_TEST_CHECK(
                        memcmp(&direct, &checked, sizeof(direct)) == 0);
                    TI_C54X_TEST_CHECK(
                        memcmp(&direct, &trusted_dispatch,
                               sizeof(direct)) == 0);
                    TI_C54X_TEST_CHECK(
                        memcmp(&direct, &trusted_leaf, sizeof(direct)) ==
                        0);
                    TI_C54X_TEST_CHECK(memcmp(&input, &input_before,
                                              sizeof(input)) == 0);

                    for (size_t index = 0U;
                         index < sizeof(reads) / sizeof(reads[0]);
                         ++index) {
                        ti_c54x_op_effect completed = {
                            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                            .payload.repeat_start = {.operand =
                                                         UINT16_C(0xCAFE)}};

                        TI_C54X_TEST_CHECK(direct.payload.memory.complete(
                                               &instruction, &input,
                                               reads[index],
                                               &completed) ==
                                           TI_C54X_OP_STATUS_OK);
                        assert_completed_write(&completed, destination,
                                               reads[index]);
                    }
                }
            }
        }
    }
}

static void check_select_and_indirect_gating(void)
{
    const ti_c54x_op_input_snapshot input = input_for(UINT8_C(0), false);
    const ti_c54x_op_input_snapshot input_before = input;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;
    ti_c54x_trusted_operand_values operands;

    /* I=1 over either select rejects as INVALID_OPERAND. */
    forged = decode(UINT8_C(0), UINT8_C(0x03));
    forged.bundle.program_words[0] |= UINT16_C(0x0080);
    TI_C54X_TEST_CHECK(ti_c54x_op_ldm_smem_direct(&forged, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = decode(UINT8_C(1), UINT8_C(0x03));
    forged.bundle.program_words[0] |= UINT16_C(0x0080);
    TI_C54X_TEST_CHECK(ti_c54x_op_ldm_smem_direct(&forged, &input,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    /* Trusted I=1 rejects as INVALID_OPERAND. */
    operands = trusted_operands(UINT8_C(0), UINT8_C(0x03));
    operands.values[1].raw_value = UINT64_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldm_smem_direct(
                           &forged, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    /* D>1 rejects as INVALID_OPERAND on the trusted path. */
    operands = trusted_operands(UINT8_C(0), UINT8_C(0x03));
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldm_smem_direct(
                           &forged, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&input, &input_before, sizeof(input)) == 0);
}

static void check_checked_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(0x03));
    const ti_c54x_op_input_snapshot input = input_for(UINT8_C(1), true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;

#define CHECK_INVALID_DECODE(candidate)                                         \
    do {                                                                        \
        effect = before;                                                        \
        TI_C54X_TEST_CHECK(ti_c54x_op_ldm_smem_direct(                          \
                               &(candidate), &input, &effect) ==                \
                           TI_C54X_OP_STATUS_INVALID_DECODE);                  \
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);      \
    } while (0)

    TI_C54X_TEST_CHECK(ti_c54x_op_ldm_smem_direct(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ldm_smem_direct(&instruction, NULL,
                                                  &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ldm_smem_direct(&instruction, &input,
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
    forged.input_resources = TI_C54X_INPUT_RESOURCES_STATUS_SXM;
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
    const ti_c54x_op_input_snapshot input = input_for(UINT8_C(0), true);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT);
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(1), UINT8_C(0x7F));

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldm_smem_direct(
                           &packet.instruction, NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldm_smem_direct(
                           &packet.instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldm_smem_direct(
                           &packet.instruction, &operands, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldm_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0x7F));
    operands.count = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldm_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0x7F));
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldm_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0x7F));
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldm_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0x7F));
    operands.values[1].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldm_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0x7F));
    operands.values[2].raw_value = UINT64_C(0x80);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldm_smem_direct(
                           &packet.instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

static void check_completion_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(1), UINT8_C(0x60));
    const ti_c54x_op_input_snapshot input = input_for(UINT8_C(1), true);
    ti_c54x_op_effect read_effect = {0};
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x1234)}};
    ti_c54x_op_effect effect = before;

    TI_C54X_TEST_CHECK(ti_c54x_op_ldm_smem_direct(&instruction, &input,
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
        forged.bundle.program_words[0] ^= UINT16_C(0x0200);
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                               &forged, &input, UINT16_C(0xFFFF), &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }

    {
        ti_c54x_decoded_instruction forged = instruction;
        forged.bundle.program_words[0] |= UINT16_C(0x0080);
        TI_C54X_TEST_CHECK(read_effect.payload.memory.complete(
                               &forged, &input, UINT16_C(0xFFFF), &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int main(void)
{
    check_metadata_and_all_forms();
    check_ti_vectors();
    check_effects_completion_and_parity();
    check_select_and_indirect_gating();
    check_checked_rejection_is_transactional();
    check_trusted_operand_rejection_is_transactional();
    check_completion_rejection_is_transactional();
    return 0;
}
