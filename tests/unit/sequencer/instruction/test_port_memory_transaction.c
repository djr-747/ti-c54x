/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT executable proof of the S6 PORT root
 * transaction hook: PORTR retires IO READ then DATA WRITE, PORTW
 * retires DATA READ then IO WRITE carrying the observed DATA word
 * (template replacement), malformed arms reject at prepare, leg
 * faults stay transactional with no second issue, and in-flight
 * cancel reaches the endpoint. Endpoint scripts are local.
 * Does not own: IO-leg planning, DATA-leg resolution, endpoint
 * implementation, opcode dispatch, CPU lifecycle, or timing.
 */

#include "isa/ops/memory/memory_effect.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "state/accumulator/accumulator.h"
#include "state/arch_state.h"
#include "state/temporary/t_register.h"
#include "../../../support/test_harness.h"

#include <ti_c54x/bus_endpoint.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct port_slot {
    bool seen;
    ti_c54x_bus_word16_request request;
    uint64_t transaction_id;
} port_slot;

typedef struct port_script {
    port_slot slots[2];
    unsigned begin_count;
    /* -1 always READY; >=0 WAIT that many begins, then READY. */
    int wait_begins;
    /* Fault the named slot index on begin (-1 never). */
    int fault_slot;
    uint16_t read_values[2];
    unsigned cancel_calls;
} port_script;

static int
port_slot_for(const port_script *script,
              const ti_c54x_bus_word16_request *request)
{
    if (script->slots[0].seen &&
        script->slots[0].request.space == request->space &&
        script->slots[0].request.address_word == request->address_word &&
        script->slots[0].request.access == request->access) {
        return 0;
    }
    if (script->slots[1].seen &&
        script->slots[1].request.space == request->space &&
        script->slots[1].request.address_word == request->address_word &&
        script->slots[1].request.access == request->access) {
        return 1;
    }
    if (!script->slots[0].seen) {
        return 0;
    }
    if (!script->slots[1].seen) {
        return 1;
    }
    return -1;
}

static void
port_script_advance(void *context, ti_c54x_bus_word16_advance_phase phase,
                    uint64_t transaction_id,
                    const ti_c54x_bus_word16_request *request,
                    ti_c54x_bus_word16_observation *observation)
{
    port_script *script = (port_script *)context;
    const int slot = port_slot_for(script, request);

    TI_C54X_TEST_CHECK(slot >= 0);
    TI_C54X_TEST_CHECK(script->slots[slot].transaction_id == UINT64_C(0) ||
                       script->slots[slot].transaction_id == transaction_id);
    script->slots[slot].seen = true;
    script->slots[slot].request = *request;
    script->slots[slot].transaction_id = transaction_id;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    if (slot == script->fault_slot) {
        /* The endpoint contract carries no read value on FAULT. */
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
        return;
    }
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        observation->read_value_word = script->read_values[slot];
    }
    if (script->wait_begins < 0) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        observation->bus_status = TI_C54X_BUS_OK;
        return;
    }
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        ++script->begin_count;
    }
    if ((int)script->begin_count <= script->wait_begins) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        observation->bus_status = TI_C54X_BUS_OK;
        return;
    }
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
    observation->bus_status = TI_C54X_BUS_OK;
}

static ti_c54x_bus_word16_cancel_result
port_script_cancel(void *context, uint64_t transaction_id,
                   const ti_c54x_bus_word16_request *request)
{
    port_script *script = (port_script *)context;

    (void)transaction_id;
    (void)request;
    ++script->cancel_calls;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint
port_endpoint_for(port_script *script)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = script, .advance = port_script_advance,
        .cancel = port_script_cancel};
    return endpoint;
}

static ti_c54x_op_effect
port_effect(ti_c54x_bus_word16_request_access data_access, uint8_t indirect,
            uint8_t smem_mode, uint8_t smem_arf, uint8_t smem_dma7,
            uint16_t port_address)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16;
    effect.payload.memory.address_spec.port.indirect = indirect;
    effect.payload.memory.address_spec.port.smem_mode = smem_mode;
    effect.payload.memory.address_spec.port.smem_arf = smem_arf;
    effect.payload.memory.address_spec.port.smem_dma7 = smem_dma7;
    effect.payload.memory.address_spec.port.port_address = port_address;
    effect.payload.memory.access = data_access;
    effect.payload.memory.value_word = UINT16_C(0);
    effect.payload.memory.complete = NULL;
    return effect;
}

