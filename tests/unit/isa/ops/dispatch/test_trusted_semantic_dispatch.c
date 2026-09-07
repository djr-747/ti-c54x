/*
 * Module contract
 *
 * Owns: executable effect-equivalence proof between checked semantic dispatch
 * and packet-only trusted dispatch for every registered opcode ID, plus
 * transactional argument failures. Does not own: decode lookup, packet
 * extraction, CPU retirement, architectural state, sequencing, buses, timing,
 * diagnostics, or benchmark policy.
 */

#include "isa/decode/opcode_decode.h"
#include "isa/decode/trusted_execution_packet.h"
#include "isa/ops/opcode_dispatch.h"
#include "../../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_every_registered_effect_matches(void)
{
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = UINT64_C(0x123456789A),
        .accumulator_b_raw40 = UINT64_C(0xFEDCBA9876),
        .accumulator_a_known = true,
        .accumulator_b_known = true,
        .status_c = UINT8_C(1),
        .status_c_known = true,
        .status_tc = UINT8_C(0),
        .status_tc_known = true,
        .status_sxm = UINT8_C(1),
        .status_sxm_known = true};
    uint32_t raw_word;

    for (raw_word = UINT32_C(0); raw_word <= (uint32_t)UINT16_MAX;
         ++raw_word) {
        const ti_c54x_trusted_opcode_binding *binding =
            ti_c54x_trusted_opcode_binding_for_first_word((uint16_t)raw_word);
        const uint16_t extensions[] = {
            UINT16_C(0), UINT16_C(0xA55A), UINT16_MAX};
        const size_t extension_count =
            binding->metadata.program_word_count == UINT8_C(2)
                ? sizeof(extensions) / sizeof(extensions[0])
                : 1U;
        size_t extension_index;

        if (binding->metadata.id == TI_C54X_OPCODE_ID_UNKNOWN) {
            continue;
        }
        for (extension_index = 0U; extension_index < extension_count;
             ++extension_index) {
            const ti_c54x_instruction_word_bundle bundle = {
                {(uint16_t)raw_word, extensions[extension_index]},
                binding->metadata.program_word_count};
            ti_c54x_trusted_execution_packet packet;
            ti_c54x_op_effect checked;
            ti_c54x_op_effect trusted;
            ti_c54x_op_status checked_status;
            ti_c54x_op_status trusted_status;

            TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                                   binding, &bundle, &packet) ==
                               TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
            (void)memset(&checked, 0xA5, sizeof(checked));
            (void)memset(&trusted, 0x5A, sizeof(trusted));
            checked_status = ti_c54x_dispatch_decoded_with_input_snapshot(
                &packet.instruction, &input, &checked);
            trusted_status = ti_c54x_dispatch_trusted(&packet, &input,
                                                     &trusted);
            TI_C54X_TEST_CHECK(trusted_status == checked_status);
            if (checked_status == TI_C54X_OP_STATUS_OK) {
                TI_C54X_TEST_CHECK(memcmp(&trusted, &checked,
                                          sizeof(checked)) == 0);
            }
        }
    }
}

static void test_trusted_status_field_effect_values(void)
{
    struct trusted_ld_case {
        ti_c54x_opcode_id id;
        uint16_t word;
        ti_c54x_op_status_field_target target;
        uint16_t raw_value;
    };
    static const struct trusted_ld_case cases[] = {
        {TI_C54X_OPCODE_ID_LD_IMMEDIATE_DP, UINT16_C(0xEBAB),
         TI_C54X_OP_STATUS_FIELD_TARGET_DP, UINT16_C(0x01AB)},
        {TI_C54X_OPCODE_ID_LD_IMMEDIATE_ASM, UINT16_C(0xED12),
         TI_C54X_OP_STATUS_FIELD_TARGET_ASM, UINT16_C(0x0012)},
        {TI_C54X_OPCODE_ID_LD_IMMEDIATE_ARP, UINT16_C(0xF4A5),
         TI_C54X_OP_STATUS_FIELD_TARGET_ARP, UINT16_C(0x0005)}};
    const ti_c54x_op_input_snapshot input = {0};
    size_t index;

    for (index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        const ti_c54x_trusted_opcode_binding *binding =
            ti_c54x_trusted_opcode_binding_for_id(cases[index].id);
        const ti_c54x_instruction_word_bundle bundle = {
            {cases[index].word, UINT16_C(0)}, UINT8_C(1)};
        ti_c54x_trusted_execution_packet packet;
        ti_c54x_op_effect effect;

        TI_C54X_TEST_CHECK(binding != NULL);
        TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                               binding, &bundle, &packet) ==
                           TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
        (void)memset(&effect, 0xA5, sizeof(effect));
        TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(&packet, &input, &effect) ==
                           TI_C54X_OP_STATUS_OK);
        TI_C54X_TEST_CHECK(effect.kind ==
                           TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE);
        TI_C54X_TEST_CHECK(effect.payload.status_field_write.target ==
                           cases[index].target);
        TI_C54X_TEST_CHECK(effect.payload.status_field_write.raw_value ==
                           cases[index].raw_value);
    }
}

static void test_argument_failures_preserve_output(void)
{
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_NOP);
    const ti_c54x_instruction_word_bundle bundle = {
        {binding->metadata.first_word_value, UINT16_C(0)}, UINT8_C(1)};
    const ti_c54x_op_input_snapshot input = {0};
    ti_c54x_trusted_execution_packet packet;
    ti_c54x_op_effect effect;
    unsigned char before[sizeof(effect)];

    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    (void)memset(&effect, 0xC3, sizeof(effect));
    (void)memcpy(before, &effect, sizeof(before));
#define CHECK_INVALID(packet_value, snapshot) \
    do { \
        TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted( \
                               (packet_value), (snapshot), &effect) == \
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT); \
        TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0); \
    } while (0)
    CHECK_INVALID(NULL, &input);
    CHECK_INVALID(&packet, NULL);
    #undef CHECK_INVALID
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                           &packet, &input, NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
}

static void test_semantic_failure_preserves_output(void)
{
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(TI_C54X_OPCODE_ID_MAX_ACC);
    const ti_c54x_instruction_word_bundle bundle = {
        {binding->metadata.first_word_value, UINT16_C(0)}, UINT8_C(1)};
    const ti_c54x_op_input_snapshot unknown_input = {0};
    ti_c54x_trusted_execution_packet packet;
    ti_c54x_op_effect effect;
    unsigned char before[sizeof(effect)];

    TI_C54X_TEST_CHECK(ti_c54x_trusted_execution_packet_prepare(
                           binding, &bundle, &packet) ==
                       TI_C54X_TRUSTED_EXECUTION_PACKET_OK);
    (void)memset(&effect, 0x7E, sizeof(effect));
    (void)memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_dispatch_trusted(
                           &packet, &unknown_input, &effect) ==
                       TI_C54X_OP_STATUS_UNKNOWN_INPUT);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);
}

int main(void)
{
    test_every_registered_effect_matches();
    test_trusted_status_field_effect_values();
    test_argument_failures_preserve_output();
    test_semantic_failure_preserves_output();
    puts("trusted_semantic_dispatch: ok");
    return 0;
}
