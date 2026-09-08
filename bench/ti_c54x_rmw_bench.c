/*
 * Module contract
 *
 * Owns: a fixed observer-off hosted throughput cohort for the three neutral
 * ANDM/ORM/XORM MOD0 WORD16 read/modify/write forms.  It supplies a
 * synchronous DATA endpoint, validates address/ID/count/transform invariants
 * around each timed window, and emits one JSON object only after timing.
 * Does not own: core semantics, machine-cycle timing, thresholds, history,
 * environment controls, observers, or diagnostic output from callbacks.
 */

#if !defined(TI_C54X_ENABLE_OBSERVERS) || TI_C54X_ENABLE_OBSERVERS != 0
#error "ti_c54x_rmw_bench requires TI_C54X_ENABLE_OBSERVERS=0"
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
    RMW_SCHEMA_VERSION = 1,
    RMW_SAMPLES = 7,
    RMW_WARMUPS = 16,
    RMW_WINDOWS = 16,
    RMW_RETIREMENTS_PER_WINDOW = 20000
};

typedef enum rmw_operation {
    RMW_AND = 0,
    RMW_OR = 1,
    RMW_XOR = 2
} rmw_operation;

typedef struct rmw_scenario {
    const char *name;
    uint16_t program_words[2];
    rmw_operation operation;
} rmw_scenario;

typedef struct rmw_bus_context {
    const uint16_t *program_words;
    rmw_operation operation;
    uint16_t data_word;
    uint16_t initial_data_word;
    uint16_t mask_word;
    uint64_t endpoint_calls;
    uint64_t read_calls;
    uint64_t write_calls;
    uint64_t program_reads;
    uint64_t first_id;
    uint64_t last_id;
    uint32_t data_address;
    bool same_address;
    bool fresh_ids;
    bool phase_order_valid;
    bool transform_valid;
    ti_c54x_bus_word16_request first_request;
    ti_c54x_bus_word16_request second_request;
} rmw_bus_context;

typedef struct rmw_sample {
    uint64_t elapsed_ns;
} rmw_sample;

typedef struct rmw_stats {
    uint64_t elapsed_ns;
    uint64_t min_elapsed_ns;
    uint64_t max_elapsed_ns;
} rmw_stats;

static uint16_t rmw_transform(rmw_operation operation, uint16_t value,
                              uint16_t mask)
{
    if (operation == RMW_AND) {
        return (uint16_t)(value & mask);
    }
    if (operation == RMW_OR) {
        return (uint16_t)(value | mask);
    }
    return (uint16_t)(value ^ mask);
}

static uint16_t rmw_expected_after(rmw_operation operation, uint16_t initial,
                                   uint16_t mask, uint64_t count)
{
    if (operation == RMW_XOR && (count % UINT64_C(2)) == UINT64_C(0)) {
        return initial;
    }
    return rmw_transform(operation, initial, mask);
}

static ti_c54x_bus_status rmw_program_read(
    void *context, ti_c54x_program_word_address address_word,
    uint16_t *value_word)
{
    rmw_bus_context *bus = (rmw_bus_context *)context;
    const size_t index = (size_t)(address_word % UINT32_C(2));
    bus->program_reads++;
    *value_word = bus->program_words[index];
    return TI_C54X_BUS_OK;
}

