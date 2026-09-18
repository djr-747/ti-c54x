/*
 * Module contract
 *
 * Owns: TI-pinned proofs for the row-less DELAY Smem leaf, including the
 * indirect worked example shape (MOD1 on AR3 holding the 0x6CAC Smem word),
 * the direct-addressing variant, the Smem delay write-back flag with a null
 * completion on every staged read, exotic-mode deferral, checked/trusted
 * parity, ignored accumulator/status inputs, and transactional rejection.
 * DELAY publishes nothing but the delay write: no T-register, accumulator,
 * or status assertions appear here.
 * Does not own: catalog rows, operand-layout descriptors, address
 * resolution, buses, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/ops/load/op_delay_smem.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* TI worked example shape: DELAY with the indirect flag set, MOD1 on AR3.
 * The Smem word under test holds 0x6CAC; the WORD16 root copies it to
 * DATA Smem+1 while Smem itself is unchanged. */
#define DELAY_TI_WORD UINT16_C(0x4D8B)

/* Direct-addressing variant: page-0 Smem 0x2A. */
#define DELAY_DIRECT_WORD UINT16_C(0x4D2A)

static ti_c54x_decoded_instruction make_instruction(uint16_t word,
                                                    uint8_t word_count)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_DELAY_SMEM;
    instruction.operand_layout = TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7;
    instruction.repeatability = TI_C54X_REPEATABILITY_UNRESOLVED;
    instruction.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
    instruction.bundle.program_words[0] = word;
    instruction.bundle.program_words[1] = UINT16_C(0);
    instruction.bundle.program_word_count = word_count;
    return instruction;
}

static ti_c54x_op_input_snapshot clean_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    return input;
}

static ti_c54x_op_input_snapshot noisy_input(void)
{
    ti_c54x_op_input_snapshot input = {0};

    input.accumulator_a_known = true;
    input.accumulator_a_raw40 = UINT64_C(0xFFFFFFFFFF);
    input.accumulator_b_known = true;
    input.accumulator_b_raw40 = UINT64_C(0x123456789A);
    input.status_frct_known = true;
    input.status_frct = UINT8_C(1);
    input.status_ovm_known = true;
    input.status_ovm = UINT8_C(1);
    input.status_c_known = true;
    input.status_c = UINT8_C(1);
    return input;
}

static ti_c54x_trusted_operand_values trusted_operands(uint8_t indirect,
                                                       uint8_t addr7)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = indirect;
    operands.values[0].width_bits = UINT8_C(1);
    operands.values[1].raw_value = addr7;
    operands.values[1].width_bits = UINT8_C(7);
    return operands;
}

static void assert_delay_read(const ti_c54x_op_effect *effect)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_MEMORY);
    TI_C54X_TEST_CHECK(effect->payload.memory.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(effect->payload.memory.value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(effect->payload.memory.complete == NULL);
    TI_C54X_TEST_CHECK(effect->payload.memory.write_smem_delay);
    TI_C54X_TEST_CHECK(!effect->payload.memory.load_t_with_read);
}

static void check_ti_example(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(DELAY_TI_WORD, UINT8_C(1));

    for (size_t variant = 0U; variant < UINT8_C(2); ++variant) {
        const ti_c54x_op_input_snapshot input =
            variant == 0U ? clean_input() : noisy_input();
        ti_c54x_op_effect effect = {0};

        TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&instruction, &input,
                                                &effect) ==
                           TI_C54X_OP_STATUS_OK);
        assert_delay_read(&effect);
        TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                           TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
        TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.mode ==
                           TI_C54X_INDIRECT_SINGLE_MOD1);
        TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.arf ==
                           UINT8_C(3));
    }
}

static void check_direct_variant(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(DELAY_DIRECT_WORD, UINT8_C(1));
    const ti_c54x_op_input_snapshot input = noisy_input();
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&instruction, &input, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    assert_delay_read(&effect);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                       TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
    TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.direct_dma7 ==
                       UINT8_C(0x2A));
}

