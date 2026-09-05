# ti-c54x implementation plan

Status: foundations in progress.

Execution history is recorded in `docs/development-ledger.md`. This plan describes
current intent; the ledger records completed work and decisions without rewriting
history.

## Objective

Create a small, high-quality, vendor-neutral C11 implementation of the TI C54x DSP
architecture. The library must build and test independently, model DSP-specific
behavior directly, and use mGBA-style macro-generated instruction wrappers and
function-pointer dispatch tables.

## Current work pieces

| Work piece | State | Exit condition |
|---|---|---|
| Repository charter and isolation | Complete | Standalone rules and dependency boundary documented |
| Legacy lineage audit | Complete | Exact imported interpreter base and Osmocom relationship recorded |
| Distribution license decision | Blocked on owner/legal review | New repository license and permitted evidence inputs selected |
| CPU and bus API shell | Complete | Two isolated instances fetch through caller buses and trap unimplemented words |
| 40-bit arithmetic foundation | Complete | C-safe representation and shift/sign helpers pass boundary and exhaustive tests |
| Standalone build and CI | Complete | CMake/CTest, warnings, sanitizers, installation, and consumer smoke tests pass in standalone CI |
| Diagnostic isolation | Complete | Optional structured observers compile out and remain outside opcode semantics |
| Source-boundary audit | Complete | Forbidden hot-path dependencies and unregistered knobs are structurally rejected |
| Knob ownership registry | Complete | Every non-architectural switch has class, disposition, owner, tests, and exit criterion |
| Common-core target and evidence registry | Complete | Device-neutral scope, official sources, requirements, owners, and test links are machine checked |
| Architectural register-state foundation | Complete | Private fixed-layout state seeds the documented common reset subset while every unspecified value remains explicitly unknown |
| Address generation | Active | All address effects can be tested without opcode execution |
| Transactional address-effect application | Complete | A private sequencer seam validates a complete result and applies at most one phase-matched AR update with no partial mutation |
| Standard-indirect mode-state policy | Active | CMPT=0 effects carry and enforce the ARP=0 invariant through owned state access without implicit repair or generic-mode coupling |
| Opcode builder and decode | Blocked on address/evidence foundations | Every first word classifies uniquely; semantics may trap |
| Emulator integration | Deferred | Provenance resolved and standalone API declared stable |

## Delivery order

1. Resolve the repository boundary and evidence policy.
2. Establish the opaque lifecycle, bus contract, result/error model, and tests.
3. Prove fixed-width 40-bit numeric primitives without undefined C behavior.
4. Prove address generation and ordered parallel memory effects.
5. Add the declarative opcode inventory and mGBA-style generated dispatcher.
6. Add the central sequencer before implementing opcode families.
7. Implement opcode families as requirement-backed vertical slices.
8. Add timing and silicon devices only from pinned evidence.
9. Vendor a release into emulators through separate adapters.

The first architecture target is a device-neutral common C54x core. A concrete
silicon profile is added separately after common CPU semantics are stable; no
device memory map, peripheral, vector table, XPC extension, or timing promise may
leak into the common target without profile-specific evidence and tests.

## Work-piece identifiers and ledger

Every implementation work piece receives a stable identifier formed from its phase
and sequence, for example `P1-W1`. The identifier appears in the implementation
commit message and its subsequent entry in `docs/development-ledger.md`.

The plan status table is updated when the intended state changes. The ledger is
append-only evidence: it records the actual commit, affected ownership modules,
tests, decisions, registered-knob changes, and remaining work. If an old entry is
wrong, add a correction that names it rather than editing history.

## Isolation gate

The repository must be clonable and testable without an emulator checkout. Firmware
and DSP images are optional external test inputs and are never compiled into the
library. Tests that use them must skip cleanly when no image path is supplied.

No emulator build file or source file will be changed during standalone development.

## Ownership and module-shape gate

Ownership must be visible in both the architecture and the files:

- Every handwritten source and header begins with a short module contract stating
  what the module owns and what it explicitly does not own.
- Public declarations document whether inputs are copied, borrowed, or caller-owned,
  including the lifetime required for callback contexts, memory windows, trace
  sinks, CPU storage, and serialization buffers.
- The CPU owns architectural and sequencer state. The caller owns CPU storage, bus
  context, mapped memory, trace context, and external image data unless a specific
  API says otherwise.
- `cpu.c` owns instruction-lifecycle orchestration only. It must not accumulate ALU,
  address-generation, opcode-family, device, serialization, or platform behavior.
- Arithmetic, status, addressing, bus mapping, decode, sequencing, on-chip devices,
  tracing, serialization, and each opcode family remain independently testable
  modules with narrow internal interfaces.
- Handwritten files are reviewed for splitting as they approach 500 lines. Exceeding
  800 lines requires an explicit recorded justification. Generated opcode/table
  output is exempt but kept in a dedicated generated directory.
- No module reaches through another module's private state. Cross-module mutation
  occurs through named internal interfaces or explicit execution-effect objects.

Exit condition: the architecture review can assign every mutable field, callback,
and side effect to exactly one owning module, and the source tree contains no general
“miscellaneous” or growing all-purpose execution file.

### Required ownership hierarchy

The source and test trees mirror each other:

