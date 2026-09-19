/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the exact three-word
 * MVDP *(abs),pmad WORD16 memory-move effect (P3-S72, TI
 * SPRU172C section MVDP with absolute addressing),
 * including absolute/pmad boundary values, checked/trusted
 * parity, non-cookie deferral, input-independence, and
 * transactional rejection. Does not own: catalog rows,
 * dispatch bindings, operand-layout descriptors, input
 * collection, address resolution, buses, CPU/state
 * mutation, sequencing, timing, observers, diagnostics,
 * or the production semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/store/op_mvdp_smem_absolute.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_decoded_instruction make_instruction(uint16_t absolute,
                                                    uint16_t pmad)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_MVDP_SMEM_ABSOLUTE;
    instruction.operand_layout =
        TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_ABSOLUTE16_PMAD;
    instruction.repeatability = TI_C54X_REPEATABILITY_INELIGIBLE;
    instruction.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
    instruction.bundle.program_words[0] = UINT16_C(0x7DF8);
    instruction.bundle.program_words[1] = absolute;
    instruction.bundle.program_words[2] = pmad;
    instruction.bundle.program_word_count = UINT8_C(3);
    return instruction;
}

static ti_c54x_trusted_operand_values trusted_operands(uint16_t absolute,
                                                       uint16_t pmad)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(4);
    operands.values[0].raw_value = UINT8_C(1);
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = UINT8_C(0x78);
    operands.values[1].width_bits = UINT8_C(7);
    operands.values[2].raw_value = absolute;
    operands.values[2].width_bits = UINT8_C(16);
    operands.values[3].raw_value = pmad;
    operands.values[3].width_bits = UINT8_C(16);
    return operands;
}

static ti_c54x_op_input_snapshot garbage_input(void)
{
    ti_c54x_op_input_snapshot input;

    memset(&input, 0xA5, sizeof(input));
    return input;
}

