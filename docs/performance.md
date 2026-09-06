# Hosted performance history

This document owns the hosted performance-capture contract. It does not add a
timing model to the standalone core and it is not evidence for TI machine-cycle
or silicon behavior. The core remains allocation-free, observer-independent,
and vendorable; benchmark tooling is a separate `bench/` and `tools/perf/`
surface.

## Capture policy

The benchmark captures end-to-end CPU execution through the public program bus
with observers disabled. A monotonic host clock measures the complete run,
including fetch, decode, dispatch, sequencing, and retirement. The result is an
implementation-throughput observation, not an architectural cost claim.

Four fixed scenarios are stable history anchors and must not be silently
redefined:

| Scenario | Purpose |
|---|---|
| `nop` | Empty semantic dispatch baseline |
| `ld-dp` | Immediate status-field load to DP |
| `ld-asm` | Immediate status-field load to ASM |
| `ld-arp` | Immediate status-field load to ARP |

`mixed-current` is deliberately different: it follows the production catalog
as the ISA grows. While an instruction requires architecturally initialized state
that has no public bootstrap surface, the stream is explicitly a
`reset-runnable-subset`; each result records `catalog_coverage_mode` and
`catalog_covered_instruction_count` alongside the total catalog row count. Every
record also carries a deterministic per-scenario semantic hash, so a mixed result
is never compared as if its workload were unchanged. The fixed scenarios are the
comparable regression canaries as catalog rows are added; their matching semantic
hashes permit observing dispatch/catalog growth without pretending the mixed
subset covers newly state-dependent instructions.

Each scenario records seven samples, each preceded by 16 fixed warmup windows;
the warmup windows are not included in history. Each sample repeats multiple
measurement windows whose
retirement count is strictly below 65,535; the repeated windows reduce short
host scheduling noise without introducing an architectural cycle assumption.
The capture output records the exact scenario, sample/window policy, compiler,
platform, CPU, build flags, clock source, clean source commit, catalog count,
scenario semantic hash, and benchmark/source hashes.

For each like-for-like scenario, comparison also reports the baseline and
current within-capture spread, `(max_elapsed_ns - min_elapsed_ns) /
elapsed_ns * 100`, where `elapsed_ns` is the median sample. These percentages
contextualize host scheduling and frequency noise; they are advisory only and
do not create a pass/fail threshold.

History is JSONL and append-only. `bench-record` may append only from a clean
commit; a dirty tree or missing identity metadata is a capture error rather
than an implicit comparison baseline. A same-cohort comparison is advisory and
requires matching CPU, platform/machine, compiler, build flags, and clock
source. Fixed-scenario percentage comparisons additionally require the same
scenario semantic hash; `mixed-current` may change catalog count, hash, or
coverage metadata and is then reported as an evolving, non-like-for-like trend
rather than a regression percentage. There is intentionally no flaky CI throughput threshold: hosted
history is for trend review, while correctness and architectural claims remain
test/evidence work.

The intended command surface is:

```text
make bench          # one observer-off capture, no history mutation
make bench-smoke    # deterministic smoke validation
make bench-record   # clean-commit capture appended to hosted history
make bench-compare  # advisory comparison with latest same-cohort history
```

## Recorded cohorts

The initial clean catalog-four record at `aa0afc0` is the fixed-scenario baseline.
The catalog-six MAX/MIN record at `a14d331` measures NOP at 65.538 ns/15.258 MIPS
and the three fixed LD scenarios at 97.485–97.982 ns/10.206–10.258 MIPS. Their
cost is 2.37–3.79% above the initial fixed cohort while the new record's
within-capture spreads are 2.19–8.76%; this is a hosted trend observation, not a
confirmed architectural or silicon regression. `mixed-current` records
89.986 ns/11.113 MIPS with explicit `reset-runnable-subset` coverage of four of
six catalog rows and is not compared to the catalog-four mixed result.

Hosted scripts own capture and same-cohort comparison. They must not be linked
into the core or required by a standalone build. The `TI_C54X_BUILD_BENCHMARKS`
knob is an ACTIVE TEST/KEEP control, default OFF, owned by `bench/`; it has no
architectural effect and remains while hosted performance tracking is supported.

## Build and tool surface

The hosted surface is now named consistently: the `TI_C54X_BUILD_BENCHMARKS`
knob gates the CMake benchmark target, while Make exposes `bench`, `bench-smoke`,
clean-commit `bench-record`, and advisory `bench-compare`.
`tools/perf/capture.py` owns strict JSONL capture/history and
`tools/perf/compare.py` owns advisory same-cohort reporting.
These tools remain outside the standalone library and do not alter core or
emulator behavior. The hosted benchmark currently requires POSIX
`CLOCK_MONOTONIC`; this restriction does not apply to the default library build.

## Residual benchmark constraint

`mixed-current` currently contains only one-word forms. Before the first
multiword instruction is admitted, its per-window retirement count and
program-word span must be derived together so the generated stream remains
below the unresolved 16-bit program-PC boundary. This is a hosted benchmark
correctness constraint; it does not resolve or claim the core's architectural
boundary behavior.

## Ownership and acceptance

`bench/` owns scenario construction and observer-off benchmark binaries.
`tools/perf/` owns hosted capture, metadata, JSONL history, and advisory
same-cohort comparison. The catalog owner supplies the semantic count/hash
without allowing benchmark code to become a second instruction inventory.
Quality tests cover schema validation, metadata construction, clean-history
append behavior, and a same-cohort comparison. They do not prove silicon timing,
memory latency, IRQ timing, or emulator scheduling.
