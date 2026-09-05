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

## Single-indirect address-effect slice

The first indirect-addressing slice is deliberately a private, checked planner
under `src/address/`. It accepts a copied ARF-selected index, a 16-bit current
auxiliary-register value, one of `MOD0` through `MOD3`, and a read/write access
kind. It returns a value-owned data-word effective address plus at most one
explicit AR update with a `NONE`, `PRE_ACCESS`, or `POST_ACCESS` phase. It does
not own CPU state, ARP or CMPT validation, bus transfers, opcode decoding, long
operands, circular or bit-reversed addressing, dual-indirect forms, or execution
scheduling.

The evidenced word effects are: MOD0 uses the old AR value with no update; MOD1
uses the old value and reports a one-word post-access decrement; MOD2 uses the
old value and reports a one-word post-access increment; and MOD3 reports a
one-word pre-access increment, uses the new value, and accepts writes only. The
returned effect carries the selected AR index so the ARF selection is testable
without pretending that this planner owns architectural register storage. The
CMPT=0 ARP=0 invariant and absence of ARP writeback remain planned state and
sequencer work. Long-operand deltas, address-domain edge behavior, and the
circular, bit-reversed, and dual-indirect forms remain explicitly planned or
unresolved; this slice does not infer them.

The project contract is separate from TI claims: checked invalid inputs,
unsupported MOD3 reads, and unresolved boundaries return explicit statuses and
an all-zero effect. The project contract does not elevate those API details into
architectural evidence.

## Private architectural register-state seed

`src/state/` owns a private fixed-width register representation used by later
address and sequencer seams. `src/state/register_layout.h` owns the raw ST0,
ST1, and PMST masks; it deliberately uses no C bitfields. ST0 is represented as
ARP[15:13], TC[12], C[11], OVA[10], OVB[9], and DP[8:0]. ST1 is represented as
BRAF[15], CPL[14], XF[13], HM[12], INTM[11], reserved bit 10, OVM[9], SXM[8],
C16[7], FRCT[6], CMPT[5], and ASM[4:0]. PMST is represented as IPTR[15:7],
MP/MC[6], OVLY[5], AVIS[4], DROM[3], CLKOFF[2], SMUL[1], and SST[0]. The
corresponding layout evidence is pinned as
`E-TI-C54X-SPRU131G-S4.1.1-P4-2-4-5` and
`E-TI-C54X-SPRU131G-S4.1.2-P4-6-4-8`.

`ti_c54x_arch_state_reset_common()` seeds only the documented common reset
values: 16-bit PC `FF80h`, raw 16-bit IFR `0`, 16-bit RC `0`, ST0 `1800h`, ST1
`2900h`, and PMST `FF80h` with known masks `FFFFh`, `FBFFh`, and `FFBCh`.
`E-TI-C54X-SPRU131G-S6.2-T6-2-P6-4`,
`E-TI-C54X-SPRU131G-S6.7-P6-20`, and
`E-TI-C54X-SPRU131G-S6.10.1-P6-27-6-28` pin the register widths and reset
actions; the §6.9 record remains the authority for the complete listed reset
subset. The private state keeps separate knownness masks, so zero-filled
AR0–AR7 and SP storage is not treated as an architectural value. Their reset
values are not asserted; callers establish them through checked writes.

The public `ti_c54x_config.reset_pc_word` remains a project bootstrap setting
owned by the CPU shell. It is not the TI hardware-reset value and is intentionally
not migrated or embedded by this work piece. A future integration must preserve
these two policies as separate contracts rather than making one field represent
both a caller-selected start address and the documented `FF80h` reset vector.

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
