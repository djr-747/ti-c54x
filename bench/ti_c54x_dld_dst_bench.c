/*
 * Module contract
 *
 * Owns: a fixed observer-off hosted throughput cohort for the public-CPU
 * DLD/DST LONG32 MOD13 forms.  It supplies a synchronous DATA endpoint,
 * validates the two-word fetch and two-word data-transfer invariants around
 * every timed window, and emits one JSON object only after timing.
 * Does not own: core semantics, machine-cycle timing, thresholds, history,
 * environment controls, observers, or diagnostic output from callbacks.
 */

#if !defined(TI_C54X_ENABLE_OBSERVERS) || TI_C54X_ENABLE_OBSERVERS != 0
#error "ti_c54x_dld_dst_bench requires TI_C54X_ENABLE_OBSERVERS=0"
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
    DLD_DST_SCHEMA_VERSION = 1,
    DLD_DST_SAMPLES = 5,
    DLD_DST_WARMUPS = 8,
    DLD_DST_WINDOWS = 8,
    DLD_DST_RETIREMENTS_PER_WINDOW = 5000
};

typedef enum dld_dst_operation {
    DLD_DST_LOAD = 0,
    DLD_DST_STORE = 1
} dld_dst_operation;

typedef struct dld_dst_scenario {
    const char *name;
    uint16_t program_words[2];
    dld_dst_operation operation;
} dld_dst_scenario;

typedef struct dld_dst_bus_context {
    const uint16_t *program_words;
    dld_dst_operation operation;
    uint16_t first_read_word;
    uint16_t second_read_word;
    uint16_t first_write_word;
    uint16_t second_write_word;
    uint32_t data_address_word;
    uint64_t endpoint_calls;
    uint64_t read_calls;
    uint64_t write_calls;
    uint64_t program_reads;
    uint64_t first_id;
    uint64_t last_id;
    uint16_t first_address_word;
    uint16_t second_address_word;
    uint16_t last_write_msw;
    uint16_t last_write_lsw;
    bool request_order_valid;
    bool address_sequence_valid;
    bool fresh_ids;
} dld_dst_bus_context;

typedef struct dld_dst_sample {
    uint64_t elapsed_ns;
} dld_dst_sample;

typedef struct dld_dst_stats {
    uint64_t elapsed_ns;
    uint64_t min_elapsed_ns;
    uint64_t max_elapsed_ns;
    uint64_t first_transaction_id;
    uint64_t last_transaction_id;
    uint16_t final_write_msw;
    uint16_t final_write_lsw;
} dld_dst_stats;

static ti_c54x_bus_status dld_dst_program_read(
    void *context, ti_c54x_program_word_address address_word,
    uint16_t *value_word)
{
    dld_dst_bus_context *bus = (dld_dst_bus_context *)context;
    const size_t index = (size_t)(address_word % UINT32_C(2));

    if (value_word == NULL) {
        return TI_C54X_BUS_FAULT;
    }
    bus->program_reads++;
    *value_word = bus->program_words[index];
    return TI_C54X_BUS_OK;
}