static void rmw_data_advance(
    void *context, ti_c54x_bus_word16_advance_phase phase,
    uint64_t transaction_id, const ti_c54x_bus_word16_request *request,
    ti_c54x_bus_word16_observation *observation)
{
    rmw_bus_context *bus = (rmw_bus_context *)context;
    const uint64_t call = bus->endpoint_calls;
    const bool read = request != NULL &&
                      request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;

    if (phase != TI_C54X_BUS_WORD16_ADVANCE_BEGIN || request == NULL ||
        observation == NULL || transaction_id == UINT64_C(0)) {
        bus->phase_order_valid = false;
        bus->same_address = false;
        bus->fresh_ids = false;
        return;
    }
    if (call == UINT64_C(0)) {
        bus->first_id = transaction_id;
        bus->first_request = *request;
    } else if (call == UINT64_C(1)) {
        bus->second_request = *request;
    }
    if (call > UINT64_C(0) && transaction_id <= bus->last_id) {
        bus->fresh_ids = false;
    }
    if (call > UINT64_C(0) && request->address_word != bus->data_address) {
        bus->same_address = false;
    }
    if (call == UINT64_C(0) && request->address_word != bus->data_address) {
        bus->same_address = false;
    }
    bus->last_id = transaction_id;
    if (read != ((call % UINT64_C(2)) == UINT64_C(0))) {
        bus->phase_order_valid = false;
    }
    if (request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        request->address_word != bus->data_address) {
        bus->same_address = false;
    }
    if (read) {
        bus->read_calls++;
        if (request->write_value_word != UINT16_C(0)) {
            bus->phase_order_valid = false;
        }
    } else if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        bus->write_calls++;
        if (request->write_value_word !=
            rmw_transform(bus->operation, bus->data_word, bus->mask_word)) {
            bus->transform_valid = false;
        }
        bus->data_word = request->write_value_word;
    } else {
        bus->same_address = false;
    }
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = read ? bus->data_word : UINT16_C(0),
        .bus_status = TI_C54X_BUS_OK};
    bus->endpoint_calls = call + UINT64_C(1);
}

static ti_c54x_bus_word16_cancel_result rmw_data_cancel(
    void *context, uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static void rmw_context_reset(rmw_bus_context *bus, const rmw_scenario *scenario)
{
    *bus = (rmw_bus_context){
        .program_words = scenario->program_words,
        .operation = scenario->operation,
        .data_word = UINT16_C(0xA55A),
        .initial_data_word = UINT16_C(0xA55A),
        .mask_word = scenario->program_words[1],
        .data_address = UINT32_C(0),
        .same_address = true,
        .fresh_ids = true,
        .phase_order_valid = true,
        .transform_valid = true};
}

static bool rmw_context_valid(const rmw_bus_context *bus, uint64_t retired,
                              const rmw_scenario *scenario)
{
    const uint16_t expected = rmw_expected_after(
        scenario->operation, bus->initial_data_word, bus->mask_word, retired);
    return bus->endpoint_calls == retired * UINT64_C(2) &&
           bus->read_calls == retired && bus->write_calls == retired &&
           bus->program_reads == retired * UINT64_C(2) && bus->same_address &&
           bus->fresh_ids && bus->phase_order_valid && bus->transform_valid &&
           bus->first_id != UINT64_C(0) && bus->last_id > bus->first_id &&
           bus->data_word == expected &&
           bus->first_request.space == TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
           bus->first_request.access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           bus->first_request.address_word == bus->data_address &&
           bus->first_request.write_value_word == UINT16_C(0) &&
           bus->second_request.space == TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
           bus->second_request.access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
           bus->second_request.address_word == bus->data_address &&
           bus->second_request.write_value_word ==
               rmw_transform(scenario->operation, bus->initial_data_word,
                             bus->mask_word);
}

static bool rmw_now_ns(uint64_t *out_ns)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0 || now.tv_sec < 0 ||
        now.tv_nsec < 0 || now.tv_nsec >= 1000000000L) {
        return false;
    }
    *out_ns = (uint64_t)now.tv_sec * UINT64_C(1000000000) +
              (uint64_t)now.tv_nsec;
    return true;
}

static bool rmw_run_window(ti_c54x_cpu *cpu, rmw_bus_context *bus,
                           const rmw_scenario *scenario, bool timed,
                           uint64_t *elapsed_out)
{
    const uint64_t budget = RMW_RETIREMENTS_PER_WINDOW;
    const uint64_t expected_pc = budget * UINT64_C(2);
    ti_c54x_run_result result;
    uint64_t start_ns = 0U;
    uint64_t end_ns = 0U;

    rmw_context_reset(bus, scenario);
    if (ti_c54x_cpu_reset(cpu) != TI_C54X_ERROR_OK) {
        return false;
    }
    if (ti_c54x_arch_state_write_cmpt(&cpu->arch_state, UINT8_C(0)) !=
            TI_C54X_ARCH_STATE_OK ||
        ti_c54x_arch_state_write_arp(&cpu->arch_state, UINT8_C(0)) !=
            TI_C54X_ARCH_STATE_OK ||
        ti_c54x_arch_state_write_ar(&cpu->arch_state, UINT8_C(0), UINT16_C(0)) !=
            TI_C54X_ARCH_STATE_OK) {
        return false;
    }
    if (timed && !rmw_now_ns(&start_ns)) {
        return false;
    }
    result = ti_c54x_cpu_run(cpu, budget);
    if (timed && !rmw_now_ns(&end_ns)) {
        return false;
    }
    if (result.reason != TI_C54X_STOP_BUDGET_EXHAUSTED ||
        result.bus_status != TI_C54X_BUS_OK ||
        result.instructions_attempted != budget ||
        result.instructions_retired != budget || result.pc_word != expected_pc ||
        !rmw_context_valid(bus, budget, scenario) ||
        (timed && end_ns < start_ns)) {
        return false;
    }
    if (timed) {
        *elapsed_out = end_ns - start_ns;
    }
    return true;
}

