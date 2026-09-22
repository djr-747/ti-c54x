/*
 * Module contract
 *
 * Owns: a fixed, hosted performance baseline for public CPU execution. It
 * measures stable NOP, immediate status-field loads, and mixed-current
 * scenarios with observer support disabled by the benchmark build.
 * Does not own: core implementation, diagnostics, environment controls,
 * persistent benchmark history, or production timing policy. The benchmark
 * uses CLOCK_MONOTONIC; elapsed_ticks are nanosecond ticks from that clock,
 * and setup/inspection time is excluded.
 */

#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <ti_c54x/cpu.h>

#ifndef CLOCK_MONOTONIC
#error "Hosted benchmark requires POSIX CLOCK_MONOTONIC"
#endif

enum {
    TI_C54X_BENCH_SCHEMA_VERSION = 1,
    TI_C54X_BENCH_SAMPLES = 7,
    TI_C54X_BENCH_WARMUPS = 16,
    TI_C54X_BENCH_WINDOWS = 16,
    TI_C54X_BENCH_RETIREMENTS_PER_WINDOW = 50000
};

#define TI_C54X_OPCODE_EXACT(...) +1
#define TI_C54X_OPCODE_MASKED(...) +1
#define TI_C54X_OPCODE_MASKED2(...) +1
enum {
    TI_C54X_BENCH_CATALOG_INSTRUCTION_COUNT = 0
#include "../src/isa/catalog/opcode_catalog.def"
};
#undef TI_C54X_OPCODE_EXACT
#undef TI_C54X_OPCODE_MASKED
#undef TI_C54X_OPCODE_MASKED2

typedef struct bench_bus_context {
    const uint16_t *words;
    uint64_t reads;
} bench_bus_context;

typedef struct bench_sample {
    uint64_t elapsed_ns;
    uint64_t retired;
} bench_sample;

typedef enum bench_catalog_coverage_mode {
    TI_C54X_BENCH_COVERAGE_FIXED = 0,
    TI_C54X_BENCH_COVERAGE_RESET_RUNNABLE_SUBSET = 1
} bench_catalog_coverage_mode;

typedef struct bench_scenario {
    const char *name;
    const uint16_t *words;
    size_t word_count;
    unsigned instruction_count;
    bool catalog_current;
    bench_catalog_coverage_mode coverage_mode;
    unsigned catalog_covered_instruction_count;
} bench_scenario;

static uint64_t scenario_hash(const bench_scenario *scenario)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    for (const unsigned char *name = (const unsigned char *)scenario->name;
         *name != 0U; ++name) {
        hash ^= (uint64_t)*name;
        hash *= UINT64_C(1099511628211);
    }
    hash ^= (uint64_t)scenario->word_count;
    hash *= UINT64_C(1099511628211);
    hash ^= (uint64_t)scenario->instruction_count;
    hash *= UINT64_C(1099511628211);
    for (size_t index = 0U; index < scenario->word_count; ++index) {
        hash ^= (uint64_t)scenario->words[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static bool valid_scenario_shape(const bench_scenario *scenario)
{
    if (scenario == NULL || scenario->name == NULL || scenario->name[0] == '\0' ||
        scenario->words == NULL ||
        scenario->word_count == 0U || scenario->word_count > UINT32_C(65536) ||
        scenario->instruction_count == 0U) {
        return false;
    }
    if (!scenario->catalog_current) {
        return scenario->coverage_mode == TI_C54X_BENCH_COVERAGE_FIXED &&
               scenario->instruction_count == 1U &&
               scenario->catalog_covered_instruction_count == 1U;
    }
    return scenario->coverage_mode ==
               TI_C54X_BENCH_COVERAGE_RESET_RUNNABLE_SUBSET &&
           scenario->catalog_covered_instruction_count > 0U &&
           scenario->catalog_covered_instruction_count <
               TI_C54X_BENCH_CATALOG_INSTRUCTION_COUNT &&
           scenario->instruction_count ==
               scenario->catalog_covered_instruction_count;
}

static const char *coverage_mode_name(bench_catalog_coverage_mode mode)
{
    return mode == TI_C54X_BENCH_COVERAGE_FIXED
               ? "fixed"
               : "reset-runnable-subset";
}

static ti_c54x_bus_status bench_program_read(
    void *context,
    ti_c54x_program_word_address address_word,
    uint16_t *value_word)
{
    bench_bus_context *bus = (bench_bus_context *)context;
    const size_t index = (size_t)address_word;

    bus->reads++;
    *value_word = bus->words[index];
    return TI_C54X_BUS_OK;
}

static bool bench_now_ns(uint64_t *out_ns)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return false;
    }
    if (now.tv_sec < 0 || now.tv_nsec < 0 || now.tv_nsec >= 1000000000L) {
        return false;
    }
    *out_ns = (uint64_t)now.tv_sec * UINT64_C(1000000000) +
              (uint64_t)now.tv_nsec;
    return true;
}

