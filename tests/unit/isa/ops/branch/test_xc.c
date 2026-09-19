/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for one-word XC n,cond effects,
 * including taken NONE versus not-taken CONDITIONAL_SKIP emission
 * over the sixteen shared single conditions plus UNC, N-to-count
 * mapping, deferred-code rejection, consumed-input-only
 * knownness, checked/trusted/dispatch parity, and transactional
 * rejection.
 * Does not own: catalog rows, operand-layout descriptors, buses,
 * memory, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or the production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/branch/op_xc.h"
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

/* FORM-T2-18-07: 1111_11N1_CCCCCCCC with N at bit 9 selecting the
 * skip count and the low byte carrying the condition field. */
static uint16_t
xc_word0(uint8_t select, uint8_t cond)
{
    return (uint16_t)(UINT16_C(0xFD00) | ((uint16_t)(select & UINT8_C(1))
                                          << 9U) |
                      cond);
}

static ti_c54x_decoded_instruction
decode_word(uint16_t word)
{
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_XC);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_XC);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_N_CONDITION8);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(uint64_t acc_a, uint64_t acc_b, uint8_t c, uint8_t tc)
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
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t select, uint8_t cond)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = select & UINT8_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = cond;
    operands.values[1].width_bits = UINT8_C(8);
    return operands;
}

/* Independent oracle mirroring the ops contract: UNC takes, the
 * sixteen shared codes follow full-40-bit two's-complement signed
 * compares against zero. */
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
    case UINT8_C(0x00):
        return true;
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
    return false;
}

static void
assert_effect(const ti_c54x_op_effect *effect, bool taken, uint8_t select)
{
    if (taken) {
        TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_NONE);
        return;
    }
    TI_C54X_TEST_CHECK(effect->kind ==
                       TI_C54X_OP_EFFECT_CONDITIONAL_SKIP);
    TI_C54X_TEST_CHECK(effect->payload.conditional_skip.skip_words ==
                       (uint8_t)(select + UINT8_C(1)));
}

static void
check_case(uint8_t select, uint8_t cond, uint64_t acc_a, uint64_t acc_b,
           uint8_t c, uint8_t tc)
{
    const bool taken = expected_taken(cond, acc_a, acc_b, c, tc);
    ti_c54x_decoded_instruction instruction =
        decode_word(xc_word0(select, cond));
    ti_c54x_op_input_snapshot clean = input_for(acc_a, acc_b, c, tc);
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_XC);
    ti_c54x_trusted_operand_values operands = trusted_operands(select, cond);
    ti_c54x_op_effect checked = {.kind = TI_C54X_OP_EFFECT_PC_WRITE};
    ti_c54x_op_effect trusted = {.kind = TI_C54X_OP_EFFECT_PC_WRITE};
    ti_c54x_op_effect via_dispatch = {.kind = TI_C54X_OP_EFFECT_PC_WRITE};

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_xc(&instruction, &clean, &checked) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&checked, taken, select);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_xc(&instruction, &operands, &clean,
                                            &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &clean, &via_dispatch) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&via_dispatch, &checked, sizeof(checked)) == 0);
}

static void
test_metadata_identity(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_XC);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 333);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_XC);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xFD00));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFD00));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_N_CONDITION8);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xFD47)) == TI_C54X_OPCODE_ID_XC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xFF00)) == TI_C54X_OPCODE_ID_XC);
}

static void
test_single_conditions_both_counts(void)
{
    static const uint8_t conds[] = {
        UINT8_C(0x00), UINT8_C(0x30), UINT8_C(0x20), UINT8_C(0x0C),
        UINT8_C(0x08), UINT8_C(0x45), UINT8_C(0x44), UINT8_C(0x46),
        UINT8_C(0x42), UINT8_C(0x43), UINT8_C(0x47), UINT8_C(0x4D),
        UINT8_C(0x4C), UINT8_C(0x4E), UINT8_C(0x4A), UINT8_C(0x4B),
        UINT8_C(0x4F)};
    static const uint64_t acc_values[] = {UINT64_C(0),
                                          UINT64_C(1),
                                          UINT64_C(0xFFFFFFFFFF)};
    size_t ci;
    size_t ai;

    for (ci = 0U; ci < sizeof(conds) / sizeof(conds[0]); ++ci) {
        for (ai = 0U;
             ai < sizeof(acc_values) / sizeof(acc_values[0]); ++ai) {
            /* A carries the sweep value with B zero, C/TC clear. */
            check_case(UINT8_C(0), conds[ci], acc_values[ai], UINT64_C(0),
                       UINT8_C(0), UINT8_C(0));
            /* B carries the sweep value with A zero, C/TC set. */
            check_case(UINT8_C(1), conds[ci], UINT64_C(0), acc_values[ai],
                       UINT8_C(1), UINT8_C(1));
        }
    }
}

