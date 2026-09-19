/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the exact two-word
 * LD *(abs),dst WORD16 load effect (P3-S75, TI SPRU172C
 * section LD with absolute addressing), including
 * destination select, absolute boundary values, SXM
 * widening parity, checked/trusted parity, non-cookie
 * deferral, and transactional rejection. Does not own:
 * catalog rows, dispatch bindings, operand-layout
 * descriptors, input collection, address resolution,
 * buses, CPU/state mutation, sequencing, timing,
 * observers, diagnostics, or the production semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/load/op_ld_smem_absolute.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction make_instruction(uint8_t destination,
                                                    uint16_t absolute)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_LD_SMEM_ABSOLUTE;
    instruction.operand_layout =
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16;
    instruction.repeatability = TI_C54X_REPEATABILITY_INELIGIBLE;
    instruction.input_resources = TI_C54X_INPUT_RESOURCES_STATUS_SXM;
    instruction.bundle.program_words[0] =
        (uint16_t)(UINT16_C(0x10F8) |
                   ((uint16_t)(destination & UINT8_C(1)) << 8));
    instruction.bundle.program_words[1] = absolute;
    instruction.bundle.program_word_count = UINT8_C(2);
    return instruction;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t destination,
                                                       uint16_t absolute)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = destination & UINT8_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT8_C(1);
    operands.values[1].width_bits = UINT8_C(1);
    operands.values[2].raw_value = UINT8_C(0x78);
    operands.values[2].width_bits = UINT8_C(7);
    operands.values[3].raw_value = absolute;
    operands.values[3].width_bits = UINT8_C(16);
    return operands;
}

static ti_c54x_op_input_snapshot sxm_input(uint8_t sxm)
{
    ti_c54x_op_input_snapshot input = {0};

    input.status_sxm_known = true;
    input.status_sxm = sxm;
    return input;
}

static void assert_absolute_load(const ti_c54x_op_effect *effect,
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
        ti_c54x_decode_first_word(UINT16_C(0x10F8));
    const ti_c54x_decode_result indirect =
        ti_c54x_decode_first_word(UINT16_C(0x1080));

    TI_C54X_TEST_CHECK(absolute.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(absolute.id ==
                       TI_C54X_OPCODE_ID_LD_SMEM_ABSOLUTE);
    TI_C54X_TEST_CHECK(indirect.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(indirect.id == TI_C54X_OPCODE_ID_LD_SMEM);
}

static void check_load_effects_and_parity(void)
{
    static const uint16_t addresses[] = {UINT16_C(0x0000),
                                         UINT16_C(0x087F),
                                         UINT16_C(0x2800),
                                         UINT16_C(0xFFFF)};

    for (size_t a = 0U; a < sizeof(addresses) / sizeof(addresses[0]); ++a) {
        for (uint8_t d = UINT8_C(0); d <= UINT8_C(1); ++d) {
            for (uint8_t s = UINT8_C(0); s <= UINT8_C(1); ++s) {
                const ti_c54x_decoded_instruction instruction =
                    make_instruction(d, addresses[a]);
                const ti_c54x_trusted_operand_values operands =
                    trusted_operands(d, addresses[a]);
                const ti_c54x_op_input_snapshot input = sxm_input(s);
                const ti_c54x_op_input_snapshot input_before = input;
                const ti_c54x_op_effect before = {
                    .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                    .payload.repeat_start = {.operand =
                                                 UINT16_C(0xA55A)}};
                ti_c54x_op_effect checked = before;
                ti_c54x_op_effect trusted = before;

                TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_absolute(
                                       &instruction, &input, &checked) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_smem_absolute(
                                       &instruction, &operands, &input,
                                       &trusted) == TI_C54X_OP_STATUS_OK);
                assert_absolute_load(&checked, addresses[a]);
                assert_absolute_load(&trusted, addresses[a]);
                TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                                          sizeof(checked)) == 0);
                TI_C54X_TEST_CHECK(memcmp(&input, &input_before,
                                          sizeof(input)) == 0);
            }
        }
    }
}

