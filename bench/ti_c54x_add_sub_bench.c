/*
 * Module contract
 *
 * Owns: an observer-off hosted public-CPU performance cohort for register-only
 * ADD/SUB syntax-9 with deterministic one-word fetches and semantic checks.
 * Does not own: core semantics, machine-cycle timing, thresholds, history,
 * environment controls, or diagnostic output from the core.
 */

#if !defined(TI_C54X_ENABLE_OBSERVERS) || TI_C54X_ENABLE_OBSERVERS != 0
#error "ti_c54x_add_sub_bench requires TI_C54X_ENABLE_OBSERVERS=0"
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
    ADD_SUB_SCHEMA_VERSION = 1,
    ADD_SUB_SAMPLES = 7,
    ADD_SUB_WARMUPS = 16,
    ADD_SUB_WINDOWS = 16,
    ADD_SUB_RETIREMENTS_PER_WINDOW = 50000
};

typedef enum add_sub_operation {
    ADD_SUB_ADD = 0,
    ADD_SUB_SUB = 1
} add_sub_operation;

typedef struct add_sub_scenario {
    const char *name;
    uint16_t program_word;
    add_sub_operation operation;
    uint64_t expected_b;
    uint8_t expected_c;
} add_sub_scenario;

typedef struct add_sub_bus_context {
    const uint16_t *program_words;
    uint64_t program_reads;
} add_sub_bus_context;

typedef struct add_sub_sample {
    uint64_t elapsed_ns;
} add_sub_sample;

typedef struct add_sub_stats {
    uint64_t elapsed_ns;
    uint64_t min_elapsed_ns;
    uint64_t max_elapsed_ns;
    uint64_t final_a_raw40;
    uint64_t final_b_raw40;
    uint8_t final_c;
    uint8_t final_ova;
    uint8_t final_ovb;
} add_sub_stats;

static ti_c54x_bus_status add_sub_program_read(
    void *context, ti_c54x_program_word_address address_word,
    uint16_t *value_word)
{
    add_sub_bus_context *bus = (add_sub_bus_context *)context;

    if (bus == NULL || value_word == NULL || bus->program_words == NULL) {
        return TI_C54X_BUS_FAULT;
    }
    (void)address_word;
    bus->program_reads++;
    *value_word = bus->program_words[0];
    return TI_C54X_BUS_OK;
}

static bool add_sub_now_ns(uint64_t *out_ns)
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

static bool add_sub_prepare_cpu(ti_c54x_cpu *cpu)
{
    return cpu != NULL && ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK &&
           ti_c54x_accumulator_write(&cpu->accumulators,
                                     TI_C54X_ACCUMULATOR_A,
                                     UINT64_C(0x0000000001)) ==
               TI_C54X_ACCUMULATOR_OK &&
           ti_c54x_accumulator_write(&cpu->accumulators,
                                     TI_C54X_ACCUMULATOR_B,
                                     UINT64_C(0x000012340000)) ==
               TI_C54X_ACCUMULATOR_OK &&
           (cpu->arch_state.status.st1 = (uint16_t)(
                cpu->arch_state.status.st1 & (uint16_t)~TI_C54X_ST1_SXM_MASK),
            cpu->arch_state.status.st1_known_mask = (uint16_t)(
                cpu->arch_state.status.st1_known_mask |
                TI_C54X_ST1_SXM_MASK),
            true) &&
           ti_c54x_status_register_write_ovm(&cpu->arch_state.status, 0U) ==
               TI_C54X_STATUS_REGISTER_OK &&
           ti_c54x_status_register_write_c(&cpu->arch_state.status, 0U) ==
               TI_C54X_STATUS_REGISTER_OK &&
           ti_c54x_status_register_write_ova(&cpu->arch_state.status, 0U) ==
               TI_C54X_STATUS_REGISTER_OK &&
           ti_c54x_status_register_write_ovb(&cpu->arch_state.status, 0U) ==
               TI_C54X_STATUS_REGISTER_OK;
}

static bool add_sub_state_matches(const ti_c54x_cpu *cpu,
                                  const add_sub_scenario *scenario,
                                  uint64_t expected_retired)
{
    uint64_t a_raw40 = 0U;
    uint64_t b_raw40 = 0U;
    const ti_c54x_status_register_field_result c =
        ti_c54x_status_register_read_c(&cpu->arch_state.status);
    const ti_c54x_status_register_field_result ova =
        ti_c54x_status_register_read_ova(&cpu->arch_state.status);
    const ti_c54x_status_register_field_result ovb =
        ti_c54x_status_register_read_ovb(&cpu->arch_state.status);

    return cpu != NULL && scenario != NULL &&
           ti_c54x_accumulator_read(&cpu->accumulators,
                                    TI_C54X_ACCUMULATOR_A, &a_raw40) ==
               TI_C54X_ACCUMULATOR_OK &&
           ti_c54x_accumulator_read(&cpu->accumulators,
                                    TI_C54X_ACCUMULATOR_B, &b_raw40) ==
               TI_C54X_ACCUMULATOR_OK &&
           a_raw40 == UINT64_C(1) && b_raw40 == scenario->expected_b &&
           c.status == TI_C54X_STATUS_REGISTER_OK &&
           c.raw_value == scenario->expected_c &&
           ova.status == TI_C54X_STATUS_REGISTER_OK && ova.raw_value == 0U &&
           ovb.status == TI_C54X_STATUS_REGISTER_OK && ovb.raw_value == 0U &&
           cpu->arch_state.pc_word == expected_retired &&
           cpu->instructions_attempted == expected_retired &&
           cpu->instructions_retired == expected_retired;
}

