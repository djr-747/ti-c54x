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

The prior clean catalog-seven record at `7cad341` remains in history as the
catalog-seven cohort:

| Scenario | Median cost | Throughput |
|---|---:|---:|
| `nop` | 69.057 ns | 14.481 MIPS |
| `ld-dp` | 99.866 ns | 10.013 MIPS |
| `ld-asm` | 100.477 ns | 9.953 MIPS |
| `ld-arp` | 96.845 ns | 10.326 MIPS |
| `mixed-current` | 89.591 ns | 11.162 MIPS |

Relative to catalog-six, the fixed scenarios are -1.16% to +5.37% per the
same-cohort comparison. The catalog-seven within-capture spreads are 7.64%
(`nop`), 63.19% (`ld-dp`), 9.49% (`ld-asm`), and 11.52% (`ld-arp`), so these
measurements do not establish a confirmed regression. `mixed-current` remains
an evolving workload: it is reset-runnable for four of seven catalog rows, and
its catalog count changed even though the reset-runnable stream remains four
rows, so it is not compared as a like-for-like result. All figures are hosted
implementation-throughput observations, not
architectural or silicon timing evidence.

The clean catalog-nine record at `705ac7b` remains the prior hosted cohort:

| Scenario | Median cost | Throughput |
|---|---:|---:|
| `nop` | 61.839 ns | 16.171 MIPS |
| `ld-dp` | 91.592 ns | 10.918 MIPS |
| `ld-asm` | 91.619 ns | 10.915 MIPS |
| `ld-arp` | 96.653 ns | 10.346 MIPS |
| `mixed-current` | 87.562 ns | 11.420 MIPS |

Relative to catalog-seven, the same-cohort fixed-scenario deltas are -10.45%
(`nop`), -8.29% (`ld-dp`), -8.82% (`ld-asm`), and -0.20% (`ld-arp`). The
catalog-nine within-capture spreads are 29.42% (`nop`), 6.05% (`ld-dp`),
9.95% (`ld-asm`), and 14.40% (`ld-arp`). These are hosted noise/trend
observations only and do not establish a confirmed improvement or regression.
`mixed-current` is evolving and non-comparable because the catalog count changed;
it is reset-runnable for four of nine rows. No result here is architectural or
silicon timing evidence.

The clean catalog-ten record at `cd88596` remains the prior hosted cohort:

| Scenario | Median cost | Throughput |
|---|---:|---:|
| `nop` | 97.138 ns | 10.295 MIPS |
| `ld-dp` | 102.519 ns | 9.754 MIPS |
| `ld-asm` | 97.918 ns | 10.213 MIPS |
| `ld-arp` | 99.295 ns | 10.071 MIPS |
| `mixed-current` | 91.728 ns | 10.902 MIPS |

Relative to catalog-nine, the same-cohort fixed-scenario deltas are +57.08%
(`nop`), +11.93% (`ld-dp`), +6.88% (`ld-asm`), and +2.73% (`ld-arp`). The
catalog-ten within-capture spreads are 36.02% (`nop`), 10.71% (`ld-dp`),
32.95% (`ld-asm`), and 10.72% (`ld-arp`). The mixed workload remains evolving
and non-comparable; it is reset-runnable for four of ten rows. Wide host
noise/spreads and the absence of fixed-scenario semantic or CPU-orchestration
changes do not establish a confirmed regression or improvement. These are hosted
implementation-throughput observations, not DSP timing evidence.

The clean catalog-thirteen record at `ef87c944ccf325617487df2946fafbfa0cfe17ba`
remains a prior hosted cohort. CI is green at
<https://github.com/djr-747/ti-c54x/actions/runs/34055843312>.

| Scenario | Median cost | Throughput |
|---|---:|---:|
| `nop` | 121.420 ns | 8.236 MIPS |
| `ld-dp` | 209.005 ns | 4.785 MIPS |
| `ld-asm` | 202.901 ns | 4.929 MIPS |
| `ld-arp` | 113.815 ns | 8.786 MIPS |
| `mixed-current` | 96.359 ns | 10.378 MIPS |

