/*
 * Module contract
 *
 * Owns: executable RED/GREEN proofs for the CMPS direct-slice
 * leaf: max(src(31:16), src(15:0)) as signed 16-bit compares
 * stages a direct Smem WRITE, with the completion staging the
 * TRN_SHIFT_WRITE follow-on ((trn << 1) | taken, TC = taken).
 * Covers S select, max-select truth table (hi>lo, hi<lo,
 * ties, negative halves, guard-bit exclusion), TRN shift
 * math, checked/trusted/dispatch parity, and transactional
 * rejection of the deferred indirect shape, unknown inputs,
 * and NULL arguments.
 * Does not own: catalog rows, operand extraction, effect
 * staging, the sequencer commits that consume the effects, or
 * CPU/state mutation.
 */

#include "../../../../../src/isa/ops/store/op_cmps_smem_direct.h"
#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct max_vector {
    uint64_t acc_raw40;
    uint16_t expected_staged;
    bool expected_taken;
} max_vector;

/* Hand-verified signed-16 max table (guard bits excluded: only
 * bits 31:0 participate). Cross-checked against an independent
 * Python oracle. */
static const max_vector VECTORS[] = {
    {UINT64_C(0x0000700012), UINT16_C(0x0070), true},
    {UINT64_C(0x0000120070), UINT16_C(0x0070), false},
    {UINT64_C(0x0012341234), UINT16_C(0x1234), false},
    {UINT64_C(0x00FF0000FF), UINT16_C(0x00FF), false},
    {UINT64_C(0x0000FF00FF), UINT16_C(0x00FF), false},
    {UINT64_C(0xFF80007FFF), UINT16_C(0x7FFF), false},
    {UINT64_C(0xFF7FFF8000), UINT16_C(0x7FFF), true},
    {UINT64_C(0x0100700012), UINT16_C(0x0070), true},
};

static ti_c54x_op_input_snapshot make_input(uint64_t acc, bool known,
                                            uint16_t trn, bool trn_known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = acc;
    input.accumulator_a_known = known;
    input.accumulator_b_raw40 = UINT64_C(0);
    input.accumulator_b_known = true;
    input.trn_register_raw16 = trn;
    input.trn_register_known = trn_known;
    return input;
}

static ti_c54x_decoded_instruction decode(uint8_t select, uint8_t addr7)
{
    const uint16_t word = (uint16_t)(
        (UINT16_C(0x8E00) | ((uint16_t)(select & UINT8_C(1)) << 8U)) |
        (uint16_t)addr7);
    ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    ti_c54x_instruction_word_bundle bundle = {0};
    ti_c54x_decoded_instruction instruction = {0};

    bundle.program_words[0] = word;
    bundle.program_word_count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) == 0);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_CMPS_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(
        instruction.operand_layout ==
        TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(
        instruction.input_resources ==
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_TRN_REGISTER);
    return instruction;
}

