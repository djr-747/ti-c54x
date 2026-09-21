/*
 * Module contract
 *
 * Owns: executable COMMON_TI proof of the single-repeat body admission
 * tri-state over the full registered catalog, with documented
 * nonrepeatable rejections, the DLD/DST asymmetry, and the audit keeping
 * the twenty W49B/W50B/W51C rows UNRESOLVED.
 * Evidence: E-TI-C54X-SPRU172C-S2-5-T2-28-P2-19-2-21-RPT-BODY (Table 2-28
 * negative list: control-transfer and repeat-control instructions and
 * long-offset or absolute single-data bodies; absence from the list never
 * implies eligibility).
 * Does not own: classification inference, decode, body execution, RC/PC
 * state, interrupts, timing, buses, memory, allocation, observers, or
 * diagnostics. The caller owns the output.
 */

#include "isa/catalog/opcode_catalog.h"
#include "sequencer/repeat/repeat_body_eligibility.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
check_admission(ti_c54x_repeatability_class repeatability,
                ti_c54x_sequencer_repeat_body_admission expected)
{
    ti_c54x_sequencer_repeat_body_admission admission =
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_repeat_body_admission_from_class(
                           repeatability, &admission) ==
                       TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_OK);
    TI_C54X_TEST_CHECK(admission == expected);
}

static void
test_admission_tri_state(void)
{
    /* No positively authorized ELIGIBLE row exists, but the class still
     * maps to admission without inference. */
    check_admission(TI_C54X_REPEATABILITY_ELIGIBLE,
                    TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT);
    check_admission(TI_C54X_REPEATABILITY_UNRESOLVED,
                    TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_BLOCK_UNRESOLVED);
    check_admission(TI_C54X_REPEATABILITY_INELIGIBLE,
                    TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
}

static void
test_invalid_class_rejected_transactionally(void)
{
    ti_c54x_sequencer_repeat_body_admission admission =
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;
    unsigned char before[sizeof(admission)];

    memcpy(before, &admission, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_repeat_body_admission_from_class(
                           TI_C54X_REPEATABILITY_COUNT, NULL) ==
                       TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_repeat_body_admission_from_class(
                           TI_C54X_REPEATABILITY_COUNT, &admission) ==
                       TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_INVALID_REPEATABILITY);
    TI_C54X_TEST_CHECK(memcmp(before, &admission, sizeof(admission)) == 0);
}

static void
check_row_admission(ti_c54x_opcode_id id,
                    ti_c54x_repeatability_class expected_class,
                    ti_c54x_sequencer_repeat_body_admission expected_admission)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(id);
    ti_c54x_sequencer_repeat_body_admission admission =
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == id);
    TI_C54X_TEST_CHECK(metadata->repeatability == expected_class);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_repeat_body_admission_from_class(
                           metadata->repeatability, &admission) ==
                       TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_OK);
    TI_C54X_TEST_CHECK(admission == expected_admission);
}

/* Every registered row carries a classified token that maps to a valid
 * admission decision; no row is left unmapped. */
static void
test_full_catalog_sweep_maps(void)
{
    ti_c54x_opcode_id id;

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 354);
    for (id = (ti_c54x_opcode_id)1; id < TI_C54X_OPCODE_ID_COUNT; ++id) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(id);
        ti_c54x_sequencer_repeat_body_admission admission =
            TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == id);
        TI_C54X_TEST_CHECK(ti_c54x_sequencer_repeat_body_admission_from_class(
                               metadata->repeatability, &admission) ==
                           TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_OK);
        TI_C54X_TEST_CHECK(
            admission == TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT ||
            admission ==
                TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_BLOCK_UNRESOLVED ||
            admission ==
                TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
    }
    TI_C54X_TEST_CHECK(ti_c54x_opcode_metadata_for_id(
                           TI_C54X_OPCODE_ID_UNKNOWN) != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_metadata_for_id(
                           TI_C54X_OPCODE_ID_UNKNOWN)->id ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_metadata_for_id(
                           (ti_c54x_opcode_id)-1) == NULL);
    TI_C54X_TEST_CHECK(
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_COUNT) == NULL);
}

/* Documented Table 2-28 nonrepeatable forms reject: status-bit writes,
 * immediate DP load, the AR compare, Smem-to-DP loads, read-modify-write
 * stores, and the long-word store. */
