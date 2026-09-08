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

Current snapshot (P2-W49A complete):

- 100 runnable catalog rows and 2,711 populated first-word lookup entries. The
  inventory has touched 40/205 instruction families (19.512%); 25/205 forms are
  strict-complete (12.195%). These are separate counts: lookup entries are not
  instruction counts.
- Standalone memory/addressing seams are proven through standard MOD0-3,
  indexed MOD5/6, circular MOD8/10, reverse-carry MOD4/7, and same-address
  WORD16 RMW modes. W46B is complete: `ANDM`, `ORM`, and `XORM` retire through
  the generic decode/effect/CPU/inventory path; MOD3 remains a deliberate
  pre-endpoint rejection.
- Performance is a first-class constraint. The trusted per-ID execution packet,
  single extraction pass, and hosted benchmark history are retained. The dedicated
  observer-off RMW benchmark has a clean baseline and baseline-relative comparison
  at [`bench/audits/p2-w46b-rmw-baseline.json`](bench/audits/p2-w46b-rmw-baseline.json);
  its semantic hash is `3b39187c40a4fd43` and it makes no TI timing claim.
- The fixed-size LONG32 memory path is green through MSW→LSW pairing, CMPT=0
  MOD13 resolution, and the root WAIT/fault/cancel/stale lifecycle. AR/PC commit
  waits for both words; the existing WORD16 path avoids the LONG32 cold helper.
  Typed PMST.SST access and copied input knownness are green without widening
  the 48-byte opcode-input snapshot. DLD/DST have exact MOD13 catalog rows,
  single-pass trusted operands, full CPU READY/WAIT/FAULT coverage, SXM load
  semantics, SXM/SST store saturation, and a dedicated observer-off full-CPU
  benchmark with semantic self-checks and no machine-dependent threshold.
- Repeat sequencing now has typed RC access and an allocation-free transactional
  progress reducer proving the full observable `K...0` sequence, terminal RC zero,
  stale/unknown rejection, and all 65,536 bodies for `K=0xffff`. Typed IFR access
  and a fixed-size pending-request reducer now prove that a post-decode maskable
  request is latched while repeat defers it, then cleared on explicit normal take;
  reset, IFR-write-one, and `INTR` clears are separate modeled paths. These seams
  add no branch to the current CPU path. Repeated-body PC flow and predecode
  pending-request arbitration still require evidence before inert RPT policy can
  become runtime behavior. A full repeatability audit also corrected DLD from an
  unsupported `INELIGIBLE` classification to `UNRESOLVED`; no current catalog row
  is positively evidenced `ELIGIBLE`.
- The shared ADD/SUB foundation now produces modulo-40 results, bit-32
  carry/no-borrow, and signed-32 overflow direction. Typed OVM/OVA/OVB access,
  copied OVM input, and atomic sticky destination-overflow effects are green;
  LP64/ILP32 carrier sizes are frozen. Register-only ADD/SUB activation is next.
- Remaining high-level gaps are broader arithmetic-family activation, multi-memory
  and parallel transactions, control/repeat/IRQ sequencing, reset/profile/device
  behavior, and machine timing. `RPT` layouts remain inert while RC/PC/IRQ
  details are unresolved; emulator integration remains deferred.

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
