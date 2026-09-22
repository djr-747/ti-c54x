/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for two-word CCD pmad,cond effects,
 * including the sixteen evidenced single conditions with taken
 * CALLD-shaped MEMORY stack-push-delayed emission versus
 * not-taken NONE, taken-only SP knownness gating per the CALLD
 * leaf convention, consumed-input-only knownness,
 * deferred-code rejection, checked/trusted/dispatch parity, and
 * transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, buses,
 * memory, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or the production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/branch/op_ccd.h"
#include "../../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/opcode_dispatch.h"
#include "../../../../../src/foundation/arithmetic40.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t CCD_WORD0_BASE = UINT16_C(0xFB00);

static uint16_t
make_word0(uint8_t cond)
{
    return (uint16_t)(CCD_WORD0_BASE | cond);
}

static ti_c54x_decoded_instruction
decode(uint8_t cond, uint16_t pmad)
{
    const uint16_t word = make_word0(cond);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, pmad}, .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_CCD);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_CCD);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC_STACK_POINTER);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint64_t acc_a, uint64_t acc_b, uint8_t c, uint8_t tc,
          uint16_t sp, bool sp_known)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_raw40 = acc_a & TI_C54X_U40_MASK;
    input.accumulator_a_known = true;
    input.accumulator_b_raw40 = acc_b & TI_C54X_U40_MASK;
    input.accumulator_b_known = true;
    input.status_c = c;
    input.status_c_known = true;
    input.status_tc = tc;
    input.status_tc_known = true;
    input.stack_pointer_raw16 = sp;
    input.stack_pointer_known = sp_known;
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t cond, uint16_t pmad)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = cond;
    operands.values[0].width_bits = UINT8_C(8);
    operands.values[1].raw_value = pmad;
    operands.values[1].width_bits = UINT8_C(16);
    return operands;
}

/* Independent oracle mirroring the ops contract: full-40-bit
 * two's-complement signed compare against zero. */
static bool
signed_is_zero(uint64_t value)
{
    return (value & TI_C54X_U40_MASK) == UINT64_C(0);
}

static bool
signed_is_negative(uint64_t value)
{
    return ((value >> 39U) & UINT64_C(1)) != UINT64_C(0);
}

static bool
expected_taken(uint8_t cond, uint64_t acc_a, uint64_t acc_b, uint8_t c,
               uint8_t tc)
{
    const uint64_t a = acc_a & TI_C54X_U40_MASK;
    const uint64_t b = acc_b & TI_C54X_U40_MASK;

    switch (cond) {
    case UINT8_C(0x30):
        return tc != UINT8_C(0);
    case UINT8_C(0x20):
        return tc == UINT8_C(0);
    case UINT8_C(0x0C):
        return c != UINT8_C(0);
    case UINT8_C(0x08):
        return c == UINT8_C(0);
    case UINT8_C(0x45):
        return signed_is_zero(a);
    case UINT8_C(0x44):
        return !signed_is_zero(a);
    case UINT8_C(0x46):
        return !signed_is_negative(a) && !signed_is_zero(a);
    case UINT8_C(0x42):
        return !signed_is_negative(a);
    case UINT8_C(0x43):
        return signed_is_negative(a);
    case UINT8_C(0x47):
        return signed_is_negative(a) || signed_is_zero(a);
    case UINT8_C(0x4D):
        return signed_is_zero(b);
    case UINT8_C(0x4C):
        return !signed_is_zero(b);
    case UINT8_C(0x4E):
        return !signed_is_negative(b) && !signed_is_zero(b);
    case UINT8_C(0x4A):
        return !signed_is_negative(b);
    case UINT8_C(0x4B):
        return signed_is_negative(b);
    case UINT8_C(0x4F):
        return signed_is_negative(b) || signed_is_zero(b);
    default:
        return false;
    }
}

static bool
is_implemented_cond(uint8_t cond)
{
    switch (cond) {
    case UINT8_C(0x30):
    case UINT8_C(0x20):
    case UINT8_C(0x0C):
    case UINT8_C(0x08):
    case UINT8_C(0x45):
    case UINT8_C(0x44):
    case UINT8_C(0x46):
    case UINT8_C(0x42):
    case UINT8_C(0x43):
    case UINT8_C(0x47):
    case UINT8_C(0x4D):
    case UINT8_C(0x4C):
    case UINT8_C(0x4E):
    case UINT8_C(0x4A):
    case UINT8_C(0x4B):
    case UINT8_C(0x4F):
        return true;
    default:
        return false;
    }
}

