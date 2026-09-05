# C54x common-core architecture baseline

Status: active baseline. Primary target: `C54X_CORE_COMMON_V1`.

The machine-readable source of truth is
[`config/evidence.json`](../config/evidence.json). This page states the boundary
that implementation work must preserve; the index gives an ID-by-ID review view.

## Targets and address boundary

`C54X_CORE_COMMON_V1` (`COMMON`, `ACTIVE`) is a device-neutral 16-bit C54x CPU
target. `C54X_VC5402_PROFILE_V1` (`PROFILE`, `PLANNED`) is a later executable
target whose parent is the common target. Only real executable architecture
targets appear in the registry; project governance records use the common target
with `claim_kind: PROJECT_CONTRACT`. The common target allows exactly
`ARCHITECTURAL` and `PROJECT_CONTRACT` claims; the profile target allows exactly
`PROFILE` claims. `PROJECT_CONTRACT` binds implementation and governance records
to the real common execution target, while mandatory `PROJECT_CONTRACT_ONLY`
evidence and test scopes prevent those records from being treated as TI
conformance. No fake project machine target is created.

The public API currently carries a program address in a 32-bit
`ti_c54x_program_word_address`. This avoids imposing a 16-bit storage limit while
the common address model is being established. It is only a carrier and ownership
decision: it does not implement or promise XPC, extended program pages, or a
device memory map. The scalar program/data/I/O word-address typedefs are owned by
`include/ti_c54x/address.h`; this module owns carriers only, not mapping, memory,
XPC semantics, timing, or CPU state.

## Common target scope

The common target includes CPU instruction/register foundations, program-word and
data-word concepts, documented reset effects, and instruction sequencing only when
each behavior has pinned evidence and a test. It excludes XPC and extended pages,
device ROM/RAM maps and wait states, exact IFR/IMR sources and vectors,
peripherals, PLL/clock and electrical behavior, silicon errata, and machine-cycle
or pipeline timing promises. The VC5402 and enhanced-peripheral sources are
profile-only and cannot widen this target implicitly.

### Exact machine exclusions

These project-owned strings intentionally duplicate `config/evidence.json` so the
scope checker can verify documentation parity:

- XPC and any extended program-page or extended-address semantics
- Device-specific ROM, RAM, memory maps, wait states, and boot maps
- Exact IFR/IMR interrupt sources, vector tables, and device interrupt wiring
- Peripherals, PLL and clock setup, electrical behavior, and silicon errata
- Machine-cycle or pipeline timing guarantees until timing evidence and tests are added
- VC5402-specific behavior, which belongs to the separate profile target
- No VC5402 behavior is implied by the common target before this profile is implemented and tested

The common target's program-address carrier has `semantic_scope: CARRIER_ONLY`,
32 bits, and unit `program words`. Its registry claim is exact: “The current
public API uses a 32-bit program-word carrier so the common interface does not
impose a 16-bit storage limit. The carrier is an implementation boundary only; it
does not implement or promise XPC, extended pages, or a device address map.”

## Initial architectural facts

The `SPRU131G` §6.9 reset evidence gives this common subset:

`PC=FF80h`, `IPTR=1FFh`, `IFR=0`, `RC=0`, `ARP=0`, `ASM=0`, `AVIS=0`,
`BRAF=0`, `C16=0`, `C=1`, `CLKOFF=0`, `CMPT=0`, `CPL=0`, `DP=0`, `DROM=0`,
`FRCT=0`, `HM=0`, `INTM=1`, `OVA=0`, `OVB=0`, `OVM=0`, `OVLY=0`, `SXM=1`,
`TC=1`, and `XF=1`. XPC is mentioned only where applicable and remains outside
the common target. SP and remaining status bits are not initialized by the reset
source, so no value is asserted for them (`UNRESOLVED`). Pin, bus, and peripheral
reset behavior is also outside this requirement.

The `SPRU172C` NOP entry identifies word `0xF495`, a one-word operation with no
status-bit effects. Ownership is split explicitly: `src/isa/decode/` recognizes
the word and width, `src/isa/ops/` produces no opcode-owned architectural/status
effects, and `src/sequencer/` owns the sole PC increment. `SPRU131G` §§6.1–6.2
provide the sequential-flow context: the PC is placed on the program address bus
and incremented for the next fetch. Cycle information is deliberately not a
common-core timing promise. The test tree mirrors that ownership split with
separate planned tests for decode, opcode semantics, and sequential flow; no
cross-owner unit test is used as a substitute for those seams.

The corrected direct-addressing evidence in `SPRU131G` §5.4 records a 7-bit
`dma`. For `CPL=0`, the implemented common requirement is the complete
valid-domain result formed by concatenating the 9-bit `DP` field and `dma` into a
16-bit data-memory word address. A separate planned requirement and test own the
source statement that direct addressing leaves `DP` and `SP` unchanged; that
register-effect proof is deliberately not part of the implemented formation
claim. The source does not specify AR effects. For `CPL=1`, the source describes
`SP` plus zero-extended `dma`, but this baseline does not choose overflow or wrap
behavior at the SP boundary (`UNRESOLVED`).

## Evidence and conformance rules

TI sources are identified by literature number, revision, ISO publication/revision
dates, official URL, scope, and access date. Evidence records are immutable and
carry a narrow locator plus an independently phrased assertion. A correction gets
a new source/evidence ID; Git review preserves the history, but the registry does
not claim to detect historical edits by itself. No manual prose, figures, opcode
tables, implementation code, or data are copied here.

Requirement state and test state are separate: requirements are `PLANNED`,
`IMPLEMENTED`, or `UNRESOLVED`; tests are `PLANNED` or `PASSING`. `PASSING` means
the path is under `tests/`, has a declared runner registered in Make and CTest (or
Python unittest discovery), and the current repository gate executes it. The
commit/CI ledger is run evidence, not a mutable timestamp stored in this registry.
An implemented requirement must link to at least one existing passing test. The
current CPU shell, 40-bit helpers, observer noninterference, and registry
governance rows are `PROJECT_CONTRACT` records, not TI-conformance claims.

Retired identifiers are never silently reused. The corrected direct-addressing
evidence supersedes `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9`; its tombstone records the
ABI-major lifetime and replacement ID.