The reset-runnable subset is four of 13 catalog rows. Relative to catalog-ten,
the fixed-scenario deltas are +25.00% (`nop`), +103.87% (`ld-dp`), +107.22%
(`ld-asm`), and +14.62% (`ld-arp`); current within-capture spreads are 51.13%,
10.86%, 5.71%, and 117.11%, respectively. Fixed code paths and CPU
orchestration are unchanged, so the large deltas and spreads do not support a
confirmed regression. This is a hosted implementation observation only, not
DSP timing evidence.

The clean catalog-fifteen record at `7164ace` remains a prior hosted cohort.
CI is green at <https://github.com/djr-747/ti-c54x/actions/runs/34059815766>.

| Scenario | Median cost | Throughput |
|---|---:|---:|
| `nop` | 109.060 ns | 9.169 MIPS |
| `ld-dp` | 182.474 ns | 5.480 MIPS |
| `ld-asm` | 194.054 ns | 5.153 MIPS |
| `ld-arp` | 111.847 ns | 8.941 MIPS |
| `mixed-current` | 88.796 ns | 11.262 MIPS |

The reset-runnable subset is four of 15 catalog rows. Relative to catalog-thirteen,
the fixed-scenario deltas are -10.18% (`nop`), -12.69% (`ld-dp`), -4.36%
(`ld-asm`), and -1.73% (`ld-arp`); current within-capture spreads are 50.31%,
3.83%, 6.90%, and 112.44%, respectively. The mixed spread is 3.58%, but that
workload is not comparable because catalog coverage and semantic hash changed.
These hosted observations establish no confirmed improvement or regression and make
no DSP or silicon timing claim.

An immediate identical-binary diagnostic rerun (binary hash `144ffa...`; the
tree was dirty only because history was appended) normalized to NOP 66.571 ns /
15.022 MIPS, LD-DP 98.326 / 10.170, LD-ASM 98.038 / 10.200, LD-ARP 96.481 /
10.365, and mixed 86.448 / 11.568. It corroborates host noise only and is not a
history baseline or performance claim.

The latest clean catalog-sixteen record at `fedbd6a` is the current hosted
cohort. CI is green at
<https://github.com/djr-747/ti-c54x/actions/runs/34062796872>.

| Scenario | Median cost | Throughput |
|---|---:|---:|
| `nop` | 124.251 ns | 8.048 MIPS |
| `ld-dp` | 205.652 ns | 4.863 MIPS |
| `ld-asm` | 209.142 ns | 4.781 MIPS |
| `ld-arp` | 222.837 ns | 4.488 MIPS |
| `mixed-current` | 206.376 ns | 4.846 MIPS |

The reset-runnable subset is four of 16 catalog rows. Relative to
catalog-fifteen, the comparable fixed-scenario deltas are +13.93% (`nop`),
+12.70% (`ld-dp`), +7.78% (`ld-asm`), and +99.23% (`ld-arp`); current
within-capture spreads are 22.39%, 0.92%, 4.17%, and 8.71%, respectively.
`mixed-current` is evolving and non-comparable because the catalog count
changed.

A second capture of the identical `def3fe...` binary, with the tree dirty only
because the history append was present, measured NOP 68.561 ns/14.586 MIPS,
LD-DP 101.587/9.844, LD-ASM 104.687/9.552, LD-ARP 106.211/9.415, and mixed
96.438/10.369. These are -44.82% to -53.27% changes against the clean record
without a code or binary change. The clean catalog-sixteen deltas therefore do
not establish a confirmed regression; they remain hosted noise/trend
observations and make no architectural or silicon timing claim.

