/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the exact two-word
 * SUB *(abs),src WORD16 memory-read effect (P3-S82,
 * TI SPRU172C section SUB syntax 1 with absolute
 * addressing), including source select, absolute
 * boundary values, SXM-widened subtraction with OVM
 * saturation and borrow/overflow status parity with
 * the SUB_SMEM leaf, checked/trusted parity,
 * non-cookie deferral, input-independence, and
 * transactional rejection.
 * Does not own: catalog rows, dispatch bindings,
 * operand-layout descriptors, input collection, address
 * resolution, buses, CPU/state mutation, sequencing,
 * timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/arithmetic/op_sub_smem_absolute.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction make_instruction(uint8_t select,
                                                    uint16_t absolute)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_SUB_SMEM_ABSOLUTE;
    instruction.operand_layout =
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16;
    instruction.repeatability = TI_C54X_REPEATABILITY_INELIGIBLE;
    instruction.input_resources =
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM;
    instruction.bundle.program_words[0] =
        (uint16_t)(UINT16_C(0x08F8) |
                   ((uint16_t)(select & UINT8_C(1)) << 8));
    instruction.bundle.program_words[1] = absolute;
    instruction.bundle.program_word_count = UINT8_C(2);
    return instruction;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t select,
                                                       uint16_t absolute)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = select & UINT8_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT8_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT8_C(0x78);
    operands.values[2].width_bits = UINT8_C(7);
    operands.values[3].raw_value = absolute;
    operands.values[3].width_bits = UINT8_C(16);
    return operands;
}

static ti_c54x_op_input_snapshot sub_input(uint8_t select, uint64_t base,
                                           uint8_t sxm, uint8_t ovm)
{
    ti_c54x_op_input_snapshot input = {0};

    if (select == UINT8_C(0)) {
        input.accumulator_a_known = true;
        input.accumulator_a_raw40 = (ti_c54x_u40_t)base;
    } else {
        input.accumulator_b_known = true;
        input.accumulator_b_raw40 = (ti_c54x_u40_t)base;
    }
    input.status_sxm_known = true;
    input.status_sxm = sxm;
    input.status_ovm_known = true;
    input.status_ovm = ovm;
    return input;
}

static void assert_absolute_read(const ti_c54x_op_effect *effect,
                                 uint16_t absolute)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
                           &effect->payload.memory));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.absolute_data
                           .address == absolute);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete != NULL);
}

static void check_row_decode(void)
{
    const ti_c54x_decode_result absolute =
        ti_c54x_decode_first_word(UINT16_C(0x08F8));

    TI_C54X_TEST_CHECK(absolute.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(absolute.id == TI_C54X_OPCODE_ID_SUB_SMEM_ABSOLUTE);
}

static void check_sub_effects_and_parity(void)
{
    /* S=0 reads A, S=1 reads B; source, SXM, and OVM
     * must be known before any bus traffic. Staged word
     * stays zero with the SUB completion chained by the
     * root. */
    static const uint16_t addresses[] = {UINT16_C(0x0000),
                                         UINT16_C(0x0881),
                                         UINT16_C(0x87F0),
                                         UINT16_C(0xFFFF)};
    static const uint64_t bases[] = {UINT64_C(0x0000000000),
                                     UINT64_C(0x0012345678)};

    for (size_t a = 0U; a < sizeof(addresses) / sizeof(addresses[0]);
         ++a) {
        for (uint8_t d = UINT8_C(0); d <= UINT8_C(1); ++d) {
            for (uint8_t sxm = UINT8_C(0); sxm <= UINT8_C(1);
                 ++sxm) {
                for (uint8_t ovm = UINT8_C(0); ovm <= UINT8_C(1);
                     ++ovm) {
                    for (size_t v = 0U;
                         v < sizeof(bases) / sizeof(bases[0]);
                         ++v) {
                        const ti_c54x_decoded_instruction instruction =
                            make_instruction(d, addresses[a]);
                        const ti_c54x_trusted_operand_values operands =
                            trusted_operands(d, addresses[a]);
                        const ti_c54x_op_input_snapshot input =
                            sub_input(d, bases[v], sxm, ovm);
                        const ti_c54x_op_input_snapshot input_before =
                            input;
                        const ti_c54x_op_effect before = {
                            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                            .payload.repeat_start = {.operand =
                                                         UINT16_C(
                                                             0xA55A)}};
                        ti_c54x_op_effect checked = before;
                        ti_c54x_op_effect trusted = before;

                        TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_absolute(
                                               &instruction, &input,
                                               &checked) ==
                                           TI_C54X_OP_STATUS_OK);
                        TI_C54X_TEST_CHECK(
                            ti_c54x_trusted_op_sub_smem_absolute(
                                &instruction, &operands, &input,
                                &trusted) == TI_C54X_OP_STATUS_OK);
                        assert_absolute_read(&checked, addresses[a]);
                        assert_absolute_read(&trusted, addresses[a]);
                        TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                                                  sizeof(checked)) == 0);
                        TI_C54X_TEST_CHECK(memcmp(&input, &input_before,
                                                  sizeof(input)) == 0);
                    }
                }
            }
        }
    }
}

