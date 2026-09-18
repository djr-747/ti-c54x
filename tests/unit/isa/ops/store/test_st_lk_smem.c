/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for the row-bound direct-only two-word
 * ST #lk,Smem WORD16 memory-write effect, including direct boundary
 * values, checked/trusted parity, indirect deferral (MOD0--MOD3 rides
 * ST_IMMEDIATE_SMEM, MOD4+ defers), input-independence, and
 * transactional rejection. Does not own: catalog rows, dispatch bindings,
 * operand-layout descriptors, input collection, address resolution, buses,
 * CPU/state mutation, sequencing, timing, observers, diagnostics, or the
 * production semantic leaf.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/store/op_st_lk_smem.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t st_lk_first_word(uint8_t indirect, uint8_t addr7)
{
    return (uint16_t)(UINT16_C(0x7600) |
                      (uint16_t)(((uint16_t)(indirect & UINT8_C(1)))
                                 << 7) |
                      (uint16_t)(addr7 & UINT8_C(0x7F)));
}

static ti_c54x_decoded_instruction make_instruction(uint8_t indirect,
                                                    uint8_t addr7,
                                                    uint16_t lk)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_ST_LK_SMEM;
    instruction.operand_layout =
        TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_EXTENSION16;
    instruction.repeatability = TI_C54X_REPEATABILITY_ELIGIBLE;
    instruction.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
    instruction.bundle.program_words[0] =
        st_lk_first_word(indirect, addr7);
    instruction.bundle.program_words[1] = lk;
    instruction.bundle.program_word_count = UINT8_C(2);
    return instruction;
}

static ti_c54x_op_input_snapshot garbage_input(void)
{
    ti_c54x_op_input_snapshot input;

    memset(&input, 0xA5, sizeof(input));
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t indirect,
                                                       uint8_t addr7,
                                                       uint16_t lk)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(3);
    operands.values[0].raw_value = indirect;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = addr7;
    operands.values[1].width_bits = UINT8_C(7);
    operands.values[2].raw_value = lk;
    operands.values[2].width_bits = UINT8_C(16);
    return operands;
}

static void assert_direct_write(const ti_c54x_op_effect *effect,
                                uint8_t addr7, uint16_t lk)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(ti_c54x_op_memory_effect_is_valid(
                           &effect->payload.memory));
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect->payload.memory.address_spec.direct_dma7 ==
                       addr7);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == lk);
    TI_C54X_TEST_CHECK(!effect->payload.memory.load_t_with_read);
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
}