static void dld_dst_data_advance(
    void *context, ti_c54x_bus_word16_advance_phase phase,
    uint64_t transaction_id, const ti_c54x_bus_word16_request *request,
    ti_c54x_bus_word16_observation *observation)
{
    dld_dst_bus_context *bus = (dld_dst_bus_context *)context;
    const uint64_t call = bus->endpoint_calls;
    const bool first = (call % UINT64_C(2)) == UINT64_C(0);
    const bool read = request != NULL &&
                      request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    const uint16_t expected_address = (uint16_t)(bus->data_address_word +
                                                  (first ? UINT16_C(0)
                                                         : UINT16_C(1)));
    const uint16_t expected_value = first ? bus->first_write_word
                                          : bus->second_write_word;

    if (phase != TI_C54X_BUS_WORD16_ADVANCE_BEGIN || request == NULL ||
        observation == NULL || transaction_id == UINT64_C(0) ||
        request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        request->address_word != expected_address ||
        read != (bus->operation == DLD_DST_LOAD)) {
        bus->request_order_valid = false;
        bus->address_sequence_valid = false;
        bus->fresh_ids = false;
        return;
    }
    if (call == UINT64_C(0)) {
        bus->first_id = transaction_id;
        bus->first_address_word = (uint16_t)request->address_word;
    } else if (call == UINT64_C(1)) {
        bus->second_address_word = (uint16_t)request->address_word;
    }
    if (call > UINT64_C(0) && transaction_id <= bus->last_id) {
        bus->fresh_ids = false;
    }
    if (read) {
        if (request->write_value_word != UINT16_C(0)) {
            bus->request_order_valid = false;
        }
        bus->read_calls++;
    } else if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        if (request->write_value_word != expected_value) {
            bus->request_order_valid = false;
        }
        if (first) {
            bus->last_write_msw = request->write_value_word;
        } else {
            bus->last_write_lsw = request->write_value_word;
        }
        bus->write_calls++;
    } else {
        bus->request_order_valid = false;
    }
    bus->last_id = transaction_id;
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = read ? (first ? bus->first_read_word
                                         : bus->second_read_word)
                                : UINT16_C(0),
        .bus_status = TI_C54X_BUS_OK};
    bus->endpoint_calls = call + UINT64_C(1);
}

