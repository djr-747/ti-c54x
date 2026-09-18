/*
 * Module contract
 *
 * Owns: executable proofs that a staged RMW-ADD candidate retires through
 * the root WORD16 transaction with the reduced write word on the bus and
 * sticky C/OVA committed from the frozen OVM/SXM modes: carry sets C,
 * overflow sets OVA, and both bits are otherwise preserved.
 * Does not own: opcode dispatch, address arithmetic, endpoint
 * implementation, timing, observer policy, or CPU lifecycle.
 */

#include "test_memory_instruction_fixture.h"

#include "state/status/registers.h"

#include <stddef.h>
#include <string.h>

typedef struct add_script {
    uint16_t read_value_word;
    unsigned calls;
    ti_c54x_bus_word16_request requests[4];
    ti_c54x_bus_word16_advance_phase phases[4];
} add_script;

static void
add_advance(void *context, ti_c54x_bus_word16_advance_phase phase,
            uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request,
            ti_c54x_bus_word16_observation *observation)
{
    add_script *script = (add_script *)context;

    TI_C54X_TEST_CHECK(script->calls < 4U);
    script->requests[script->calls] = *request;
    script->phases[script->calls] = phase;
    ++script->calls;
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word =
            request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
                ? script->read_value_word
                : UINT16_C(0),
        .bus_status = TI_C54X_BUS_OK};
}

static ti_c54x_bus_word16_cancel_result
add_cancel(void *context, uint64_t transaction_id,
           const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_op_effect add_effect(uint16_t addend_word)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_MEMORY;
    effect.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
    effect.payload.memory.address_spec.mode = TI_C54X_INDIRECT_SINGLE_MOD2;
    effect.payload.memory.address_spec.arf = UINT8_C(2);
    effect.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    effect.payload.memory.value_word = addend_word;
    effect.payload.memory.rmw_operation =
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_ADD;
    effect.payload.memory.complete = NULL;
    return effect;
}

static ti_c54x_op_input_snapshot modes_input(uint8_t ovm, uint8_t sxm)
{
    ti_c54x_op_input_snapshot input = input_snapshot();

    input.status_ovm = ovm;
    input.status_ovm_known = 1;
    input.status_sxm = sxm;
    input.status_sxm_known = 1;
    return input;
}

static uint8_t read_c(const ti_c54x_arch_state *state)
{
    const ti_c54x_status_register_field_result field =
        ti_c54x_status_register_read_c(&state->status);

    TI_C54X_TEST_CHECK(field.known);
    return (uint8_t)field.raw_value;
}

static uint8_t read_ova(const ti_c54x_arch_state *state)
{
    const ti_c54x_status_register_field_result field =
        ti_c54x_status_register_read_ova(&state->status);

    TI_C54X_TEST_CHECK(field.known);
    return (uint8_t)field.raw_value;
}

typedef struct add_commit_vector {
    uint16_t read_value_word;
    uint16_t addend_word;
    uint8_t overflow_mode;
    uint8_t sign_extension_mode;
    uint16_t expected_write_word;
    uint8_t expected_c;
    uint8_t expected_ova;
} add_commit_vector;

/* TI SPRU172C ADDM Example 1 analog plus the Example 2 saturation
 * vector (mem 0x8007 + lk 0xFFF8 under OVM=1/SXM=1 writes 0x8000
 * with C=1 and OVA=1). */
static const add_commit_vector commit_vectors[] = {
    {UINT16_C(0x0004), UINT16_C(0x123B), UINT8_C(0), UINT8_C(0),
     UINT16_C(0x123F), UINT8_C(0), UINT8_C(0)},
    {UINT16_C(0x8007), UINT16_C(0xFFF8), UINT8_C(1), UINT8_C(1),
     UINT16_C(0x8000), UINT8_C(1), UINT8_C(1)},
    {UINT16_C(0x8007), UINT16_C(0xFFF8), UINT8_C(0), UINT8_C(1),
     UINT16_C(0x7FFF), UINT8_C(1), UINT8_C(1)},
    {UINT16_C(0xFFFF), UINT16_C(0x0001), UINT8_C(1), UINT8_C(0),
     UINT16_C(0xFFFF), UINT8_C(1), UINT8_C(1)},
    {UINT16_C(0x7FFF), UINT16_C(0x0001), UINT8_C(1), UINT8_C(1),
     UINT16_C(0x7FFF), UINT8_C(0), UINT8_C(1)},
};

