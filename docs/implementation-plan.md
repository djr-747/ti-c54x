# ti-c54x implementation plan

Status: foundations in progress.

## Objective

Create a small, high-quality, vendor-neutral C11 implementation of the TI C54x DSP
architecture. The library must build and test independently, model DSP-specific
behavior directly, and use mGBA-style macro-generated instruction wrappers and
function-pointer dispatch tables.

## Current work pieces

| Work piece | State | Exit condition |
|---|---|---|
| Repository charter and isolation | Complete | Standalone rules and dependency boundary documented |
| Provenance and licensing ledger | Active | Source lineage, evidence rules, and unresolved license decision recorded |
| CPU and bus API shell | Active | Two isolated instances fetch through caller buses and trap unimplemented words |
| 40-bit arithmetic foundation | Active | C-safe representation and shift/sign helpers pass boundary and exhaustive tests |
| Standalone build and CI | Active | CMake/CTest, warnings, sanitizers, and consumer smoke test pass locally |
| Address generation | Blocked on foundations | All address effects can be tested without opcode execution |
| Opcode builder and decode | Blocked on foundations | Every first word classifies uniquely; semantics may trap |
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
