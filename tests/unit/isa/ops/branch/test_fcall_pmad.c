/*
 * Module contract
 *
 * Owns: RED proofs for two-word FCALL pmad far-call effects,
 * including STACK_PUSH_FAR carriage over the 7-bit page and full
 * 16-bit pmad space, WRITE-only TOS shape with a zero
 * return-address placeholder, SP-knownness gating with
 * transactional unknown-SP rejection, checked/trusted/dispatch
 * parity, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, buses,
 * memory, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or the production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/branch/op_fcall_pmad.h"
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

static const uint16_t FCALL_PMAD_WORD0_BASE = UINT16_C(0xF980);

static ti_c54x_decoded_instruction
decode(uint8_t page, uint16_t pmad)
{
    const uint16_t word0 = (uint16_t)(FCALL_PMAD_WORD0_BASE | page);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word0);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word0, pmad},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_FCALL_PMAD);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_FCALL_PMAD);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_FAR_PMAD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint16_t sp_value, bool sp_known, uint8_t seed)
{
    /* STACK_POINTER resources: the leaf must ignore every non-SP
     * input field, so hostile non-SP seeds must produce identical
     * effects while SP knownness gates success. */
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
trusted_operands(uint8_t page, uint16_t pmad)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = page;
    operands.values[0].width_bits = UINT8_C(7);
    operands.values[1].raw_value = pmad;
    operands.values[1].width_bits = UINT8_C(16);
    return operands;
}

static void
assert_far_push(const ti_c54x_op_effect *effect)
{
    const ti_c54x_op_memory_effect *memory;

    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    memory = &effect->payload.memory;
    TI_C54X_TEST_CHECK(memory->address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_WORD16);
    TI_C54X_TEST_CHECK(memory->address_spec.mode == 0);
    TI_C54X_TEST_CHECK(memory->address_spec.indexed_mode == 0);
    TI_C54X_TEST_CHECK(memory->address_spec.circular_mode == 0);
    TI_C54X_TEST_CHECK(memory->address_spec.reverse_carry_mode == 0);
    TI_C54X_TEST_CHECK(memory->address_spec.arf == UINT8_C(0));
    TI_C54X_TEST_CHECK(memory->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    /* The pure handler cannot know PC: the root stages the live
     * return address into the frozen copy at prepare. The second
     * push (frozen XPC) rides root storage, not the effect. */
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
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FCALL_PMAD);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_FCALL_PMAD);
    TI_C54X_TEST_CHECK(metadata->first_word_value ==
                       FCALL_PMAD_WORD0_BASE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFF80));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_FAR_PMAD);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
}

static void
check_ti_vector(void)
{
    /* TI SPRU172C FCALL 013333h: PC 0025 to 3333, XPC 00 to 01.
     * The leaf proves the staged kind; the CPU test proves the
     * pushed words and target commit. */
    ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x01), UINT16_C(0x3333));
    const ti_c54x_op_input_snapshot clean =
        input_for(UINT16_C(0x1111), true, UINT8_C(0));
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(&instruction, &clean,
                                             &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_far_push(&effect);
}

