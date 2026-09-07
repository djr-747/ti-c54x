/*
 * Module contract
 *
 * Owns: executable proof that the trusted packet constructor accepts every
 * canonical opcode binding, copies complete instruction facts, extracts each
 * operand once from the static plan, and rejects forged or incomplete inputs
 * transactionally. Does not own: opcode semantics, CPU orchestration, effect
 * commit, sequencing, buses, timing, diagnostics, or benchmark policy.
 */

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/opcode_decode.h"
#include "isa/decode/operand_layout.h"
#include "isa/decode/trusted_execution_packet.h"
#include "isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_instruction_word_bundle bundle_for(
    const ti_c54x_opcode_metadata *metadata)
{
    const ti_c54x_instruction_word_bundle bundle = {
        {metadata->first_word_value, UINT16_C(0xBEEF)},
        metadata->program_word_count};
    return bundle;
}

static void test_every_binding_matches_checked_decode_and_extraction(void)
{
    int raw_id;

    for (raw_id = (int)TI_C54X_OPCODE_ID_UNKNOWN + 1;
         raw_id < (int)TI_C54X_OPCODE_ID_COUNT; ++raw_id) {
        const ti_c54x_opcode_id id = (ti_c54x_opcode_id)raw_id;
        const ti_c54x_trusted_opcode_binding *binding =
            ti_c54x_trusted_opcode_binding_for_id(id);
        const ti_c54x_instruction_word_bundle bundle =
            bundle_for(&binding->metadata);
        const ti_c54x_decode_result first =
            ti_c54x_decode_first_word(bundle.program_words[0]);
        ti_c54x_decoded_instruction checked = {0};
        ti_c54x_trusted_execution_packet packet;
        ti_c54x_operand_layout_set_view view;
        uint8_t operand_index;

        TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                               &first, &bundle, &checked) ==
                           TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                               binding, &bundle, &packet) ==
                           TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
        TI_C54X_TEST_CHECK(packet.handler == binding->handler);
        TI_C54X_TEST_CHECK(binding->operand_plan != NULL);
        TI_C54X_TEST_CHECK(binding->operand_plan->id ==
                           binding->metadata.operand_layout);
        TI_C54X_TEST_CHECK(binding->operand_plan->program_word_count ==
                           binding->metadata.program_word_count);
        TI_C54X_TEST_CHECK(memcmp(&packet.instruction, &checked,
                                  sizeof(checked)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                               checked.operand_layout,
                               checked.bundle.program_word_count, &view) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK(packet.operands.count ==
                           view.descriptor->operand_count);
        for (operand_index = UINT8_C(0);
             operand_index < packet.operands.count; ++operand_index) {
            ti_c54x_operand_field_value expected = {0};
            const ti_c54x_program_words_view words = {
                checked.bundle.program_words,
                checked.bundle.program_word_count};

            TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                                   &view, &words, operand_index, &expected) ==
                               TI_C54X_OPERAND_LAYOUT_STATUS_OK);
            TI_C54X_TEST_CHECK(packet.operands.values[operand_index].raw_value ==
                               expected.raw_value);
            TI_C54X_TEST_CHECK(packet.operands.values[operand_index].width_bits ==
                               expected.width_bits);
        }
    }
}

static void check_failure(
    const ti_c54x_trusted_opcode_binding *binding,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_trusted_execution_packet_status expected)
{
    ti_c54x_trusted_execution_packet packet;
    unsigned char before[sizeof(packet)];

    (void)memset(&packet, 0xA5, sizeof(packet));
    (void)memcpy(before, &packet, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, bundle, &packet) == expected);
    TI_C54X_TEST_CHECK(memcmp(before, &packet, sizeof(packet)) == 0);
}

static void test_rejections_are_transactional(void)
{
    const ti_c54x_trusted_opcode_binding *nop =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_NOP);
    const ti_c54x_trusted_opcode_binding *unknown =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_UNKNOWN);
    ti_c54x_trusted_opcode_binding forged = *nop;
    const ti_c54x_trusted_opcode_binding *ld_dp =
        ti_c54x_trusted_opcode_binding_for_id(
            TI_C54X_OPCODE_ID_LD_IMMEDIATE_DP);
    ti_c54x_instruction_word_bundle bundle = bundle_for(&nop->metadata);
    ti_c54x_trusted_execution_packet packet = {0};

    check_failure(unknown, &bundle,
                  TI_C54X_TRUSTED_EXECUTION_PACKET_UNKNOWN_OPCODE);
    check_failure(&forged, &bundle,
                  TI_C54X_TRUSTED_EXECUTION_PACKET_INVALID_BINDING);
    forged = *nop;
    forged.operand_plan = ld_dp->operand_plan;
    check_failure(&forged, &bundle,
                  TI_C54X_TRUSTED_EXECUTION_PACKET_INVALID_BINDING);
    bundle.program_word_count = UINT8_C(2);
    check_failure(nop, &bundle,
                  TI_C54X_TRUSTED_EXECUTION_PACKET_BUNDLE_COUNT_MISMATCH);
    bundle = bundle_for(&nop->metadata);
    bundle.program_words[0] ^= UINT16_C(1);
    check_failure(nop, &bundle,
                  TI_C54X_TRUSTED_EXECUTION_PACKET_FIRST_WORD_MISMATCH);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           NULL, &bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           nop, NULL, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           nop, &bundle, NULL) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_INVALID_ARGUMENT);
}

static void test_fast_and_checked_packets_are_equivalent(void)
{
    int raw_id;

    for (raw_id = (int)TI_C54X_OPCODE_ID_UNKNOWN + 1;
         raw_id < (int)TI_C54X_OPCODE_ID_COUNT; ++raw_id) {
        const ti_c54x_trusted_opcode_binding *binding =
            ti_c54x_trusted_opcode_binding_for_id((ti_c54x_opcode_id)raw_id);
        const ti_c54x_instruction_word_bundle bundle =
            bundle_for(&binding->metadata);
        ti_c54x_trusted_execution_packet checked;
        ti_c54x_trusted_execution_packet fast;

        TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                               binding, &bundle, &checked) ==
                           TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare_fast(
                               binding, &bundle, &fast) ==
                           TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
        TI_C54X_TEST_CHECK(memcmp(&checked, &fast, sizeof(checked)) == 0);
    }
}

int main(void)
{
    test_every_binding_matches_checked_decode_and_extraction();
    test_rejections_are_transactional();
    test_fast_and_checked_packets_are_equivalent();
    puts("trusted_execution_packet: ok");
    return 0;
}