static void
test_ti_example_vector(void)
{
    /* TI SPRU172C §XC Example: XC 1,ALEQ with A holding -1 takes,
     * so the follower executes. */
    check_case(UINT8_C(0), UINT8_C(0x47), UINT64_C(0xFFFFFFFFFF),
               UINT64_C(0), UINT8_C(0), UINT8_C(0));
}

static void
test_unc_takes_without_inputs(void)
{
    ti_c54x_decoded_instruction instruction = decode_word(xc_word0(1, 0x00));
    ti_c54x_op_input_snapshot empty = {0};
    ti_c54x_op_effect effect = {.kind = TI_C54X_OP_EFFECT_PC_WRITE};

    TI_C54X_TEST_CHECK(ti_c54x_op_xc(&instruction, &empty, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&effect, true, UINT8_C(1));
}

static void
test_deferred_conditions_reject(void)
{
    static const uint8_t deferred[] = {
        UINT8_C(0x03), UINT8_C(0x02), UINT8_C(0x70), UINT8_C(0x60),
        UINT8_C(0x78), UINT8_C(0x68), UINT8_C(0x76), UINT8_C(0x3C)};
    size_t di;

    for (di = 0U; di < sizeof(deferred) / sizeof(deferred[0]); ++di) {
        ti_c54x_decoded_instruction instruction =
            decode_word(xc_word0(0, deferred[di]));
        ti_c54x_op_input_snapshot clean =
            input_for(UINT64_C(1), UINT64_C(1), UINT8_C(1), UINT8_C(1));
        ti_c54x_trusted_operand_values operands =
            trusted_operands(0, deferred[di]);
        ti_c54x_op_effect effect = {.kind = TI_C54X_OP_EFFECT_PC_WRITE};

        TI_C54X_TEST_CHECK(ti_c54x_op_xc(&instruction, &clean, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_PC_WRITE);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_xc(&instruction, &operands,
                                                &clean,
                                                &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_PC_WRITE);
    }
}

static void
test_unknown_inputs_stop(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_word(xc_word0(0, UINT8_C(0x46)));
    ti_c54x_op_input_snapshot input =
        input_for(UINT64_C(1), UINT64_C(0), UINT8_C(0), UINT8_C(0));
    ti_c54x_op_effect effect = {.kind = TI_C54X_OP_EFFECT_PC_WRITE};

    input.accumulator_a_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_xc(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_PC_WRITE);

    instruction = decode_word(xc_word0(0, UINT8_C(0x30)));
    input = input_for(UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(0));
    input.status_tc_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_op_xc(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_PC_WRITE);
}

static void
test_trusted_tamper_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_word(xc_word0(0, UINT8_C(0x47)));
    ti_c54x_op_input_snapshot clean =
        input_for(UINT64_C(0xFFFFFFFFFF), UINT64_C(0), UINT8_C(0),
                  UINT8_C(0));
    ti_c54x_trusted_operand_values operands = trusted_operands(1, 0x47);
    ti_c54x_op_effect effect = {.kind = TI_C54X_OP_EFFECT_PC_WRITE};

    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_xc(&instruction, &operands, &clean,
                                            &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_PC_WRITE);
}

static void
test_bad_identity_rejects(void)
{
    ti_c54x_decoded_instruction instruction =
        decode_word(xc_word0(0, UINT8_C(0x47)));
    ti_c54x_op_input_snapshot clean =
        input_for(UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(0));
    ti_c54x_op_effect effect = {.kind = TI_C54X_OP_EFFECT_PC_WRITE};

    instruction.id = TI_C54X_OPCODE_ID_RC;
    TI_C54X_TEST_CHECK(ti_c54x_op_xc(&instruction, &clean, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_PC_WRITE);
    TI_C54X_TEST_CHECK(ti_c54x_op_xc(NULL, &clean, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_xc(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
}

int
main(void)
{
    test_metadata_identity();
    test_single_conditions_both_counts();
    test_ti_example_vector();
    test_unc_takes_without_inputs();
    test_deferred_conditions_reject();
    test_unknown_inputs_stop();
    test_trusted_tamper_rejects();
    test_bad_identity_rejects();
    return 0;
}