static void
check_effects_and_parity(void)
{
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_FCALL_PMAD);
    static const uint16_t pmads[] = {
        UINT16_C(0x0000), UINT16_C(0x0001), UINT16_C(0x0002),
        UINT16_C(0x00FF), UINT16_C(0x2000), UINT16_C(0x3333),
        UINT16_C(0x7FFF), UINT16_C(0x8000), UINT16_C(0xFFFF)};

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_fcall_pmad);

    for (uint32_t page = 0U; page <= UINT32_C(0x7F); ++page) {
        for (size_t pi = 0U;
             pi < sizeof(pmads) / sizeof(pmads[0]); ++pi) {
            ti_c54x_decoded_instruction instruction =
                decode((uint8_t)page, pmads[pi]);
            ti_c54x_trusted_execution_packet packet;
            const ti_c54x_op_input_snapshot clean =
                input_for(UINT16_C(0x1111), true, UINT8_C(0));
            const ti_c54x_op_input_snapshot hostile =
                input_for(UINT16_C(0x2222), true, UINT8_C(0x1D));
            ti_c54x_trusted_operand_values operands =
                trusted_operands((uint8_t)page, pmads[pi]);
            ti_c54x_op_effect checked = {0};
            ti_c54x_op_effect checked_hostile = {0};
            ti_c54x_op_effect trusted = {0};
            ti_c54x_op_effect via_dispatch = {0};
            ti_c54x_op_effect via_trusted = {0};

            /* The page and extension are verbatim copies: varying
             * them exercises the leaf transform without
             * re-decoding. */
            instruction.bundle.program_words[0] =
                (uint16_t)(FCALL_PMAD_WORD0_BASE | page);
            instruction.bundle.program_words[1] = pmads[pi];
            TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                   binding, &instruction.bundle,
                                   &packet) ==
                               TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
            TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(
                                   &instruction, &clean,
                                   &checked) == TI_C54X_OP_STATUS_OK);
            assert_far_push(&checked);
            TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(
                                   &instruction, &hostile,
                                   &checked_hostile) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(memcmp(&checked_hostile, &checked,
                                      sizeof(checked)) == 0);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcall_pmad(
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
                                   &via_trusted) == TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(memcmp(&via_trusted, &checked,
                                      sizeof(checked)) == 0);
        }
    }
}

static void
check_argument_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x01), UINT16_C(0x3333));
    const ti_c54x_op_input_snapshot valid =
        input_for(UINT16_C(0x1111), true, 0U);
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x01), UINT16_C(0x3333));
    ti_c54x_op_effect effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(&instruction, NULL,
                                             &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(&instruction, &valid,
                                             NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcall_pmad(
                           &instruction, NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcall_pmad(
                           &instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcall_pmad(
                           &instruction, &operands, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void
check_input_and_decode_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x01), UINT16_C(0x3333));
    const ti_c54x_op_input_snapshot valid =
        input_for(UINT16_C(0x1111), true, UINT8_C(0xA5));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    /* Unknown SP rejects; the output stays transactional. */
    {
        ti_c54x_op_input_snapshot unknown_sp = valid;

        unknown_sp.stack_pointer_known = false;
        TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(&instruction,
                                                &unknown_sp,
                                                &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(&forged, &valid,
                                                &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(&forged, &valid,
                                                &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.repeatability = TI_C54X_REPEATABILITY_UNRESOLVED;
        TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(&forged, &valid,
                                                &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_word_count = UINT8_C(1);
        TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(&forged, &valid,
                                                &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        /* Bit 7 is a discriminator bit: flipping it leaves the
         * mask. */
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0080);
        TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(&forged, &valid,
                                                &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        /* Low 7 bits are the page operand: flipping bit 0 is
         * still a valid FCALL, and the staged effect (which
         * carries no page) is identical. */
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0001);
        TI_C54X_TEST_CHECK(ti_c54x_op_fcall_pmad(&forged, &valid,
                                                &effect) ==
                           TI_C54X_OP_STATUS_OK);
        assert_far_push(&effect);
    }
}

static void
check_trusted_operand_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x01), UINT16_C(0x3333));
    const ti_c54x_op_input_snapshot valid =
        input_for(UINT16_C(0x1111), true, 0U);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x01), UINT16_C(0x3333));

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcall_pmad(
                           &instruction, &operands, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0x01), UINT16_C(0x3333));
    operands.values[0].width_bits = UINT8_C(8);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcall_pmad(
                           &instruction, &operands, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0x01), UINT16_C(0x3333));
    operands.values[1].width_bits = UINT8_C(15);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_fcall_pmad(
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
    check_argument_rejection();
    check_input_and_decode_rejection();
    check_trusted_operand_rejection();
    puts("fcall_pmad: ok");
    return 0;
}
