/*
 * Module contract
 *
 * Owns: exhaustive RED proofs for two-word BANZ pmad,Sind effects,
 * including the image-evidenced star-ARx and star-ARx-minus Sind
 * subset over all
 * eight auxiliary registers with verbatim pmad carriage,
 * always-applied side-effect staging (post-decrement value-after
 * with update flag versus no-mod observation), taken/not-taken
 * emission with non-delayed retirement, consumed-input knownness,
 * deferred-Sind rejection, checked/trusted/dispatch parity, and
 * transactional rejection.
 * Does not own: catalog rows, operand-layout descriptors, buses,
 * memory, CPU/state mutation, sequencing, timing, observers,
 * diagnostics, or the production implementation.
 */

#include "../../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../../src/isa/decode/opcode_decode.h"
#include "../../../../../src/isa/decode/trusted_execution_packet.h"
#include "../../../../../src/isa/ops/branch/op_banz.h"
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

static const uint16_t BANZ_WORD0_BASE = UINT16_C(0x6C00);

static uint16_t
make_word0(uint8_t sind)
{
    return (uint16_t)(BANZ_WORD0_BASE | sind);
}

/* Image-evidenced Sind subset: I=1 with MOD0 (*ARx) or MOD1 (*ARx-).
 * Sind byte layout is I[7], MOD[6:3], ARX[2:0]. */
static uint8_t
make_sind(uint8_t mod, uint8_t arx)
{
    return (uint8_t)(UINT8_C(0x80) | (uint8_t)(mod << 3U) | arx);
}

static ti_c54x_decoded_instruction
decode(uint8_t sind, uint16_t pmad)
{
    const uint16_t word = make_word0(sind);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, pmad}, .program_word_count = UINT8_C(2)};
    ti_c54x_decoded_instruction instruction;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_BANZ);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_BANZ);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_SIND_PMAD16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
    return instruction;
}

static ti_c54x_op_input_snapshot
input_for(const uint16_t ars[8], uint8_t known_mask)
{
    ti_c54x_op_input_snapshot input = {0};
    size_t index;

    for (index = 0U; index < 8U; ++index) {
        input.auxiliary_registers_raw16[index] = ars[index];
    }
    input.auxiliary_registers_known_mask = known_mask;
    return input;
}

static ti_c54x_trusted_operand_values
trusted_operands(uint8_t sind, uint16_t pmad)
{
    ti_c54x_trusted_operand_values operands = {0};

    operands.count = UINT8_C(2);
    operands.values[0].raw_value = sind;
    operands.values[0].width_bits = UINT8_C(8);
    operands.values[1].raw_value = pmad;
    operands.values[1].width_bits = UINT8_C(16);
    return operands;
}

static void
assert_effect(const ti_c54x_op_effect *effect, uint8_t arx,
              uint16_t value_after, bool update, bool taken,
              uint16_t pmad)
{
    TI_C54X_TEST_CHECK(effect->kind ==
                       TI_C54X_OP_EFFECT_BRANCH_AUXILIARY);
    TI_C54X_TEST_CHECK(effect->payload.branch_auxiliary.ar_index == arx);
    TI_C54X_TEST_CHECK(effect->payload.branch_auxiliary.value_after ==
                       value_after);
    TI_C54X_TEST_CHECK(effect->payload.branch_auxiliary.update == update);
    TI_C54X_TEST_CHECK(effect->payload.branch_auxiliary.taken == taken);
    TI_C54X_TEST_CHECK(effect->payload.branch_auxiliary.delayed == false);
    TI_C54X_TEST_CHECK(effect->payload.branch_auxiliary.pmad == pmad);
}

static void
check_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_BANZ);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_BANZ);
    TI_C54X_TEST_CHECK(metadata->first_word_value == BANZ_WORD0_BASE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFF00));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_SIND_PMAD16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
}