static ti_c54x_arch_state
known_arch_state(void)
{
    ti_c54x_arch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2110)) ==
                       TI_C54X_ARCH_STATE_OK);
    state.pc_word = UINT32_C(0x20);
    return state;
}

static ti_c54x_decoded_instruction
decoded_nop(void)
{
    const ti_c54x_decoded_instruction instruction = {
        .id = TI_C54X_OPCODE_ID_NOP,
        .operand_layout = TI_C54X_OPERAND_LAYOUT_NONE,
        .repeatability = TI_C54X_REPEATABILITY_UNRESOLVED,
        .input_resources = TI_C54X_INPUT_RESOURCES_NONE,
        .bundle = {.program_words = {UINT16_C(0xF495), UINT16_C(0)},
                   .program_word_count = UINT8_C(1)}};
    return instruction;
}

static ti_c54x_op_input_snapshot
known_input(void)
{
    const ti_c54x_op_input_snapshot input = {
        .accumulator_a_raw40 = UINT64_C(0x1122),
        .accumulator_b_raw40 = UINT64_C(0x3344),
        .accumulator_a_known = 1,
        .accumulator_b_known = 1,
        .status_c = 0,
        .status_c_known = 1,
        .status_tc = 0,
        .status_tc_known = 1,
        .status_sxm = 0,
        .status_sxm_known = 1,
        .status_dp = 0,
        .status_dp_known = 1};
    return input;
}

static ti_c54x_memory_instruction_status
prepare(ti_c54x_memory_instruction_transaction *transaction,
        const ti_c54x_op_effect *effect, ti_c54x_arch_state *state,
        uint64_t attempted, uint64_t retired)
{
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = known_input();

    return ti_c54x_memory_instruction_transaction_prepare(
        transaction, &instruction, &input, effect, state, attempted,
        retired);
}

static ti_c54x_op_input_snapshot
dp_input(void)
{
    return known_input();
}