static int compare_samples(const void *left, const void *right)
{
    const rmw_sample *a = (const rmw_sample *)left;
    const rmw_sample *b = (const rmw_sample *)right;
    return a->elapsed_ns < b->elapsed_ns
               ? -1
               : a->elapsed_ns > b->elapsed_ns;
}

static bool rmw_run_scenario(const rmw_scenario *scenario, rmw_stats *stats)
{
    rmw_sample samples[RMW_SAMPLES];
    rmw_bus_context bus = {0};
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = &bus, .advance = rmw_data_advance, .cancel = rmw_data_cancel};
    const ti_c54x_bus interface = {
        .context = &bus, .program_read_word = rmw_program_read,
        .data_endpoint = endpoint};
    const ti_c54x_config config = {0U, {0U, NULL, NULL}};
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;

    if (storage == NULL || ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                             &interface, &cpu) !=
                               TI_C54X_ERROR_OK) {
        free(storage);
        return false;
    }
    for (unsigned warmup = 0U; warmup < RMW_WARMUPS; ++warmup) {
        if (!rmw_run_window(cpu, &bus, scenario, false, NULL)) {
            ti_c54x_cpu_deinit(cpu);
            free(storage);
            return false;
        }
    }
    for (unsigned sample = 0U; sample < RMW_SAMPLES; ++sample) {
        uint64_t elapsed = 0U;
        for (unsigned window = 0U; window < RMW_WINDOWS; ++window) {
            uint64_t window_elapsed = 0U;
            if (!rmw_run_window(cpu, &bus, scenario, true, &window_elapsed) ||
                UINT64_MAX - elapsed < window_elapsed) {
                ti_c54x_cpu_deinit(cpu);
                free(storage);
                return false;
            }
            elapsed += window_elapsed;
        }
        samples[sample].elapsed_ns = elapsed;
    }
    qsort(samples, RMW_SAMPLES, sizeof(samples[0]), compare_samples);
    stats->elapsed_ns = samples[RMW_SAMPLES / 2U].elapsed_ns;
    stats->min_elapsed_ns = samples[0].elapsed_ns;
    stats->max_elapsed_ns = samples[RMW_SAMPLES - 1U].elapsed_ns;
    (void)ti_c54x_cpu_deinit(cpu);
    free(storage);
    return true;
}

static uint64_t rmw_hash_bytes(uint64_t value, const unsigned char *bytes,
                               size_t count)
{
    for (size_t index = 0U; index < count; ++index) {
        value ^= (uint64_t)bytes[index];
        value *= UINT64_C(1099511628211);
    }
    return value;
}

static uint64_t rmw_semantic_hash(const rmw_scenario *scenarios, size_t count)
{
    uint64_t value = UINT64_C(14695981039346656037);
    for (size_t index = 0U; index < count; ++index) {
        const rmw_scenario *scenario = &scenarios[index];
        value = rmw_hash_bytes(value, (const unsigned char *)scenario->name,
                               strlen(scenario->name));
        value = rmw_hash_bytes(value, (const unsigned char *)"\0", 1U);
        for (size_t word = 0U; word < 2U; ++word) {
            const unsigned char encoded[] = {
                (unsigned char)(scenario->program_words[word] >> 8U),
                (unsigned char)(scenario->program_words[word] & UINT16_C(0xFF))};
            value = rmw_hash_bytes(value, encoded, sizeof(encoded));
        }
        {
            const unsigned char descriptor[] = {
                UINT8_C(0x00), UINT8_C(0x00),
                UINT8_C(0x00), UINT8_C(0x00), UINT8_C(0x00), UINT8_C(0x00),
                UINT8_C(0xA5), UINT8_C(0x5A),
                (unsigned char)((scenario->operation >> 0U) & UINT8_C(0xFF)),
                UINT8_C(0x02), UINT8_C(0x01), UINT8_C(0x02)};
            value = rmw_hash_bytes(value, descriptor, sizeof(descriptor));
        }
    }
    return value;
}

