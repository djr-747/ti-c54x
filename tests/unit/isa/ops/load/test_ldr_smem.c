/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for one-word LDR Smem,dst effects,
 * including D/ARF/MOD selection over all 48 MOD0--MOD2 forms,
 * exhaustive read coverage, SXM-controlled widening with the
 * 0x8000 rounding constant, C/TC/OV preservation with no OVM
 * input, MOD3 write-only rejection, unknown-SXM rejection,
 * checked/trusted/dispatch parity, and transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, buses,
 * memory, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or the production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/load/op_ldr.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/foundation/arithmetic40.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
ldr_word(uint8_t destination, uint8_t mode, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x1680) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (uint16_t)(((uint16_t)(mode & UINT8_C(3))) << 3U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_decoded_instruction
decode(uint8_t destination, uint8_t mode, uint8_t arf)
{
    const uint16_t word = ldr_word(destination, mode, arf);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LDR);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LDR);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint8_t sxm, bool sxm_known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.status_sxm = sxm;
    input.status_sxm_known = sxm_known;
    return input;
}

/* Independent oracle: SXM-widen, shift left 16, add rounding 2^15. */
static uint64_t
expected_rounded(uint16_t read_value, uint8_t sxm)
{
    uint64_t widened = (uint64_t)read_value;

    if (sxm != UINT8_C(0) &&
        (read_value & UINT16_C(0x8000)) != UINT16_C(0)) {
        widened |= UINT64_C(0xFFFFFFFF0000);
    }
    return ((widened << 16U) + UINT64_C(0x8000)) & TI_C54X_U40_MASK;
}

static void
assert_memory_read(const ti_c54x_op_effect *effect, uint8_t mode,
                   uint8_t arf)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.mode == mode);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.arf == arf);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
}

static void
assert_accumulator_write(const ti_c54x_op_effect *effect,
                         uint8_t destination, uint64_t value)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.destination ==
                       destination);
    TI_C54X_TEST_CHECK_U64(effect->payload.accumulator_write.value_raw40,
                           value);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.c_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(effect->payload.accumulator_write.status_delta.tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(
        effect->payload.accumulator_write.status_delta
            .destination_overflow_action ==
        TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE);
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t destination, uint8_t arf, uint8_t mode)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = destination;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = arf;
    operands.values[1].width_bits = UINT8_C(3);
    operands.values[2].raw_value = mode;
    operands.values[2].width_bits = UINT8_C(2);
    return operands;
}

static void
check_valid_forms_and_completion(void)
{
    static const uint16_t reads[] = {UINT16_C(0x0000), UINT16_C(0x0001),
                                     UINT16_C(0x7FFF), UINT16_C(0x8000),
                                     UINT16_C(0xFEDC), UINT16_C(0xFFFF)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        for (uint8_t mode = 0U; mode <= UINT8_C(2); ++mode) {
            for (uint8_t arf = 0U; arf <= UINT8_C(7); ++arf) {
                const ti_c54x_decoded_instruction instruction =
                    decode(destination, mode, arf);
                const ti_c54x_trusted_operand_values operands =
                    trusted_operands(destination, arf, mode);

                for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
                    const ti_c54x_op_input_snapshot input =
                        input_for(sxm, true);
                    ti_c54x_op_effect direct = {0};
                    ti_c54x_op_effect trusted = {0};

                    TI_C54X_TEST_CHECK(ti_c54x_op_ldr(&instruction, &input,
                                                     &direct) ==
                                       TI_C54X_OP_STATUS_OK);
                    assert_memory_read(&direct, mode, arf);
                    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldr(
                                           &instruction, &operands, &input,
                                           &trusted) == TI_C54X_OP_STATUS_OK);
                    TI_C54X_TEST_CHECK(memcmp(&direct, &trusted,
                                              sizeof(direct)) == 0);

                    for (size_t read_index = 0U;
                         read_index < sizeof(reads) / sizeof(reads[0]);
                         ++read_index) {
                        ti_c54x_op_effect completed = {
                            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                            .payload.repeat_start = {
                                .operand = UINT16_C(0xA55A)}};
                        const uint64_t expected = expected_rounded(
                            reads[read_index], sxm);

                        TI_C54X_TEST_CHECK(direct.payload.memory.complete(
                                               &instruction, &input,
                                               reads[read_index],
                                               &completed) ==
                                           TI_C54X_OP_STATUS_OK);
                        assert_accumulator_write(&completed, destination,
                                                 expected);
                    }
                }
            }
        }
    }
}

static void
check_mod3_rejects_as_write_only(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(3), UINT8_C(1));
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT8_C(1), UINT8_C(3));
    const ti_c54x_op_input_snapshot input = input_for(UINT8_C(0), true);
    ti_c54x_op_effect effect = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};

    TI_C54X_TEST_CHECK(ti_c54x_op_ldr(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_REPEAT_START);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ldr(&instruction, &operands,
                                             &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_REPEAT_START);
}

static void
check_unknown_and_malformed_input_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0), UINT8_C(0), UINT8_C(0));
    const ti_c54x_op_input_snapshot unknown_sxm =
        input_for(UINT8_C(0), false);
    const ti_c54x_op_input_snapshot malformed =
        input_for(UINT8_C(2), true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xCAFE)}};
    ti_c54x_op_effect effect = before;
    unsigned char before_bytes[sizeof(effect)];
    (void)memcpy(before_bytes, &before, sizeof(before_bytes));

    TI_C54X_TEST_CHECK(ti_c54x_op_ldr(NULL, &unknown_sxm, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ldr(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ldr(&instruction, &unknown_sxm, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, before_bytes, sizeof(effect)) == 0);

    TI_C54X_TEST_CHECK(ti_c54x_op_ldr(&instruction, &unknown_sxm,
                                     &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ldr(&instruction, &malformed, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    {
        ti_c54x_decoded_instruction forged = instruction;
        const ti_c54x_op_input_snapshot valid =
            input_for(UINT8_C(0), true);

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_ldr(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ldr(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_ACCUMULATORS;
        TI_C54X_TEST_CHECK(ti_c54x_op_ldr(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0200);
        TI_C54X_TEST_CHECK(ti_c54x_op_ldr(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void
test_dispatch_parity(void)
{
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_LDR);
    ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(1), UINT8_C(2), UINT8_C(5));
    const ti_c54x_op_input_snapshot input = input_for(UINT8_C(1), true);
    ti_c54x_trusted_execution_packet packet;
    ti_c54x_op_effect direct = {0};
    ti_c54x_op_effect via_dispatch = {0};
    ti_c54x_op_effect via_trusted = {0};

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_ldr);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_ldr(&instruction, &input, &direct) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &input, &via_dispatch) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&direct, &via_dispatch, sizeof(direct)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(&packet, &input,
                                               &via_trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&direct, &via_trusted, sizeof(direct)) == 0);
}

int
main(void)
{
    check_valid_forms_and_completion();
    check_mod3_rejects_as_write_only();
    check_unknown_and_malformed_input_is_transactional();
    test_dispatch_parity();
    return 0;
}