```text
include/ti_c54x/             stable public contracts
  address.h                  scalar program/data/I/O word-address carriers
  cpu.h
  bus.h
  state.h
  trace.h

src/
  cpu/                       instance lifecycle and top-level orchestration
  foundation/                fixed-width arithmetic and status primitives
  bus/                       bus routing and mapped-window fast paths
  address/                   effective-address plans and AR side effects
  isa/
    decode/                  opcode metadata, operand decoding, disassembly
    ops/                     one directory/file group per semantic family
  sequencer/                 retire, PC/XPC, delay, repeat, XC, IRQ, IDLE
  devices/                   documented optional on-chip peripherals by device
  state/                     architectural serialization and state views
  debug/                     optional structured tracing and breakpoints

generated/                   reproducible opcode/table output only

tests/
  support/                   dependency-free test harness and shared generators
  unit/                      mirrors each src ownership directory
  contract/                  public API, ABI, and standalone-consumer tests
  differential/             normalized external-oracle transcripts
  fuzz/                      bounded decoder, arithmetic, and sequencer fuzzers
```

Rules:

- Public headers express stable contracts, never private layouts.
- A module's private header stays beside its implementation, not in a general
  `src/internal/` dumping ground.
- Tests for `src/<owner>/` live under `tests/unit/<owner>/`; cross-module behavior
  belongs under `tests/contract/`.
- Cross-module headers are introduced only for a named architectural seam and list
  both producer and consumer ownership.
- Generated declarations and dispatch tables are never hand-edited or mixed into an
  opcode semantic source file.

## Diagnostic isolation gate

Diagnostics are optional observers of architectural events, not opcode behavior.

- Production library code never calls `getenv`, `setenv`, or equivalent process
  configuration APIs. A hosted executable may read its environment once before
  `ti_c54x_cpu_init` and convert it to explicit configuration.
- Enabled observation classes are stored as one copied, per-instance `uint64_t`
  event mask in cold/debug state. No mutable process-global flag or per-feature
  boolean is read from the execution path. `TEST` and `A/B` controls never share this
  mask because they may select behavior rather than merely observe it.
- Hot call sites use one internal macro with short-circuit semantics, conceptually:

  ```text
  if (TI_C54X_OBSERVE_ENABLED(cpu, EVENT_BIT) && event_specific_condition) { ... }
  ```

  When observation is disabled at build time, `TI_C54X_OBSERVE_ENABLED` is a constant
  false expression and the compiler removes the site and event-specific condition.
- The macro and mask are owned by `src/debug/`. No opcode-family module defines its
  own gate, reads diagnostic configuration, formats text, or invokes host logging.
- Opcode handlers return architectural execution effects. The sequencer/debug seam
  may convert completed effects into compact structured events after retirement.
- The core never owns output files, streams, sockets, format strings, or logging
  policy. A caller-owned sink and context receive structured events only.
- Observer configuration is non-architectural cold state: it is not included in CPU
  snapshots, does not survive reinitialization unless the caller supplies it again,
  and cannot be changed from inside an observer callback.
- Observer callbacks receive immutable event values and no implicit CPU pointer.
  Re-entering or mutating a running CPU from a callback is forbidden and tested.
- A repository structural test scans `include/` and `src/` and rejects environment
  APIs, stdio/file APIs, mutable diagnostic globals, platform names, or diagnostics
  included directly by opcode semantic modules.

Exit condition: diagnostic-off builds contain no diagnostic branches or callback
symbols in the instruction hot path, diagnostic-on builds read a single instance
mask, and the structural boundary test passes.

## Knob ownership and lifecycle gate

Non-architectural switches are exceptional and centrally registered in
`docs/knobs.md`. A knob includes runtime configuration, a build option, a test hook,
an observation bit, or an implementation-selection switch. It may not be introduced
as an anonymous environment variable, preprocessor branch, static boolean, or magic
configuration field.

Each knob has two independent labels:

- **Class `TEST`:** builds or enables tests, examples, sanitizers, validation, or
  controlled fault/input injection. Behavioral test hooks are unavailable in normal
  production builds and cannot falsify an architectural result.
- **Class `OBSERVE`:** records structured facts without changing architectural
  state, memory effects, timing, interrupts, or scheduling.
- **Class `A/B`:** selects two explicit implementations or models for comparison.
  Both paths share the same contract and tests; an A/B knob cannot hide unresolved
  ISA semantics in a stable release.

And one lifecycle disposition:

- **`KEEP`:** supported configuration with a stable owner and compatibility policy.
- **`TEMP`:** bounded experiment or migration aid with a named exit criterion. It
  cannot enter the stable public API.
- **`RETIRE`:** compatibility input awaiting removal; new call sites are forbidden,
  and the registry states the removal release/condition and replacement.

Lifecycle transitions are explicit: `TEMP` must become `KEEP` or `RETIRE`; `KEEP`
may become `RETIRE`; `RETIRE` is removed after its recorded compatibility condition.
Removal preserves the retired identifier/bit in a tombstone list for the current ABI
major so persisted host configuration cannot silently acquire a different meaning.

Every registry entry records identifier, class, disposition, owning module, default,
compile/runtime scope, architectural effect, test coverage, and exit/removal
criterion. The source-boundary audit rejects knobs absent from the registry and
rejects `TEST`/`TEMP` knobs in installed public headers.

The long-term source of truth is a small declarative registry under `config/`, from
which enum identifiers, allowed compile definitions, documentation checks, and the
retired-ID tombstone list are generated or verified. The Markdown registry remains
the human review surface. Direct `#ifdef` feature names and ad-hoc configuration
fields that are absent from that registry fail CI.

The 64-bit hot mask is reserved for `OBSERVE` event classes only. Its bit allocation
is stable within an ABI major, statically checked for uniqueness/exhaustion, copied
once during CPU initialization, and read through the single compile-time-aware
macro. `TEST` controls live only in test-build configuration; `A/B` controls use
typed cold configuration with an explicit comparison owner and never appear as an
anonymous hot-path bit.

Exit condition: every switch is discoverable from the registry, owned by exactly one
module, tested according to class, and has an explicit keep or removal decision.