int main(void)
{
    static const rmw_scenario scenarios[] = {
        {"andm-mod0", {UINT16_C(0x6880), UINT16_C(0x0F3C)}, RMW_AND},
        {"orm-mod0", {UINT16_C(0x6980), UINT16_C(0x0F3C)}, RMW_OR},
        {"xorm-mod0", {UINT16_C(0x6A80), UINT16_C(0x0F3C)}, RMW_XOR}};
    rmw_stats stats[sizeof(scenarios) / sizeof(scenarios[0])];
    const uint64_t retired = (uint64_t)RMW_WINDOWS * RMW_RETIREMENTS_PER_WINDOW;
    const uint64_t semantic = rmw_semantic_hash(
        scenarios, sizeof(scenarios) / sizeof(scenarios[0]));

    for (size_t index = 0U; index < sizeof(scenarios) / sizeof(scenarios[0]);
         ++index) {
        if (!rmw_run_scenario(&scenarios[index], &stats[index])) {
            (void)fprintf(stderr, "RMW benchmark semantic failure: %s\n",
                          scenarios[index].name);
            return EXIT_FAILURE;
        }
    }
    (void)printf(
        "{\"schema_version\":%d,\"cohort\":\"ti-c54x-rmw-observer-off-v1\","
        "\"clock_source\":\"CLOCK_MONOTONIC\",\"observer_off\":true,"
        "\"roster\":[\"andm-mod0\",\"orm-mod0\",\"xorm-mod0\"],"
        "\"semantic_hash\":\"%016" PRIx64 "\",\"samples\":%d,"
        "\"warmup_windows\":%d,\"window_count\":%d,"
        "\"retirements_per_window\":%d,\"results\":[",
        RMW_SCHEMA_VERSION, semantic, RMW_SAMPLES, RMW_WARMUPS, RMW_WINDOWS,
        RMW_RETIREMENTS_PER_WINDOW);
    for (size_t index = 0U; index < sizeof(scenarios) / sizeof(scenarios[0]);
         ++index) {
        const rmw_stats *result = &stats[index];
        const double ns_per_instruction =
            (double)result->elapsed_ns / (double)retired;
        const double instructions_per_second =
            (double)retired * 1000000000.0 / (double)result->elapsed_ns;
        (void)printf(
            "%s{\"scenario\":\"%s\",\"program_words\":[%u,%u],"
            "\"arf\":0,\"mode\":0,\"operation\":%d,"
            "\"data_address_word\":0,\"initial_data_word\":%u,"
            "\"retired_count\":%" PRIu64
            ",\"elapsed_ns\":%" PRIu64 ",\"elapsed_ticks\":%" PRIu64
            ",\"min_elapsed_ns\":%" PRIu64 ",\"max_elapsed_ns\":%" PRIu64
            ",\"endpoint_read_count\":%" PRIu64
            ",\"endpoint_write_count\":%" PRIu64
            ",\"same_address\":true,\"fresh_transaction_ids\":true,"
            "\"phase_order_valid\":true,"
            "\"transform_valid\":true,\"ns_per_instruction\":%.6f,"
            "\"instructions_per_second\":%.6f}",
            index == 0U ? "" : ",", scenarios[index].name,
            (unsigned)scenarios[index].program_words[0],
            (unsigned)scenarios[index].program_words[1],
            (int)scenarios[index].operation, (unsigned)UINT16_C(0xA55A), retired,
            result->elapsed_ns, result->elapsed_ns, result->min_elapsed_ns,
            result->max_elapsed_ns, retired, retired, ns_per_instruction,
            instructions_per_second);
    }
    (void)printf("]}\n");
    return EXIT_SUCCESS;
}
