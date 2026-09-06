# ti-c54x

`ti-c54x` is a standalone C11 implementation of the TI C54x DSP foundations. It
is being developed as a small, test-first library that can eventually be vendored
by emulators and other tools without pulling in a phone model, firmware image, or
host application.

The repository is intentionally independent of the Nokia DCT3 emulator. The
emulator may provide optional black-box traces and externally supplied DSP images
for later validation, but it is not a build or test dependency. Integration is
deferred until the standalone contracts and provenance review are complete.

## Status

Foundations are under active development. The public API, supported C54x variant,
opcode inventory, and distribution license are not final. In particular, the
provenance and licensing review is still open; this repository does not currently
make a license-selection or compatibility claim. See the [provenance review](docs/provenance.md),
[architecture baseline](docs/architecture-baseline.md), and [evidence index](docs/evidence-index.md).
The machine-readable [`config/evidence.json`](config/evidence.json) registry is checked by
the standalone Python quality gate.

Current progress:

- Production decode and CPU execution now cover 15 declarative one-word
  opcode rows: evidence-backed `NOP`; the three immediate status-field forms
  `LD #k9,DP`, `LD #k5,ASM`, and `LD #k3,ARP`; and `MAX`, `MIN`, `CMPL`, `ROL`,
  `ROR`, `SFTL`, `AND`, `OR`, `XOR`, `ROLTC`, and `SFTC`. The logical families
  add 384
  register-accumulator encodings with full-40-bit Boolean/shift semantics and
  preserve C and all other status. Those rows expand to 1081 populated
  first-word encodings; that is a lookup-population count, not instruction
  count. Exact/masked
  catalog generation, complete instruction bundles, extension fetch, operand
  extraction, dispatch, PC staging, and retirement are covered by the standalone
  test surfaces. Repeatability and machine timing remain unresolved; no `cpu.c`,
  public API, memory, RPT/IRQ, or timing change was needed for the two new rows.
- P2-W23 now adds four exact MAX/MIN accumulator encodings (`0xF486`, `0xF586`,
  `0xF487`, `0xF587`) with signed full-40-bit selection, tie/C semantics,
  checked accumulator knownness, transactional sequencer commit, and public CPU
  retirement proof. The full standalone local gate is green; repeatability and
  machine timing remain unresolved.
- P2-W24 now generalizes the accumulator effect to explicit C `PRESERVE`/`WRITE`
  actions and proves all four CMPL source/destination forms with exhaustive
  decode, full-40-bit one's-complement semantics, and atomic CPU retirement.
  Repeatability and machine timing remain unresolved; no silicon timing claim is
  made.
- P2-W25 now provides a generic resource-driven input collector with
  `ACCUMULATORS | STATUS_C`, typed `status_c`/`status_c_known`, transactional
  unknownness handling, and no opcode-specific CPU branch. ROL/ROR use the
  existing accumulator-write plus C-write effect path; benchmark coverage
  remains the four reset-runnable NOP/LD rows out of 13.
- P2-W26 now proves SFTL's generic source/destination/shift5 layout, signed
  `-16..15` handling, low-32 shift semantics with explicit C output, guard
  clearing, and atomic retirement through existing generic effects. The full
  local P2-W26 gate is green. No `cpu.c` change, public API, memory,
  RPT/IRQ, or timing claim was added; reset-runnable coverage is four of 13
  rows.