static void
check_case(uint8_t mod, uint8_t arx, uint16_t ar_value, uint16_t pmad)
{
    const uint8_t sind = make_sind(mod, arx);
    ti_c54x_decoded_instruction instruction = decode(sind, pmad);
    ti_c54x_trusted_execution_packet packet;
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_BANZ);
    const uint16_t ars[8] = {UINT16_C(0x1111), UINT16_C(0x2222),
                             UINT16_C(0x3333), UINT16_C(0x4444),
                             UINT16_C(0x5555), UINT16_C(0x6666),
                             UINT16_C(0x7777), UINT16_C(0x8888)};
    const ti_c54x_op_input_snapshot clean = input_for(ars, UINT8_C(0xFF));
    ti_c54x_trusted_operand_values operands = trusted_operands(sind, pmad);
    ti_c54x_op_effect checked = {0};
    ti_c54x_op_effect trusted = {0};
    ti_c54x_op_effect via_dispatch = {0};
    ti_c54x_op_effect via_trusted = {0};
    const bool taken = ar_value != UINT16_C(0);
    const uint16_t value_after =
        (uint16_t)(mod == UINT8_C(1) ? ar_value - UINT16_C(1) : ar_value);
    const bool update = mod == UINT8_C(1);
    ti_c54x_op_input_snapshot seeded = clean;

    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->handler == ti_c54x_trusted_op_banz);

    /* Seed the selected AR with the case value; hostile neighbors
     * prove the leaf reads only the selected register. */
    seeded.auxiliary_registers_raw16[arx] = ar_value;
    instruction.bundle.program_words[1] = pmad;
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &instruction.bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    TI_C54X_TEST_CHECK(ti_c54x_op_banz(&instruction, &seeded,
                                       &checked) ==
                       TI_C54X_OP_STATUS_OK);
    assert_effect(&checked, arx, value_after, update, taken, pmad);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_banz(
                           &instruction, &operands, &seeded,
                           &trusted) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&trusted, &checked, sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded_with_input_snapshot(
                           &instruction, &seeded,
                           &via_dispatch) == TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&via_dispatch, &checked,
                              sizeof(checked)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(&packet, &seeded,
                                               &via_trusted) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&via_trusted, &checked,
                              sizeof(checked)) == 0);
}

static void
check_effects_and_parity(void)
{
    static const uint16_t ar_values[] = {
        UINT16_C(0x0000), UINT16_C(0x0001), UINT16_C(0x0002),
        UINT16_C(0x00FF), UINT16_C(0x7FFF), UINT16_C(0x8000),
        UINT16_C(0xFFFE), UINT16_C(0xFFFF)};
    static const uint16_t pmads[] = {
        UINT16_C(0x0000), UINT16_C(0x1000), UINT16_C(0x2000),
        UINT16_C(0xFFFF)};
    uint8_t mod;
    uint8_t arx;
    size_t vi;
    size_t pi;

    for (mod = UINT8_C(0); mod <= UINT8_C(1); ++mod) {
        for (arx = UINT8_C(0); arx <= UINT8_C(7); ++arx) {
            for (vi = 0U; vi < sizeof(ar_values) / sizeof(ar_values[0]);
                 ++vi) {
                for (pi = 0U;
                     pi < sizeof(pmads) / sizeof(pmads[0]); ++pi) {
                    check_case(mod, arx, ar_values[vi], pmads[pi]);
                }
            }
        }
    }
}

