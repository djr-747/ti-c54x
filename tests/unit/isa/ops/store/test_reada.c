/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the one-word Smem-to-program
 * DATA copy effect addressed by accumulator A, including
 * direct/indirect extraction, checked/trusted/dispatch parity,
 * ignored input state, DP and A gating, mode rejection,
 * crossed-row rejection, and the trusted registry bindings.
 * Does not own: catalog rows, operand-layout descriptors,
 * address resolution, buses, CPU or architectural state,
 * sequencing, timing, observers, diagnostics, or the
 * production semantic leaves.
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
#include "../../../../../src/isa/ops/store/op_reada.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* TI SPRU172C READA p. 4-136: word 0 pins 0x7E00 with the Smem
 * cookie at bits 6:0 (I at bit 7); the PROGRAM address comes
 * from accumulator A (PAR <- A), so there is no second word.
 * The 0x7C/0x7D neighbors stay MVPD/MVDP and 0x7F stays
 * undecoded for WRITA. */
static ti_c54x_decoded_instruction
decode_direct(uint8_t dma)
{
    const uint16_t first_word =
        (uint16_t)(UINT16_C(0x7E00) | (dma & UINT8_C(0x7F)));
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_READA_DIRECT);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_READA_DIRECT);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_DP);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count ==
                       UINT8_C(1));
    return instruction;
}

static ti_c54x_decoded_instruction
decode_indirect(uint8_t mode4, uint8_t arf)
{
    const uint16_t first_word = (uint16_t)(UINT16_C(0x7E80) |
                                           ((uint16_t)(mode4 & UINT8_C(0x0F))
                                            << 3U) |
                                           (arf & UINT8_C(0x07)));
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {first_word},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_READA_SMEM);
    TI_C54X_TEST_CHECK(first.first_word == first_word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_READA_SMEM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count ==
                       UINT8_C(1));
    return instruction;
}

static ti_c54x_op_input_snapshot
ignored_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = UINT64_C(0x123456789A);
    input.accumulator_b_raw40 = UINT64_C(0x0FEDCBA987);
    input.accumulator_a_known = true;
    input.accumulator_b_known = false;
    input.status_dp = UINT16_C(0x004);
    input.status_dp_known = true;
    input.auxiliary_registers_raw16[3] = UINT16_C(0xAAAA);
    input.auxiliary_registers_known_mask = UINT8_C(0);
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t indirect, uint8_t cookie)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = indirect;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = cookie;
    operands.values[1].width_bits = UINT8_C(7);
    return operands;
}

static void
assert_pair_read(const ti_c54x_op_effect *effect, uint8_t indirect,
                 uint8_t mode, uint8_t arf, uint8_t dma)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.smem_program_a.indirect ==
                       indirect);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.smem_program_a.smem_mode ==
                       mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.smem_program_a.smem_arf ==
                       arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.smem_program_a.smem_dma7 ==
                       dma);
    /* PSHD convention: the access names the completing leg. */
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void
check_direct_form(uint8_t dma)
{
    const ti_c54x_decoded_instruction instruction = decode_direct(dma);
    const ti_c54x_op_input_snapshot input = ignored_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), dma & UINT8_C(0x7F));
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect dispatched = {0};
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_READA_DIRECT);

    TI_C54X_TEST_CHECK(ti_c54x_op_reada_direct(&instruction, &input,
                                               &checked) ==
                       TI_C54X_OP_STATUS_OK);
    assert_pair_read(&checked, UINT8_C(0), UINT8_C(0), UINT8_C(0),
                     dma & UINT8_C(0x7F));
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_reada_direct(
                           &instruction, &operands, &input,
                           &trusted) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler ==
                       ti_c54x_trusted_op_reada_direct);
    TI_C54X_TEST_CHECK(
        ti_c54x_trusted_opcode_binding_is_canonical(binding));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&dispatched, &checked, sizeof(checked)) ==
                       0);
}

static void
check_indirect_form(uint8_t mode4, uint8_t arf)
{
    const ti_c54x_decoded_instruction instruction =
        decode_indirect(mode4, arf);
    const ti_c54x_op_input_snapshot input = ignored_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(1),
                         (uint8_t)(((mode4 & UINT8_C(0x0F)) << 3U) |
                                   (arf & UINT8_C(0x07))));
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect dispatched = {0};
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_READA_SMEM);

    TI_C54X_TEST_CHECK(ti_c54x_op_reada_smem(&instruction, &input,
                                             &checked) ==
                       TI_C54X_OP_STATUS_OK);
    assert_pair_read(&checked, UINT8_C(1), mode4 & UINT8_C(0x0F),
                     arf & UINT8_C(0x07), UINT8_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_reada_smem(
                           &instruction, &operands, &input,
                           &trusted) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler ==
                       ti_c54x_trusted_op_reada_smem);
    TI_C54X_TEST_CHECK(
        ti_c54x_trusted_opcode_binding_is_canonical(binding));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &dispatched) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&dispatched, &checked, sizeof(checked)) ==
                       0);
}

