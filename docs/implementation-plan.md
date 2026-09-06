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
| Ownership-mirrored build manifests | Complete | Root Make/CMake files retain policy and orchestration only; source and unit-test registrations live with their owning module, with exactness, parity, reachability, and nested-owner checks |
| Diagnostic isolation | Complete | Optional structured observers compile out and remain outside opcode semantics |
| Source-boundary audit | Complete | Forbidden hot-path dependencies and unregistered knobs are structurally rejected |
| Knob ownership registry | Complete | Every non-architectural switch has class, disposition, owner, tests, and exit criterion |
| Common-core target and evidence registry | Complete | Device-neutral scope, official sources, requirements, owners, and test links are machine checked |
| Architectural register-state foundation | Complete | Private fixed-layout state seeds the documented common reset subset while every unspecified value remains explicitly unknown |
| Address generation | Active | All address effects can be tested without opcode execution |
| Transactional address-effect application | Complete | A private sequencer seam validates a complete result and applies at most one phase-matched AR update with no partial mutation |
| Standard-indirect mode-state policy | Complete | CMPT=0 effects carry and enforce the ARP=0 invariant through owned state access without implicit repair or generic-mode coupling |
| 32-bit single-indirect operand shape | Complete | Long32 MOD0–MOD3 effects carry explicit MSW/LSW addresses, evidenced two-word AR deltas, and pass transactional application without bus or opcode coupling |
| Direct-address source snapshots | Complete | WORD16 CPL=0 DP and non-overflow CPL=1 SP effects carry truthful source snapshots, reject stale state transactionally, and prove DP/SP nonmutation without AR coupling |
| AR0-indexed single-indirect effects | Complete | WORD16 CMPT=0 MOD5/MOD6 effects snapshot distinct ARx and AR0 sources, apply one safe post-update transactionally, and leave wrap and alias behavior unresolved |
| Signed long-offset premodify | Complete | WORD16/LONG32 CMPT=0 MOD13 effects copy the raw signed offset word, form and apply one checked PRE_ACCESS selected-AR update, and leave MOD12 signedness and wrap unresolved |
| Reverse-carry arithmetic and effects | Complete | WORD16 CMPT=0 MOD4/MOD7 effects prove nonescaping MSB-to-LSB AR0 updates while final propagation, selected-AR0 alias ordering, and LONG32 remain unresolved |
| BK architectural state prerequisite | Complete | Private 16-bit BK storage and checked knownness-aware access prove the circular-buffer size dependency without claiming reset, MMR routing, or circular arithmetic |
| Sequencer address-effect decomposition | Complete | Pure effect validation and ordered dependency reads move behind a named sequencer seam before circular effects extend the transaction model |
| Sequencer validation subownership | Complete | Intrinsic effect checks and ordered architectural dependency reads have separate private owners while the validator only orchestrates |
| Circular MOD8/MOD10 WORD16 effects | Complete | Evidence-pinned fixed-step circular arithmetic, BK snapshots, and either one post-access AR update or the BK=0 no-op pass planner and transactional tests |
| Shared-memory transaction semantics | Planned before first memory-mutating opcode | Word-native requests, explicit byte-lane adapters, deterministic multi-master arbitration, resumable waits, and exactly-once visibility are proven before load/store effects |
| Opcode builder and decode | Complete | Exact-row NOP catalog, direct 16-bit first-word decoder, semantic dispatch seam, and checked sequential-PC contract are tested; unknown words remain explicit, while masked encodings and CPU-shell integration remain outside this slice |
| Instruction timing and resource model | Planned after opcode inventory | Generated opcode metadata and the sequencer account for evidenced CPU cost while device profiles own memory/peripheral waits |
| Emulator integration | Deferred | Provenance resolved and standalone API declared stable |

### P2-W16: NOP-seeded opcode architecture

This completed work piece establishes the smallest evidence-backed ISA architecture slice
without integrating it into the public CPU shell. The ownership boundary is
deliberate:

- `src/isa/catalog/` owns one declarative X-macro catalog, and that catalog is
  the only instruction inventory. No second handwritten opcode list or copied
  legacy table is permitted.
- Macro passes generate opcode identity and metadata, a direct 16-bit first-word
  lookup, and a function-pointer semantic dispatch table. The generated views
  remain derived from the one catalog.
- The first registered row is NOP `0xF495` with width one program word. Words
  absent from the catalog classify explicitly as unknown; they do not alias NOP
  or silently select a default handler.
- NOP emits an explicit no-opcode-owned-effect result. Opcode semantics do not
  mutate the program counter, status, memory, buses, devices, or observers.
