/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the exact two-word
 * DST src,*(abs) WORD32 memory-move effect (P3-S74, TI
 * SPRU172C section DST with absolute addressing),
 * including source select, absolute boundary values,
 * SST/SXM saturation parity with the MOD13 leaf,
 * checked/trusted parity, non-cookie deferral,
 * input-independence, and transactional rejection.
 * Does not own: catalog rows, dispatch bindings,
 * operand-layout descriptors, input collection, address
 * resolution, buses, CPU/state mutation, sequencing,
 * timing, observers, diagnostics, or the production
 * semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/store/op_dst_smem_absolute.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction make_instruction(uint8_t source,
                                                    uint16_t absolute)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_DST_SMEM_ABSOLUTE;
    instruction.operand_layout =
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16;
    instruction.repeatability = TI_C54X_REPEATABILITY_INELIGIBLE;
    instruction.input_resources =
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_SST;
    instruction.bundle.program_words[0] =
        (uint16_t)(UINT16_C(0x4EF8) |
                   ((uint16_t)(source & UINT8_C(1)) << 8));
    instruction.bundle.program_words[1] = absolute;
    instruction.bundle.program_word_count = UINT8_C(2);
    return instruction;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t source,
                                                       uint16_t absolute)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = source & UINT8_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT8_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT8_C(0x78);
    operands.values[2].width_bits = UINT8_C(7);
    operands.values[3].raw_value = absolute;
    operands.values[3].width_bits = UINT8_C(16);
    return operands;
}

static ti_c54x_op_input_snapshot saturated_input(uint64_t raw40,
                                                 bool use_b)
{
    ti_c54x_op_input_snapshot input = {0};

    input.status_sxm_known = true;
    input.status_sst_known = true;
    input.status_sxm = UINT8_C(0);
    input.status_sst = UINT8_C(0);
    if (use_b) {
        input.accumulator_b_known = true;
        input.accumulator_b_raw40 = raw40;
    } else {
        input.accumulator_a_known = true;
        input.accumulator_a_raw40 = raw40;
    }
    return input;
}

static void assert_absolute_store(const ti_c54x_op_effect *effect,
                                  uint16_t absolute, uint16_t msw,
                                  uint16_t lsw)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
                           &effect->payload.memory));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_LONG32_ABSOLUTE_STORE_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.long32_absolute
                           .absolute == absolute);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.first_value_msw == msw);
    TI_C54X_TEST_CHECK(effect->payload.memory.second_value_lsw == lsw);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete_long32 == NULL);
}

static void check_row_decode(void)
{
    const ti_c54x_decode_result absolute =
        ti_c54x_decode_first_word(UINT16_C(0x4EF8));

    TI_C54X_TEST_CHECK(absolute.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(absolute.id ==
                       TI_C54X_OPCODE_ID_DST_SMEM_ABSOLUTE);
}

static void check_store_effects_and_parity(void)
{
    /* S=0 reads A, S=1 reads B; low 32 bits split MSW/LSW. */
    static const uint64_t raws[] = {UINT64_C(0x0000000000),
                                    UINT64_C(0x0001234567),
                                    UINT64_C(0xFFEDCBA987),
                                    UINT64_C(0xFFFFFFFFFF)};
    static const uint16_t addresses[] = {UINT16_C(0x0000),
                                         UINT16_C(0x0060),
                                         UINT16_C(0x87F0),
                                         UINT16_C(0xFFFE)};

    for (size_t r = 0U; r < sizeof(raws) / sizeof(raws[0]); ++r) {
        for (size_t a = 0U; a < sizeof(addresses) / sizeof(addresses[0]);
             ++a) {
            for (uint8_t s = UINT8_C(0); s <= UINT8_C(1); ++s) {
                const ti_c54x_decoded_instruction instruction =
                    make_instruction(s, addresses[a]);
                const ti_c54x_trusted_operand_values operands =
                    trusted_operands(s, addresses[a]);
                const ti_c54x_op_input_snapshot input =
                    saturated_input(raws[r], s != UINT8_C(0));
                const ti_c54x_op_input_snapshot input_before = input;
                const ti_c54x_op_effect before = {
                    .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                    .payload.repeat_start = {.operand =
                                                 UINT16_C(0xA55A)}};
                ti_c54x_op_effect checked = before;
                ti_c54x_op_effect trusted = before;
                const uint16_t msw =
                    (uint16_t)((raws[r] >> 16U) & UINT64_C(0xFFFF));
                const uint16_t lsw =
                    (uint16_t)(raws[r] & UINT64_C(0xFFFF));

                TI_C54X_TEST_CHECK(ti_c54x_op_dst_smem_absolute(
                                       &instruction, &input, &checked) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dst_smem_absolute(
                                       &instruction, &operands, &input,
                                       &trusted) == TI_C54X_OP_STATUS_OK);
                assert_absolute_store(&checked, addresses[a], msw, lsw);
                assert_absolute_store(&trusted, addresses[a], msw, lsw);
                TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                                          sizeof(checked)) == 0);
                TI_C54X_TEST_CHECK(memcmp(&input, &input_before,
                                          sizeof(input)) == 0);
            }
        }
    }
}

