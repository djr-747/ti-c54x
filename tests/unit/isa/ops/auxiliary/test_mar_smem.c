/*
 * Module contract
 *
 * Owns: executable proof for the MAR Smem postdecrement
 * leaf (P3-S77, TI SPRU172C section MAR, modify
 * auxiliary register): checked/trusted parity, AR
 * knownness gating, non-cookie deferral, and the
 * AR_WRITE value-after (ARn minus one, wrapping).
 * Does not own: opcode inventory, catalog rows,
 * layout sets, AR state, sequencing, timing, or
 * handler bodies.
 */

#include "../../../../../src/isa/ops/auxiliary/op_mar_smem.h"

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_input.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MAR_SMEM_EVIDENCED_BYTE UINT8_C(0x8A)

static ti_c54x_decoded_instruction
make_instruction(uint8_t smem)
{
    ti_c54x_decoded_instruction instruction = {0};

    instruction.id = TI_C54X_OPCODE_ID_MAR_SMEM;
    instruction.operand_layout = TI_C54X_OPERAND_LAYOUT_MAR_SMEM8;
    instruction.bundle.program_words[0] =
        (uint16_t)(UINT16_C(0x6D00) | (uint16_t)smem);
    instruction.bundle.program_word_count = UINT8_C(1);
    return instruction;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t smem)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(1);
    operands.values[0].raw_value = smem;
    operands.values[0].width_bits = UINT8_C(8);
    return operands;
}

static ti_c54x_op_input_snapshot
ar_input(uint8_t ar, uint16_t value)
{
    ti_c54x_op_input_snapshot input = {0};

    input.auxiliary_registers_raw16[ar] = value;
    input.auxiliary_registers_known_mask =
        (uint8_t)(UINT8_C(1) << ar);
    return input;
}

static void
assert_mar_effect(const ti_c54x_op_effect *effect, uint8_t ar,
                  uint16_t value_after)
{
    TI_C54X_TEST_CHECK(effect->kind == TI_C54X_OP_EFFECT_AR_WRITE);
    TI_C54X_TEST_CHECK(effect->payload.ar_write.ar_index == ar);
    TI_C54X_TEST_CHECK(effect->payload.ar_write.value_after ==
                       value_after);
}

static void
check_effects_and_parity(void)
{
    /* Postdecrement wraps mod 65536; sweep every AR index. */
    static const uint16_t values[] = {UINT16_C(0x0000),
                                      UINT16_C(0x0001),
                                      UINT16_C(0x1234),
                                      UINT16_C(0xFFFF)};
    uint8_t ar;

    for (ar = UINT8_C(0); ar < UINT8_C(8); ++ar) {
        size_t v;
        const uint8_t smem =
            (uint8_t)(UINT8_C(0x88) | (ar & UINT8_C(0x07)));

        for (v = 0U; v < sizeof(values) / sizeof(values[0]); ++v) {
            const ti_c54x_decoded_instruction instruction =
                make_instruction(smem);
            const ti_c54x_trusted_operand_values operands =
                trusted_operands(smem);
            const ti_c54x_op_input_snapshot input =
                ar_input(ar, values[v]);
            const ti_c54x_op_input_snapshot input_before = input;
            const ti_c54x_op_effect before = {
                .kind = TI_C54X_OP_EFFECT_REPEAT_START,
                .payload.repeat_start = {.operand = UINT16_C(0xA55A)}};
            const uint16_t expected =
                (uint16_t)(values[v] - UINT16_C(1));
            ti_c54x_op_effect checked = before;
            ti_c54x_op_effect trusted = before;

            TI_C54X_TEST_CHECK(ti_c54x_op_mar_smem(
                                   &instruction, &input, &checked) ==
                               TI_C54X_OP_STATUS_OK);
            TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mar_smem(
                                   &instruction, &operands, &input,
                                   &trusted) == TI_C54X_OP_STATUS_OK);
            assert_mar_effect(&checked, ar, expected);
            assert_mar_effect(&trusted, ar, expected);
            TI_C54X_TEST_CHECK(memcmp(&checked, &trusted,
                                      sizeof(checked)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&input, &input_before,
                                      sizeof(input)) == 0);
        }
    }
}

static void
check_unknown_ar_rejects(void)
{
    const ti_c54x_decoded_instruction instruction =
        make_instruction(MAR_SMEM_EVIDENCED_BYTE);
    const ti_c54x_op_input_snapshot input = {0};
    ti_c54x_op_effect effect = {0};

    TI_C54X_TEST_CHECK(ti_c54x_op_mar_smem(&instruction, &input,
                                           &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
}

static void
check_cookie_deferral_and_parity(void)
{
    /* Only the indirect MOD1 cookie composes here: direct
     * words and other I=1 modifiers reject. Both paths
     * leave the effect and input untouched. */
    static const uint8_t smems[] = {UINT8_C(0x13), UINT8_C(0x02),
                                    UINT8_C(0x82), UINT8_C(0x92),
                                    UINT8_C(0x9A), UINT8_C(0xFF)};
    size_t w;

    for (w = 0U; w < sizeof(smems) / sizeof(smems[0]); ++w) {
        ti_c54x_decoded_instruction instruction =
            make_instruction(smems[w]);
        ti_c54x_trusted_operand_values operands =
            trusted_operands(smems[w]);
        const ti_c54x_op_input_snapshot input =
            ar_input(UINT8_C(2), UINT16_C(0x1234));
        const ti_c54x_op_input_snapshot input_before = input;
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0x5A5A)}};
        ti_c54x_op_effect checked = before;
        ti_c54x_op_effect trusted = before;

        TI_C54X_TEST_CHECK(ti_c54x_op_mar_smem(&instruction, &input,
                                               &checked) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_op_mar_smem(
                               &instruction, &operands, &input,
                               &trusted) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&checked, &before,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&trusted, &before,
                                  sizeof(trusted)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&input, &input_before,
                                  sizeof(input)) == 0);
    }
}

int
main(void)
{
    check_effects_and_parity();
    check_unknown_ar_rejects();
    check_cookie_deferral_and_parity();
    return 0;
}