static bool add_sub_run_sequence(ti_c54x_cpu *cpu, add_sub_bus_context *bus,
                                  const add_sub_scenario *scenario,
                                  bool timed, uint64_t *elapsed_out)
{
    const uint64_t budget = ADD_SUB_RETIREMENTS_PER_WINDOW;
    uint64_t elapsed = 0U;

    if (cpu == NULL || bus == NULL || scenario == NULL ||
        (timed && elapsed_out == NULL)) {
        return false;
    }
    bus->program_reads = 0U;
    for (unsigned window = 0U; window < ADD_SUB_WINDOWS; ++window) {
        uint64_t start_ns = 0U;
        uint64_t end_ns = 0U;
        ti_c54x_run_result result;

        if (!add_sub_prepare_cpu(cpu)) {
            return false;
        }
        if (timed && !add_sub_now_ns(&start_ns)) {
            return false;
        }
        result = ti_c54x_cpu_run(cpu, budget);
        if (timed && !add_sub_now_ns(&end_ns)) {
            return false;
        }
        if (result.reason != TI_C54X_STOP_BUDGET_EXHAUSTED ||
            result.bus_status != TI_C54X_BUS_OK ||
            result.instructions_attempted != budget ||
            result.instructions_retired != budget ||
            result.pc_word != budget ||
            bus->program_reads != ((uint64_t)(window + 1U) * budget) ||
            (timed && (end_ns < start_ns ||
                       UINT64_MAX - elapsed < end_ns - start_ns))) {
            return false;
        }
        if (timed) {
            elapsed += end_ns - start_ns;
        }
    }
    if (!add_sub_state_matches(cpu, scenario, budget)) {
        return false;
    }
    if (timed) {
        *elapsed_out = elapsed;
    }
    return true;
}

static int add_sub_compare_samples(const void *left, const void *right)
{
    const add_sub_sample *a = (const add_sub_sample *)left;
    const add_sub_sample *b = (const add_sub_sample *)right;

    return a->elapsed_ns < b->elapsed_ns
               ? -1
               : a->elapsed_ns > b->elapsed_ns;
}

static bool add_sub_run_scenario(const add_sub_scenario *scenario,
                                 add_sub_stats *stats)
{
    add_sub_sample samples[ADD_SUB_SAMPLES];
    add_sub_bus_context bus = {&scenario->program_word, 0U};
    const ti_c54x_bus interface = {.context = &bus,
                                   .program_read_word = add_sub_program_read,
                                   .data_endpoint = {0}};
    const ti_c54x_config config = {0U, {0U, NULL, NULL}};
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;

    if (scenario == NULL || stats == NULL || storage == NULL ||
        ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config, &interface,
                         &cpu) != TI_C54X_ERROR_OK) {
        free(storage);
        return false;
    }
    for (unsigned warmup = 0U; warmup < ADD_SUB_WARMUPS; ++warmup) {
        if (!add_sub_run_sequence(cpu, &bus, scenario, false, NULL)) {
            (void)ti_c54x_cpu_deinit(cpu);
            free(storage);
            return false;
        }
    }
    for (unsigned sample = 0U; sample < ADD_SUB_SAMPLES; ++sample) {
        if (!add_sub_run_sequence(cpu, &bus, scenario, true,
                                  &samples[sample].elapsed_ns)) {
            (void)ti_c54x_cpu_deinit(cpu);
            free(storage);
            return false;
        }
    }
    qsort(samples, ADD_SUB_SAMPLES, sizeof(samples[0]),
          add_sub_compare_samples);
    stats->elapsed_ns = samples[ADD_SUB_SAMPLES / 2U].elapsed_ns;
    stats->min_elapsed_ns = samples[0].elapsed_ns;
    stats->max_elapsed_ns = samples[ADD_SUB_SAMPLES - 1U].elapsed_ns;
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
                             &cpu->arch_state.status)
                         .raw_value;
    stats->final_ova = (uint8_t)ti_c54x_status_register_read_ova(
                               &cpu->arch_state.status)
                           .raw_value;
    stats->final_ovb = (uint8_t)ti_c54x_status_register_read_ovb(
                               &cpu->arch_state.status)
                           .raw_value;
    (void)ti_c54x_cpu_deinit(cpu);
    free(storage);
    return true;
}

static uint64_t add_sub_hash_bytes(uint64_t value, const unsigned char *bytes,
                                   size_t count)
{
    for (size_t index = 0U; index < count; ++index) {
        value ^= (uint64_t)bytes[index];
        value *= UINT64_C(1099511628211);
    }
    return value;
}

