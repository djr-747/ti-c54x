/*
 * Module contract
 *
 * Owns: executable tests for the private NOP semantic handler and generated
 * opcode dispatch seam.
 * Does not own: catalog/decode implementation, sequential PC planning, CPU
 * integration, timing, buses, architectural state, or diagnostics.
 */

#include "isa/ops/opcode_dispatch.h"
#ifdef TI_C54X_ISA_OP_NOP_H
#error "opcode_dispatch.h must not include the NOP-specific handler header"
#endif
#include "isa/ops/op_nop.h"
#include "isa/catalog/opcode_catalog.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_decode_result
valid_nop(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_NOP);
    const ti_c54x_decode_result instruction = {
        .status = TI_C54X_DECODE_STATUS_OK,
        .id = metadata->id,
        .first_word = metadata->first_word_value,
        .program_word_count = metadata->program_word_count
    };
    return instruction;
}

static void
check_failure_preserves_effect(ti_c54x_op_status status,
                               ti_c54x_op_status expected,
                               const ti_c54x_op_effect *effect,
                               const unsigned char *before)
{
    TI_C54X_TEST_CHECK(status == expected);
    TI_C54X_TEST_CHECK(memcmp(effect, before, sizeof(*effect)) == 0);
}

static void
test_direct_nop_success_and_no_effect(void)
{
    const ti_c54x_decode_result instruction = valid_nop();
    ti_c54x_op_effect effect;

    (void)memset(&effect, 0xa5, sizeof(effect));
    TI_C54X_TEST_CHECK(ti_c54x_op_nop(&instruction, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_NONE);
}

static void
test_direct_nop_rejects_malformed_descriptors_transactionally(void)
{
    ti_c54x_decode_result instruction = valid_nop();
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_NOP);
    const uint16_t words[] = {
        (uint16_t)(metadata->first_word_value ^ UINT16_C(1)),
        (uint16_t)(metadata->first_word_value ^ UINT16_C(2))};
    const uint8_t counts[] = {UINT8_C(0),
                              (uint8_t)(metadata->program_word_count + 1U)};
    size_t index;

    for (index = 0U; index < sizeof(words) / sizeof(words[0]); ++index) {
        ti_c54x_op_effect effect;
        unsigned char before[sizeof(effect)];

        instruction.first_word = words[index];
        (void)memset(&effect, 0x3c, sizeof(effect));
        (void)memcpy(before, &effect, sizeof(before));
        check_failure_preserves_effect(
            ti_c54x_op_nop(&instruction, &effect),
            TI_C54X_OP_STATUS_INVALID_DECODE, &effect, before);
    }

    instruction = valid_nop();
    for (index = 0U; index < sizeof(counts) / sizeof(counts[0]); ++index) {
        ti_c54x_op_effect effect;
        unsigned char before[sizeof(effect)];

        instruction.program_word_count = counts[index];
        (void)memset(&effect, 0x4d, sizeof(effect));
        (void)memcpy(before, &effect, sizeof(before));
        check_failure_preserves_effect(
            ti_c54x_op_nop(&instruction, &effect),
            TI_C54X_OP_STATUS_INVALID_DECODE, &effect, before);
    }

    instruction = valid_nop();
    instruction.status = TI_C54X_DECODE_STATUS_UNKNOWN_WORD;
    {
        ti_c54x_op_effect effect;
        unsigned char before[sizeof(effect)];

        (void)memset(&effect, 0x5e, sizeof(effect));
        (void)memcpy(before, &effect, sizeof(before));
        check_failure_preserves_effect(
            ti_c54x_op_nop(&instruction, &effect),
            TI_C54X_OP_STATUS_INVALID_DECODE, &effect, before);
    }

    instruction = valid_nop();
    instruction.id = TI_C54X_OPCODE_ID_UNKNOWN;
    {
        ti_c54x_op_effect effect;
        unsigned char before[sizeof(effect)];

        (void)memset(&effect, 0x6f, sizeof(effect));
        (void)memcpy(before, &effect, sizeof(before));
        check_failure_preserves_effect(
            ti_c54x_op_nop(&instruction, &effect),
            TI_C54X_OP_STATUS_INVALID_DECODE, &effect, before);
    }
}