static int compare_samples(const void *left, const void *right)
{
    const bench_sample *a = (const bench_sample *)left;
    const bench_sample *b = (const bench_sample *)right;

    if (a->elapsed_ns < b->elapsed_ns) {
        return -1;
    }
    if (a->elapsed_ns > b->elapsed_ns) {
        return 1;
    }
    return 0;
}

static bool run_scenario(const bench_scenario *scenario)
{
    bench_sample samples[TI_C54X_BENCH_SAMPLES];
    void *storage = malloc(ti_c54x_cpu_size());
    if (storage == NULL) {
        return false;
    }
    for (size_t sample_index = 0; sample_index < TI_C54X_BENCH_SAMPLES;
         ++sample_index) {
        uint16_t *image = (uint16_t *)malloc(UINT32_C(65536) * sizeof(*image));
        bench_bus_context bus_context = {image, 0U};
        ti_c54x_bus bus = {0};
        const ti_c54x_config config = {0U, {0U, NULL, NULL}};
        ti_c54x_cpu *cpu = NULL;
        ti_c54x_run_result result;
        ti_c54x_state state;
        uint64_t elapsed_ns = 0U;
        if (!valid_scenario_shape(scenario) || image == NULL) {
            free(image);
            free(storage);
            return false;
        }
        for (size_t image_index = 0U; image_index < UINT32_C(65536); ++image_index) {
            image[image_index] = scenario->words[image_index % scenario->word_count];
        }
        bus.context = &bus_context;
        bus.program_read_word = bench_program_read;
        if (ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config, &bus,
                             &cpu) != TI_C54X_ERROR_OK) {
            free(image);
            free(storage);
            return false;
        }
        for (unsigned warmup = 0U; warmup < TI_C54X_BENCH_WARMUPS; ++warmup) {
            result = ti_c54x_cpu_run(cpu,
                                     TI_C54X_BENCH_RETIREMENTS_PER_WINDOW);
            if (result.reason != TI_C54X_STOP_BUDGET_EXHAUSTED ||
                result.instructions_retired !=
                    TI_C54X_BENCH_RETIREMENTS_PER_WINDOW ||
                ti_c54x_cpu_reset(cpu) != TI_C54X_ERROR_OK) {
                ti_c54x_cpu_deinit(cpu);
                free(image);
                free(storage);
                return false;
            }
        }
        for (unsigned window = 0U; window < TI_C54X_BENCH_WINDOWS; ++window) {
            uint64_t start_ns = 0U;
            uint64_t end_ns = 0U;
            bus_context.reads = 0U;
            if (ti_c54x_cpu_reset(cpu) != TI_C54X_ERROR_OK ||
                !bench_now_ns(&start_ns)) {
                ti_c54x_cpu_deinit(cpu);
                free(image);
                free(storage);
                return false;
            }
            result = ti_c54x_cpu_run(cpu,
                                     TI_C54X_BENCH_RETIREMENTS_PER_WINDOW);
            if (!bench_now_ns(&end_ns) ||
                result.reason != TI_C54X_STOP_BUDGET_EXHAUSTED ||
                result.bus_status != TI_C54X_BUS_OK ||
                result.instructions_attempted !=
                    TI_C54X_BENCH_RETIREMENTS_PER_WINDOW ||
                result.instructions_retired !=
                    TI_C54X_BENCH_RETIREMENTS_PER_WINDOW ||
                ti_c54x_cpu_inspect(cpu, &state) != TI_C54X_ERROR_OK ||
                state.instructions_attempted_total !=
                    TI_C54X_BENCH_RETIREMENTS_PER_WINDOW ||
                state.instructions_retired_total !=
                    TI_C54X_BENCH_RETIREMENTS_PER_WINDOW ||
                bus_context.reads != TI_C54X_BENCH_RETIREMENTS_PER_WINDOW ||
                end_ns < start_ns || UINT64_MAX - elapsed_ns < end_ns - start_ns) {
                ti_c54x_cpu_deinit(cpu);
                free(image);
                free(storage);
                return false;
            }
            elapsed_ns += end_ns - start_ns;
        }
        samples[sample_index].elapsed_ns = elapsed_ns;
        samples[sample_index].retired =
            (uint64_t)TI_C54X_BENCH_WINDOWS *
            (uint64_t)TI_C54X_BENCH_RETIREMENTS_PER_WINDOW;
        ti_c54x_cpu_deinit(cpu);
        free(image);
    }
    qsort(samples, TI_C54X_BENCH_SAMPLES, sizeof(samples[0]), compare_samples);
    {
        const uint64_t median_ns = samples[TI_C54X_BENCH_SAMPLES / 2U].elapsed_ns;
        const uint64_t min_ns = samples[0].elapsed_ns;
        const uint64_t max_ns = samples[TI_C54X_BENCH_SAMPLES - 1U].elapsed_ns;
        const uint64_t retired = samples[0].retired;
        (void)printf(
            "{\"schema_version\":%d,\"clock_source\":\"CLOCK_MONOTONIC\","
            "\"scenario\":\"%s\",\"retired_count\":%" PRIu64
            ",\"elapsed_ns\":%" PRIu64 ",\"elapsed_ticks\":%" PRIu64
            ",\"min_elapsed_ns\":%" PRIu64 ",\"max_elapsed_ns\":%" PRIu64
            ",\"samples\":%d,\"warmup_windows\":%d,\"window_count\":%d"
            ",\"retirements_per_window\":%d,\"semantic_hash\":\"%016" PRIx64 "\""
            ",\"scenario_instruction_count\":%u,\"catalog_instruction_count\":%d"
            ",\"catalog_coverage_mode\":\"%s\""
            ",\"catalog_covered_instruction_count\":%u"
            ",\"ns_per_instruction\":%.3f"
            ",\"instructions_per_second\":%.3f}\n",
            TI_C54X_BENCH_SCHEMA_VERSION, scenario->name, retired, median_ns,
            median_ns, min_ns, max_ns, TI_C54X_BENCH_SAMPLES,
            TI_C54X_BENCH_WARMUPS, TI_C54X_BENCH_WINDOWS,
            TI_C54X_BENCH_RETIREMENTS_PER_WINDOW,
            scenario_hash(scenario), scenario->instruction_count,
            TI_C54X_BENCH_CATALOG_INSTRUCTION_COUNT,
            coverage_mode_name(scenario->coverage_mode),
            scenario->catalog_covered_instruction_count,
            (double)median_ns / (double)retired,
            (double)retired * 1000000000.0 / (double)median_ns);
    }
    free(storage);
    return true;
}