The prior clean catalog-twenty-two record at `9c22a33` remains a hosted cohort.
CI is green at
<https://github.com/djr-747/ti-c54x/actions/runs/34067048536>.

| Scenario | Median cost | Throughput |
|---|---:|---:|
| `nop` | 131.883 ns | 7.582 MIPS |
| `ld-dp` | 233.294 ns | 4.286 MIPS |
| `ld-asm` | 240.304 ns | 4.161 MIPS |
| `ld-arp` | 206.770 ns | 4.836 MIPS |
| `mixed-current` | 109.273 ns | 9.151 MIPS |

The reset-runnable subset remains four of 22 catalog rows. The six new two-word
immediate AND/OR/XOR rows are not benchmark-exercised: each requires initialized
accumulator state, and the public API intentionally provides no accumulator
bootstrap surface. `mixed-current` therefore retains the same four-row stream
and semantic hash; the fixed scenarios also retain their existing semantics.
An identical-binary diagnostic capture (binary hash `d1d24...`; the tree was
dirty only because history was appended) measured NOP 80.803 ns, LD-DP
114.568 ns, LD-ASM 127.581 ns, LD-ARP 118.947 ns, and mixed 108.974 ns.
The host spread and unchanged workloads mean this cohort establishes no
confirmed regression or improvement; it is a hosted throughput observation,
not a DSP or silicon timing claim.

The clean same-catalog-twenty-two record at `66ef384` is the current hosted
cohort. Full local and remote CI are green at
<https://github.com/djr-747/ti-c54x/actions/runs/34069533137>; its binary hash
starts `b0c7d039`, and its scenario semantic hashes match the prior record.

| Scenario | Median cost | Throughput |
|---|---:|---:|
| `nop` | 111.770 ns | 8.947 MIPS |
| `ld-dp` | 116.084 ns | 8.614 MIPS |
| `ld-asm` | 117.836 ns | 8.486 MIPS |
| `ld-arp` | 159.492 ns | 6.270 MIPS |
| `mixed-current` | 170.433 ns | 5.867 MIPS |

Reset-runnable coverage remains four of 22, and the benchmark workloads and
catalog are unchanged. Relative to the prior clean `9c22a33` record, the
same-catalog deltas in median cost are -15.25% (`nop`), -50.24% (`ld-dp`),
-50.96% (`ld-asm`), -22.87% (`ld-arp`), and +55.97% (`mixed-current`). The
current within-capture spreads are 42.40%, 29.18%, 5.14%, 49.67%, and 48.57%,
respectively (the prior record's corresponding spreads were 50.40%, 10.02%,
6.80%, 69.92%, and 14.95%). An identical-binary diagnostic measured 76.978,
119.232, 158.135, 173.612, and 108.285 ns in that scenario order. These
spreads and same-binary variation establish no confirmed regression or
improvement. The architectural change adds canonical state ownership and one
additional normally-not-taken SXM resource-mask check in the generic collector;
it does not change the workloads or catalog and is not a DSP-timing result.

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

`mixed-current` currently contains only reset-runnable one-word forms and
deliberately excludes every accumulator-input row because the public API has no
initialized-accumulator bootstrap surface. Multiword rows are admitted in the
catalog but remain outside the benchmark stream. Any future benchmark inclusion
must derive per-window retirement count and program-word span together and keep
the generated stream below the unresolved 16-bit program-PC boundary. This is a
hosted benchmark correctness constraint; it does not resolve or claim the
core's architectural boundary behavior.

## Ownership and acceptance

`bench/` owns scenario construction and observer-off benchmark binaries.
`tools/perf/` owns hosted capture, metadata, JSONL history, and advisory
same-cohort comparison. The catalog owner supplies the semantic count/hash
without allowing benchmark code to become a second instruction inventory.
Quality tests cover schema validation, metadata construction, clean-history
append behavior, and a same-cohort comparison. They do not prove silicon timing,
memory latency, IRQ timing, or emulator scheduling.