static void check_delay_flag_every_staged_read(void)
{
    static const uint8_t direct_dma[] = {UINT8_C(0x00), UINT8_C(0x01),
                                         UINT8_C(0x2A), UINT8_C(0x7F)};
    const ti_c54x_op_input_snapshot input = clean_input();
    size_t direct_index;
    uint8_t mode4;
    uint8_t arf;

    for (direct_index = 0U;
         direct_index < sizeof(direct_dma) / sizeof(direct_dma[0]);
         ++direct_index) {
        const ti_c54x_decoded_instruction instruction = make_instruction(
            (uint16_t)(UINT16_C(0x4D00) | direct_dma[direct_index]),
            UINT8_C(1));
        ti_c54x_op_effect effect = {0};

        TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&instruction, &input,
                                                &effect) ==
                           TI_C54X_OP_STATUS_OK);
        assert_delay_read(&effect);
        TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.kind ==
                           TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16);
        TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.direct_dma7 ==
                           direct_dma[direct_index]);
    }

    for (mode4 = UINT8_C(0); mode4 <= UINT8_C(3); ++mode4) {
        for (arf = UINT8_C(0); arf <= UINT8_C(7); ++arf) {
            const uint8_t addr7 = (uint8_t)((uint8_t)(mode4 << 3) | arf);
            const ti_c54x_decoded_instruction instruction = make_instruction(
                (uint16_t)(UINT16_C(0x4D80) | addr7), UINT8_C(1));
            ti_c54x_op_effect effect = {0};

            TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&instruction, &input,
                                                    &effect) ==
                               TI_C54X_OP_STATUS_OK);
            assert_delay_read(&effect);
            TI_C54X_TEST_CHECK(
                effect.payload.memory.address_spec.kind ==
                TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16);
            TI_C54X_TEST_CHECK(
                effect.payload.memory.address_spec.mode ==
                (ti_c54x_indirect_single_mode)mode4);
            TI_C54X_TEST_CHECK(effect.payload.memory.address_spec.arf ==
                               arf);
        }
    }
}

static void check_exotic_deferral(void)
{
    static const uint8_t exotic_addr7[] = {UINT8_C(0x20), UINT8_C(0x3B),
                                           UINT8_C(0x40), UINT8_C(0x7F)};
    const ti_c54x_op_input_snapshot input = clean_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    size_t index;

    for (index = 0U;
         index < sizeof(exotic_addr7) / sizeof(exotic_addr7[0]); ++index) {
        const ti_c54x_decoded_instruction instruction = make_instruction(
            (uint16_t)(UINT16_C(0x4D80) | exotic_addr7[index]),
            UINT8_C(1));
        ti_c54x_op_effect effect = before;
        ti_c54x_trusted_operand_values operands =
            trusted_operands(UINT8_C(1), exotic_addr7[index]);

        TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&instruction, &input,
                                                &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_delay_smem(
                               &instruction, &operands, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void check_decode_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(DELAY_TI_WORD, UINT8_C(1));
    const ti_c54x_op_input_snapshot input = clean_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
    static const uint16_t bad_words[] = {UINT16_C(0x0000),
                                         UINT16_C(0x4800),
                                         UINT16_C(0x4C00),
                                         UINT16_C(0xFFFF)};
    ti_c54x_op_effect effect = before;
    size_t index;

    TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(NULL, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&instruction, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_STATUS_SXM;
        TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_word_count = UINT8_C(0);
        TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_word_count = UINT8_C(2);
        TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }

    for (index = 0U;
         index < sizeof(bad_words) / sizeof(bad_words[0]); ++index) {
        const ti_c54x_decoded_instruction forged =
            make_instruction(bad_words[index], UINT8_C(1));

        TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&forged, &input, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void check_trusted_parity(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(DELAY_TI_WORD, UINT8_C(1));
    const ti_c54x_op_input_snapshot input = noisy_input();
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(1), UINT8_C(0x0B));
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_delay_smem(&instruction, &input, &checked) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_delay_smem(&instruction, &operands,
                                                    &input,
                                                    &trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&checked, &trusted, sizeof(checked)) == 0);
    assert_delay_read(&trusted);
}

static void check_trusted_operand_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(DELAY_TI_WORD, UINT8_C(1));
    const ti_c54x_op_input_snapshot input = clean_input();
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(UINT8_C(1), UINT8_C(0x0B));

    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_delay_smem(&instruction, NULL,
                                                    &input,
                                                    &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_delay_smem(&instruction, &operands,
                                                    NULL,
                                                    &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_delay_smem(&instruction, &operands,
                                                    &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_delay_smem(&instruction, &operands,
                                                    &input,
                                                    &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0x0B));
    operands.values[0].width_bits = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_delay_smem(&instruction, &operands,
                                                    &input,
                                                    &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0x0B));
    operands.values[0].raw_value = UINT64_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_delay_smem(&instruction, &operands,
                                                    &input,
                                                    &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0x0B));
    operands.values[1].width_bits = UINT8_C(6);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_delay_smem(&instruction, &operands,
                                                    &input,
                                                    &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(UINT8_C(1), UINT8_C(0x0B));
    operands.values[1].raw_value = UINT64_C(0x80);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_delay_smem(&instruction, &operands,
                                                    &input,
                                                    &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_ti_example();
    check_direct_variant();
    check_delay_flag_every_staged_read();
    check_exotic_deferral();
    check_decode_rejection();
    check_trusted_parity();
    check_trusted_operand_rejection();
    return 0;
}