int main(void)
{
    static const uint16_t nop[] = {UINT16_C(0xF495)};
    static const uint16_t ld_dp[] = {UINT16_C(0xEA00) | UINT16_C(0x01FF)};
    static const uint16_t ld_asm[] = {UINT16_C(0xED00) | UINT16_C(0x001F)};
    static const uint16_t ld_arp[] = {UINT16_C(0xF4A0) | UINT16_C(0x0007)};
    static const uint16_t mixed[] = {UINT16_C(0xF495),
                                    UINT16_C(0xEA00) | UINT16_C(0x01FF),
                                    UINT16_C(0xED00) | UINT16_C(0x001F),
                                    UINT16_C(0xF4A0) | UINT16_C(0x0007)};
    static const bench_scenario scenarios[] = {
        {"nop", nop, 1U, 1U, false, TI_C54X_BENCH_COVERAGE_FIXED, 1U},
        {"ld-dp", ld_dp, 1U, 1U, false, TI_C54X_BENCH_COVERAGE_FIXED, 1U},
        {"ld-asm", ld_asm, 1U, 1U, false, TI_C54X_BENCH_COVERAGE_FIXED, 1U},
        {"ld-arp", ld_arp, 1U, 1U, false, TI_C54X_BENCH_COVERAGE_FIXED, 1U},
        {"mixed-current", mixed, 4U, 4U, true,
         TI_C54X_BENCH_COVERAGE_RESET_RUNNABLE_SUBSET, 4U}};

    for (size_t index = 0; index < sizeof(scenarios) / sizeof(scenarios[0]);
         ++index) {
        if (!valid_scenario_shape(&scenarios[index])) {
            (void)fprintf(stderr, "invalid benchmark scenario shape: %s\n",
                          scenarios[index].name);
            return EXIT_FAILURE;
        }
        if (!run_scenario(&scenarios[index])) {
            (void)fprintf(stderr, "benchmark semantic failure: %s\n",
                          scenarios[index].name);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
