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