static ti_c54x_bus_word16_cancel_result dld_dst_data_cancel(
    void *context, uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static void dld_dst_context_reset(dld_dst_bus_context *bus,
                                  const dld_dst_scenario *scenario)
{
    *bus = (dld_dst_bus_context){
        .program_words = scenario->program_words,
        .operation = scenario->operation,
        .first_read_word = UINT16_C(0x1234),
        .second_read_word = UINT16_C(0x5678),
        .first_write_word = UINT16_C(0x1234),
        .second_write_word = UINT16_C(0x5678),
        .data_address_word = UINT32_C(0x0100),
        .request_order_valid = true,
        .address_sequence_valid = true,
        .fresh_ids = true};
}

static bool dld_dst_context_valid(const dld_dst_bus_context *bus,
                                  uint64_t retired,
                                  const dld_dst_scenario *scenario)
{
    const bool load = scenario->operation == DLD_DST_LOAD;
    return bus->endpoint_calls == retired * UINT64_C(2) &&
           bus->program_reads == retired * UINT64_C(2) &&
           bus->read_calls == (load ? retired * UINT64_C(2) : UINT64_C(0)) &&
           bus->write_calls == (load ? UINT64_C(0) : retired * UINT64_C(2)) &&
           bus->request_order_valid && bus->address_sequence_valid &&
           bus->fresh_ids && bus->first_id != UINT64_C(0) &&
           bus->last_id > bus->first_id &&
           bus->first_address_word == (uint16_t)bus->data_address_word &&
           bus->second_address_word ==
               (uint16_t)(bus->data_address_word + UINT32_C(1));
}

static bool dld_dst_now_ns(uint64_t *out_ns)
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

static bool dld_dst_prepare_cpu(ti_c54x_cpu *cpu,
                                const dld_dst_scenario *scenario)
{
    const uint64_t source_raw40 = UINT64_C(0x000012345678);
    if (ti_c54x_cpu_reset(cpu) != TI_C54X_ERROR_OK ||
        ti_c54x_arch_state_write_cmpt(&cpu->arch_state, UINT8_C(0)) !=
            TI_C54X_ARCH_STATE_OK ||
        ti_c54x_arch_state_write_arp(&cpu->arch_state, UINT8_C(0)) !=
            TI_C54X_ARCH_STATE_OK ||
        ti_c54x_arch_state_write_ar(&cpu->arch_state, UINT8_C(0),
                                    UINT16_C(0x0100)) !=
            TI_C54X_ARCH_STATE_OK ||
        ti_c54x_status_register_write_sst(&cpu->arch_state.status,
                                          UINT8_C(0)) !=
            TI_C54X_STATUS_REGISTER_OK ||
        ti_c54x_accumulator_write(&cpu->accumulators,
                                  TI_C54X_ACCUMULATOR_A, source_raw40) !=
            TI_C54X_ACCUMULATOR_OK) {
        return false;
    }
    /* Keep the signed-extension input explicit and known, while choosing the
     * no-sign-extension cohort so the final values are independent of guard
     * bits. */
    cpu->arch_state.status.st1 = (uint16_t)(cpu->arch_state.status.st1 &
                                             (uint16_t)~TI_C54X_ST1_SXM_MASK);
    cpu->arch_state.status.st1_known_mask = (uint16_t)(
        cpu->arch_state.status.st1_known_mask | TI_C54X_ST1_SXM_MASK);
    (void)scenario;
    return true;
}

static bool dld_dst_run_window(ti_c54x_cpu *cpu, dld_dst_bus_context *bus,
                               const dld_dst_scenario *scenario, bool timed,
                               uint64_t *elapsed_out)
{
    const uint64_t budget = DLD_DST_RETIREMENTS_PER_WINDOW;
    const uint64_t expected_pc = budget * UINT64_C(2);
    ti_c54x_run_result result;
    uint64_t start_ns = 0U;
    uint64_t end_ns = 0U;
    uint64_t accumulator = UINT64_C(0);

    dld_dst_context_reset(bus, scenario);
    if (!dld_dst_prepare_cpu(cpu, scenario)) {
        return false;
    }
    if (timed && !dld_dst_now_ns(&start_ns)) {
        return false;
    }
    result = ti_c54x_cpu_run(cpu, budget);
    if (timed && !dld_dst_now_ns(&end_ns)) {
        return false;
    }
    if (ti_c54x_accumulator_read(&cpu->accumulators,
                                 TI_C54X_ACCUMULATOR_A, &accumulator) !=
        TI_C54X_ACCUMULATOR_OK) {
        return false;
    }
    if (result.reason != TI_C54X_STOP_BUDGET_EXHAUSTED ||
        result.bus_status != TI_C54X_BUS_OK ||
        result.instructions_attempted != budget ||
        result.instructions_retired != budget || result.pc_word != expected_pc ||
        !dld_dst_context_valid(bus, budget, scenario) ||
        accumulator != UINT64_C(0x000012345678) ||
        (timed && end_ns < start_ns)) {
        return false;
    }
    if (timed) {
        *elapsed_out = end_ns - start_ns;
    }
    return true;
}

static int dld_dst_compare_samples(const void *left, const void *right)
{
    const dld_dst_sample *a = (const dld_dst_sample *)left;
    const dld_dst_sample *b = (const dld_dst_sample *)right;
    return a->elapsed_ns < b->elapsed_ns
               ? -1
               : a->elapsed_ns > b->elapsed_ns;
}

static bool dld_dst_run_scenario(const dld_dst_scenario *scenario,
                                 dld_dst_stats *stats)
{
    dld_dst_sample samples[DLD_DST_SAMPLES];
    dld_dst_bus_context bus = {0};
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = &bus,
        .advance = dld_dst_data_advance,
        .cancel = dld_dst_data_cancel};
    const ti_c54x_bus interface = {.context = &bus,
                                   .program_read_word = dld_dst_program_read,
                                   .data_endpoint = endpoint};
    const ti_c54x_config config = {0U, {0U, NULL, NULL}};
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;

    if (stats == NULL || storage == NULL ||
        ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config, &interface,
                         &cpu) != TI_C54X_ERROR_OK) {
        free(storage);
        return false;
    }
    for (unsigned warmup = 0U; warmup < DLD_DST_WARMUPS; ++warmup) {
        if (!dld_dst_run_window(cpu, &bus, scenario, false, NULL)) {
            (void)ti_c54x_cpu_deinit(cpu);
            free(storage);
            return false;
        }
    }
    for (unsigned sample = 0U; sample < DLD_DST_SAMPLES; ++sample) {
        uint64_t elapsed = 0U;
        for (unsigned window = 0U; window < DLD_DST_WINDOWS; ++window) {
            uint64_t window_elapsed = 0U;
            if (!dld_dst_run_window(cpu, &bus, scenario, true,
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
    qsort(samples, DLD_DST_SAMPLES, sizeof(samples[0]),
          dld_dst_compare_samples);
    stats->elapsed_ns = samples[DLD_DST_SAMPLES / 2U].elapsed_ns;
    stats->min_elapsed_ns = samples[0].elapsed_ns;
    stats->max_elapsed_ns = samples[DLD_DST_SAMPLES - 1U].elapsed_ns;
    stats->first_transaction_id = bus.first_id;
    stats->last_transaction_id = bus.last_id;
    stats->final_write_msw = bus.last_write_msw;
    stats->final_write_lsw = bus.last_write_lsw;
    (void)ti_c54x_cpu_deinit(cpu);
    free(storage);
    return true;
}

static uint64_t dld_dst_hash_bytes(uint64_t value, const unsigned char *bytes,
                                   size_t count)
{
    for (size_t index = 0U; index < count; ++index) {
        value ^= (uint64_t)bytes[index];
        value *= UINT64_C(1099511628211);
    }
    return value;
}

static uint64_t dld_dst_semantic_hash(const dld_dst_scenario *scenarios,
                                      size_t count)
{
    uint64_t value = UINT64_C(14695981039346656037);
    for (size_t index = 0U; index < count; ++index) {
        const dld_dst_scenario *scenario = &scenarios[index];
        value = dld_dst_hash_bytes(value,
                                   (const unsigned char *)scenario->name,
                                   strlen(scenario->name));
        value = dld_dst_hash_bytes(value, (const unsigned char *)"\0", 1U);
        for (size_t word = 0U; word < 2U; ++word) {
            const unsigned char encoded[] = {
                (unsigned char)(scenario->program_words[word] >> 8U),
                (unsigned char)(scenario->program_words[word] & UINT16_C(0xFF))};
            value = dld_dst_hash_bytes(value, encoded, sizeof(encoded));
        }
        {
            const unsigned char descriptor[] = {
                UINT8_C(0x00), UINT8_C(0x00), UINT8_C(0x01),
                UINT8_C(0x00), UINT8_C(0x01), UINT8_C(0x00), UINT8_C(0x01),
                UINT8_C(0x00), UINT8_C(0x12), UINT8_C(0x34), UINT8_C(0x56),
                UINT8_C(0x78), (unsigned char)scenario->operation};
            value = dld_dst_hash_bytes(value, descriptor, sizeof(descriptor));
        }
    }
    return value;
}

int main(void)
{
    static const dld_dst_scenario scenarios[] = {
        {"dld-long32-mod13", {UINT16_C(0x56E8), UINT16_C(0x0000)},
         DLD_DST_LOAD},
        {"dst-long32-mod13", {UINT16_C(0x4EE8), UINT16_C(0x0000)},
         DLD_DST_STORE}};
    dld_dst_stats stats[sizeof(scenarios) / sizeof(scenarios[0])];
    const uint64_t retired = (uint64_t)DLD_DST_WINDOWS *
                             DLD_DST_RETIREMENTS_PER_WINDOW;
    const uint64_t semantic = dld_dst_semantic_hash(
        scenarios, sizeof(scenarios) / sizeof(scenarios[0]));

    for (size_t index = 0U; index < sizeof(scenarios) / sizeof(scenarios[0]);
         ++index) {
        if (!dld_dst_run_scenario(&scenarios[index], &stats[index])) {
            (void)fprintf(stderr, "DLD/DST benchmark semantic failure: %s\n",
                          scenarios[index].name);
            return EXIT_FAILURE;
        }
    }
    (void)printf(
        "{\"schema_version\":%d,\"cohort\":\"ti-c54x-dld-dst-long32-observer-off-v1\","
        "\"clock_source\":\"CLOCK_MONOTONIC\",\"observer_off\":true,"
        "\"roster\":[\"dld-long32-mod13\",\"dst-long32-mod13\"],"
        "\"semantic_hash\":\"%016" PRIx64 "\",\"samples\":%d,"
        "\"warmup_windows\":%d,\"window_count\":%d,"
        "\"retirements_per_window\":%d,\"results\":[",
        DLD_DST_SCHEMA_VERSION, semantic, DLD_DST_SAMPLES, DLD_DST_WARMUPS,
        DLD_DST_WINDOWS, DLD_DST_RETIREMENTS_PER_WINDOW);
    for (size_t index = 0U; index < sizeof(scenarios) / sizeof(scenarios[0]);
         ++index) {
        const dld_dst_stats *result = &stats[index];
        const bool load = scenarios[index].operation == DLD_DST_LOAD;
        const double ns_per_instruction =
            (double)result->elapsed_ns / (double)retired;
        const double instructions_per_second =
            (double)retired * 1000000000.0 / (double)result->elapsed_ns;
        (void)printf(
            "%s{\"scenario\":\"%s\",\"program_words\":[%u,%u],"
            "\"arf\":0,\"mode\":13,\"operation\":%d,"
            "\"long_offset_word\":0,"
            "\"data_address_word\":256,\"initial_accumulator_raw40\":305419896,"
            "\"retired_count\":%" PRIu64 ",\"attempted_count\":%" PRIu64
            ",\"final_pc_word\":%" PRIu64 ",\"program_reads\":%" PRIu64
            ",\"endpoint_request_count\":%" PRIu64
            ",\"endpoint_read_count\":%" PRIu64
            ",\"endpoint_write_count\":%" PRIu64
            ",\"first_address_word\":256,\"second_address_word\":257,"
            "\"first_transaction_id\":%" PRIu64 ",\"last_transaction_id\":%" PRIu64
            ",\"request_order_valid\":true,\"address_sequence_valid\":true,"
            "\"fresh_transaction_ids\":true,\"pc_progression_valid\":true,"
            "\"final_accumulator_raw40\":%u,\"source_unchanged\":%s,"
            "\"final_write_msw\":%u,\"final_write_lsw\":%u,"
            "\"elapsed_ns\":%" PRIu64 ",\"elapsed_ticks\":%" PRIu64
            ",\"min_elapsed_ns\":%" PRIu64 ",\"max_elapsed_ns\":%" PRIu64
            ",\"ns_per_instruction\":%.6f,"
            "\"instructions_per_second\":%.6f}",
            index == 0U ? "" : ",", scenarios[index].name,
            (unsigned)scenarios[index].program_words[0],
            (unsigned)scenarios[index].program_words[1],
            (int)scenarios[index].operation, retired, retired,
            retired * UINT64_C(2), retired * UINT64_C(2),
            retired * UINT64_C(2),
            load ? retired * UINT64_C(2) : UINT64_C(0),
            load ? UINT64_C(0) : retired * UINT64_C(2),
            result->first_transaction_id, result->last_transaction_id,
            UINT32_C(0x12345678), "true", (unsigned)result->final_write_msw,
            (unsigned)result->final_write_lsw,
            result->elapsed_ns, result->elapsed_ns, result->min_elapsed_ns,
            result->max_elapsed_ns, ns_per_instruction,
            instructions_per_second);
    }
    (void)printf("]}\n");
    return EXIT_SUCCESS;
}
