/*
 * Module contract
 *
 * Owns: executable RED proofs for the one-word FBACC far-accumulator
 * row, including accumulator-low/page carriage with S source select,
 * FAR_PC_WRITE kind with pmad plus page and no status/T/memory
 * effect, unknown-accumulator rejection, checked/trusted/dispatch
 * parity, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, buses,
 * memory, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or the production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/branch/op_fbacc.h"
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

/* TI SPRU172C section FBACC (p. 4-54): 1111_01ZS_1110_0110 with Z
 * pinned 0 on this row and S bit 8 selecting src. */
static const uint16_t FBACC_WORD_S0 = UINT16_C(0xF4E6);
static const uint16_t FBACC_WORD_S1 = UINT16_C(0xF5E6);

static ti_c54x_decoded_instruction
decode(uint16_t word)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_FBACC);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_FBACC);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint8_t select, uint64_t target, uint8_t seed)
{
    /* The unselected accumulator plus all status fields ride hostile
     * seeds; the selected accumulator stays pinned to the target. */
    ti_c54x_op_input_snapshot input = {0};

    if (select == UINT8_C(0)) {
        input.accumulator_a_raw40 = target;
        input.accumulator_a_known = true;
        input.accumulator_b_raw40 = UINT64_C(0x2ABCDEF012);
        input.accumulator_b_known = (seed & UINT8_C(2)) != 0U;
    } else {
        input.accumulator_a_raw40 = UINT64_C(0x123456789A);
        input.accumulator_a_known = (seed & UINT8_C(1)) != 0U;
        input.accumulator_b_raw40 = target;
        input.accumulator_b_known = true;
    }
    input.status_c = (uint8_t)(seed & UINT8_C(1));
    input.status_c_known = (seed & UINT8_C(4)) != 0U;
    input.status_tc = (uint8_t)((seed >> 1U) & UINT8_C(1));
    input.status_tc_known = (seed & UINT8_C(8)) != 0U;
    input.status_ovm = (uint8_t)((seed >> 2U) & UINT8_C(1));
    input.status_ovm_known = (seed & UINT8_C(16)) != 0U;
    input.status_sxm = (uint8_t)((seed >> 3U) & UINT8_C(1));
    input.status_sxm_known = (seed & UINT8_C(32)) != 0U;
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
assert_far_pc_write(const ti_c54x_op_effect *effect, uint16_t pmad,
                    uint8_t page)
{
    TI_C54X_TEST_CHECK(effect->kind ==
                       TI_C54X_OP_EFFECT_FAR_PC_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.far_pc_write.pmad == pmad);
    TI_C54X_TEST_CHECK(effect->payload.far_pc_write.page == page);
}

static void
check_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FBACC);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_FBACC);
    TI_C54X_TEST_CHECK(metadata->first_word_value == FBACC_WORD_S0);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFEFF));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
}