static void
drive_to_retired(ti_c54x_memory_instruction_transaction *transaction,
                 port_script *script, ti_c54x_arch_state *state,
                 ti_c54x_memory_instruction_result *result)
{
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    ti_c54x_bus_word16_endpoint endpoint = port_endpoint_for(script);

    for (unsigned step = 0U; step < 6U; ++step) {
        TI_C54X_TEST_CHECK(
            ti_c54x_memory_instruction_transaction_advance(
                transaction, &endpoint, state, &accumulators, &t_register,
                result) == TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
        if (result->status == TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
            return;
        }
        TI_C54X_TEST_CHECK(result->status ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    }
    TI_C54X_TEST_CHECK(false);
}

static void
test_portr_indirect_retires(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_op_effect effect = port_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT8_C(1), UINT8_C(2),
        UINT8_C(2), UINT8_C(0), UINT16_C(0x0005));
    port_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.wait_begins = -1;
    script.fault_slot = -1;
    script.read_values[0] = UINT16_C(0xBEEF);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, UINT64_C(0),
                               UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    drive_to_retired(&transaction, &script, &state, &result);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    /* IO READ first, DATA WRITE second with the observed word. */
    TI_C54X_TEST_CHECK(script.slots[0].seen && script.slots[1].seen);
    TI_C54X_TEST_CHECK(script.slots[0].request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_IO);
    TI_C54X_TEST_CHECK(script.slots[0].request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(script.slots[0].request.address_word ==
                       UINT16_C(0x0005));
    TI_C54X_TEST_CHECK(script.slots[1].request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.slots[1].request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(script.slots[1].request.address_word ==
                       UINT16_C(0x2110));
    TI_C54X_TEST_CHECK(script.slots[1].request.write_value_word ==
                       UINT16_C(0xBEEF));
    TI_C54X_TEST_CHECK(script.slots[0].transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(script.slots[1].transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(script.slots[0].transaction_id !=
                       script.slots[1].transaction_id);
}

static void
test_portw_indirect_retires(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_op_effect effect = port_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT8_C(1), UINT8_C(2),
        UINT8_C(2), UINT8_C(0), UINT16_C(0x0005));
    port_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.wait_begins = -1;
    script.fault_slot = -1;
    script.read_values[0] = UINT16_C(0x1234);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, UINT64_C(0),
                               UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    drive_to_retired(&transaction, &script, &state, &result);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* DATA READ first, IO WRITE second carrying the observed word:
     * the zero template never reaches the bus. */
    TI_C54X_TEST_CHECK(script.slots[0].seen && script.slots[1].seen);
    TI_C54X_TEST_CHECK(script.slots[0].request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.slots[0].request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(script.slots[0].request.address_word ==
                       UINT16_C(0x2110));
    TI_C54X_TEST_CHECK(script.slots[1].request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_IO);
    TI_C54X_TEST_CHECK(script.slots[1].request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(script.slots[1].request.address_word ==
                       UINT16_C(0x0005));
    TI_C54X_TEST_CHECK(script.slots[1].request.write_value_word ==
                       UINT16_C(0x1234));
}

static void
test_portr_direct_retires(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = dp_input();
    ti_c54x_op_effect effect = port_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT8_C(0), UINT8_C(0),
        UINT8_C(0), UINT8_C(0x60), UINT16_C(0x0005));
    port_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.wait_begins = -1;
    script.fault_slot = -1;
    script.read_values[0] = UINT16_C(0x00A5);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_prepare(
                           &transaction, &instruction, &input, &effect,
                           &state, UINT64_C(0), UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    drive_to_retired(&transaction, &script, &state, &result);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(script.slots[1].request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.slots[1].request.address_word ==
                       UINT16_C(0x0060));
    TI_C54X_TEST_CHECK(script.slots[1].request.write_value_word ==
                       UINT16_C(0x00A5));
}

static void
test_portw_direct_retires(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = dp_input();
    ti_c54x_op_effect effect = port_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT8_C(0), UINT8_C(0),
        UINT8_C(0), UINT8_C(0x60), UINT16_C(0x0005));
    port_script script = {0};
    ti_c54x_memory_instruction_result result = {0};

    script.wait_begins = -1;
    script.fault_slot = -1;
    script.read_values[0] = UINT16_C(0x1234);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_prepare(
                           &transaction, &instruction, &input, &effect,
                           &state, UINT64_C(0), UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    drive_to_retired(&transaction, &script, &state, &result);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* DATA direct-DP READ first, IO WRITE second carrying the
     * observed word. */
    TI_C54X_TEST_CHECK(script.slots[0].seen && script.slots[1].seen);
    TI_C54X_TEST_CHECK(script.slots[0].request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(script.slots[0].request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(script.slots[0].request.address_word ==
                       UINT16_C(0x0060));
    TI_C54X_TEST_CHECK(script.slots[1].request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_IO);
    TI_C54X_TEST_CHECK(script.slots[1].request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(script.slots[1].request.address_word ==
                       UINT16_C(0x0005));
    TI_C54X_TEST_CHECK(script.slots[1].request.write_value_word ==
                       UINT16_C(0x1234));
}

static void
test_malformed_prepare_rejects(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_op_effect effect = port_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT8_C(2), UINT8_C(0),
        UINT8_C(0), UINT8_C(0x60), UINT16_C(0x0005));

    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, UINT64_C(0),
                               UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE);
}

static void
test_leg_fault_is_transactional(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_op_effect effect = port_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT8_C(1), UINT8_C(2),
        UINT8_C(2), UINT8_C(0), UINT16_C(0x0005));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    port_script script = {0};
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_memory_instruction_result result = {0};

    script.wait_begins = -1;
    script.fault_slot = 0;
    script.read_values[0] = UINT16_C(0xBEEF);
    endpoint = port_endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, UINT64_C(0),
                               UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(!script.slots[1].seen);
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x20));
}

static void
test_cancel_in_flight(void)
{
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_op_effect effect = port_effect(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT8_C(1), UINT8_C(2),
        UINT8_C(2), UINT8_C(0), UINT16_C(0x0005));
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    port_script script = {0};
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_memory_instruction_result result = {0};

    script.wait_begins = 100;
    script.fault_slot = -1;
    endpoint = port_endpoint_for(&script);
    TI_C54X_TEST_CHECK(prepare(&transaction, &effect, &state, UINT64_C(0),
                               UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_cancel(
                           &transaction, &endpoint) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED);
    TI_C54X_TEST_CHECK(script.cancel_calls == 1U);
}

int
main(void)
{
    test_portr_indirect_retires();
    test_portw_indirect_retires();
    test_portr_direct_retires();
    test_portw_direct_retires();
    test_malformed_prepare_rejects();
    test_leg_fault_is_transactional();
    test_cancel_in_flight();
    return 0;
}