static void check_row_decode(void)
{
    const ti_c54x_decode_result direct =
        ti_c54x_decode_first_word(st_lk_first_word(UINT8_C(0), UINT8_C(0)));
    /* 0x7690 belongs to the carved-out ST_IMMEDIATE_SMEM row, and
     * 0x76E0 (MOD4+) is undecoded. */
    const ti_c54x_decode_result carved =
        ti_c54x_decode_first_word(UINT16_C(0x7690));
    const ti_c54x_decode_result deferred =
        ti_c54x_decode_first_word(
            st_lk_first_word(UINT8_C(1), UINT8_C(0x60)));

    TI_C54X_TEST_CHECK(direct.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(direct.id == TI_C54X_OPCODE_ID_ST_LK_SMEM);
    TI_C54X_TEST_CHECK(carved.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(carved.id == TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);
    TI_C54X_TEST_CHECK(deferred.status ==
                       TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
    TI_C54X_TEST_CHECK(deferred.id == TI_C54X_OPCODE_ID_UNKNOWN);
}

static void check_direct_effects_and_parity(void)
{
    static const uint8_t addresses[] = {UINT8_C(0x00), UINT8_C(0x01),
                                        UINT8_C(0x20), UINT8_C(0x7E),
                                        UINT8_C(0x7F)};
    static const uint16_t values[] = {UINT16_C(0x0000), UINT16_C(0x0001),
                                      UINT16_C(0x7FFF), UINT16_C(0x8000),
                                      UINT16_C(0xFFFF)};

    for (size_t a = 0U; a < sizeof(addresses) / sizeof(addresses[0]); ++a) {
        for (size_t v = 0U; v < sizeof(values) / sizeof(values[0]); ++v) {
            const ti_c54x_decoded_instruction instruction = make_instruction(
                UINT8_C(0), addresses[a], values[v]);
            const ti_c54x_trusted_operand_values operands =
                trusted_operands(UINT8_C(0), addresses[a], values[v]);
            const ti_c54x_op_input_snapshot input = garbage_input();
            const ti_c54x_op_input_snapshot input_before = input;
            const ti_c54x_op_effect before = {
                .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
            ti_c54x_op_effect checked = before;
            ti_c54x_op_effect trusted = before;

            TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                               st_lk_first_word(UINT8_C(0), addresses[a]));
            TI_C54X_TEST_CHECK(ti_c54x_op_st_lk_smem(
                                   &instruction, &input, &checked) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_lk_smem(
                                   &instruction, &operands, &input,
                                   &trusted) == TI_C54X_OP_STATUS_OK);
            assert_direct_write(&checked, addresses[a], values[v]);
            assert_direct_write(&trusted, addresses[a], values[v]);
            TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                                      sizeof(checked)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&input, &input_before,
                                      sizeof(input)) == 0);
        }
    }
}

static void check_indirect_deferral_and_parity(void)
{
    /* I=1 words never compose here: the checked mask pins I=0
     * (INVALID_DECODE) while the trusted entry rejects indirect
     * outright (INVALID_OPERAND). MOD0--MOD3 composes through
     * ST_IMMEDIATE_SMEM; MOD4+ defers to later mode rows. Both
     * paths leave the effect and input untouched. */
    static const uint8_t addresses[] = {UINT8_C(0x00), UINT8_C(0x05),
                                        UINT8_C(0x15), UINT8_C(0x1B),
                                        UINT8_C(0x3F), UINT8_C(0x7F)};
    static const uint16_t values[] = {UINT16_C(0x0000), UINT16_C(0x4210),
                                      UINT16_C(0xFFFF)};

    for (size_t a = 0U; a < sizeof(addresses) / sizeof(addresses[0]); ++a) {
        for (size_t v = 0U; v < sizeof(values) / sizeof(values[0]); ++v) {
            const ti_c54x_decoded_instruction instruction =
                make_instruction(UINT8_C(1), addresses[a], values[v]);
            const ti_c54x_trusted_operand_values operands =
                trusted_operands(UINT8_C(1), addresses[a], values[v]);
            const ti_c54x_op_input_snapshot input = garbage_input();
            const ti_c54x_op_input_snapshot input_before = input;
            const ti_c54x_op_effect before = {
                .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                .payload.repeat_start = {.operand = UINT16_C(0x5A5A)}};
            ti_c54x_op_effect checked = before;
            ti_c54x_op_effect trusted = before;

            TI_C54X_TEST_CHECK(
                ti_c54x_op_st_lk_smem(&instruction, &input, &checked) ==
                TI_C54X_OP_STATUS_INVALID_DECODE);
            TI_C54X_TEST_CHECK(memcmp(&checked, &before,
                                      sizeof(checked)) == 0);
            TI_C54X_TEST_CHECK(
                ti_c54x_trusted_op_st_lk_smem(&instruction, &operands,
                                             &input, &trusted) ==
                TI_C54X_OP_STATUS_INVALID_OPERAND);
            TI_C54X_TEST_CHECK(memcmp(&trusted, &before,
                                      sizeof(trusted)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&input, &input_before,
                                      sizeof(input)) == 0);
        }
    }
}

static void check_checked_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT8_C(0x10), UINT16_C(0x4210));
    const ti_c54x_op_input_snapshot input = garbage_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_decoded_instruction forged;

#define CHECK_INVALID_DECODE(candidate)                                         \
    do {                                                                        \
        effect = before;                                                        \
        TI_C54X_TEST_CHECK(ti_c54x_op_st_lk_smem(                                \
                               &(candidate), &input, &effect) ==                \
                           TI_C54X_OP_STATUS_INVALID_DECODE);                  \
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);      \
    } while (0)

    TI_C54X_TEST_CHECK(ti_c54x_op_st_lk_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_lk_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_st_lk_smem(&instruction, &input, NULL) ==
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
    forged.bundle.program_words[0] ^= UINT16_C(0x0100);
    CHECK_INVALID_DECODE(forged);
    forged = instruction;
    forged.bundle.program_words[1] = UINT16_C(0x0000);
    forged.bundle.program_words[0] =
        st_lk_first_word(UINT8_C(1), UINT8_C(0x7F));
    /* I=1 fails the direct-only mask before the leaf's own
     * indirect deferral is reached; the trusted entry pins the
     * INVALID_OPERAND deferral in check_indirect_deferral. */
    TI_C54X_TEST_CHECK(ti_c54x_op_st_lk_smem(&forged, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_DECODE);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

#undef CHECK_INVALID_DECODE
}

static void check_trusted_operand_rejection_is_transactional(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(UINT8_C(0), UINT8_C(0x7F), UINT16_C(0x8000));
    const ti_c54x_op_input_snapshot input = garbage_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(0), UINT8_C(0x7F), UINT16_C(0x8000));

    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_lk_smem(&instruction, NULL,
                                                    &input,
                                                    &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_lk_smem(&instruction, &operands,
                                                    NULL,
                                                    &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_lk_smem(&instruction, &operands,
                                                    &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

    operands.count = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_lk_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(0x7F),
                                UINT16_C(0x8000));
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_lk_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(0x7F),
                                UINT16_C(0x8000));
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_lk_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(0), UINT8_C(0x7F),
                                UINT16_C(0x8000));
    operands.values[1].raw_value = UINT64_C(0x80);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_st_lk_smem(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_row_decode();
    check_direct_effects_and_parity();
    check_indirect_deferral_and_parity();
    check_checked_rejection_is_transactional();
    check_trusted_operand_rejection_is_transactional();
    return 0;
}
