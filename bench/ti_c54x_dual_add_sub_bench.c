/*
 * Module contract
 *
 * Owns: an observer-off hosted throughput cohort for four dual-memory WORD16
 * ADD/SUB MOD0 forms. It drives the public CPU with a synchronous DATA
 * endpoint, proves one X-then-Y read pair per retirement, and emits one JSON
 * object after timing.
 * Does not own: ISA semantics, machine-cycle timing, thresholds, history,
 * environment controls, observers, or callback diagnostics.
 */
#if !defined(TI_C54X_ENABLE_OBSERVERS) || TI_C54X_ENABLE_OBSERVERS != 0
#error "ti_c54x_dual_add_sub_bench requires TI_C54X_ENABLE_OBSERVERS=0"
#endif
#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ti_c54x/cpu.h>
#include "../src/cpu/cpu_internal.h"
#ifndef CLOCK_MONOTONIC
#error "Hosted benchmark requires POSIX CLOCK_MONOTONIC"
#endif
enum {
    DUAL_ADD_SUB_SCHEMA_VERSION = 1,
    DUAL_ADD_SUB_SAMPLES = 7,
    DUAL_ADD_SUB_WARMUPS = 16,
    DUAL_ADD_SUB_WINDOWS = 16,
    DUAL_ADD_SUB_RETIREMENTS_PER_WINDOW = 20000,
    DUAL_ADD_SUB_X_AR = 2,
    DUAL_ADD_SUB_Y_AR = 3,
    DUAL_ADD_SUB_X_ADDRESS = 0x0100,
    DUAL_ADD_SUB_Y_ADDRESS = 0x0200,
    DUAL_ADD_SUB_X_WORD = 0x1234,
    DUAL_ADD_SUB_Y_WORD = 0x0002,
    DUAL_ADD_SUB_INITIAL_A = 0x0000000001,
    DUAL_ADD_SUB_INITIAL_B = 0x0000000002
};
typedef enum dual_add_sub_operation {
    DUAL_ADD_SUB_ADD = 0,
    DUAL_ADD_SUB_SUB = 1
} dual_add_sub_operation;
typedef enum dual_add_sub_destination {
    DUAL_ADD_SUB_DESTINATION_A = 0,
    DUAL_ADD_SUB_DESTINATION_B = 1
} dual_add_sub_destination;
typedef struct dual_add_sub_scenario {
    const char *name;
    uint16_t program_word;
    dual_add_sub_operation operation;
    dual_add_sub_destination destination;
    uint64_t expected_destination_raw40;
} dual_add_sub_scenario;
typedef struct dual_add_sub_bus_context {
    const dual_add_sub_scenario *scenario;
    uint64_t endpoint_calls;
    uint64_t read_calls;
    uint64_t write_calls;
    uint64_t program_reads;
    uint64_t first_id;
    uint64_t last_id;
    bool request_order_valid;
    bool address_sequence_valid;
    bool fresh_ids;
} dual_add_sub_bus_context;
typedef struct dual_add_sub_sample {
    uint64_t elapsed_ns;
} dual_add_sub_sample;
typedef struct dual_add_sub_stats {
    uint64_t elapsed_ns;
    uint64_t min_elapsed_ns;
    uint64_t max_elapsed_ns;
    uint64_t first_transaction_id;
    uint64_t last_transaction_id;
    uint64_t final_a_raw40;
    uint64_t final_b_raw40;
    uint8_t final_c;
    uint8_t final_ova;
    uint8_t final_ovb;
} dual_add_sub_stats;
static ti_c54x_bus_status dual_add_sub_program_read(
    void *context, ti_c54x_program_word_address address_word,
    uint16_t *value_word)
{
    dual_add_sub_bus_context *bus = (dual_add_sub_bus_context *)context;
    if (bus == NULL || bus->scenario == NULL || value_word == NULL) {
        return TI_C54X_BUS_FAULT;
    }
    (void)address_word;
    bus->program_reads++;
    *value_word = bus->scenario->program_word;
    return TI_C54X_BUS_OK;
}
static void dual_add_sub_data_advance(
    void *context, ti_c54x_bus_word16_advance_phase phase,
    uint64_t transaction_id, const ti_c54x_bus_word16_request *request,
    ti_c54x_bus_word16_observation *observation)
{
    dual_add_sub_bus_context *bus = (dual_add_sub_bus_context *)context;
    const uint64_t call = bus == NULL ? UINT64_C(0) : bus->endpoint_calls;
    const bool x_phase = (call % UINT64_C(2)) == UINT64_C(0);
    const uint32_t expected_address =
        x_phase ? (uint32_t)DUAL_ADD_SUB_X_ADDRESS
                : (uint32_t)DUAL_ADD_SUB_Y_ADDRESS;
    const uint16_t expected_word =
        x_phase ? UINT16_C(DUAL_ADD_SUB_X_WORD)
                : UINT16_C(DUAL_ADD_SUB_Y_WORD);
    const bool valid =
        bus != NULL && phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN &&
        request != NULL && observation != NULL &&
        transaction_id != UINT64_C(0) &&
        request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
        request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
        request->address_word == expected_address &&
        request->write_value_word == UINT16_C(0);
    if (bus == NULL || bus->scenario == NULL || !valid) {
        if (bus != NULL) {
            bus->request_order_valid = false;
            bus->address_sequence_valid = false;
            bus->fresh_ids = false;
        }
        return;
    }
    if (call == UINT64_C(0)) {
        bus->first_id = transaction_id;
    } else if (transaction_id <= bus->last_id) {
        bus->fresh_ids = false;
    }
    bus->last_id = transaction_id;
    bus->read_calls++;
    bus->endpoint_calls = call + UINT64_C(1);
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = expected_word,
        .bus_status = TI_C54X_BUS_OK};
}
static ti_c54x_bus_word16_cancel_result dual_add_sub_data_cancel(
    void *context, uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}