static void assert_status_delta(const ti_c54x_op_effect *next,
                                ti_c54x_op_status_bit_action c_action,
                                bool ov_write)
{
    TI_C54X_TEST_CHECK(next->payload.accumulator_write.status_delta
                           .c_action == c_action);
    TI_C54X_TEST_CHECK(next->payload.accumulator_write.status_delta
                           .tc_action ==
                       TI_C54X_OP_STATUS_BIT_PRESERVE);
    TI_C54X_TEST_CHECK(next->payload.accumulator_write.status_delta
                           .destination_overflow_action ==
                       (ov_write
                            ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                            : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE));
}

static void check_completion_sub(void)
{
    /* 0 - 1 borrows with no overflow: full-width wrap,
     * C clear, OV preserved, TC preserved. */
    const ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT16_C(0x0881));
    const ti_c54x_op_input_snapshot borrow =
        sub_input(UINT8_C(0), UINT64_C(0x0000000000), UINT8_C(0),
                  UINT8_C(0));
    const ti_c54x_op_input_snapshot clean =
        sub_input(UINT8_C(0), UINT64_C(0x0012345678), UINT8_C(0),
                  UINT8_C(0));
    ti_c54x_op_effect next = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sub_smem_absolute_complete(
                           &instruction, &borrow, UINT16_C(0x0001),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.kind ==
                       TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                       UINT8_C(0));
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0xFFFFFFFFFF));
    assert_status_delta(&next, TI_C54X_OP_STATUS_BIT_WRITE_0, false);

    /* 0x0012345678 - 0x1234 retires clean with C set. */
    TI_C54X_TEST_CHECK(ti_c54x_sub_smem_absolute_complete(
                           &instruction, &clean, UINT16_C(0x1234),
                           &next) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                       UINT64_C(0x0012344444));
    assert_status_delta(&next, TI_C54X_OP_STATUS_BIT_WRITE_1, false);

    /* SXM=1 widens 0x8001 before subtracting:
     * 0 - (-32767) = 32767 with a low-32 borrow. */
    {
        const ti_c54x_op_input_snapshot widen =
            sub_input(UINT8_C(0), UINT64_C(0x0000000000), UINT8_C(1),
                      UINT8_C(0));

        TI_C54X_TEST_CHECK(ti_c54x_sub_smem_absolute_complete(
                               &instruction, &widen, UINT16_C(0x8001),
                               &next) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                           UINT64_C(0x0000007FFF));
        assert_status_delta(&next, TI_C54X_OP_STATUS_BIT_WRITE_0,
                            false);
    }

    /* OVM=1 saturates the positive overflow back to
     * 0x007FFFFFFF with OV written. */
    {
        const ti_c54x_op_input_snapshot saturate =
            sub_input(UINT8_C(0), UINT64_C(0x007FFFFFFF), UINT8_C(1),
                      UINT8_C(1));

        TI_C54X_TEST_CHECK(ti_c54x_sub_smem_absolute_complete(
                               &instruction, &saturate, UINT16_C(0xFFFF),
                               &next) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                           UINT64_C(0x007FFFFFFF));
        assert_status_delta(&next, TI_C54X_OP_STATUS_BIT_WRITE_0,
                            true);
    }

    /* OVM=1 saturates the negative overflow back to
     * 0xFF80000000 with OV written and C set. */
    {
        const ti_c54x_op_input_snapshot saturate =
            sub_input(UINT8_C(0), UINT64_C(0xFF80000000), UINT8_C(0),
                      UINT8_C(1));

        TI_C54X_TEST_CHECK(ti_c54x_sub_smem_absolute_complete(
                               &instruction, &saturate, UINT16_C(0x0001),
                               &next) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                           UINT64_C(0xFF80000000));
        assert_status_delta(&next, TI_C54X_OP_STATUS_BIT_WRITE_1,
                            true);
    }

    /* S=1 writes B. */
    {
        const ti_c54x_decoded_instruction b_instruction =
            make_instruction(UINT8_C(1), UINT16_C(0x0881));
        const ti_c54x_op_input_snapshot b_input =
            sub_input(UINT8_C(1), UINT64_C(0x0012345678), UINT8_C(0),
                      UINT8_C(0));

        TI_C54X_TEST_CHECK(ti_c54x_sub_smem_absolute_complete(
                               &b_instruction, &b_input, UINT16_C(0x1234),
                               &next) == TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(next.payload.accumulator_write.destination ==
                           UINT8_C(1));
        TI_C54X_TEST_CHECK(next.payload.accumulator_write.value_raw40 ==
                           UINT64_C(0x0012344444));
    }

    {
        const ti_c54x_op_input_snapshot unknown = {0};

        TI_C54X_TEST_CHECK(ti_c54x_sub_smem_absolute_complete(
                               &instruction, &unknown, UINT16_C(0x1234),
                               &next) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    }
}

static void check_unknown_inputs_reject(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT16_C(0x0881));
    const ti_c54x_op_input_snapshot input = {0};
    ti_c54x_op_effect effect = {0};

    /* Zeroed snapshot: source accumulator unknown. */
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_absolute(
                           &instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void check_unknown_sxm_ovm_reject(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT16_C(0x0881));
    ti_c54x_op_effect effect = {0};

    /* Source known, SXM unknown. */
    {
        ti_c54x_op_input_snapshot input =
            sub_input(UINT8_C(0), UINT64_C(0x0012345678), UINT8_C(0),
                      UINT8_C(0));

        input.status_sxm_known = false;
        TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_absolute(
                               &instruction, &input, &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    }

    /* Source and SXM known, OVM unknown. */
    {
        ti_c54x_op_input_snapshot input =
            sub_input(UINT8_C(0), UINT64_C(0x0012345678), UINT8_C(0),
                      UINT8_C(0));

        input.status_ovm_known = false;
        TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_absolute(
                               &instruction, &input, &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    }

    /* Out-of-range SXM/OVM values reject as operands. */
    {
        ti_c54x_op_input_snapshot input =
            sub_input(UINT8_C(0), UINT64_C(0x0012345678), UINT8_C(2),
                      UINT8_C(0));

        TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_absolute(
                               &instruction, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
    }
    {
        ti_c54x_op_input_snapshot input =
            sub_input(UINT8_C(0), UINT64_C(0x0012345678), UINT8_C(0),
                      UINT8_C(2));

        TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_absolute(
                               &instruction, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
    }
}

static void check_cookie_deferral_and_parity(void)
{
    /* Only the 0x78 absolute selector composes here: I=0
     * direct words and other I=1 cookies reject. Both paths
     * leave the effect and input untouched. */
    static const uint16_t first_words[] = {UINT16_C(0x0800),
                                           UINT16_C(0x0880),
                                           UINT16_C(0x08F7),
                                           UINT16_C(0x08F9),
                                           UINT16_C(0x08FF)};

    for (size_t w = 0U;
         w < sizeof(first_words) / sizeof(first_words[0]); ++w) {
        ti_c54x_decoded_instruction instruction =
            make_instruction(UINT8_C(0), UINT16_C(0x0881));
        ti_c54x_trusted_operand_values operands =
            trusted_operands(UINT8_C(0), UINT16_C(0x0881));
        const ti_c54x_op_input_snapshot input =
            sub_input(UINT8_C(0), UINT64_C(0x0012345678), UINT8_C(0),
                      UINT8_C(0));
        const ti_c54x_op_input_snapshot input_before = input;
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0x5A5A)}};
        ti_c54x_op_effect checked = before;
        ti_c54x_op_effect trusted = before;

        instruction.bundle.program_words[0] = first_words[w];
        instruction.id = TI_C54X_OPCODE_ID_SUB_SMEM_ABSOLUTE;
        operands.values[1].raw_value =
            (uint64_t)((first_words[w] >> UINT16_C(7)) & UINT16_C(1));
        operands.values[2].raw_value =
            (uint64_t)(first_words[w] & UINT16_C(0x7F));
        TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_absolute(
                               &instruction, &input, &checked) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&checked, &before,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_sub_smem_absolute(
                               &instruction, &operands, &input,
                               &trusted) == TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&trusted, &before,
                                  sizeof(trusted)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&input, &input_before,
                                  sizeof(input)) == 0);
    }
}

static void check_checked_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT16_C(0x0881));
    const ti_c54x_op_input_snapshot input =
        sub_input(UINT8_C(0), UINT64_C(0x0012345678), UINT8_C(0),
                  UINT8_C(0));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;

#define CHECK_INVALID_DECODE(candidate)                                        \
    do {                                                                       \
        effect = before;                                                       \
        TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_absolute(                       \
                               &(candidate), &input, &effect) ==               \
                           TI_C54X_OP_STATUS_INVALID_DECODE);                 \
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);     \
    } while (0)

    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_absolute(
                           NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_absolute(
                           &instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_sub_smem_absolute(
                           &instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    forged = instruction;
    forged.id = TI_C54X_OPCODE_ID_NOP;
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.bundle.program_word_count = UINT8_C(1);
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.bundle.program_words[0] = UINT16_C(0x0880);
    CHECK_INVALID_DECODE(forged);

#undef CHECK_INVALID_DECODE
}

int main(void)
{
    check_row_decode();
    check_sub_effects_and_parity();
    check_completion_sub();
    check_unknown_inputs_reject();
    check_unknown_sxm_ovm_reject();
    check_cookie_deferral_and_parity();
    check_checked_rejection_is_transactional();
    return 0;
}