static void check_saturation_parity(void)
{
    /* SST=1 with SXM=0 clamps above 32 bits to all-ones. */
    const ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT16_C(0x0060));
    const ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT16_C(0x0060));
    ti_c54x_op_input_snapshot input =
        saturated_input(UINT64_C(0x0112345678), false);
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};

    input.status_sst = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_dst_smem_absolute(&instruction, &input,
                                                    &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dst_smem_absolute(
                           &instruction, &operands, &input,
                           &trusted) == TI_C54X_OP_STATUS_OK);
    assert_absolute_store(&checked, UINT16_C(0x0060), UINT16_C(0xFFFF),
                          UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(memcmp(&checked, &trusted, sizeof(checked)) == 0);

    /* SST=1 with SXM=1 saturates the positive overflow. */
    input = saturated_input(UINT64_C(0x0180000000), false);
    input.status_sst = UINT8_C(1);
    input.status_sxm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_op_dst_smem_absolute(&instruction, &input,
                                                    &checked) ==
                       TI_C54X_OP_STATUS_OK);
    assert_absolute_store(&checked, UINT16_C(0x0060), UINT16_C(0x7FFF),
                          UINT16_C(0xFFFF));
}

static void check_unknown_inputs_reject(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT16_C(0x0060));
    const ti_c54x_op_input_snapshot input = {0};
    ti_c54x_op_effect effect = {0};

    /* Zeroed snapshot: accumulator and SXM/SST unknown. */
    TI_C54X_TEST_CHECK(ti_c54x_op_dst_smem_absolute(&instruction, &input,
                                                    &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void check_cookie_deferral_and_parity(void)
{
    /* Only the 0x78 absolute selector composes here: I=0
     * direct words and other I=1 cookies reject. Both paths
     * leave the effect and input untouched. */
    static const uint16_t first_words[] = {UINT16_C(0x4E00),
                                           UINT16_C(0x4E80),
                                           UINT16_C(0x4EF7),
                                           UINT16_C(0x4EF9),
                                           UINT16_C(0x4EFF)};

    for (size_t w = 0U;
         w < sizeof(first_words) / sizeof(first_words[0]); ++w) {
        ti_c54x_decoded_instruction instruction =
            make_instruction(UINT8_C(0), UINT16_C(0x0060));
        ti_c54x_trusted_operand_values operands =
            trusted_operands(UINT8_C(0), UINT16_C(0x0060));
        const ti_c54x_op_input_snapshot input =
            saturated_input(UINT64_C(0x0001234567), false);
        const ti_c54x_op_input_snapshot input_before = input;
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0x5A5A)}};
        ti_c54x_op_effect checked = before;
        ti_c54x_op_effect trusted = before;

        instruction.bundle.program_words[0] = first_words[w];
        instruction.id = TI_C54X_OPCODE_ID_DST_SMEM_ABSOLUTE;
        operands.values[1].raw_value =
            (uint64_t)((first_words[w] >> UINT16_C(7)) & UINT16_C(1));
        operands.values[2].raw_value =
            (uint64_t)(first_words[w] & UINT16_C(0x7F));
        TI_C54X_TEST_CHECK(ti_c54x_op_dst_smem_absolute(
                               &instruction, &input, &checked) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&checked, &before,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_dst_smem_absolute(
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
        make_instruction(UINT8_C(0), UINT16_C(0x0060));
    const ti_c54x_op_input_snapshot input =
        saturated_input(UINT64_C(0x0001234567), false);
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;

#define CHECK_INVALID_DECODE(candidate)                                        \
    do {                                                                       \
        effect = before;                                                       \
        TI_C54X_TEST_CHECK(ti_c54x_op_dst_smem_absolute(                       \
                               &(candidate), &input, &effect) ==               \
                           TI_C54X_OP_STATUS_INVALID_DECODE);                 \
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);     \
    } while (0)

    TI_C54X_TEST_CHECK(ti_c54x_op_dst_smem_absolute(NULL, &input,
                                                     &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_dst_smem_absolute(&instruction, NULL,
                                                     &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_dst_smem_absolute(&instruction, &input,
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
    forged.bundle.program_word_count = UINT8_C(1);
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.bundle.program_words[0] = UINT16_C(0x4E80);
    CHECK_INVALID_DECODE(forged);

#undef CHECK_INVALID_DECODE
}

int main(void)
{
    check_row_decode();
    check_store_effects_and_parity();
    check_saturation_parity();
    check_unknown_inputs_reject();
    check_cookie_deferral_and_parity();
    check_checked_rejection_is_transactional();
    return 0;
}