static void dual_add_sub_context_reset(dual_add_sub_bus_context *bus,
                                       const dual_add_sub_scenario *scenario)
{
    *bus = (dual_add_sub_bus_context){
        .scenario = scenario,
        .request_order_valid = true,
        .address_sequence_valid = true,
        .fresh_ids = true};
}
static bool dual_add_sub_context_valid(const dual_add_sub_bus_context *bus,
                                       uint64_t retired)
{
    return bus != NULL && bus->endpoint_calls == retired * UINT64_C(2) &&
           bus->read_calls == retired * UINT64_C(2) &&
           bus->write_calls == UINT64_C(0) &&
           bus->program_reads == retired &&
           bus->request_order_valid && bus->address_sequence_valid &&
           bus->fresh_ids && bus->first_id != UINT64_C(0) &&
           bus->last_id > bus->first_id;
}
static bool dual_add_sub_now_ns(uint64_t *out_ns)
{
    struct timespec now;
    if (out_ns == NULL || clock_gettime(CLOCK_MONOTONIC, &now) != 0 ||
        now.tv_sec < 0 || now.tv_nsec < 0 || now.tv_nsec >= 1000000000L) {
        return false;
    }
    *out_ns = (uint64_t)now.tv_sec * UINT64_C(1000000000) +
              (uint64_t)now.tv_nsec;
    return true;
}
static bool dual_add_sub_prepare_cpu(ti_c54x_cpu *cpu)
{
    return cpu != NULL && ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK &&
           ti_c54x_arch_state_write_cmpt(&cpu->arch_state, UINT8_C(0)) ==
               TI_C54X_ARCH_STATE_OK &&
           ti_c54x_arch_state_write_arp(&cpu->arch_state, UINT8_C(0)) ==
               TI_C54X_ARCH_STATE_OK &&
           ti_c54x_arch_state_write_ar(
               &cpu->arch_state, UINT8_C(DUAL_ADD_SUB_X_AR),
               UINT16_C(DUAL_ADD_SUB_X_ADDRESS)) == TI_C54X_ARCH_STATE_OK &&
           ti_c54x_arch_state_write_ar(
               &cpu->arch_state, UINT8_C(DUAL_ADD_SUB_Y_AR),
               UINT16_C(DUAL_ADD_SUB_Y_ADDRESS)) == TI_C54X_ARCH_STATE_OK &&
           ti_c54x_accumulator_write(
               &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
               (uint64_t)DUAL_ADD_SUB_INITIAL_A) == TI_C54X_ACCUMULATOR_OK &&
           ti_c54x_accumulator_write(
               &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
               (uint64_t)DUAL_ADD_SUB_INITIAL_B) == TI_C54X_ACCUMULATOR_OK &&
           (cpu->arch_state.status.st1 = (uint16_t)(
                cpu->arch_state.status.st1 & (uint16_t)~TI_C54X_ST1_SXM_MASK),
            cpu->arch_state.status.st1_known_mask = (uint16_t)(
                cpu->arch_state.status.st1_known_mask |
                TI_C54X_ST1_SXM_MASK),
            true) &&
           ti_c54x_status_register_write_ovm(&cpu->arch_state.status,
                                             UINT8_C(0)) ==
               TI_C54X_STATUS_REGISTER_OK &&
           ti_c54x_status_register_write_c(&cpu->arch_state.status,
                                           UINT8_C(0)) ==
               TI_C54X_STATUS_REGISTER_OK &&
           ti_c54x_status_register_write_ova(&cpu->arch_state.status,
                                             UINT8_C(0)) ==
               TI_C54X_STATUS_REGISTER_OK &&
           ti_c54x_status_register_write_ovb(&cpu->arch_state.status,
                                             UINT8_C(0)) ==
               TI_C54X_STATUS_REGISTER_OK;
}
static bool dual_add_sub_state_valid(const ti_c54x_cpu *cpu,
                                     const dual_add_sub_scenario *scenario,
                                     uint64_t expected_pc)
{
    uint64_t a_raw40 = UINT64_C(0);
    uint64_t b_raw40 = UINT64_C(0);
    const ti_c54x_status_register_field_result c =
        ti_c54x_status_register_read_c(&cpu->arch_state.status);
    const ti_c54x_status_register_field_result ova =
        ti_c54x_status_register_read_ova(&cpu->arch_state.status);
    const ti_c54x_status_register_field_result ovb =
        ti_c54x_status_register_read_ovb(&cpu->arch_state.status);
    const uint64_t expected_a =
        scenario->destination == DUAL_ADD_SUB_DESTINATION_A
            ? scenario->expected_destination_raw40
            : (uint64_t)DUAL_ADD_SUB_INITIAL_A;
    const uint64_t expected_b =
        scenario->destination == DUAL_ADD_SUB_DESTINATION_B
            ? scenario->expected_destination_raw40
            : (uint64_t)DUAL_ADD_SUB_INITIAL_B;

    return cpu != NULL && scenario != NULL &&
           ti_c54x_accumulator_read(&cpu->accumulators,
                                    TI_C54X_ACCUMULATOR_A, &a_raw40) ==
               TI_C54X_ACCUMULATOR_OK &&
           ti_c54x_accumulator_read(&cpu->accumulators,
                                    TI_C54X_ACCUMULATOR_B, &b_raw40) ==
               TI_C54X_ACCUMULATOR_OK &&
           a_raw40 == expected_a && b_raw40 == expected_b &&
           c.status == TI_C54X_STATUS_REGISTER_OK &&
           c.raw_value == UINT16_C(0) &&
           ova.status == TI_C54X_STATUS_REGISTER_OK && ova.raw_value == 0U &&
           ovb.status == TI_C54X_STATUS_REGISTER_OK && ovb.raw_value == 0U &&
           cpu->arch_state.pc_word == expected_pc;
}
static bool dual_add_sub_run_window(ti_c54x_cpu *cpu,
                                    dual_add_sub_bus_context *bus,
                                    const dual_add_sub_scenario *scenario,
                                    bool timed, uint64_t *elapsed_out)
{
    const uint64_t budget = DUAL_ADD_SUB_RETIREMENTS_PER_WINDOW;
    ti_c54x_run_result result;
    uint64_t start_ns = UINT64_C(0);
    uint64_t end_ns = UINT64_C(0);

    dual_add_sub_context_reset(bus, scenario);
    if (!dual_add_sub_prepare_cpu(cpu) ||
        (timed && !dual_add_sub_now_ns(&start_ns))) {
        return false;
    }
    result = ti_c54x_cpu_run(cpu, budget);
    if (timed && !dual_add_sub_now_ns(&end_ns)) {
        return false;
    }
    if (result.reason != TI_C54X_STOP_BUDGET_EXHAUSTED ||
        result.bus_status != TI_C54X_BUS_OK ||
        result.instructions_attempted != budget ||
        result.instructions_retired != budget || result.pc_word != budget ||
        !dual_add_sub_context_valid(bus, budget) ||
        !dual_add_sub_state_valid(cpu, scenario, budget) ||
        (timed && end_ns < start_ns)) {
        return false;
    }
    if (timed) {
        *elapsed_out = end_ns - start_ns;
    }
    return true;
}
static int dual_add_sub_compare_samples(const void *left, const void *right)
{
    const dual_add_sub_sample *a = (const dual_add_sub_sample *)left;
    const dual_add_sub_sample *b = (const dual_add_sub_sample *)right;
    return a->elapsed_ns < b->elapsed_ns
               ? -1
               : a->elapsed_ns > b->elapsed_ns;
}
static bool dual_add_sub_run_scenario(const dual_add_sub_scenario *scenario,
                                      dual_add_sub_stats *stats)
{
    dual_add_sub_sample samples[DUAL_ADD_SUB_SAMPLES];
    dual_add_sub_bus_context bus = {0};
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = &bus,
        .advance = dual_add_sub_data_advance,
        .cancel = dual_add_sub_data_cancel};
    const ti_c54x_bus interface = {
        .context = &bus,
        .program_read_word = dual_add_sub_program_read,
        .data_endpoint = endpoint};
    const ti_c54x_config config = {0U, {0U, NULL, NULL}};
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;

    if (scenario == NULL || stats == NULL || storage == NULL ||
        ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config, &interface,
                         &cpu) != TI_C54X_ERROR_OK) {
        free(storage);
        return false;
    }
    for (unsigned warmup = 0U; warmup < DUAL_ADD_SUB_WARMUPS; ++warmup) {
        if (!dual_add_sub_run_window(cpu, &bus, scenario, false, NULL)) {
            (void)ti_c54x_cpu_deinit(cpu);
            free(storage);
            return false;
        }
    }
    for (unsigned sample = 0U; sample < DUAL_ADD_SUB_SAMPLES; ++sample) {
        uint64_t elapsed = UINT64_C(0);
        for (unsigned window = 0U; window < DUAL_ADD_SUB_WINDOWS; ++window) {
            uint64_t window_elapsed = UINT64_C(0);
            if (!dual_add_sub_run_window(cpu, &bus, scenario, true,
                                         &window_elapsed) ||
                UINT64_MAX - elapsed < window_elapsed) {
                (void)ti_c54x_cpu_deinit(cpu);
                free(storage);
                return false;
            }
            elapsed += window_elapsed;
        }
        samples[sample].elapsed_ns = elapsed;
    }
    qsort(samples, DUAL_ADD_SUB_SAMPLES, sizeof(samples[0]),
          dual_add_sub_compare_samples);
    stats->elapsed_ns = samples[DUAL_ADD_SUB_SAMPLES / 2U].elapsed_ns;
    stats->min_elapsed_ns = samples[0].elapsed_ns;
    stats->max_elapsed_ns = samples[DUAL_ADD_SUB_SAMPLES - 1U].elapsed_ns;
    stats->first_transaction_id = bus.first_id;
    stats->last_transaction_id = bus.last_id;
    if (ti_c54x_accumulator_read(&cpu->accumulators,
                                 TI_C54X_ACCUMULATOR_A,
                                 &stats->final_a_raw40) !=
            TI_C54X_ACCUMULATOR_OK ||
        ti_c54x_accumulator_read(&cpu->accumulators,
                                 TI_C54X_ACCUMULATOR_B,
                                 &stats->final_b_raw40) !=
            TI_C54X_ACCUMULATOR_OK) {
        (void)ti_c54x_cpu_deinit(cpu);
        free(storage);
        return false;
    }
    stats->final_c = (uint8_t)ti_c54x_status_register_read_c(
                          &cpu->arch_state.status).raw_value;
    stats->final_ova = (uint8_t)ti_c54x_status_register_read_ova(
                           &cpu->arch_state.status).raw_value;
    stats->final_ovb = (uint8_t)ti_c54x_status_register_read_ovb(
                           &cpu->arch_state.status).raw_value;
    (void)ti_c54x_cpu_deinit(cpu);
    free(storage);
    return true;
}
static uint64_t dual_add_sub_hash_bytes(uint64_t value,
                                        const unsigned char *bytes,
                                        size_t count)
{
    for (size_t index = 0U; index < count; ++index) {
        value ^= (uint64_t)bytes[index];
        value *= UINT64_C(1099511628211);
    }
    return value;
}
static uint64_t dual_add_sub_hash_fixed(uint64_t value, uint64_t number,
                                        size_t width)
{
    unsigned char bytes[5];
    for (size_t index = 0U; index < width; ++index) {
        const size_t shift = (width - 1U - index) * 8U;
        bytes[index] = (unsigned char)(number >> shift);
    }
    return dual_add_sub_hash_bytes(value, bytes, width);
}
static uint64_t dual_add_sub_semantic_hash(
    const dual_add_sub_scenario *scenarios, size_t count)
{
    uint64_t value = UINT64_C(14695981039346656037);
    for (size_t index = 0U; index < count; ++index) {
        const dual_add_sub_scenario *scenario = &scenarios[index];
        value = dual_add_sub_hash_bytes(
            value, (const unsigned char *)scenario->name,
            strlen(scenario->name));
        value = dual_add_sub_hash_bytes(value, (const unsigned char *)"\0", 1U);
        value = dual_add_sub_hash_fixed(value, scenario->program_word, 2U);
        value = dual_add_sub_hash_bytes(
            value, (const unsigned char[]){(unsigned char)scenario->operation,
                                           (unsigned char)scenario->destination,
                                           DUAL_ADD_SUB_X_AR,
                                           DUAL_ADD_SUB_Y_AR, 0U, 0U}, 6U);
        value = dual_add_sub_hash_fixed(value, DUAL_ADD_SUB_X_ADDRESS, 2U);
        value = dual_add_sub_hash_fixed(value, DUAL_ADD_SUB_Y_ADDRESS, 2U);
        value = dual_add_sub_hash_fixed(value, DUAL_ADD_SUB_X_WORD, 2U);
        value = dual_add_sub_hash_fixed(value, DUAL_ADD_SUB_Y_WORD, 2U);
        value = dual_add_sub_hash_fixed(value, DUAL_ADD_SUB_INITIAL_A, 5U);
        value = dual_add_sub_hash_fixed(value, DUAL_ADD_SUB_INITIAL_B, 5U);
        value = dual_add_sub_hash_bytes(value,
                                        (const unsigned char[]){0U, 0U, 0U}, 3U);
        value = dual_add_sub_hash_fixed(
            value, scenario->destination == DUAL_ADD_SUB_DESTINATION_A
                       ? scenario->expected_destination_raw40
                       : DUAL_ADD_SUB_INITIAL_A, 5U);
        value = dual_add_sub_hash_fixed(
            value, scenario->destination == DUAL_ADD_SUB_DESTINATION_B
                       ? scenario->expected_destination_raw40
                       : DUAL_ADD_SUB_INITIAL_B, 5U);
    }
    return value;
}
int main(void)
{
    static const dual_add_sub_scenario scenarios[] = {
        {"add-dual-mod0-a", UINT16_C(0xA001), DUAL_ADD_SUB_ADD,
         DUAL_ADD_SUB_DESTINATION_A, UINT64_C(0x000012360000)},
        {"add-dual-mod0-b", UINT16_C(0xA101), DUAL_ADD_SUB_ADD,
         DUAL_ADD_SUB_DESTINATION_B, UINT64_C(0x000012360000)},
        {"sub-dual-mod0-a", UINT16_C(0xA201), DUAL_ADD_SUB_SUB,
         DUAL_ADD_SUB_DESTINATION_A, UINT64_C(0x000012320000)},
        {"sub-dual-mod0-b", UINT16_C(0xA301), DUAL_ADD_SUB_SUB,
         DUAL_ADD_SUB_DESTINATION_B, UINT64_C(0x000012320000)}};
    dual_add_sub_stats stats[sizeof(scenarios) / sizeof(scenarios[0])];
    const uint64_t retired = (uint64_t)DUAL_ADD_SUB_WINDOWS *
                             DUAL_ADD_SUB_RETIREMENTS_PER_WINDOW;
    const uint64_t semantic = dual_add_sub_semantic_hash(
        scenarios, sizeof(scenarios) / sizeof(scenarios[0]));

    for (size_t index = 0U;
         index < sizeof(scenarios) / sizeof(scenarios[0]); ++index) {
        if (!dual_add_sub_run_scenario(&scenarios[index], &stats[index])) {
            (void)fprintf(stderr,
                          "dual ADD/SUB benchmark semantic failure: %s\n",
                          scenarios[index].name);
            return EXIT_FAILURE;
        }
    }
    (void)printf(
        "{\"schema_version\":%d,\"cohort\":\"ti-c54x-dual-add-sub-word16-observer-off-v1\","
        "\"clock_source\":\"CLOCK_MONOTONIC\",\"observer_off\":true,"
        "\"roster\":[\"add-dual-mod0-a\",\"add-dual-mod0-b\",\"sub-dual-mod0-a\",\"sub-dual-mod0-b\"],"
        "\"semantic_hash\":\"%016" PRIx64 "\",\"samples\":%d,"
        "\"warmup_windows\":%d,\"window_count\":%d,\"retirements_per_window\":%d,\"results\":[",
        DUAL_ADD_SUB_SCHEMA_VERSION, semantic, DUAL_ADD_SUB_SAMPLES,
        DUAL_ADD_SUB_WARMUPS, DUAL_ADD_SUB_WINDOWS,
        DUAL_ADD_SUB_RETIREMENTS_PER_WINDOW);
    for (size_t index = 0U;
         index < sizeof(scenarios) / sizeof(scenarios[0]); ++index) {
        const dual_add_sub_scenario *scenario = &scenarios[index];
        const dual_add_sub_stats *result = &stats[index];
        const double ns_per_instruction =
            (double)result->elapsed_ns / (double)retired;
        const double instructions_per_second =
            (double)retired * 1000000000.0 / (double)result->elapsed_ns;
        (void)printf(
            "%s{\"scenario\":\"%s\",\"program_words\":[%u],"
            "\"operation\":%d,\"destination\":%d,\"x_ar\":%d,\"y_ar\":%d,"
            "\"x_address_word\":%u,\"y_address_word\":%u,\"x_data_word\":%u,\"y_data_word\":%u,"
            "\"initial_a_raw40\":%" PRIu64 ",\"initial_b_raw40\":%" PRIu64 ","
            "\"initial_sxm\":0,\"initial_ovm\":0,\"initial_c\":0,"
            "\"retired_count\":%" PRIu64 ",\"attempted_count\":%" PRIu64 ",\"final_pc_word\":%" PRIu64 ",\"program_reads\":%" PRIu64 ","
            "\"endpoint_request_count\":%" PRIu64 ",\"endpoint_read_count\":%" PRIu64 ",\"endpoint_write_count\":0,"
            "\"first_address_word\":%u,\"second_address_word\":%u,\"first_transaction_id\":%" PRIu64 ",\"last_transaction_id\":%" PRIu64 ","
            "\"request_order_valid\":true,\"address_sequence_valid\":true,\"fresh_transaction_ids\":true,\"pc_progression_valid\":true,\"state_valid\":true,"
            "\"final_a_raw40\":%" PRIu64 ",\"final_b_raw40\":%" PRIu64 ",\"final_c\":%u,\"final_ova\":%u,\"final_ovb\":%u,"
            "\"elapsed_ns\":%" PRIu64 ",\"elapsed_ticks\":%" PRIu64 ",\"min_elapsed_ns\":%" PRIu64 ",\"max_elapsed_ns\":%" PRIu64 ","
            "\"ns_per_instruction\":%.6f,\"instructions_per_second\":%.6f}",
            index == 0U ? "" : ",", scenario->name,
            (unsigned)scenario->program_word, (int)scenario->operation,
            (int)scenario->destination, DUAL_ADD_SUB_X_AR, DUAL_ADD_SUB_Y_AR,
            DUAL_ADD_SUB_X_ADDRESS, DUAL_ADD_SUB_Y_ADDRESS,
            DUAL_ADD_SUB_X_WORD, DUAL_ADD_SUB_Y_WORD,
            (uint64_t)DUAL_ADD_SUB_INITIAL_A,
            (uint64_t)DUAL_ADD_SUB_INITIAL_B, retired, retired, retired, retired,
            retired * UINT64_C(2), retired * UINT64_C(2),
            DUAL_ADD_SUB_X_ADDRESS, DUAL_ADD_SUB_Y_ADDRESS,
            result->first_transaction_id, result->last_transaction_id,
            result->final_a_raw40, result->final_b_raw40,
            (unsigned)result->final_c, (unsigned)result->final_ova,
            (unsigned)result->final_ovb, result->elapsed_ns, result->elapsed_ns,
            result->min_elapsed_ns, result->max_elapsed_ns, ns_per_instruction,
            instructions_per_second);
    }
    (void)printf("]}\n");
    return EXIT_SUCCESS;
}
