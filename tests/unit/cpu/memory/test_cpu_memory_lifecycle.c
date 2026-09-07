/* Module contract
 * Owns: W31B1 public CPU lifecycle proofs: copied/validated endpoint
 * descriptors, fresh/deinitialized storage, run(0), and observer silence.
 * Does not own: root transaction semantics, opcode dispatch, adapter internals,
 * or pending-work cancellation/resume proofs.
 */

#include "fixture.h"

#include <stddef.h>

static void
test_endpoint_is_copied_once_and_absent_is_canonical(void)
{
    cpu_memory_fixture fixture = {0};
    cpu_memory_fixture replacement = {0};
    ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    const ti_c54x_bus_word16_endpoint original = endpoint;
    ti_c54x_error status;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(storage, &fixture, &endpoint, NULL, &status);

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(cpu != NULL);
    endpoint.context = &replacement;
    endpoint.advance = NULL;
    endpoint.cancel = NULL;
    TI_C54X_TEST_CHECK(cpu->bus.data_endpoint.context == original.context);
    TI_C54X_TEST_CHECK(cpu->bus.data_endpoint.advance == original.advance);
    TI_C54X_TEST_CHECK(cpu->bus.data_endpoint.cancel == original.cancel);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);

    {
        ti_c54x_bus absent_bus = make_bus(&fixture, NULL);
        ti_c54x_cpu *absent_cpu = NULL;

        TI_C54X_TEST_CHECK(ti_c54x_cpu_init(
                               storage, ti_c54x_cpu_size(), NULL, &absent_bus,
                               &absent_cpu) == TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(absent_cpu->bus.data_endpoint.context == NULL);
        TI_C54X_TEST_CHECK(absent_cpu->bus.data_endpoint.advance == NULL);
        TI_C54X_TEST_CHECK(absent_cpu->bus.data_endpoint.cancel == NULL);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(absent_cpu) ==
                           TI_C54X_ERROR_OK);
    }
}

static void
test_partial_endpoint_rejected_and_deinit_allows_reuse(void)
{
    cpu_memory_fixture fixture = {0};
    ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    ti_c54x_bus partial_bus = make_bus(&fixture, &endpoint);
    ti_c54x_error status;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    _Alignas(max_align_t) unsigned char partial_storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(storage, &fixture, &endpoint, NULL, &status);
    ti_c54x_cpu *partial_cpu = NULL;
    ti_c54x_cpu *reused_cpu = NULL;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    partial_bus.data_endpoint.cancel = NULL;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(
                           partial_storage, ti_c54x_cpu_size(), NULL,
                           &partial_bus, &partial_cpu) ==
                       TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(partial_cpu == NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(
                           storage, ti_c54x_cpu_size(), NULL, &partial_bus,
                           &reused_cpu) == TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(reused_cpu == NULL);
    partial_bus.data_endpoint.cancel = endpoint.cancel;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(
                           storage, ti_c54x_cpu_size(), NULL, &partial_bus,
                           &reused_cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(reused_cpu) == TI_C54X_ERROR_OK);
}

static void
test_run_zero_and_null_deinit_are_side_effect_free(void)
{
    cpu_memory_fixture fixture = {0};
    ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    ti_c54x_trace_sink trace = {TI_C54X_TRACE_MASK_ALL, &fixture, emit_trace};
    ti_c54x_error status;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(storage, &fixture, &endpoint, &trace, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(0));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.advance_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.fetch_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.fetch_events == 0U);
    TI_C54X_TEST_CHECK(fixture.stop_events == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(NULL) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

_Static_assert(TI_C54X_STOP_BUS_WAIT == 9,
               "BUS_WAIT stop value is appended after existing ABI values");

int
main(void)
{
    test_endpoint_is_copied_once_and_absent_is_canonical();
    test_partial_endpoint_rejected_and_deinit_allows_reuse();
    test_run_zero_and_null_deinit_are_side_effect_free();
    return 0;
}