static void
assert_effect(const ti_c54x_op_effect *effect, bool taken)
{
    if (taken) {
        const ti_c54x_op_memory_effect *memory;

        TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
        memory = &effect->payload.memory;
        TI_C54X_TEST_CHECK(memory->address_spec.kind ==
                           TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16);
        TI_C54X_TEST_CHECK(memory->address_spec.mode == 0);
        TI_C54X_TEST_CHECK(memory->address_spec.indexed_mode == 0);
        TI_C54X_TEST_CHECK(memory->address_spec.circular_mode == 0);
        TI_C54X_TEST_CHECK(memory->address_spec.reverse_carry_mode == 0);
        TI_C54X_TEST_CHECK(memory->address_spec.arf == UINT8_C(0));
        TI_C54X_TEST_CHECK(memory->access ==
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
        /* The pure handler cannot know PC: the root stages the
         * live PC+4 return address into the frozen copy. */
        TI_C54X_TEST_CHECK(memory->value_word == UINT16_C(0));
        TI_C54X_TEST_CHECK(memory->rmw_operation ==
                           (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE);
        TI_C54X_TEST_CHECK(memory->transfer_reserved == UINT8_C(0));
        TI_C54X_TEST_CHECK(memory->complete == NULL);
    } else {
        TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_NONE);
    }
}

static void
check_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_CCD);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_CCD);
    TI_C54X_TEST_CHECK(metadata->first_word_value == CCD_WORD0_BASE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFF80));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC_STACK_POINTER);
}

static void
check_case(uint8_t cond, uint16_t pmad, uint64_t acc_a, uint64_t acc_b,
           uint8_t c, uint8_t tc)
{
    ti_c54x_decoded_instruction instruction = decode(cond, pmad);
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_CCD);
    const ti_c54x_op_input_snapshot clean =
        input_for(acc_a, acc_b, c, tc, UINT16_C(0x0200), true);
    ti_c54x_trusted_operand_values operands = trusted_operands(cond, pmad);
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect via_dispatch = {0};
    ti_c54x_op_effect via_trusted = {0};
    const bool taken = expected_taken(cond, acc_a, acc_b, c, tc);

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_ccd);

    /* The extension word is a verbatim copy: varying it exercises the
     * leaf without re-decoding. */
    instruction.bundle.program_words[1] = pmad;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&instruction, &clean,
                                      &checked) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&checked, taken);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ccd(
                           &instruction, &operands, &clean,
                           &trusted) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked, sizeof(checked)) == 0);
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

static void
check_effects_and_parity(void)
{
    static const uint64_t acc_boundaries[] = {
        UINT64_C(0x0000000000), UINT64_C(0x0000000001),
        UINT64_C(0x0000FFFFFF), UINT64_C(0x007FFFFFFF),
        UINT64_C(0x0080000000), UINT64_C(0x00FFFFFFFF),
        UINT64_C(0x0100000000), UINT64_C(0x7FFFFFFFFF),
        UINT64_C(0x8000000000), UINT64_C(0xFFFFFFFFFF),
        UINT64_C(0xFF00000000), UINT64_C(0xFFFFFF0000)};
    static const uint16_t pmads[] = {
        UINT16_C(0x0000), UINT16_C(0x0002), UINT16_C(0x1000),
        UINT16_C(0x2000), UINT16_C(0xFFFF)};
    static const uint8_t conds[] = {
        UINT8_C(0x30), UINT8_C(0x20), UINT8_C(0x0C), UINT8_C(0x08),
        UINT8_C(0x45), UINT8_C(0x44), UINT8_C(0x46), UINT8_C(0x42),
        UINT8_C(0x43), UINT8_C(0x47), UINT8_C(0x4D), UINT8_C(0x4C),
        UINT8_C(0x4E), UINT8_C(0x4A), UINT8_C(0x4B), UINT8_C(0x4F)};
    size_t ci;
    size_t ai;
    size_t pi;

    for (ci = 0U; ci < sizeof(conds) / sizeof(conds[0]); ++ci) {
        for (ai = 0U;
             ai < sizeof(acc_boundaries) / sizeof(acc_boundaries[0]);
             ++ai) {
            for (pi = 0U; pi < sizeof(pmads) / sizeof(pmads[0]); ++pi) {
                check_case(conds[ci], pmads[pi], acc_boundaries[ai],
                           acc_boundaries[(ai + 5U) %
                                          (sizeof(acc_boundaries) /
                                           sizeof(acc_boundaries[0]))],
                           (uint8_t)(ai & UINT64_C(1)),
                           (uint8_t)((ai >> 1U) & UINT64_C(1)));
            }
        }
    }
}