- `src/sequencer/` alone plans checked private 16-bit sequential PC `+1`. A
  source PC of `0xffff` returns an explicit unresolved-boundary result and
  leaves the input and output state unchanged.
- The public CPU shell is not integrated in this slice: its current carrier is
  32-bit, while high-carrier and XPC policy remain unresolved. The private
  16-bit seam is therefore tested independently of `include/ti_c54x/cpu.h`.
- This slice makes no machine-cycle or pipeline timing claim and adds no memory,
  bus, device, or observer behavior to opcode operations. It does not copy code,
  tables, comments, or tests from legacy emulators or other external projects.

The TDD order is catalog contract and decoder classification first, explicit NOP
semantic no-effect second, and checked sequential-PC planning third. Each owner
has its own test path; cross-owner integration is deferred until the central
sequencer and public CPU-shell contracts are ready.

## Delivery order

1. Resolve the repository boundary and evidence policy.
2. Establish the opaque lifecycle, bus contract, result/error model, and tests.
3. Prove fixed-width 40-bit numeric primitives without undefined C behavior.
4. Prove address generation and ordered parallel memory effects.
5. Add the declarative opcode inventory and mGBA-style generated dispatcher.
6. Add the central sequencer before implementing opcode families.
7. Prove the shared-memory transaction gate before any opcode family can mutate
   memory.
8. Implement opcode families as requirement-backed vertical slices.
9. Add timing and silicon devices only from pinned evidence.
10. Vendor a release into emulators through separate adapters.

## Timing and execution-budget gate

Instruction count and machine-cycle cost are separate units and separate API
contracts. Early execution continues to use instruction budgets; a machine-cycle
budget API is not exposed until its common-core timing claims have pinned evidence
and executable tests.

- The eventual timed API uses explicit units. `ti_c54x_cpu_tick()` advances
  exactly one DSP machine cycle and never overshoots; a one-cycle instruction may
  start and retire in that call. `ti_c54x_cpu_run_cycles()` consumes no more than
  its machine-cycle budget, while `ti_c54x_cpu_step_instruction()` continues only
  until the next retirement or an explicit stop. Existing instruction-budget
  entry points are not silently redefined as cycle-budget entry points.
- A multi-cycle instruction is a private resumable sequencer transaction. Budget
  exhaustion leaves its decoded identity, phase, resource classification,
  prepared effects, and any pending bus transaction in the CPU instance; the
  next tick resumes that transaction instead of decoding or applying it again.
  Results distinguish cycles consumed, instructions started, instructions
  retired, in-flight progress, and the exact reason execution stopped.
- Architectural register, status, and PC effects are prepared by semantics and
  committed exactly once at retirement. Bus activity occurs only at an
  evidence-backed sequencer phase; a read result remains staged until retirement,
  while externally visible writes occur exactly once at the evidenced bus-
  completion point. Ending a budget mid-instruction must not expose a partial
  architectural commit or reissue a completed bus side effect.
- Opcode semantic handlers produce architectural effects. They do not increment a
  global cycle counter or embed ad hoc numeric costs.
- The generated opcode inventory provides a named timing/resource classification
  seam alongside decode metadata. Exact values remain absent until evidenced; an
  unknown classification is explicit and cannot silently default to one cycle.
- `src/sequencer/` owns common CPU pipeline, branch/delay, repeat, and resource-
  conflict accounting. Timing that depends on a sequence is tested as a sequence,
  not reduced to a misleading per-opcode scalar.
- `src/bus/` and `src/devices/` profile owners add documented memory, peripheral,
  and wait-state costs. Device costs never leak into the common target.
- A future timed result reports machine cycles with an explicitly named unit.
  Instruction-budget APIs remain available for deterministic semantic execution;
  neither budget is presented as the other.
- An instruction whose timing classification is unknown does not enter the timed
  transaction and never defaults to one cycle. The timed API returns an explicit
  unknown-timing stop, with no instruction side effect; an already in-flight
  transaction can only report states admitted by the contract under which it
  started.
- Per-instance cumulative instruction and machine-cycle counters are `uint64_t`
  and use checked increments. Exhaustion returns an explicit sticky counter-
  overflow stop before beginning another cycle; neither cumulative values nor
  per-call deltas silently wrap.
- A future timed bus profile must support an allocation-free begin/poll/complete
  transaction (or an equivalent explicit READY/WAIT protocol), because the
  current synchronous callback cannot represent wait cycles without overshoot.
  Retry, completion, and fault paths must prove that reads and writes are issued
  and committed at most once.
- Interrupt sampling is a named sequencer boundary but remains unresolved until
  primary interrupt/pipeline evidence pins it. The core must not assume sampling
  on every tick or only at retirement by convenience.