static void check_commit_vectors(void)
{
    for (size_t index = 0U;
         index < sizeof(commit_vectors) / sizeof(commit_vectors[0]);
         ++index) {
        const add_commit_vector *vector = &commit_vectors[index];
        ti_c54x_op_effect effect = add_effect(vector->addend_word);
        ti_c54x_memory_instruction_transaction transaction = {0};
        ti_c54x_arch_state state = known_arch_state();
        ti_c54x_accumulator_state accumulators = {0};
        ti_c54x_t_register_state t_register = {0};
        add_script script = {vector->read_value_word, 0, {{0}}, {0}};
        const ti_c54x_bus_word16_endpoint endpoint = {
            .context = &script, .advance = add_advance, .cancel = add_cancel};
        const ti_c54x_decoded_instruction instruction = decoded_nop();
        const ti_c54x_op_input_snapshot input = modes_input(
            vector->overflow_mode, vector->sign_extension_mode);
        ti_c54x_memory_instruction_result result = {0};

        TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                           TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                           TI_C54X_T_REGISTER_OK);
        /* Start from cleared sticky bits so the commit vectors prove
         * the set path; preservation is covered separately. */
        TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&state.status,
                                                           UINT8_C(0)) ==
                           TI_C54X_STATUS_REGISTER_OK);
        TI_C54X_TEST_CHECK(ti_c54x_status_register_write_ova(&state.status,
                                                             UINT8_C(0)) ==
                           TI_C54X_STATUS_REGISTER_OK);
        TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_prepare(
                               &transaction, &instruction, &input, &effect,
                               &state, UINT64_C(0), UINT64_C(0)) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                               &transaction, &endpoint, &state, &accumulators,
                               &t_register, &result) ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
        TI_C54X_TEST_CHECK(script.calls == 2U);
        TI_C54X_TEST_CHECK(
            script.requests[0].access ==
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
        TI_C54X_TEST_CHECK(
            script.requests[1].access ==
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
        TI_C54X_TEST_CHECK(script.requests[1].write_value_word ==
                           vector->expected_write_word);
        TI_C54X_TEST_CHECK(read_c(&state) == vector->expected_c);
        TI_C54X_TEST_CHECK(read_ova(&state) == vector->expected_ova);
    }
}

static void check_sticky_preservation(void)
{
    ti_c54x_op_effect effect = add_effect(UINT16_C(0x123B));
    ti_c54x_memory_instruction_transaction transaction = {0};
    ti_c54x_arch_state state = known_arch_state();
    ti_c54x_accumulator_state accumulators = {0};
    ti_c54x_t_register_state t_register = {0};
    add_script script = {UINT16_C(0x0004), 0, {{0}}, {0}};
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = &script, .advance = add_advance, .cancel = add_cancel};
    const ti_c54x_decoded_instruction instruction = decoded_nop();
    const ti_c54x_op_input_snapshot input = modes_input(UINT8_C(0),
                                                        UINT8_C(0));
    ti_c54x_memory_instruction_result result = {0};

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_reset(&accumulators) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&t_register) ==
                       TI_C54X_T_REGISTER_OK);
    /* Pre-set both sticky bits: a carry/overflow-free ADDM must leave
     * them set. */
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&state.status,
                                                       UINT8_C(1)) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_ova(&state.status,
                                                         UINT8_C(1)) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_prepare(
                           &transaction, &instruction, &input, &effect,
                           &state, UINT64_C(0), UINT64_C(0)) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_memory_instruction_transaction_advance(
                           &transaction, &endpoint, &state, &accumulators,
                           &t_register, &result) ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_OK);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED);
    TI_C54X_TEST_CHECK(script.requests[1].write_value_word ==
                       UINT16_C(0x123F));
    TI_C54X_TEST_CHECK(read_c(&state) == UINT8_C(1));
    TI_C54X_TEST_CHECK(read_ova(&state) == UINT8_C(1));
}

int main(void)
{
    /* Fixture helpers owned by other transaction tests in this directory. */
    (void)memory_effect;
    (void)prepare;
    (void)advance;
    check_commit_vectors();
    check_sticky_preservation();
    return 0;
}