static void check_max_vectors(void)
{
    size_t index;

    for (index = 0U;
         index < sizeof(VECTORS) / sizeof(VECTORS[0]); ++index) {
        ti_c54x_decoded_instruction instruction = decode(UINT8_C(0),
                                                         UINT8_C(0x10));
        ti_c54x_op_input_snapshot input =
            make_input(VECTORS[index].acc_raw40, true, UINT16_C(0x1234),
                       true);
        ti_c54x_op_effect effect = {0};
        ti_c54x_op_effect follow_on = {0};
        uint16_t want_trn =
            (uint16_t)(((uint16_t)(UINT16_C(0x1234) << 1)) |
                       (uint16_t)(VECTORS[index].expected_taken ? 0U : 1U));

        TI_C54X_TEST_CHECK(ti_c54x_op_cmps_smem_direct(
                               &instruction, &input,
                               &effect) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_MEMORY);
        TI_C54X_TEST_CHECK(
            effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
        TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.direct_dma7 ==
                           UINT8_C(0x10));
        TI_C54X_TEST_CHECK(effect.payload.memory.access ==
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
        TI_C54X_TEST_CHECK(effect.payload.memory.value_word ==
                           VECTORS[index].expected_staged);
        TI_C54X_TEST_CHECK(effect.payload.memory.complete != NULL);
        TI_C54X_TEST_CHECK(effect.payload.memory.complete(
                               &instruction, &input, UINT16_C(0xBEEF),
                               &follow_on) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(follow_on.kind ==
                           TI_C54X_OP_EFFECT_TRN_SHIFT_WRITE);
        TI_C54X_TEST_CHECK(follow_on.payload.trn_shift_write.trn_word ==
                           want_trn);
        TI_C54X_TEST_CHECK(follow_on.payload.trn_shift_write.tc_bit ==
                           (VECTORS[index].expected_taken ? UINT8_C(0)
                                                          : UINT8_C(1)));
    }
}

static void check_b_source_and_trn_edges(void)
{
    ti_c54x_decoded_instruction instruction = decode(UINT8_C(1),
                                                     UINT8_C(0x20));
    ti_c54x_op_input_snapshot input = make_input(UINT64_C(0), true,
                                                 UINT16_C(0xFFFF), true);
    ti_c54x_op_effect effect = {0};
    ti_c54x_op_effect follow_on = {0};

    /* S=1 stores from B: B hi 0x4321 beats lo 0x1234. */
    input.accumulator_b_raw40 = UINT64_C(0x0043211234);
    TI_C54X_TEST_CHECK(ti_c54x_op_cmps_smem_direct(
                           &instruction, &input,
                           &effect) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.payload.memory.value_word ==
                       UINT16_C(0x4321));
    TI_C54X_TEST_CHECK(effect.payload.memory.complete(
                           &instruction, &input, UINT16_C(0),
                           &follow_on) == TI_C54X_OP_STATUS_OK);
    /* TRN saturates through the shift: (0xFFFF << 1) | 0. */
    TI_C54X_TEST_CHECK(follow_on.payload.trn_shift_write.trn_word ==
                       UINT16_C(0xFFFE));
    TI_C54X_TEST_CHECK(follow_on.payload.trn_shift_write.tc_bit ==
                       UINT8_C(0));
}

static void check_rejections_are_transactional(void)
{
    ti_c54x_decoded_instruction indirect = decode(UINT8_C(0),
                                                  UINT8_C(0x10));
    ti_c54x_decoded_instruction valid = decode(UINT8_C(0),
                                               UINT8_C(0x10));
    ti_c54x_op_input_snapshot input =
        make_input(UINT64_C(0x0000700012), true, UINT16_C(0x1234), true);
    ti_c54x_op_input_snapshot unknown_acc = input;
    ti_c54x_op_input_snapshot unknown_trn = input;
    ti_c54x_op_effect sentinel = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect;

    /* Deferred indirect shape (I=1 over the same selector). */
    indirect.bundle.program_words[0] |= UINT16_C(0x0080);
    effect = sentinel;
    TI_C54X_TEST_CHECK(ti_c54x_op_cmps_smem_direct(
                           &indirect, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    unknown_acc.accumulator_a_known = false;
    effect = sentinel;
    TI_C54X_TEST_CHECK(ti_c54x_op_cmps_smem_direct(
                           &valid, &unknown_acc,
                           &effect) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    unknown_trn.trn_register_known = false;
    effect = sentinel;
    TI_C54X_TEST_CHECK(ti_c54x_op_cmps_smem_direct(
                           &valid, &unknown_trn,
                           &effect) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);

    effect = sentinel;
    TI_C54X_TEST_CHECK(ti_c54x_op_cmps_smem_direct(
                           NULL, &input,
                           &effect) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_cmps_smem_direct(
                           &valid, NULL,
                           &effect) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_cmps_smem_direct(
                           &valid, &input,
                           NULL) == TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &sentinel, sizeof(effect)) == 0);
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t select,
                                                       uint8_t dma)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = select;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT64_C(0);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = dma;
    operands.values[2].width_bits = UINT8_C(7);
    return operands;
}

static void check_dispatch_and_trusted_parity(void)
{
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_CMPS_SMEM_DIRECT);

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler ==
                       ti_c54x_trusted_op_cmps_smem_direct);
    for (uint8_t select = UINT8_C(0); select < UINT8_C(2); ++select) {
        const ti_c54x_decoded_instruction instruction =
            decode(select, UINT8_C(0x10));
        ti_c54x_trusted_execution_packet packet;

        TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                               binding, &instruction.bundle, &packet) ==
                           TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
        TI_C54X_TEST_CHECK(packet.instruction.id ==
                           TI_C54X_OPCODE_ID_CMPS_SMEM_DIRECT);
        TI_C54X_TEST_CHECK(packet.operands.count == UINT8_C(3));
        TI_C54X_TEST_CHECK(packet.operands.values[0].raw_value == select);
        TI_C54X_TEST_CHECK(packet.operands.values[0].width_bits ==
                           UINT8_C(1));
        TI_C54X_TEST_CHECK(packet.operands.values[1].raw_value ==
                           UINT64_C(0));
        TI_C54X_TEST_CHECK(packet.operands.values[1].width_bits ==
                           UINT8_C(1));
        TI_C54X_TEST_CHECK(packet.operands.values[2].raw_value ==
                           UINT64_C(0x10));
        TI_C54X_TEST_CHECK(packet.operands.values[2].width_bits ==
                           UINT8_C(7));

        for (size_t index = 0U;
             index < sizeof(VECTORS) / sizeof(VECTORS[0]); ++index) {
            ti_c54x_op_input_snapshot input = {0};
            const ti_c54x_trusted_operand_values operands =
                trusted_operands(select, UINT8_C(0x10));
            ti_c54x_op_effect direct = {0};
            ti_c54x_op_effect via_decode = {0};
            ti_c54x_op_effect via_trusted = {0};
            ti_c54x_op_effect via_leaf = {0};
            ti_c54x_op_effect direct_follow = {0};
            ti_c54x_op_effect trusted_follow = {0};

            if (select == UINT8_C(0)) {
                input.accumulator_a_raw40 = VECTORS[index].acc_raw40;
                input.accumulator_a_known = true;
                input.accumulator_b_raw40 = UINT64_C(0);
                input.accumulator_b_known = true;
            } else {
                input.accumulator_a_raw40 = UINT64_C(0);
                input.accumulator_a_known = true;
                input.accumulator_b_raw40 = VECTORS[index].acc_raw40;
                input.accumulator_b_known = true;
            }
            input.trn_register_raw16 = UINT16_C(0x1234);
            input.trn_register_known = true;

            TI_C54X_TEST_CHECK(ti_c54x_op_cmps_smem_direct(
                                   &instruction, &input, &direct) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(
                ti_c54x_dispatch_decoded_with_input_snapshot(
                    &instruction, &input, &via_decode) ==
                TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                   &packet, &input, &via_trusted) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_cmps_smem_direct(
                                   &packet.instruction, &operands, &input,
                                   &via_leaf) == TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(memcmp(&direct, &via_decode,
                                      sizeof(direct)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&direct, &via_trusted,
                                      sizeof(direct)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&direct, &via_leaf,
                                      sizeof(direct)) == 0);
            TI_C54X_TEST_CHECK(direct.payload.memory.value_word ==
                               VECTORS[index].expected_staged);
            TI_C54X_TEST_CHECK(direct.payload.memory.complete != NULL);
            TI_C54X_TEST_CHECK(direct.payload.memory.complete(
                                   &instruction, &input, UINT16_C(0xBEEF),
                                   &direct_follow) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(via_leaf.payload.memory.complete(
                                   &packet.instruction, &input,
                                   UINT16_C(0xBEEF),
                                   &trusted_follow) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(memcmp(&direct_follow, &trusted_follow,
                                      sizeof(direct_follow)) == 0);
        }
    }
}

int main(void)
{
    check_max_vectors();
    check_b_source_and_trn_edges();
    check_rejections_are_transactional();
    check_dispatch_and_trusted_parity();
    return 0;
}