static void
check_all_codes_classified(void)
{
    /* Every condition field value is either implemented (OK with
     * oracle outcome) or deferred (transactional INVALID_OPERAND).
     * The P3-S118 tightening pins bit 7 clear, so the set half
     * decodes to the far-call row (covered by its own leaf). */
    for (uint32_t raw = 0U; raw <= UINT32_C(0x7F); ++raw) {
        const uint8_t cond = (uint8_t)raw;
        const ti_c54x_decoded_instruction instruction =
            decode(cond, UINT16_C(0x2000));
        const ti_c54x_op_input_snapshot clean =
            input_for(UINT64_C(0x53), UINT64_C(0x53), UINT8_C(1),
                      UINT8_C(1), UINT16_C(0x0200), true);
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
        ti_c54x_op_effect effect = before;
        const ti_c54x_op_status status =
            ti_c54x_op_ccd(&instruction, &clean, &effect);

        if (is_implemented_cond(cond)) {
            TI_C54X_TEST_CHECK(status == TI_C54X_OP_STATUS_OK);
            assert_effect(&effect,
                          expected_taken(cond, UINT64_C(0x53),
                                         UINT64_C(0x53), UINT8_C(1),
                                         UINT8_C(1)));
        } else {
            TI_C54X_TEST_CHECK(status ==
                               TI_C54X_OP_STATUS_INVALID_OPERAND);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before,
                                      sizeof(effect)) == 0);
        }
    }
}

static void
check_sp_gating(void)
{
    /* Taken branches gate known SP per the CALLD convention;
     * not-taken branches consume condition inputs only. */
    const ti_c54x_decoded_instruction taken =
        decode(UINT8_C(0x30), UINT16_C(0x2000));
    const ti_c54x_decoded_instruction not_taken =
        decode(UINT8_C(0x20), UINT16_C(0x2000));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1),
                  UINT16_C(0x0200), false);

    TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&taken, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&not_taken, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&effect, false);
}

static void
check_unknown_inputs(void)
{
    const ti_c54x_decoded_instruction tc_cond =
        decode(UINT8_C(0x30), UINT16_C(0x2000));
    const ti_c54x_decoded_instruction aeq =
        decode(UINT8_C(0x45), UINT16_C(0x2000));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    /* TC consumes only TC: unknown accumulators and C are ignored. */
    {
        ti_c54x_op_input_snapshot input =
            input_for(UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1),
                      UINT16_C(0x0200), true);

        input.accumulator_a_known = false;
        input.accumulator_b_known = false;
        input.status_c_known = false;
        TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&tc_cond, &input, &effect) ==
                           TI_C54X_OP_STATUS_OK);
        assert_effect(&effect, true);
    }

    /* Unknown TC rejects as UNKNOWN_INPUT without touching output. */
    {
        ti_c54x_op_input_snapshot input =
            input_for(UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1),
                      UINT16_C(0x0200), true);

        effect = before;
        input.status_tc_known = false;
        TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&tc_cond, &input, &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }

    /* AEQ consumes only A: unknown B/C/TC/SP are ignored. */
    {
        ti_c54x_op_input_snapshot input =
            input_for(UINT64_C(0x53), UINT64_C(0), UINT8_C(0),
                      UINT8_C(0), UINT16_C(0x0200), false);

        input.accumulator_b_known = false;
        input.status_c_known = false;
        input.status_tc_known = false;
        TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&aeq, &input, &effect) ==
                           TI_C54X_OP_STATUS_OK);
        assert_effect(&effect, false);
    }

    /* Unknown A rejects as UNKNOWN_INPUT without touching output. */
    {
        ti_c54x_op_input_snapshot input =
            input_for(UINT64_C(0x53), UINT64_C(0), UINT8_C(0),
                      UINT8_C(0), UINT16_C(0x0200), true);

        effect = before;
        input.accumulator_a_known = false;
        TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&aeq, &input, &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void
check_argument_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x30), UINT16_C(0x2000));
    const ti_c54x_op_input_snapshot valid =
        input_for(UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1),
                  UINT16_C(0x0200), true);
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x30), UINT16_C(0x2000));
    ti_c54x_op_effect effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_ccd(NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&instruction, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ccd(
                           &instruction, NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ccd(
                           &instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ccd(
                           &instruction, &operands, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void
check_decode_and_trusted_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(UINT8_C(0x30), UINT16_C(0x2000));
    const ti_c54x_op_input_snapshot valid =
        input_for(UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1),
                  UINT16_C(0x0200), true);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0x30), UINT16_C(0x2000));

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.repeatability = TI_C54X_REPEATABILITY_UNRESOLVED;
        TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_word_count = UINT8_C(1);
        TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        /* Flipping a condition-code bit selects a different
         * (deferred) condition, so the leaf rejects
         * INVALID_OPERAND, not INVALID_DECODE, without touching
         * the output. */
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0001);
        TI_C54X_TEST_CHECK(ti_c54x_op_ccd(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ccd(
                           &instruction, &operands, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0x30), UINT16_C(0x2000));
    operands.values[0].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ccd(
                           &instruction, &operands, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0x30), UINT16_C(0x2000));
    operands.values[1].width_bits = UINT8_C(15);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ccd(
                           &instruction, &operands, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_metadata();
    check_effects_and_parity();
    check_all_codes_classified();
    check_sp_gating();
    check_unknown_inputs();
    check_argument_rejection();
    check_decode_and_trusted_rejection();
    return 0;
}