static void
test_documented_nonrepeatable_rows_reject(void)
{
    check_row_admission(TI_C54X_OPCODE_ID_SSBX_STATUS,
                        TI_C54X_REPEATABILITY_INELIGIBLE,
                        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
    check_row_admission(TI_C54X_OPCODE_ID_LD_IMMEDIATE_DP,
                        TI_C54X_REPEATABILITY_INELIGIBLE,
                        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
    check_row_admission(TI_C54X_OPCODE_ID_CMPR_AR,
                        TI_C54X_REPEATABILITY_INELIGIBLE,
                        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
    check_row_admission(TI_C54X_OPCODE_ID_LD_DP_SMEM,
                        TI_C54X_REPEATABILITY_INELIGIBLE,
                        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
    check_row_admission(TI_C54X_OPCODE_ID_ANDM_SMEM_MOD0_3,
                        TI_C54X_REPEATABILITY_INELIGIBLE,
                        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
    check_row_admission(TI_C54X_OPCODE_ID_ADDM_SMEM_MOD0_3,
                        TI_C54X_REPEATABILITY_INELIGIBLE,
                        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
    check_row_admission(TI_C54X_OPCODE_ID_DST_LONG32_MOD13,
                        TI_C54X_REPEATABILITY_INELIGIBLE,
                        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
    /* P3-S59: MVMM is Table 2-28 nonrepeatable (TI SPRU172C
     * p. 2-20), so it rejects rather than blocking. */
    check_row_admission(TI_C54X_OPCODE_ID_MVMM,
                        TI_C54X_REPEATABILITY_INELIGIBLE,
                        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
    /* DLD is absent from both negative lists and its TI entry states no
     * repeat rule, so it blocks as unresolved rather than rejecting. */
    check_row_admission(TI_C54X_OPCODE_ID_DLD_LONG32_MOD13,
                        TI_C54X_REPEATABILITY_UNRESOLVED,
                        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_BLOCK_UNRESOLVED);
}

/* Audit of the twenty W49B/W50B/W51C rows against the Table 2-28 negative
 * list: register-only shifted ADD/SUB and dual-memory ADD/SUB match no
 * positively listed nonrepeatable instruction or operand/addressing form
 * (the list covers repeat-control, control-transfer, status-bit, DP,
 * read-modify-write, and long-word-store forms). All twenty therefore stay
 * UNRESOLVED: blocked under repeat, with neither eligibility nor
 * ineligibility inferred. */
static void
test_new_rows_remain_unresolved(void)
{
    static const ti_c54x_opcode_id unresolved_rows[] = {
        TI_C54X_OPCODE_ID_ADD_ACC_SHIFT,
        TI_C54X_OPCODE_ID_SUB_ACC_SHIFT,
        TI_C54X_OPCODE_ID_ADD_DUAL_MOD0,
        TI_C54X_OPCODE_ID_SUB_DUAL_MOD0,
        TI_C54X_OPCODE_ID_ADD_DUAL_M0_M1,
        TI_C54X_OPCODE_ID_ADD_DUAL_M0_M2,
        TI_C54X_OPCODE_ID_ADD_DUAL_M1_M0,
        TI_C54X_OPCODE_ID_ADD_DUAL_M1_M1,
        TI_C54X_OPCODE_ID_ADD_DUAL_M1_M2,
        TI_C54X_OPCODE_ID_ADD_DUAL_M2_M0,
        TI_C54X_OPCODE_ID_ADD_DUAL_M2_M1,
        TI_C54X_OPCODE_ID_ADD_DUAL_M2_M2,
        TI_C54X_OPCODE_ID_SUB_DUAL_M0_M1,
        TI_C54X_OPCODE_ID_SUB_DUAL_M0_M2,
        TI_C54X_OPCODE_ID_SUB_DUAL_M1_M0,
        TI_C54X_OPCODE_ID_SUB_DUAL_M1_M1,
        TI_C54X_OPCODE_ID_SUB_DUAL_M1_M2,
        TI_C54X_OPCODE_ID_SUB_DUAL_M2_M0,
        TI_C54X_OPCODE_ID_SUB_DUAL_M2_M1,
        TI_C54X_OPCODE_ID_SUB_DUAL_M2_M2,
    };
    size_t index;

    TI_C54X_TEST_CHECK(sizeof(unresolved_rows) / sizeof(unresolved_rows[0]) ==
                       20U);
    for (index = 0U;
         index < sizeof(unresolved_rows) / sizeof(unresolved_rows[0]);
         ++index) {
        check_row_admission(unresolved_rows[index],
                            TI_C54X_REPEATABILITY_UNRESOLVED,
                            TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_BLOCK_UNRESOLVED);
    }
}

int
main(void)
{
    test_admission_tri_state();
    test_invalid_class_rejected_transactionally();
    test_full_catalog_sweep_maps();
    test_documented_nonrepeatable_rows_reject();
    test_new_rows_remain_unresolved();
    puts("repeat_body_eligibility_common: ok");
    return 0;
}
