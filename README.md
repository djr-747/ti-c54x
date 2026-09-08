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
opcode inventory, and distribution license are not final. Provenance and licensing
review remain open; this repository makes no license-selection or compatibility
claim. See the [provenance review](docs/provenance.md), [architecture baseline](docs/architecture-baseline.md),
and [evidence index](docs/evidence-index.md). The machine-readable
[`config/evidence.json`](config/evidence.json) registry is checked by the standalone
Python quality gate.

Current snapshot (P2-W49B complete):

- 102 runnable catalog rows cover 2,967 first-word encodings. Inventory coverage
  is 42/205 instruction forms touched (20.488%) and 27/205 strict-complete
  (13.171%); lookup encodings are not instruction counts.
- Register-only ADD/SUB syntax 9 now executes through the generic trusted path,
  including signed full-40-bit shifts, SXM fill, bit-32 carry/no-borrow, OVM
  saturation, sticky OVA/OVB, alias-safe reads, and atomic CPU retirement.
- Memory execution covers standard, indexed, circular, and reverse-carry WORD16
  addressing; same-address logical RMW; and DLD/DST LONG32 MOD13 with READY,
  WAIT, fault, cancel, and stale-work handling.
- Performance remains a release constraint. Execution uses immutable direct
  lookup, one-pass trusted packets, allocation-free opcode leaves, and dedicated
  observer-off CPU benchmarks. These are host-performance checks, not TI
  machine-cycle claims.
- RC progression and pending IFR deferral are modeled transactionally, but runtime
  RPT activation still waits on positive repeatability evidence, repeated-body PC
  flow, and predecode interrupt arbitration. No catalog row is currently proven
  repeat-eligible.
- Major remaining work is broader arithmetic coverage, multi-memory and parallel
  transactions, control/repeat/IRQ execution, profiles/devices, and evidenced
  machine timing. Emulator integration remains deferred.

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
families; decode coverage, sequencing, interrupt behavior, and timing are added
as their specifications become pinned. The dispatcher follows the mGBA pattern
of a declarative instruction definition expanded into generated wrappers and
dispatch tables, while keeping semantic implementations readable and
independently testable. The [architecture baseline](docs/architecture-baseline.md)
describes the current ownership contracts, while the [evidence index](docs/evidence-index.md)
maps requirements to their sources and tests. `config/evidence.json` is the
machine-checked counterpart; it is validated without emulator or firmware gates.

## External inputs

Tests that need a firmware or DSP image will accept an explicit path and skip when
one is not supplied. Images are never committed, downloaded, or embedded by the
build. This keeps a clean checkout fast and reproducible.