- P2-W28A establishes typed TC status access, resource-driven TC collection, and
  an atomic value-owned C+TC status delta; the full local gate is green and
  [remote CI is green](https://github.com/djr-747/ti-c54x/actions/runs/34058489923).
  P2-W28B adds the
  ROLTC/SFTC handlers and CPU proofs through the existing generic path: targeted
  local checks, independent review, and the full `make test` gate are green.
  Catalog coverage is now
  15 rows/1081 populated encodings with reset-runnable coverage at 4/15; no
  `cpu.c`, public API, memory, RPT/IRQ, timing, knob, diagnostic, or environment
  change was added. The central generator schema was minimally extended for TC
  resource tokens. Repeatability remains unresolved; the distinct SFTC
  guard-only-plus-low32-zero behavior boundary is also unresolved.
- The registry now records 110 evidence items, 162 requirements (131 implemented,
  9 planned, 22 unresolved), and 120 tests (111 passing, 9 planned). The latest clean
  catalog-fifteen hosted performance record is captured at `7164ace` (CI
  [green](https://github.com/djr-747/ti-c54x/actions/runs/34059815766)): NOP
  109.060 ns / 9.169 MIPS, LD-DP 182.474 ns / 5.480 MIPS, LD-ASM 194.054 ns /
  5.153 MIPS, LD-ARP 111.847 ns / 8.941 MIPS, and mixed-current 88.796 ns /
  11.262 MIPS with reset-runnable coverage of 4/15. Versus catalog-thirteen,
  fixed deltas are -10.18%, -12.69%, -4.36%, and -1.73%; current spreads are
  50.31%, 3.83%, 6.90%, and 112.44%. The mixed spread is 3.58% but is not
  comparable because catalog coverage and semantic hash changed. These hosted
  observations establish no confirmed improvement or regression and make no DSP
  or silicon timing claim.
- Immediate `RPT` layouts, effects, and repeat policy are proved but remain inert;
  unresolved RC/PC/IRQ details are not guessed.
- The standalone DSP foundation includes checked 40-bit arithmetic and addressing,
  explicit WORD16 requests and BIG/LITTLE conversion, and a callback-free resumable
  transaction reducer. External memory visibility, arbitration, and timing remain
  demand-driven work.
- Speculative foundation expansion is frozen. The active track is end-to-end ISA
  vertical slices, adding infrastructure only when a concrete opcode family needs
  it. Emulator integration remains deferred.
- `ti_c54x_state` may grow observation fields before API stability; these fields
  are not a raw snapshot or persistence ABI, and no stable persistence guarantee
  is made.
- Hosted performance history is implemented and covered by the full local and
  remote gates:
  observer-off public-bus throughput, stable `nop`/`ld-dp`/`ld-asm`/`ld-arp`
  scenarios, catalog-aware mixed workload, seven samples each preceded by 16
  unrecorded warmup windows, and clean-commit append-only JSONL. The initial
  clean baseline at `aa0afc0` records 15.62 MIPS for NOP and 11.39 MIPS for
  `mixed-current`. The catalog-six `a14d331` record retains 15.26 MIPS for NOP;
  the catalog-seven `7cad341`, catalog-nine `705ac7b`, and catalog-ten `cd88596`
  records remain as prior cohorts. An identical-binary diagnostic rerun (hash
  `144ffa...`, dirty only because history was appended) normalized to NOP 66.571
  ns / 15.022 MIPS, LD-DP 98.326 / 10.170, LD-ASM 98.038 / 10.200, LD-ARP
  96.481 / 10.365, and mixed 86.448 / 11.568. This corroborates host noise
  only, not a history baseline or performance claim. See [hosted performance
  history](docs/performance.md).

See the [implementation plan](docs/implementation-plan.md) and
[development ledger](docs/development-ledger.md) for detailed status and evidence.

## Build and test

The C-only compile/link path requires only a C11 compiler and `make`:
`make build` does not invoke CMake, Python, the emulator, firmware, or a DSP
image. The default `make test` gate additionally runs the repository's Python
3 standard-library quality checkers and therefore requires a `python3`-compatible
interpreter (override it with `PYTHON=/path/to/python`). CMake 3.21 or newer and
CTest are used for packaging, IDE/CI validation, and the same Python quality
checks when tests are enabled.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The equivalent fast path is:

```sh
make test
```

For the compiler-only path, use:

```sh
make build
```

Useful options are:

```text
TI_C54X_BUILD_TESTS=ON             standalone unit tests (default)
TI_C54X_BUILD_EXAMPLES=OFF        minimal consumer example (default while API settles)
TI_C54X_ENABLE_SANITIZERS=OFF     AddressSanitizer + UBSan where supported
TI_C54X_ENABLE_OBSERVERS=OFF      structured lifecycle observers (test/diagnostic builds)
TI_C54X_WARNINGS_AS_ERRORS=ON     strict warnings (default)
TI_C54X_BUILD_BENCHMARKS=OFF      hosted observer-off performance tooling (no architectural effect)
PYTHON=python3                     quality-check interpreter used by `make test`
```

For example:

```sh
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DTI_C54X_ENABLE_SANITIZERS=ON
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

Observer callbacks receive borrowed, structured event values only. The core
copies the event mask and sink descriptor during initialization; the sink
context and callback must remain valid until deinitialization. Callbacks must
not re-enter or mutate the running CPU. Observer support is disabled in the
default library build and is exercised by a separate enabled test variant.

The install tree exports the CMake package target `ti_c54x::core`. A consumer
should link that target and include only the installed public headers; internal
implementation headers are not part of the vendor surface.

The current implementation lineage and evidence boundary are pinned in
[`docs/provenance.md`](docs/provenance.md); the distribution-license decision
remains open pending the documented provenance review.

## Design boundaries

The core owns architectural state and instruction semantics. A caller owns memory,
I/O, device timing, logging, and persistence through explicit interfaces. Runtime
state is instance-owned: there are no environment reads, file loads, mutable
process-wide emulator hooks, or hidden firmware paths in the execution path.

Requirements are recorded with evidence identifiers and are accompanied by
executable tests. Pure arithmetic and address helpers are tested before opcode
families; decode coverage, sequencing, interrupt behavior, and timing are added as
their specifications become pinned. The dispatcher follows the mGBA pattern of a
declarative instruction definition expanded into generated wrappers and dispatch
tables, while keeping semantic implementations readable and independently
testable. The [architecture baseline](docs/architecture-baseline.md) describes
the current ownership contracts, while the [evidence index](docs/evidence-index.md)
maps requirements to their sources and tests. `config/evidence.json` is the
machine-checked counterpart; it is validated without emulator or firmware gates.

## External inputs

Tests that need a firmware or DSP image will accept an explicit path and skip when
one is not supplied. Images are never committed, downloaded, or embedded by the
build. This keeps a clean checkout fast and reproducible.