static void check_completion_widens_sxm(void)
{
    /* The read-value completion sign-extends under SXM=1 and
     * zero-fills under SXM=0, writing the selected
     * accumulator with C/TC preserved. */
    static const uint16_t words[] = {UINT16_C(0x0000), UINT16_C(0x7FFF),
                                     UINT16_C(0x8000), UINT16_C(0xFFFF)};

    for (size_t w = 0U; w < sizeof(words) / sizeof(words[0]); ++w) {
        for (uint8_t d = UINT8_C(0); d <= UINT8_C(1); ++d) {
            for (uint8_t s = UINT8_C(0); s <= UINT8_C(1); ++s) {
                const ti_c54x_decoded_instruction instruction =
                    make_instruction(d, UINT16_C(0x087F));
                const ti_c54x_op_input_snapshot input = sxm_input(s);
                ti_c54x_op_effect effect = {0};
                ti_c54x_op_effect next = {0};
                uint64_t expected = words[w];

                TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_absolute(
                                       &instruction, &input, &effect) ==
                                   TI_C54X_OP_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
                                       &effect.payload.memory));
                TI_C54X_TEST_CHECK(ti_c54x_ld_smem_absolute_complete(
                                       &instruction, &input, words[w],
                                       &next) == TI_C54X_OP_STATUS_OK);
                if (s != UINT8_C(0) &&
                    (words[w] & UINT16_C(0x8000)) != UINT16_C(0)) {
                    expected |= UINT64_C(0xFFFFFF0000);
                }
                TI_C54X_TEST_CHECK(next.kind ==
                                   TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE);
                TI_C54X_TEST_CHECK(next.payload.accumulator_write
                                       .destination == d);
                TI_C54X_TEST_CHECK(next.payload.accumulator_write
                                       .value_raw40 == expected);
            }
        }
    }
}

static void check_unknown_sxm_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT16_C(0x087F));
    const ti_c54x_op_input_snapshot input = {0};
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_absolute(&instruction, &input,
                                                    &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(ti_c54x_ld_smem_absolute_complete(
                           &instruction, &input, UINT16_C(0x1234),
                           &effect) == TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void check_cookie_deferral_and_parity(void)
{
    /* Only the 0x78 absolute selector composes here: I=0
     * direct words and other I=1 cookies reject. Both paths
     * leave the effect and input untouched. */
    static const uint16_t first_words[] = {UINT16_C(0x1000),
                                           UINT16_C(0x1080),
                                           UINT16_C(0x10F7),
                                           UINT16_C(0x10F9),
                                           UINT16_C(0x10FF)};

    for (size_t w = 0U;
         w < sizeof(first_words) / sizeof(first_words[0]); ++w) {
        ti_c54x_decoded_instruction instruction =
            make_instruction(UINT8_C(0), UINT16_C(0x087F));
        ti_c54x_trusted_operand_values operands =
            trusted_operands(UINT8_C(0), UINT16_C(0x087F));
        const ti_c54x_op_input_snapshot input = sxm_input(UINT8_C(0));
        const ti_c54x_op_input_snapshot input_before = input;
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0x5A5A)}};
        ti_c54x_op_effect checked = before;
        ti_c54x_op_effect trusted = before;

        instruction.bundle.program_words[0] = first_words[w];
        instruction.id = TI_C54X_OPCODE_ID_LD_SMEM_ABSOLUTE;
        operands.values[1].raw_value =
            (uint64_t)((first_words[w] >> UINT16_C(7)) & UINT16_C(1));
        operands.values[2].raw_value =
            (uint64_t)(first_words[w] & UINT16_C(0x7F));
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_absolute(
                               &instruction, &input, &checked) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&checked, &before,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_ld_smem_absolute(
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
        make_instruction(UINT8_C(0), UINT16_C(0x087F));
    const ti_c54x_op_input_snapshot input = sxm_input(UINT8_C(0));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;

#define CHECK_INVALID_DECODE(candidate)                                        \
    do {                                                                       \
        effect = before;                                                       \
        TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_absolute(                        \
                               &(candidate), &input, &effect) ==               \
                           TI_C54X_OP_STATUS_INVALID_DECODE);                 \
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);     \
    } while (0)

    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_absolute(NULL, &input,
                                                     &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_absolute(&instruction, NULL,
                                                     &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_ld_smem_absolute(&instruction, &input,
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
    forged.bundle.program_words[0] = UINT16_C(0x1080);
    CHECK_INVALID_DECODE(forged);

#undef CHECK_INVALID_DECODE
}

int main(void)
{
    check_row_decode();
    check_load_effects_and_parity();
    check_completion_widens_sxm();
    check_unknown_sxm_rejects();
    check_cookie_deferral_and_parity();
    check_checked_rejection_is_transactional();
    return 0;
}
