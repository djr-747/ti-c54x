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

## P2-W31P performance architecture freeze

The current hosted numbers are high enough to pause W31B2 and optimize the
common retired-instruction path first. They are implementation-throughput
observations only: host scheduling, frequency scaling, compiler choices, and
linker decisions are visible in the measurement, and none is a TI machine-cycle
or silicon timing claim.

The frozen audit (median nanoseconds per retired instruction) is committed in
`bench/audits/p2-w31p-baseline.json` with exact commands, source/binary hashes,
host/toolchain metadata, ranges, and semantic hashes:

| Build/profile | `nop` | `ld-dp` | `ld-asm` | `ld-arp` | `mixed-current` |
|---|---:|---:|---:|---:|---:|
| Portable O2, no LTO, capture A | 75.475 | 134.411 | 134.614 | 181.788 | 125.770 |
| Portable O2, no LTO, capture B | 111.626 | 124.096 | 121.174 | 121.829 | 108.691 |
| Portable O3 + LTO, capture A | 31.476 | 51.368 | 50.925 | 49.440 | 44.582 |

The two identical-binary O2 observations also demonstrate why a single hosted
run cannot establish a performance conclusion. The mixed workload is evolving
and is never substituted for a fixed scenario. These baseline observations
freeze the problem only; none is the later acceptance set.

The corresponding architecture decision is one catalog-derived binding and one
private value-owned trusted execution packet per instruction. The packet is
prepared once from the checked decode boundary by consuming a generated/static
operand plan; it carries copied decode facts, the selected process-lifetime code
pointer, and copied operand values, but retains no data pointer or caller-owned
storage. The fast CPU path performs one first-word ID lookup and one binding
selection; it does not repeat metadata or layout validation or field extraction.
Generic resource collection, one
trusted PC plan/commit, effect atomicity, observer isolation, and the existing
memory WAIT/RESUME path remain intact. Checked decode/dispatch APIs are retained
for forged-input tests. The portable default rejects both a 65,536-entry pointer
table and GNU computed goto; ordinary consumer IPO/LTO remains opt-in, while the
benchmark profile may use portable O3+LTO and must record its flags.

The performance gate is advisory and same-host: three stable observer-off captures
of both the portable O2/no-LTO diagnostic profile and portable O3/LTO release
profile must put every fixed scenario below 60 ns per retired instruction. Requiring
the no-LTO profile to pass prevents IPO from hiding an unnecessarily expensive
execution shape. There is no CI throughput threshold and no runtime knob for this
work. Structural tests prove one lookup/one binding, one packet-preparation
extraction pass, one PC plan, and the absence of opcode-specific CPU dispatch;
the production core gains no test counters or diagnostic instrumentation.

One acceptance set is exactly three consecutive captures with identical binary
and scenario hashes. For every target scenario in every capture,
`(max_elapsed_ns - min_elapsed_ns) / elapsed_ns` must be at most 0.15 and the
median must be below 60 ns. A failing set is retained as `INCONCLUSIVE`; it is
not silently discarded or selectively averaged. A replacement set has a new ID
and records the external condition that changed.

### P2-W31P acceptance capture sets

The first post-freeze three-capture sets are retained as inconclusive evidence.
They are not averaged, discarded, or treated as an acceptance result. Both sets
used one clean source commit (`0d4f333f58208e7ed0d749aec8eec39898c1342b`) and
identical binaries within the set; the complete raw records remain appended to
`bench/history/hosted-v1.jsonl`.

Set `P2-W31P-O2-0D4F333-SET1` (`portable-o2-no-lto`) used binary
`e5bb3ea8bd86cda00fa5b98aac90958d62beb9f6857c1a6f86e616ba6e325362`:

| Capture (UTC) | Fixed medians ns/instruction (`nop`, `ld-dp`, `ld-asm`, `ld-arp`) | Within-capture ranges (%) in same order | Checker result |
|---|---|---|---|
| 2026-09-07T12:04:56.602516+00:00 | 47.526, 62.364, 62.170, 62.382 | 18.111, 5.748, 2.064, 3.383 | `nop` stability and LD medians failed |
| 2026-09-07T12:05:11.139467+00:00 | 45.956, 63.241, 65.017, 65.721 | 9.123, 20.231, 34.394, 3.755 | LD medians/stability failed |
| 2026-09-07T12:05:23.662102+00:00 | 46.539, 64.027, 63.916, 59.520 | 8.463, 21.673, 22.469, 37.852 | LD medians/stability failed |

Set `P2-W31P-O3-LTO-0D4F333-SET1` (`portable-o3-lto`) used binary
`356e780749de36e7de1ae3bc2bc43b552128bd79b1f5bbb7aba593ec54857285`:

