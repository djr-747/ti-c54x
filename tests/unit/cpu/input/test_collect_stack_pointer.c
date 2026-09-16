/* Module contract
 * Owns: executable proof for stack-pointer input collection, including
 * canonical unknownness after common reset, independent SP selection,
 * typed-write knownness, and transactional rejection.
 * Does not own: collection implementation, arch-state accessors, opcode
 * semantics, decode, CPU lifecycle, sequencing, timing, memory, buses, or
 * diagnostics.
 */

#include "../../../../src/cpu/input/collect.h"
#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/ops/op_input.h"
#include "../../../../src/state/arch_state.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_input_sources sources_for(const ti_c54x_arch_state *arch)
{
    ti_c54x_input_sources sources = {0};

    sources.arch_state = arch;
    return sources;
}

static ti_c54x_op_input_snapshot sentinel(void)
{
    ti_c54x_op_input_snapshot output = {0};

    output.stack_pointer_raw16 = UINT16_C(0xBEEF);
    output.stack_pointer_known = true;
    output.status_sxm = 1U;
    output.status_sxm_known = true;
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

int main(void)
{
    ti_c54x_arch_state arch;
    ti_c54x_input_sources sources;
    ti_c54x_op_input_snapshot output;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&arch) ==
                       TI_C54X_ARCH_STATE_OK);
    sources = sources_for(&arch);

    /* NONE remains source-independent and canonicalizes SP fields. */
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_NONE, NULL, &output) ==
                       TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.stack_pointer_raw16 == UINT16_C(0));
    TI_C54X_TEST_CHECK(!output.stack_pointer_known);
    TI_C54X_TEST_CHECK(output.status_sxm == 0U);
    TI_C54X_TEST_CHECK(!output.status_sxm_known);

    /* Common reset deliberately leaves SP unknown. */
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STACK_POINTER, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.stack_pointer_raw16 == UINT16_C(0));
    TI_C54X_TEST_CHECK(!output.stack_pointer_known);
    TI_C54X_TEST_CHECK(output.status_sxm == 0U);
    TI_C54X_TEST_CHECK(!output.status_sxm_known);

    /* A typed write makes SP known without changing SXM knownness. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&arch,
                                                  UINT16_C(0x1110)) ==
                       TI_C54X_ARCH_STATE_OK);
    output = sentinel();
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STACK_POINTER, &sources,
                           &output) == TI_C54X_INPUT_COLLECT_OK);
    TI_C54X_TEST_CHECK(output.stack_pointer_raw16 == UINT16_C(0x1110));
    TI_C54X_TEST_CHECK(output.stack_pointer_known);
    TI_C54X_TEST_CHECK(output.status_sxm == 0U);
    TI_C54X_TEST_CHECK(!output.status_sxm_known);

    /* Missing SP sources, invalid masks, and NULL outputs are nonmutating. */
    output = sentinel();
    expect_rejected(TI_C54X_INPUT_RESOURCES_STACK_POINTER, NULL, &output);
    output = sentinel();
    expect_rejected((ti_c54x_input_resource_mask)0x1000U, &sources, &output);
    TI_C54X_TEST_CHECK(ti_c54x_input_collect(
                           TI_C54X_INPUT_RESOURCES_STACK_POINTER, &sources,
                           NULL) != TI_C54X_INPUT_COLLECT_OK);
    return 0;
}