static void
test_direct_nop_rejects_null_arguments(void)
{
    ti_c54x_decode_result instruction = valid_nop();
    ti_c54x_op_effect effect;
    unsigned char before[sizeof(effect)];

    (void)memset(&effect, 0x77, sizeof(effect));
    (void)memcpy(before, &effect, sizeof(before));
    check_failure_preserves_effect(
        ti_c54x_op_nop(NULL, &effect), TI_C54X_OP_STATUS_INVALID_ARGUMENT,
        &effect, before);
    check_failure_preserves_effect(
        ti_c54x_op_nop(&instruction, NULL), TI_C54X_OP_STATUS_INVALID_ARGUMENT,
        &effect, before);
}

static void
test_dispatch_success_and_no_effect(void)
{
    const ti_c54x_decode_result instruction = valid_nop();
    ti_c54x_op_effect effect;

    (void)memset(&effect, 0xa5, sizeof(effect));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_decoded(&instruction, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_NONE);
}

static void
test_dispatch_rejects_decode_and_unsupported_ids_transactionally(void)
{
    ti_c54x_decode_result instruction = valid_nop();
    const ti_c54x_opcode_id ids[] = {
        TI_C54X_OPCODE_ID_UNKNOWN,
        TI_C54X_OPCODE_ID_COUNT,
        (ti_c54x_opcode_id)(TI_C54X_OPCODE_ID_COUNT + 1)
    };
    size_t index;

    instruction.status = TI_C54X_DECODE_STATUS_UNKNOWN_WORD;
    for (index = 0U; index < sizeof(ids) / sizeof(ids[0]); ++index) {
        ti_c54x_op_effect effect;
        unsigned char before[sizeof(effect)];

        instruction.id = ids[index];
        (void)memset(&effect, 0x88, sizeof(effect));
        (void)memcpy(before, &effect, sizeof(before));
        check_failure_preserves_effect(
            ti_c54x_dispatch_decoded(&instruction, &effect),
            TI_C54X_OP_STATUS_INVALID_DECODE, &effect, before);
    }

    instruction = valid_nop();
    for (index = 0U; index < sizeof(ids) / sizeof(ids[0]); ++index) {
        ti_c54x_op_effect effect;
        unsigned char before[sizeof(effect)];

        instruction.id = ids[index];
        (void)memset(&effect, 0x99, sizeof(effect));
        (void)memcpy(before, &effect, sizeof(before));
        check_failure_preserves_effect(
            ti_c54x_dispatch_decoded(&instruction, &effect),
            TI_C54X_OP_STATUS_UNSUPPORTED_OPCODE, &effect, before);
    }
}

static void
test_dispatch_rejects_null_arguments(void)
{
    ti_c54x_decode_result instruction = valid_nop();
    ti_c54x_op_effect effect;
    unsigned char before[sizeof(effect)];

    (void)memset(&effect, 0xaa, sizeof(effect));
    (void)memcpy(before, &effect, sizeof(before));
    check_failure_preserves_effect(
        ti_c54x_dispatch_decoded(NULL, &effect),
        TI_C54X_OP_STATUS_INVALID_ARGUMENT, &effect, before);
    check_failure_preserves_effect(
        ti_c54x_dispatch_decoded(&instruction, NULL),
        TI_C54X_OP_STATUS_INVALID_ARGUMENT, &effect, before);
}

int
main(void)
{
    test_direct_nop_success_and_no_effect();
    test_direct_nop_rejects_malformed_descriptors_transactionally();
    test_direct_nop_rejects_null_arguments();
    test_dispatch_success_and_no_effect();
    test_dispatch_rejects_decode_and_unsupported_ids_transactionally();
    test_dispatch_rejects_null_arguments();
    return 0;
}