| Capture (UTC) | Fixed medians ns/instruction (`nop`, `ld-dp`, `ld-asm`, `ld-arp`) | Within-capture ranges (%) in same order | Checker result |
|---|---|---|---|
| 2026-09-07T12:05:39.050401+00:00 | 41.604, 46.251, 45.624, 46.989 | 16.894, 11.077, 6.944, 23.555 | `nop`/`ld-arp` stability failed |
| 2026-09-07T12:05:51.674315+00:00 | 45.790, 49.198, 49.232, 47.205 | 52.454, 42.901, 32.959, 8.859 | `nop`/LD stability failed |
| 2026-09-07T12:06:05.168771+00:00 | 46.880, 55.415, 62.579, 66.884 | 82.219, 74.285, 32.160, 85.569 | `nop`/LD medians and stability failed |

The acceptance status of both sets is `INCONCLUSIVE`; current W31P acceptance
remains pending. These are hosted implementation-throughput observations only,
with no TI machine-cycle, DSP, or silicon timing claim. Any replacement must use
a new set ID and document the external-condition change that motivated it.

#### SET2 replacement attempt (CPU2)

The next clean-commit replacement attempt used `taskset` CPU2 with agents and
build activity from this thread quiesced. Read-only `mpstat`/`ps` snapshots at
capture time nevertheless showed substantial unrelated host load, including
heavy work on SMT-sibling CPU3. This external condition explains deferring the
replacement decision; it does not change the acceptance rule.

Set `P2-W31P-O2-5613EB3-CPU2-SET2` (`portable-o2-no-lto`) used clean commit
`5613eb3dad890e091c86adfebddc1122f230f7d9` and binary
`3471856af42d10128572be44038e323d4e15b23321032c7b32f70548e4363a58`:

| Capture (UTC) | Fixed medians ns/instruction (`nop`, `ld-dp`, `ld-asm`, `ld-arp`) | Stability ranges (%) in same order | Checker failure |
|---|---|---|---|
| 2026-09-07T12:28:03.331306+00:00 | 26.970, 42.074, 40.214, 42.450 | 13.973, 30.038, 9.350, 3.791 | `ld-dp` 30.038% > 15% |
| 2026-09-07T12:28:24.242092+00:00 | 36.456, 47.938, 45.216, 44.283 | 8.081, 25.330, 5.127, 4.900 | `ld-dp` 25.330% > 15% |
| 2026-09-07T12:28:36.450514+00:00 | 35.104, 43.390, 43.643, 43.788 | 6.685, 12.853, 42.819, 12.210 | `ld-asm` 42.819% > 15% |

Set `P2-W31P-O3-LTO-5613EB3-CPU2-SET2` (`portable-o3-lto`) used the same clean
commit and binary
`2f36515659092aa944fede68b4333a485bcab1e75c78938168af9aeacbb1635e`:

| Capture (UTC) | Fixed medians ns/instruction (`nop`, `ld-dp`, `ld-asm`, `ld-arp`) | Stability ranges (%) in same order | Checker failure |
|---|---|---|---|
| 2026-09-07T12:28:52.571417+00:00 | 19.310, 27.085, 26.733, 25.227 | 71.814, 21.306, 15.944, 47.219 | all four stability ranges > 15% |
| 2026-09-07T12:29:04.340243+00:00 | 20.452, 25.198, 27.075, 30.221 | 62.047, 58.279, 49.250, 77.394 | all four stability ranges > 15% |
| 2026-09-07T12:29:18.697893+00:00 | 21.386, 27.869, 27.762, 27.605 | 25.687, 33.391, 15.898, 22.598 | all four stability ranges > 15% |

Every fixed-scenario median in SET2 is below the 60 ns target, but both sets are
`INCONCLUSIVE` solely because stability exceeded 15%; no timing target is
claimed as accepted. The six raw records are retained without averaging or
discarding, and no TI machine-cycle, DSP, or silicon timing claim is implied.
The next replacement requires a new set ID and a documented external-condition
change.

#### SET3 replacement attempt (CPU1 warm-up)

Each capture below used CPU1 affinity and a two-second same-core warm-up with
`timeout 2s taskset -c 1 sha256sum /dev/zero` immediately before the benchmark. The clean
source commit was `f6713e19b9516e89015b1163fa95eea3f0da46e5` for all six
records.

Set `P2-W31P-O2-F6713E1-CPU1-WARM-SET3` (`portable-o2-no-lto`) used binary
`3471856af42d10128572be44038e323d4e15b23321032c7b32f70548e4363a58`:

| Capture (UTC) | Fixed medians ns/instruction (`nop`, `ld-dp`, `ld-asm`, `ld-arp`) | Stability ranges (%) in same order | Checker failure |
|---|---|---|---|
| 2026-09-07T12:36:32.801561+00:00 | 29.290, 42.266, 42.099, 42.935 | 9.378, 2.687, 48.169, 1.945 | `ld-asm` 48.169% > 15% |
| 2026-09-07T12:36:55.817960+00:00 | 26.806, 40.612, 41.655, 42.086 | 56.331, 16.782, 6.175, 5.785 | `nop` 56.331%; `ld-dp` 16.782% > 15% |
| 2026-09-07T12:37:21.898408+00:00 | 25.409, 38.726, 38.065, 39.001 | 14.463, 5.095, 2.295, 3.455 | none; individually meets all gates |