static uint64_t add_sub_semantic_hash(const add_sub_scenario *scenarios,
                                      size_t count)
{
    uint64_t value = UINT64_C(14695981039346656037);

    for (size_t index = 0U; index < count; ++index) {
        const add_sub_scenario *scenario = &scenarios[index];
        const unsigned char descriptor[] = {
            (unsigned char)(scenario->program_word >> 8U),
            (unsigned char)(scenario->program_word & UINT16_C(0xFF)),
            UINT8_C(0x01), UINT8_C(0x00), UINT8_C(0x00), UINT8_C(0x00),
            UINT8_C(0x00), UINT8_C(0x00), UINT8_C(0x00), UINT8_C(0x00),
            UINT8_C(0x00), UINT8_C(0x00), scenario->expected_c};
        value = add_sub_hash_bytes(value,
                                   (const unsigned char *)scenario->name,
                                   strlen(scenario->name));
        value = add_sub_hash_bytes(value, (const unsigned char *)"\0", 1U);
        value = add_sub_hash_bytes(value, descriptor, sizeof(descriptor));
    }
    return value;
}

int main(void)
{
    static const add_sub_scenario scenarios[] = {
        {"add-acc-shift5", UINT16_C(0xF500), ADD_SUB_ADD,
         UINT64_C(0x00001234C350), UINT8_C(0)},
        {"sub-acc-shift5", UINT16_C(0xF520), ADD_SUB_SUB,
         UINT64_C(0x000012333CB0), UINT8_C(1)}};
    add_sub_stats stats[sizeof(scenarios) / sizeof(scenarios[0])];
    const uint64_t retired = (uint64_t)ADD_SUB_WINDOWS *
                             ADD_SUB_RETIREMENTS_PER_WINDOW;
    const uint64_t semantic = add_sub_semantic_hash(
        scenarios, sizeof(scenarios) / sizeof(scenarios[0]));

    for (size_t index = 0U; index < sizeof(scenarios) / sizeof(scenarios[0]);
         ++index) {
        if (!add_sub_run_scenario(&scenarios[index], &stats[index])) {
            (void)fprintf(stderr, "ADD/SUB benchmark semantic failure: %s\n",
                          scenarios[index].name);
            return EXIT_FAILURE;
        }
    }
    (void)printf(
        "{\"schema_version\":%d,\"cohort\":\"ti-c54x-add-sub-acc-shift5-observer-off-v1\","
        "\"clock_source\":\"CLOCK_MONOTONIC\",\"observer_off\":true,"
        "\"roster\":[\"add-acc-shift5\",\"sub-acc-shift5\"],"
        "\"semantic_hash\":\"%016" PRIx64 "\",\"samples\":%d,"
        "\"warmup_windows\":%d,\"window_count\":%d,"
        "\"retirements_per_window\":%d,\"results\":[",
        ADD_SUB_SCHEMA_VERSION, semantic, ADD_SUB_SAMPLES, ADD_SUB_WARMUPS,
        ADD_SUB_WINDOWS, ADD_SUB_RETIREMENTS_PER_WINDOW);
    for (size_t index = 0U; index < sizeof(scenarios) / sizeof(scenarios[0]);
         ++index) {
        const add_sub_scenario *scenario = &scenarios[index];
        const add_sub_stats *result = &stats[index];
        const double ns_per_instruction =
            (double)result->elapsed_ns / (double)retired;
        const double instructions_per_second =
            (double)retired * 1000000000.0 / (double)result->elapsed_ns;

        (void)printf(
            "%s{\"scenario\":\"%s\",\"program_words\":[%u],"
            "\"initial_a_raw40\":1,\"initial_b_raw40\":305397760,"
            "\"initial_sxm\":0,\"initial_ovm\":0,"
            "\"retired_count\":%" PRIu64 ",\"attempted_count\":%" PRIu64
            ",\"final_pc_word\":%u,\"program_reads\":%" PRIu64
            ",\"final_a_raw40\":%" PRIu64 ",\"final_b_raw40\":%" PRIu64
            ",\"final_c\":%u,\"final_ova\":%u,\"final_ovb\":%u,"
            "\"state_valid\":true,\"pc_progression_valid\":true,"
            "\"elapsed_ns\":%" PRIu64 ",\"elapsed_ticks\":%" PRIu64
            ",\"min_elapsed_ns\":%" PRIu64 ",\"max_elapsed_ns\":%" PRIu64
            ",\"ns_per_instruction\":%.6f,\"instructions_per_second\":%.6f}",
            index == 0U ? "" : ",", scenario->name,
            (unsigned)scenario->program_word, retired, retired,
            ADD_SUB_RETIREMENTS_PER_WINDOW, retired,
            result->final_a_raw40, result->final_b_raw40,
            (unsigned)result->final_c, (unsigned)result->final_ova,
            (unsigned)result->final_ovb, result->elapsed_ns, result->elapsed_ns,
            result->min_elapsed_ns, result->max_elapsed_ns, ns_per_instruction,
            instructions_per_second);
    }
    (void)printf("]}\n");
    return EXIT_SUCCESS;
}