static void
check_deferred_sind_rejection(void)
{
    /* Every Sind combination outside I=1/MOD0-MOD1 is deferred and
     * rejects transactionally. */
    for (uint32_t raw = 0U; raw <= UINT32_C(0xFF); ++raw) {
        const uint8_t sind = (uint8_t)raw;
        const uint8_t mod = (uint8_t)((sind >> 3U) & UINT8_C(0x0F));
        const bool implemented =
            (sind & UINT8_C(0x80)) != UINT8_C(0) &&
            (mod == UINT8_C(0) || mod == UINT8_C(1));
        const ti_c54x_decoded_instruction instruction =
            decode(sind, UINT16_C(0x2000));
        const uint16_t ars[8] = {UINT16_C(0x53), UINT16_C(0x53),
                                 UINT16_C(0x53), UINT16_C(0x53),
                                 UINT16_C(0x53), UINT16_C(0x53),
                                 UINT16_C(0x53), UINT16_C(0x53)};
        const ti_c54x_op_input_snapshot clean =
            input_for(ars, UINT8_C(0xFF));
        const ti_c54x_op_effect before = {
            .kind = TI_C54X_OP_EFFECT_REPEAT_START,
            .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
        ti_c54x_op_effect effect = before;
        const ti_c54x_op_status status =
            ti_c54x_op_banz(&instruction, &clean, &effect);

        if (implemented) {
            TI_C54X_TEST_CHECK(status == TI_C54X_OP_STATUS_OK);
        } else {
            TI_C54X_TEST_CHECK(status ==
                               TI_C54X_OP_STATUS_INVALID_OPERAND);
            TI_C54X_TEST_CHECK(memcmp(&effect, &before,
                                      sizeof(effect)) == 0);
        }
    }
}

static void
check_argument_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(make_sind(UINT8_C(1), UINT8_C(3)), UINT16_C(0x2000));
    const uint16_t ars[8] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    const ti_c54x_op_input_snapshot valid = input_for(ars, UINT8_C(0xFF));
    ti_c54x_trusted_operand_values operands =
        trusted_operands(make_sind(UINT8_C(1), UINT8_C(3)),
                         UINT16_C(0x2000));
    ti_c54x_op_effect effect;

    TI_C54X_TEST_CHECK(ti_c54x_op_banz(NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_banz(&instruction, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_banz(&instruction, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_banz(
                           &instruction, NULL, &valid, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_banz(
                           &instruction, &operands, NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_banz(
                           &instruction, &operands, &valid, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void
check_input_and_decode_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(make_sind(UINT8_C(1), UINT8_C(3)), UINT16_C(0x2000));
    const uint16_t ars[8] = {UINT16_C(0xDEAD), UINT16_C(0xBEEF),
                             UINT16_C(0xCAFE), UINT16_C(0x53),
                             UINT16_C(0x11), UINT16_C(0x22),
                             UINT16_C(0x33), UINT16_C(0x44)};
    const ti_c54x_op_input_snapshot valid = input_for(ars, UINT8_C(0xFF));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0x55AA)}};
    ti_c54x_op_effect effect = before;

    /* Unknown selected AR3 rejects without touching output. */
    {
        ti_c54x_op_input_snapshot unknown_ar = valid;

        unknown_ar.auxiliary_registers_known_mask = UINT8_C(0xF7);
        TI_C54X_TEST_CHECK(ti_c54x_op_banz(&instruction, &unknown_ar,
                                           &effect) ==
                           TI_C54X_OP_STATUS_UNKNOWN_INPUT);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }

    {
        ti_c54x_decoded_instruction forged = instruction;

        forged.id = TI_C54X_OPCODE_ID_NOP;
        TI_C54X_TEST_CHECK(ti_c54x_op_banz(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.operand_layout = TI_C54X_OPERAND_LAYOUT_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_banz(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.repeatability = TI_C54X_REPEATABILITY_UNRESOLVED;
        TI_C54X_TEST_CHECK(ti_c54x_op_banz(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.input_resources = TI_C54X_INPUT_RESOURCES_NONE;
        TI_C54X_TEST_CHECK(ti_c54x_op_banz(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        forged = instruction;
        forged.bundle.program_word_count = UINT8_C(1);
        TI_C54X_TEST_CHECK(ti_c54x_op_banz(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_DECODE);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);

        /* Clearing the Sind indirect bit selects a deferred Sind,
         * so the leaf rejects INVALID_OPERAND, not INVALID_DECODE,
         * without touching the output. */
        forged = instruction;
        forged.bundle.program_words[0] ^= UINT16_C(0x0080);
        TI_C54X_TEST_CHECK(ti_c54x_op_banz(&forged, &valid, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    }
}

static void
check_trusted_operand_rejection(void)
{
    const ti_c54x_decoded_instruction instruction =
        decode(make_sind(UINT8_C(1), UINT8_C(3)), UINT16_C(0x2000));
    const uint16_t ars[8] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    const ti_c54x_op_input_snapshot input = input_for(ars, UINT8_C(0xFF));
    const ti_c54x_op_effect before = {
        .kind = TI_C54X_OP_EFFECT_REPEAT_START,
        .payload.repeat_start = {.operand = UINT16_C(0xBEEF)}};
    ti_c54x_op_effect effect = before;
    ti_c54x_trusted_operand_values operands =
        trusted_operands(make_sind(UINT8_C(1), UINT8_C(3)),
                         UINT16_C(0x2000));

    operands.count = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_banz(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(make_sind(UINT8_C(1), UINT8_C(3)),
                                UINT16_C(0x2000));
    operands.values[0].width_bits = UINT8_C(7);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_banz(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
    operands = trusted_operands(make_sind(UINT8_C(1), UINT8_C(3)),
                                UINT16_C(0x2000));
    operands.values[1].width_bits = UINT8_C(15);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_op_banz(
                           &instruction, &operands, &input, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(&effect, &before, sizeof(effect)) == 0);
}

int main(void)
{
    check_metadata();
    check_effects_and_parity();
    check_deferred_sind_rejection();
    check_argument_rejection();
    check_input_and_decode_rejection();
    check_trusted_operand_rejection();
    return 0;
}