Set `P2-W31P-O3-LTO-F6713E1-CPU1-WARM-SET3` (`portable-o3-lto`) used binary
`2f36515659092aa944fede68b4333a485bcab1e75c78938168af9aeacbb1635e`:

| Capture (UTC) | Fixed medians ns/instruction (`nop`, `ld-dp`, `ld-asm`, `ld-arp`) | Stability ranges (%) in same order | Checker failure |
|---|---|---|---|
| 2026-09-07T12:37:45.078671+00:00 | 20.189, 26.511, 25.401, 25.777 | 8.705, 3.087, 0.532, 1.528 | none; individually meets all gates |
| 2026-09-07T12:38:07.072135+00:00 | 22.442, 29.807, 29.142, 28.271 | 21.975, 21.771, 36.809, 4.729 | `nop`, `ld-dp`, `ld-asm` stability > 15% |
| 2026-09-07T12:38:29.359592+00:00 | 20.448, 27.694, 28.147, 27.044 | 20.412, 5.644, 15.974, 4.199 | `nop` 20.412%; `ld-asm` 15.974% > 15% |

Both SET3 cohorts are `INCONCLUSIVE` solely on the exact consecutive-three
stability rule. O2 capture 3 and O3 capture 1 individually met every median and
stability gate, but cannot be cherry-picked: acceptance requires three
consecutive captures in one set. Records are retained without averaging or
discarding, and no TI machine-cycle, DSP, or silicon timing claim is made. Any
next replacement requires a new set ID and a documented changed condition.

#### SET4 replacement attempt (CPU1 warm-up with transient CPU weight)

Each capture used CPU1 affinity, a five-second same-core warm-up with
`taskset -c 1 sha256sum /dev/zero`, and transient
`systemd-run --user --scope -p CPUWeight=10000` execution. The clean source
commit was `cbc62d4e3d9e28c31fdae024426ed315abc83b94` for all six records.

Set `P2-W31P-O2-CBC62D4-CPU1-WARM5-WEIGHT-SET4` (`portable-o2-no-lto`) used
binary `3471856af42d10128572be44038e323d4e15b23321032c7b32f70548e4363a58`:

| Capture (UTC) | Fixed medians ns/instruction (`nop`, `ld-dp`, `ld-asm`, `ld-arp`) | Stability ranges (%) in same order | Checker failure |
|---|---|---|---|
| 2026-09-07T12:44:13.777100+00:00 | 28.304, 39.588, 39.640, 40.816 | 14.538, 4.405, 14.522, 7.121 | none |
| 2026-09-07T13:07:28.167435+00:00 | 25.877, 37.505, 37.147, 37.878 | 8.868, 13.517, 2.420, 5.764 | none |
| 2026-09-07T13:07:58.672466+00:00 | 33.030, 44.124, 46.112, 45.768 | 12.036, 4.896, 13.117, 44.213 | `ld-arp` 44.2131% > 15% |

Set `P2-W31P-O3-LTO-CBC62D4-CPU1-WARM5-WEIGHT-SET4` (`portable-o3-lto`) used
binary `2f36515659092aa944fede68b4333a485bcab1e75c78938168af9aeacbb1635e`:

| Capture (UTC) | Fixed medians ns/instruction (`nop`, `ld-dp`, `ld-asm`, `ld-arp`) | Stability ranges (%) in same order | Checker failure |
|---|---|---|---|
| 2026-09-07T13:08:28.845269+00:00 | 19.371, 25.197, 25.859, 25.883 | 40.151, 3.118, 6.049, 4.753 | `nop` 40.1510% > 15% |
| 2026-09-07T13:08:57.583995+00:00 | 23.665, 33.315, 31.357, 30.794 | 11.201, 19.838, 8.710, 7.333 | `ld-dp` 19.8382% > 15% |
| 2026-09-07T13:09:27.692673+00:00 | 20.428, 27.285, 26.646, 27.143 | 10.001, 42.990, 7.759, 3.659 | `ld-dp` 42.9899% > 15% |

All SET4 fixed-scenario medians are below 60 ns; both cohorts are
`INCONCLUSIVE` solely on stability. The exact consecutive-three rule forbids
cherry-picking individual captures. Records remain retained without averaging
or discarding, with no TI machine-cycle, DSP, or silicon timing claim. Any next
replacement requires a new set ID and a documented changed condition.

W31B2 remains paused until this gate is met. Its first two immediate-READY memory
forms, `LD *ARx+,dst` and `STL src,*ARx+`, must each receive dedicated benchmark
scenarios and meet the same fixed-scenario target before ISA fanout resumes. The
benchmark must not turn a WAIT-capable endpoint into an immediate-only semantic
shortcut: memory WAIT, READY, FAULT, cancellation, and commit visibility continue
to be tested through their existing contract.

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