static void
check_ti_example(void)
{
    /* TI Example 1 (p. 4-54): FBACC A with A 00 0001 3000 branches
     * PC 1000 to 3000 with XPC 00 to 01. */
    ti_c54x_decoded_instruction instruction = decode(FBACC_WORD_S0);
    const ti_c54x_op_input_snapshot input =
        input_for(UINT8_C(0), UINT64_C(0x0000013000), UINT8_C(0));
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_fbacc(&instruction, &input,
                                       &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_far_pc_write(&effect, UINT16_C(0x3000), UINT8_C(0x01));
}

static void
check_source_select(void)
{
    /* S=1 reads B: FBACC with B 00 007F 2000 carries pmad 0x2000
     * with page 0x7F. */
    ti_c54x_decoded_instruction instruction = decode(FBACC_WORD_S1);
    const ti_c54x_op_input_snapshot input =
        input_for(UINT8_C(1), UINT64_C(0x00007F2000), UINT8_C(0));
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_fbacc(&instruction, &input,
                                       &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_far_pc_write(&effect, UINT16_C(0x2000), UINT8_C(0x7F));
}

static void
check_effects_and_parity(void)
{
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_FBACC);

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_fbacc);

    {
        /* Full S sweep: both source halves carry pmad plus page. */
        static const uint16_t words[2] = {FBACC_WORD_S0,
                                          FBACC_WORD_S1};
        static const uint64_t targets[2] = {UINT64_C(0x0000013000),
                                            UINT64_C(0x00007F2000)};
        static const uint16_t pmads[2] = {UINT16_C(0x3000),
                                          UINT16_C(0x2000)};
        static const uint8_t pages[2] = {UINT8_C(0x01),
                                         UINT8_C(0x7F)};

        for (size_t select = 0U; select < 2U; ++select) {
            ti_c54x_decoded_instruction instruction =
                decode(words[select]);
            ti_c54x_trusted_execution_packet packet;
            const ti_c54x_op_input_snapshot clean =
                input_for((uint8_t)select, targets[select],
                          UINT8_C(0));
            ti_c54x_trusted_operand_values operands =
                trusted_operands((uint8_t)select);
            ti_c54x_op_effect checked = {0};
            ti_c54x_op_effect trusted = {0};
            ti_c54x_op_effect via_dispatch = {0};
            ti_c54x_op_effect via_trusted = {0};

            TI_C54X_TEST_CHECK(
                ti_c54x_trusted_execution_packet_prepare(
                    binding, &instruction.bundle, &packet) ==
                TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
            TI_C54X_TEST_CHECK(ti_c54x_op_fbacc(&instruction, &clean,
                                               &checked) ==
                               TI_C54X_OP_STATUS_OK);
            assert_far_pc_write(&checked, pmads[select],
                                pages[select]);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fbacc(
                                   &instruction, &operands, &clean,
                                   &trusted) == TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(memcmp(&trusted, &checked,
                                      sizeof(checked)) == 0);
            TI_C54X_TEST_CHECK(
                ti_c54x_dispatch_decoded_with_input_snapshot(
                    &instruction, &clean,
                    &via_dispatch) == TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(memcmp(&via_dispatch, &checked,
                                      sizeof(checked)) == 0);
            TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                                   &packet, &clean,
                                   &via_trusted) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(memcmp(&via_trusted, &checked,
                                      sizeof(checked)) == 0);
        }
    }
}

static void
check_argument_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(FBACC_WORD_S0);
    const ti_c54x_op_input_snapshot valid =
        input_for(UINT8_C(0), UINT64_C(0x0000013000), UINT8_C(0));
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0));
    ti_c54x_op_effect effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_fbacc(NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_fbacc(&instruction, NULL,
                                       &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_fbacc(&instruction, &valid,
                                       NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fbacc(
                           &instruction, NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fbacc(
                           &instruction, &operands, NULL,
                           &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fbacc(
                           &instruction, &operands, &valid,
                           NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void
check_input_and_decode_rejection(void)
{
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    /* Unknown selected accumulator rejects; unknown unselected
     * accumulator and unknown status ride through. */
    {
        ti_c54x_decoded_instruction instruction =
            decode(FBACC_WORD_S0);
        ti_c54x_op_input_snapshot input =
            input_for(UINT8_C(0), UINT64_C(0x0000013000),
                      UINT8_C(0));

        input.accumulator_a_known = false;
        TI_C54X_TEST_CHECK(ti_c54x_op_fbacc(&instruction, &input,
                                           &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }

    {
        ti_c54x_decoded_instruction instruction =
            decode(FBACC_WORD_S0);
        ti_c54x_op_input_snapshot valid =
            input_for(UINT8_C(0), UINT64_C(0x0000013000),
                      UINT8_C(0xA5));
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_fbacc(&forged, &valid,
                                           &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_fbacc(&forged, &valid,
                                           &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.repeatability = TI_C54X_REPEATABILITY_UNRESOLVED;
        TI_C54X_TEST_CHECK(ti_c54x_op_fbacc(&forged, &valid,
                                           &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_fbacc(&forged, &valid,
                                           &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        /* Flipping Z (bit 9) crosses into the FBACCD row. */
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0200);
        TI_C54X_TEST_CHECK(ti_c54x_op_fbacc(&forged, &valid,
                                           &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

int
main(void)
{
    check_metadata();
    check_ti_example();
    check_source_select();
    check_effects_and_parity();
    check_argument_rejection();
    check_input_and_decode_rejection();
    puts("fbacc: ok");
    return 0;
}