static void
test_direct_forms(void)
{
    check_direct_form(UINT8_C(0x00));
    check_direct_form(UINT8_C(0x0A));
    check_direct_form(UINT8_C(0x7F));
}

static void
test_indirect_forms(void)
{
    /* MOD1 cookie with ARF 3. */
    check_indirect_form(UINT8_C(1), UINT8_C(3));
    check_indirect_form(UINT8_C(0), UINT8_C(0));
    check_indirect_form(UINT8_C(2), UINT8_C(7));
    check_indirect_form(UINT8_C(3), UINT8_C(5));
}

static void
test_direct_dp_unknown_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_direct(UINT8_C(0x0A));
    ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_op_effect effect = {0};

    input.status_dp_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_reada_direct(&instruction, &input,
                                               &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void
test_direct_a_unknown_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_direct(UINT8_C(0x0A));
    ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_op_effect effect = {0};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_reada_direct(&instruction, &input,
                                               &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void
test_indirect_bad_mode_rejects(void)
{
    /* mode4 4-7 need extension words (indexed/circular/MOD5+):
     * the one-word leaf must reject them. */
    static const uint8_t bad_modes[] = {4, 5, 6, 7};
    const ti_c54x_op_input_snapshot input = ignored_input();
    size_t index;

    for (index = 0U;
         index < sizeof(bad_modes) / sizeof(bad_modes[0]); ++index) {
        const ti_c54x_decoded_instruction instruction =
            decode_indirect(bad_modes[index], UINT8_C(2));
        ti_c54x_op_effect effect = {0};

        (void)memset(&effect, 0xA5, sizeof(effect));
        TI_C54X_TEST_CHECK(ti_c54x_op_reada_smem(&instruction, &input,
                                                 &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
    }
}

static void
test_crossed_row_rejects(void)
{
    const ti_c54x_decoded_instruction direct = decode_direct(UINT8_C(0x0A));
    const ti_c54x_decoded_instruction indirect =
        decode_indirect(UINT8_C(1), UINT8_C(3));
    const ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_reada_smem(&direct, &input,
                                             &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(ti_c54x_op_reada_direct(&indirect, &input,
                                               &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

static void
test_neighbor_rows_reject(void)
{
    /* 0x7C is MVPD, 0x7D is MVDP (both directions); 0x7F is the
     * future WRITA row. None may decode as READA. */
    static const uint16_t neighbors[] = {
        UINT16_C(0x7C05), UINT16_C(0x7C9F), UINT16_C(0x7D05),
        UINT16_C(0x7D9F), UINT16_C(0x7F05), UINT16_C(0x7F9F)};
    size_t index;

    for (index = 0U;
         index < sizeof(neighbors) / sizeof(neighbors[0]); ++index) {
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(neighbors[index]);

        if (first.status == TI_C54X_DECODE_STATUS_OK) {
            TI_C54X_TEST_CHECK(first.id != TI_C54X_OPCODE_ID_READA_DIRECT);
            TI_C54X_TEST_CHECK(first.id != TI_C54X_OPCODE_ID_READA_SMEM);
        } else {
            TI_C54X_TEST_CHECK(first.status ==
                               TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        }
    }
    /* The MVPD/MVDP neighbors still claim their words. */
    TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x7C05)).id ==
                       TI_C54X_OPCODE_ID_MVPD_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(UINT16_C(0x7D05)).id ==
                       TI_C54X_OPCODE_ID_MVDP_DIRECT);
}

static void
test_trusted_operand_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_direct(UINT8_C(0x0A));
    const ti_c54x_op_input_snapshot input = ignored_input();
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT8_C(0x0A));
    ti_c54x_op_effect effect = {0};

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_reada_direct(
                           &instruction, &operands, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0x0A));
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_reada_direct(
                           &instruction, &operands, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
}

static void
test_null_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode_direct(UINT8_C(0x0A));
    const ti_c54x_op_input_snapshot input = ignored_input();
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT8_C(0x0A));
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_reada_direct(NULL, &input,
                                               &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_reada_direct(&instruction, NULL,
                                               &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_reada_direct(&instruction, &input,
                                               NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_reada_direct(
                           &instruction, NULL, &input,
                           &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_reada_direct(
                           &instruction, &operands, NULL,
                           &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_reada_direct(
                           &instruction, &operands, &input,
                           NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_direct_forms();
    test_indirect_forms();
    test_direct_dp_unknown_rejects();
    test_direct_a_unknown_rejects();
    test_indirect_bad_mode_rejects();
    test_crossed_row_rejects();
    test_neighbor_rows_reject();
    test_trusted_operand_rejects();
    test_null_rejects();
    return 0;
}