- ARM-to-DSP scheduling belongs solely to an emulator adapter. It uses a reduced
  rational ratio and integer phase accumulator to decide how many exact DSP
  cycles to request per ARM tick; the standalone core never receives an ARM tick,
  host-clock ratio, PLL model, or fractional cycle.
- Timing metadata is static/generated and allocation-free. Runtime accounting uses
  compact typed fields or resource flags passed through the sequencer, with no
  environment lookup, logging hook, string dispatch, or mutable process global in
  the execution path.
- Each base-cost, pipeline-conflict, repeat, branch, bus, and wait-state behavior
  needs a requirement, narrow primary evidence, and direct or sequence-level test.
  Performance benchmarks may measure implementation overhead but cannot substitute
  for architectural timing evidence.

Exit condition: every implemented instruction form has either an evidenced timing
classification or an explicit unknown marker; the sequencer and device profile
compose costs without double counting; instruction and machine-cycle budgets are
provably distinct; one-cycle budgets never overshoot and can resume an in-flight
instruction; retirement and bus effects occur exactly once; counters cannot wrap;
the external rational scheduler introduces no fractional core state; and no
handler-local magic cycle increment exists.

The first architecture target is a device-neutral common C54x core. A concrete
silicon profile is added separately after common CPU semantics are stable; no
device memory map, peripheral, vector table, XPC extension, or timing promise may
leak into the common target without profile-specific evidence and tests.

## Shared-memory transaction and byte-order gate

The CPU core is word-native and host-endian-neutral. It never owns, aliases, or
casts a caller's byte-backed RAM. The caller owns mapped storage and arbitration;
the core exchanges fixed-width architectural values through an explicit bus
transaction boundary.

- Every request identifies program, data, or I/O space; architectural word
  address; read or write direction; transfer width/composition; and its named
  sequencer phase. A C54x CPU instance does not contain an ARM identity, shared-
  RAM pointer, device map, or host mutex.
- One architectural 16-bit word is the primitive transfer. Multiword values are
  composed from separately named word transfers in evidenced architectural word
  order; they are never treated as an accidentally atomic host `uint32_t`.
- Byte-oriented adapters use explicit load/store conversion helpers. Program-
  image encoding, physical shared-RAM byte-lane order, and snapshot/wire format
  are three separate policies with separate owners and tests. None may inherit
  the compiler host's byte order through a pointer cast, packed struct, union
  pun, or native `memcpy` interpretation.
- The timed bus seam begins one transaction, polls an owned pending token through
  READY/WAIT/FAULT states, and completes it exactly once. Budget exhaustion,
  wait-state polling, or retry must not reissue an MMIO read, duplicate a write,
  or expose half of a multiword transfer.
- Opcode handlers declare value-owned memory effects; they never call memory
  directly. For parallel operations the complete read/write set is available to
  the sequencer before issue, so overlap, read-before-write, resource conflicts,
  and commit order are resolved once from evidence.
- Architectural register/status/PC changes remain staged around memory progress.
  A fault or unsupported transaction returns an explicit outcome under a tested
  rollback/partial-visibility contract; it never relies on incidental callback
  order.
- DSP CPU, ARM CPU, DMA, and device masters are scheduled by the external bus or
  emulator adapter. That owner supplies deterministic arbitration and visibility
  boundaries. Host thread timing is never emulated hardware ordering; shared
  backing storage must be serialized or synchronized by the adapter.
- The common test bus proves request shape, wait/resume, fault propagation,
  exactly-once issue/completion, word order, byte-lane conversion, overlapping
  parallel accesses, and two independently scheduled masters. Profile tests then
  pin device-specific maps, lanes, arbitration, and wait states.

Exit condition: no instruction capable of writing memory is admitted until its
handler emits a declarative memory effect, the sequencer and bus transaction tests
prove exactly-once ordered visibility across suspension, every byte boundary has
an explicit conversion policy, and the device-neutral core remains free of raw
shared-memory pointers and platform arbitration.

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
    catalog/                 sole declarative X-macro opcode inventory and generated views
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
- Root build files own toolchain policy, global options, aggregate targets,
  packaging, and inclusion order only. Source lists and unit-test registrations
  live in small fragments or `CMakeLists.txt` files under the same `src/<owner>/`
  and `tests/unit/<owner>/` directories as the code they register. Shared test-
  target construction may live in one `tests/cmake/` helper, but it must not
  become a second flat target inventory.
- Both direct Make and CMake consume that ownership hierarchy and must continue
  producing the same library, observer-disabled artifact, test executables, CTest
  names, install package, and consumer surface. A manifest split is structural;
  it does not justify changing compile options, target behavior, or test scope.

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
