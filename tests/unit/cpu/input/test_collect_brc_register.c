/* Module contract
 * Owns: the generic BRC_REGISTER input-resource proof, including copied
 * raw16 values from the MMR file BRC cell, explicit knownness, selective
 * collection, and transactional rejection of missing or invalid requests.
 * Does not own: collection implementation, MMR file storage, opcode
 * semantics, decode, CPU lifecycle, sequencing, timing, memory, buses,
 * diagnostics, or the test harness implementation.
 */

#include "../../../../src/cpu/input/collect.h"
#include "../../../../src/isa/ops/op_input.h"
#include "../../../../src/state/mmr_file.h"
#include "../../../../src/state/status/registers.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_input_sources sources_for(
    const ti_c54x_mmr_file_state *mmr_file,
    const ti_c54x_status_register_state *status)
{
    return (ti_c54x_input_sources){
        .mmr_file = mmr_file,
        .status = status,
    };
}

static ti_c54x_op_input_snapshot sentinel(void)
{
    ti_c54x_op_input_snapshot output = {0};

    output.accumulator_a_raw40 = UINT64_C(0x123456789A);
    output.accumulator_b_raw40 = UINT64_C(0x0ABCDEFFF0);
    output.accumulator_a_known = true;
    output.accumulator_b_known = true;
    output.status_c = 1U;
    output.status_c_known = true;
    output.status_tc = 1U;
    output.status_tc_known = true;
    output.status_sxm = 1U;
    output.status_sxm_known = true;
    output.brc_register_raw16 = UINT16_C(0xA55A);
    output.brc_register_known = true;
    return output;
}

static void expect_rejected(
    ti_c54x_input_resource_mask resources,
    const ti_c54x_input_sources *sources,
    ti_c54x_op_input_snapshot *output)
{
    const ti_c54x_op_input_snapshot before = *output;

    TI_C54X_TEST_CHECK(ti_c54x_input_collect(resources, sources, output) !=
                       TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(memcmp(&before, output, sizeof(before)) == 0);
}

static void expect_brc_zero_unknown(const ti_c54x_op_input_snapshot *output)
{
    TI_C54X_TEST_CHECK(output->brc_register_raw16 == UINT16_C(0));
    TI_C54X_TEST_CHECK(!output->brc_register_known);
}

int main(void)
{
    static const uint16_t known_values[] = {
        UINT16_C(0x0000),
        UINT16_C(0x01FF),
        UINT16_C(0x8000),
        UINT16_C(0xFFFF),
    };
    ti_c54x_mmr_file_state mmr_file;
    ti_c54x_status_register_state status;
    ti_c54x_input_sources sources;
    ti_c54x_input_sources missing_sources;
    ti_c54x_op_input_snapshot output;
    size_t index;

    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_reset(&mmr_file) ==
                       TI_C54X_MMR_FILE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_reset_common(&status) ==
                       TI_C54X_STATUS_REGISTER_OK);

    /* Known BRC copies the exact raw WORD16 value and marks it known. */
    sources = sources_for(&mmr_file, NULL);
    for (index = 0U; index < sizeof(known_values) / sizeof(known_values[0]);
         ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_mmr_file_write_brc(
                               &mmr_file, known_values[index]) ==
                           TI_C54X_MMR_FILE_OK);
        output = sentinel();
        TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                               TI_C54X_INPUT_RESOURCES_BRC_REGISTER, &sources,
                               &output) == TI_C54X_INPUT_COLLECT_OK);
        TI_C54X_TEST_CHECK(output.brc_register_raw16 == known_values[index]);
        TI_C54X_TEST_CHECK(output.brc_register_known);
        TI_C54X_TEST_CHECK(!output.accumulator_a_known);
        TI_C54X_TEST_CHECK(!output.accumulator_b_known);
        TI_C54X_TEST_CHECK(!output.status_c_known);
        TI_C54X_TEST_CHECK(!output.status_tc_known);
        TI_C54X_TEST_CHECK(!output.status_sxm_known);
    }

    /* Reset and unknown BRC canonicalize raw storage to zero. */
    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_reset(&mmr_file) ==
                       TI_C54X_MMR_FILE_OK);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_BRC_REGISTER, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    expect_brc_zero_unknown(&output);

    mmr_file.brc = UINT16_C(0xD00D);
    mmr_file.known_mask = (uint8_t)(mmr_file.known_mask &
                                    (uint8_t)~TI_C54X_MMR_FILE_KNOWN_BRC);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_BRC_REGISTER, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    expect_brc_zero_unknown(&output);

    /* An unrequested BRC source is not required to collect another resource. */
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    sources = sources_for(NULL, &status);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STATUS_C, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.status_c == 1U);
    TI_C54X_TEST_CHECK(output.status_c_known);
    expect_brc_zero_unknown(&output);

    /* Composite collection retains the existing status contract. */
    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_write_brc(
                           &mmr_file, UINT16_C(0x4210)) ==
                       TI_C54X_MMR_FILE_OK);
    sources = sources_for(&mmr_file, &status);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           (ti_c54x_input_resource_mask)(
                               TI_C54X_INPUT_RESOURCES_BRC_REGISTER |
                               TI_C54X_INPUT_RESOURCES_STATUS_C),
                           &sources, &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.brc_register_raw16 == UINT16_C(0x4210));
    TI_C54X_TEST_CHECK(output.brc_register_known);
    TI_C54X_TEST_CHECK(output.status_c == 1U);
    TI_C54X_TEST_CHECK(output.status_c_known);

    /* Missing requested BRC or status sources reject without mutation. */
    output = sentinel();
    missing_sources = sources_for(NULL, &status);
    expect_rejected(TI_C54X_INPUT_RESOURCES_BRC_REGISTER, &missing_sources,
                    &output);
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_BRC_REGISTER, NULL, &output);
    output = sentinel();
    expect_rejected((ti_c54x_input_resource_mask)(
                        TI_C54X_INPUT_RESOURCES_BRC_REGISTER |
                        TI_C54X_INPUT_RESOURCES_STATUS_C),
                    &missing_sources, &output);

    /* Existing invalid-resource and null-output contracts remain intact. */
    output = sentinel();
    expect_rejected((ti_c54x_input_resource_mask)UINT32_C(0x8000), &sources,
                    &output);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_BRC_REGISTER, &sources,
                           NULL) != TI_C54X_INPUT_COLLECT_OK);
    return 0;
}
