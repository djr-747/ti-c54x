/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT executable proof of the S6 PORT IO-leg
 * plan: direct and indirect PORT specs plan IO-space READ/WRITE
 * requests at the absolute port address with no state observation,
 * WRITE templates carry a documented zero placeholder the reducer
 * replaces before issue, malformed arms reject, and NULL inputs
 * reject. Planning never touches state.
 * Does not own: DATA-leg resolution, request issue, endpoint
 * traffic, opcode semantics, CPU lifecycle, or timing.
 */

#include "sequencer/memory/port_access_plan.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_op_memory_address_spec
port_spec(uint8_t indirect, uint8_t smem_mode, uint8_t smem_arf,
          uint8_t smem_dma7, uint16_t port_address)
{
    ti_c54x_op_memory_address_spec spec = {0};

    spec.kind = TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16;
    spec.port.indirect = indirect;
    spec.port.smem_mode = smem_mode;
    spec.port.smem_arf = smem_arf;
    spec.port.smem_dma7 = smem_dma7;
    spec.port.port_address = port_address;
    return spec;
}

static void
check_io_request(const ti_c54x_port_access *plan,
                 ti_c54x_bus_word16_request_access access, uint16_t address,
                 uint16_t value)
{
    TI_C54X_TEST_CHECK(plan->status == TI_C54X_PORT_ACCESS_IO_REQUEST);
    TI_C54X_TEST_CHECK(plan->request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_IO);
    TI_C54X_TEST_CHECK(plan->request.access == access);
    TI_C54X_TEST_CHECK(plan->request.address_word == address);
    TI_C54X_TEST_CHECK(plan->request.write_value_word == value);
}

static void
test_direct_portr_plans_io_read(void)
{
    /* DATA-leg WRITE (PORTR) plans the inverse IO-leg READ. */
    const ti_c54x_op_memory_address_spec spec =
        port_spec(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0x60),
                  UINT16_C(0x0005));
    const ti_c54x_port_access plan = ti_c54x_sequencer_plan_port_access(
        &spec, TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);

    check_io_request(&plan, TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                     UINT16_C(0x0005), UINT16_C(0));
}

static void
test_direct_portw_plans_io_write_template(void)
{
    /* DATA-leg READ (PORTW) plans the inverse IO-leg WRITE template;
     * the zero value is a placeholder the reducer replaces with the
     * observed DATA word before issue. */
    const ti_c54x_op_memory_address_spec spec =
        port_spec(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0x07),
                  UINT16_C(0x0005));
    const ti_c54x_port_access plan = ti_c54x_sequencer_plan_port_access(
        &spec, TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

    check_io_request(&plan, TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                     UINT16_C(0x0005), UINT16_C(0));
}

static void
test_indirect_legs_plan(void)
{
    for (uint16_t mode = UINT16_C(0); mode <= UINT16_C(10); ++mode) {
        const ti_c54x_op_memory_address_spec spec = port_spec(
            UINT8_C(1), (uint8_t)mode, UINT8_C(3), UINT8_C(0),
            UINT16_C(0xFFFF));
        const ti_c54x_port_access plan =
            ti_c54x_sequencer_plan_port_access(
                &spec, TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);

        check_io_request(&plan, TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                         UINT16_C(0xFFFF), UINT16_C(0));
    }
}

static void
test_malformed_arms_reject(void)
{
    static const ti_c54x_op_memory_address_spec bad[] = {
        /* indirect select out of range */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16,
         .port = {.indirect = 2, .smem_dma7 = 0x60, .port_address = 5}},
        /* direct leg with mode set */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16,
         .port = {.smem_mode = 1, .smem_dma7 = 0x60, .port_address = 5}},
        /* direct leg with arf set */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16,
         .port = {.smem_arf = 1, .smem_dma7 = 0x60, .port_address = 5}},
        /* direct dma out of range */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16,
         .port = {.smem_dma7 = 0x80, .port_address = 5}},
        /* indirect leg with dma set */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16,
         .port = {.indirect = 1,
                  .smem_mode = 2,
                  .smem_arf = 3,
                  .smem_dma7 = 1,
                  .port_address = 5}},
        /* indirect arf out of range */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16,
         .port = {.indirect = 1, .smem_arf = 8, .port_address = 5}},
        /* indirect mode out of range */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16,
         .port = {.indirect = 1, .smem_mode = 11, .port_address = 5}},
        /* reserved half set */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16,
         .port = {.smem_dma7 = 0x60,
                  .port_address = 5,
                  .reserved_half = 1}},
        /* reserved word set */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16,
         .port = {.smem_dma7 = 0x60,
                  .port_address = 5,
                  .reserved_word1 = 1}},
        /* wrong kind */
        {.kind = TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16,
         .direct_dma7 = 0x60},
    };

    for (size_t index = 0U; index < sizeof(bad) / sizeof(bad[0]);
         ++index) {
        const ti_c54x_port_access plan = ti_c54x_sequencer_plan_port_access(
            &bad[index], TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

        TI_C54X_TEST_CHECK(plan.status == TI_C54X_PORT_ACCESS_INVALID_SPEC);
        TI_C54X_TEST_CHECK(plan.request.space ==
                           TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID);
        TI_C54X_TEST_CHECK(plan.request.address_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(plan.request.write_value_word == UINT16_C(0));
    }
}

static void
test_bad_arguments_reject(void)
{
    const ti_c54x_op_memory_address_spec spec =
        port_spec(UINT8_C(0), UINT8_C(0), UINT8_C(0), UINT8_C(0x60),
                  UINT16_C(0x0005));
    ti_c54x_port_access plan;

    plan = ti_c54x_sequencer_plan_port_access(
        NULL, TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(plan.status == TI_C54X_PORT_ACCESS_INVALID_ARGUMENT);
    plan = ti_c54x_sequencer_plan_port_access(
        &spec, (ti_c54x_bus_word16_request_access)UINT8_C(7));
    TI_C54X_TEST_CHECK(plan.status == TI_C54X_PORT_ACCESS_INVALID_SPEC);
}

int
main(void)
{
    test_direct_portr_plans_io_read();
    test_direct_portw_plans_io_write_template();
    test_indirect_legs_plan();
    test_malformed_arms_reject();
    test_bad_arguments_reject();
    return 0;
}