static void assert_absolute_move(const ti_c54x_op_effect *effect,
                                 uint16_t absolute, uint16_t pmad)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
                           &effect->payload.memory));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PROGRAM_PAIR_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.smem_absolute_program
                           .absolute == absolute);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.smem_absolute_program
                           .pmad == pmad);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(!effect->payload.memory.load_t_with_read);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void check_row_decode(void)
{
    const ti_c54x_decode_result absolute =
        ti_c54x_decode_first_word(UINT16_C(0x7DF8));
    const ti_c54x_decode_result indirect =
        ti_c54x_decode_first_word(UINT16_C(0x7D80));

    TI_C54X_TEST_CHECK(absolute.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(absolute.id ==
                       TI_C54X_OPCODE_ID_MVDP_SMEM_ABSOLUTE);
    TI_C54X_TEST_CHECK(indirect.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(indirect.id == TI_C54X_OPCODE_ID_MVDP_SMEM);
}

static void check_absolute_effects_and_parity(void)
{
    static const uint16_t addresses[] = {UINT16_C(0x0000),
                                         UINT16_C(0x0803),
                                         UINT16_C(0x87F0),
                                         UINT16_C(0xFFFF)};
    static const uint16_t pmads[] = {UINT16_C(0x0000), UINT16_C(0x0F00),
                                     UINT16_C(0xFF87), UINT16_C(0xFFFF)};

    for (size_t a = 0U; a < sizeof(addresses) / sizeof(addresses[0]); ++a) {
        for (size_t v = 0U; v < sizeof(pmads) / sizeof(pmads[0]); ++v) {
            const ti_c54x_decoded_instruction instruction =
                make_instruction(addresses[a], pmads[v]);
            const ti_c54x_trusted_operand_values operands =
                trusted_operands(addresses[a], pmads[v]);
            const ti_c54x_op_input_snapshot input = garbage_input();
            const ti_c54x_op_input_snapshot input_before = input;
            const ti_c54x_op_effect before = {
                .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
            ti_c54x_op_effect checked = before;
            ti_c54x_op_effect trusted = before;

            TI_C54X_TEST_CHECK(ti_c54x_op_mvdp_smem_absolute(
                                   &instruction, &input, &checked) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvdp_smem_absolute(
                                   &instruction, &operands, &input,
                                   &trusted) == TI_C54X_OP_STATUS_OK);
            assert_absolute_move(&checked, addresses[a], pmads[v]);
            assert_absolute_move(&trusted, addresses[a], pmads[v]);
            TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                                      sizeof(checked)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&input, &input_before,
                                      sizeof(input)) == 0);
        }
    }
}

static void check_cookie_deferral_and_parity(void)
{
    /* Only the 0x78 absolute selector composes here: I=0
     * direct words and other I=1 cookies reject. Both paths
     * leave the effect and input untouched. */
    static const uint16_t first_words[] = {UINT16_C(0x7D00),
                                           UINT16_C(0x7D80),
                                           UINT16_C(0x7DF7),
                                           UINT16_C(0x7DF9),
                                           UINT16_C(0x7DFF)};

    for (size_t w = 0U;
         w < sizeof(first_words) / sizeof(first_words[0]); ++w) {
        ti_c54x_decoded_instruction instruction =
            make_instruction(UINT16_C(0x0803), UINT16_C(0xFF87));
        ti_c54x_trusted_operand_values operands =
            trusted_operands(UINT16_C(0x0803), UINT16_C(0xFF87));
        const ti_c54x_op_input_snapshot input = garbage_input();
        const ti_c54x_op_input_snapshot input_before = input;
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0x5A5A)}};
        ti_c54x_op_effect checked = before;
        ti_c54x_op_effect trusted = before;

        instruction.bundle.program_words[0] = first_words[w];
        instruction.id = TI_C54X_OPCODE_ID_MVDP_SMEM_ABSOLUTE;
        /* The trusted path takes operands as authoritative and
         * ignores the instruction, so the forged cookie rides
         * the operand pair exactly as extraction would stage
         * it from the forged first word. */
        operands.values[0].raw_value =
            (uint64_t)((first_words[w] >> UINT16_C(7)) & UINT16_C(1));
        operands.values[1].raw_value =
            (uint64_t)(first_words[w] & UINT16_C(0x7F));
        TI_C54X_TEST_CHECK(ti_c54x_op_mvdp_smem_absolute(
                               &instruction, &input, &checked) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&checked, &before,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvdp_smem_absolute(
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
        make_instruction(UINT16_C(0x0803), UINT16_C(0xFF87));
    const ti_c54x_op_input_snapshot input = garbage_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;

#define CHECK_INVALID_DECODE(candidate)                                        \
    do {                                                                       \
        effect = before;                                                       \
        TI_C54X_TEST_CHECK(ti_c54x_op_mvdp_smem_absolute(                      \
                               &(candidate), &input, &effect) ==               \
                           TI_C54X_OP_STATUS_INVALID_DECODE);                 \
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);     \
    } while (0)

    TI_C54X_TEST_CHECK(ti_c54x_op_mvdp_smem_absolute(NULL, &input,
                                                      &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mvdp_smem_absolute(&instruction, NULL,
                                                      &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_mvdp_smem_absolute(&instruction, &input,
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
    forged.bundle.program_word_count = UINT8_C(2);
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.bundle.program_words[0] = UINT16_C(0x7D80);
    CHECK_INVALID_DECODE(forged);

#undef CHECK_INVALID_DECODE
}

static void check_trusted_operand_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(UINT16_C(0x0803), UINT16_C(0xFF87));
    const ti_c54x_op_input_snapshot input = garbage_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x5A5A)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT16_C(0x0803), UINT16_C(0xFF87));

    operands.count = UINT8_C(3);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvdp_smem_absolute(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands = trusted_operands(UINT16_C(0x0803), UINT16_C(0xFF87));
    operands.values[0].raw_value = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvdp_smem_absolute(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands = trusted_operands(UINT16_C(0x0803), UINT16_C(0xFF87));
    operands.values[1].raw_value = UINT8_C(0x77);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mvdp_smem_absolute(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_row_decode();
    check_absolute_effects_and_parity();
    check_cookie_deferral_and_parity();
    check_checked_rejection_is_transactional();
    check_trusted_operand_rejection_is_transactional();
    return 0;
}
