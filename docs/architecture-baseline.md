# C54x common-core architecture baseline

Status: active baseline. Primary target: `C54X_CORE_COMMON_V1`.

The machine-readable source of truth is
[`config/evidence.json`](../config/evidence.json). This page states the boundary
that implementation work must preserve; the index gives an ID-by-ID review view.
The current registry contains 2 targets, 7 sources, 55 immutable evidence
records, 64 requirements, and 49 tests (43 `PASSING`, 6 `PLANNED`).

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
16-bit data-memory word address. For `CPL=1`, the implemented common requirement
covers the complete safe domain where `SP` plus zero-extended `dma` remains in
the 16-bit data-word range. Overflow and wrap behavior remain explicitly
unresolved at the SP boundary. A separate implemented requirement and test own
the source statement that direct addressing leaves `DP` and `SP` unchanged;
that register-effect proof is deliberately not part of either address-formation
claim. The source does not specify AR effects. The sequencer test also proves
that a direct effect selects the tagged DP source only under `CPL=0`, or the
tagged SP source only under `CPL=1`, while preserving the planned data-word
address; that source-selection contract is kept narrow and does not claim
direct long32 behavior.

## Single-indirect address-effect slice

The first indirect-addressing slice is deliberately a private, checked planner
under `src/address/`. It accepts a copied ARF-selected index, a 16-bit current
auxiliary-register value, one of `MOD0` through `MOD3`, a canonical word16 or
long32 data-operand descriptor, and a read/write access kind. It returns a
value-owned operand shape plus at most one explicit AR update with a `NONE`,
`PRE_ACCESS`, or `POST_ACCESS` phase. It does not own CPU state, ARP or CMPT
validation, bus transfers, opcode decoding, circular or bit-reversed
addressing, dual-indirect forms, or execution scheduling.

The evidenced word effects are: MOD0 uses the old AR value with no update; MOD1
uses the old value and reports a one-word post-access decrement; MOD2 uses the
old value and reports a one-word post-access increment; and MOD3 reports a
one-word pre-access increment, uses the new value, and accepts writes only. The
returned effect carries a tagged, value-owned AR source snapshot (selected AR
index plus value-before) so ARF selection is testable without pretending that
this planner owns architectural register storage. The
CMPT=0 ARP=0 invariant and absence of ARP writeback are now enforced at the
sequencer application seam: effects tagged `CMPT0_ARP0` require known CMPT=0 and
ARP=0 before the selected-AR update is applied, and ARP is never written back by
that seam. For the implemented long32 slice, MOD0 uses the first data-word
address with no update; MOD1 and MOD2 use that address with post-access
decrement/increment by two data words; and MOD3 is write-only, pre-increments by
two, and uses the updated first address. The long32 operand stores first/MSW and second/LSW
addresses explicitly: even first addresses pair with the next higher word and
odd first addresses with the previous lower word. The data-operand validator
owns canonical width and pair-shape checks; it does not select address wrap.
Circular, bit-reversed, and dual-indirect forms remain planned or unresolved.
The constructor/parity proof is owned by `src/address/data_operand.*`; the
sequencer validation proof is separate under `src/sequencer/` and anchored at
`ti_c54x_sequencer_apply_address_result`.

The project contract is separate from TI claims: checked invalid planner inputs,
unsupported MOD3 reads, and unresolved boundaries return explicit statuses and
all-zero effects. Data-operand constructors return value-owned descriptors, and
the validator returns false for zero or unknown widths and noncanonical
first/second-word relationships. Constructors retain no borrow; the validator
temporarily borrows a read-only descriptor for the call and neither path mutates
caller storage.
Endpoint cases that would overflow an unsigned 16-bit auxiliary address are
rejected under the project contract; this is not a claim that hardware wraps.
The project contract does not elevate those API details into architectural
evidence.

The sequencer application seam in `src/sequencer/` consumes that planner result
without redoing its arithmetic. It validates the result status, selected AR and
update shape, requested phase, and PRE/POST effective-address relation before
reading architectural state. A known selected AR must still equal the effect's
expected before value (or the no-update effective address); otherwise the call
returns a stale-state status. Unknown AR state is reported separately. A valid
AR update performs one checked write, while a valid NONE effect does not mutate
state. Every rejection preserves the complete private state byte-for-byte. This
is a project sequencing contract: it does not add ARP, bus, memory, timing,
opcode, or address-boundary behavior, and it deliberately does not require
`value_after` to differ from `value_before` by one or two.

### AR0-indexed WORD16 slice

The next narrow address slice models CMPT=0 single-indirect MOD5 and MOD6
effects. TI `SPRU131G` §5.5.3/Table 5-4 (printed p. 5-13) records the ordering:
MOD5 uses the old ARx for the operand and then forms ARx−AR0, while MOD6 uses
the old ARx and then forms ARx+AR0. Section 5.5.3.3 (printed p. 5-15) describes
AR0 as indexing another auxiliary register. Existing §5.5.1 and §5.5.5 records
continue to supply the CMPT=0 ARF selection and ARP=0 policy.

The planner admits selected AR1–AR7 only. It uses the old selected-AR value as
the WORD16 operand and schedules one POST_ACCESS update to that same selected
AR, copying the AR0 value as a second source without making AR0 writable by the
effect. MOD5 is admitted only where ARx≥AR0; MOD6 only where ARx+AR0≤FFFFh.
These are safe unsigned domains chosen by the project contract, not a claim of
hardware wrap or circular arithmetic. Selected-AR0 alias ordering is explicitly
unsupported pending stronger evidence. LONG32 indexed operands, MOD4/MOD7 and
other modes, offset/circular/bit-reversed forms, buses, timing, and opcode
integration remain outside this slice.

Together, the COMMON_TI and project-contract address tests cover both access
kinds, all selected AR1–AR7 values, complete single-source dimensions,
representative opposite-source vectors, and exact admitted/rejected boundary
classes. The COMMON_TI test stays inside the admitted safe domain; the project
contract owns checked rejection outside it. This is intentionally not a full
Cartesian claim over both 16-bit source dimensions. The sequencer tests
separately prove the tagged two-source shape, intrinsic validation and
precedence, generic unknown/stale status for either dependency, bytewise
transactional rejection, one selected-AR commit, and AR0 nonmutation.

### Signed long-offset MOD13 slice

The signed long-offset slice models CMPT=0 `*+ARx(lk)` MOD13 effects for both
WORD16 and LONG32 operands. `SPRU131G` §5.5.3/Table 5-4 (printed p. 5-14)
explicitly identifies `lk` as a signed 16-bit offset, adds it to ARx before
access, and uses the resulting value both as the operand address and the
replacement ARx value. Section 5.5.3.2 (printed pp. 5-14–5-15) supplies the
raw extension/update context; it is recorded without selecting instruction
fetch, repeat, or machine-cycle behavior. `SPRU172C` classes 9B and 13B
(printed pp. 3-21 and 3-29) cross-check long-offset indirect long-word read/write
operand forms. Existing CMPT=0/ARP=0 and LONG32 pairing evidence remains the
authority for those independent shape constraints.

The planner copies the raw offset word, performs explicit two's-complement
sign extension, and accepts only mathematical sums in the 16-bit data-word
domain. A successful effect carries a canonical WORD16 or LONG32 descriptor,
the copied selected-AR source, and exactly one PRE_ACCESS update to the
computed value. The project contract rejects invalid fields and unsafe
unsigned endpoints with explicit statuses and an all-zero result; hardware
wrap is not chosen. The sequencer validates this intrinsic shape before the
CMPT=0/ARP=0 and selected-AR state checks, then commits at most the described
selected-AR write without recomputing arithmetic. MOD12 remains an explicit
`UNRESOLVED` signedness requirement because its source text does not classify
the 16-bit carrier as signed or unsigned. MOD4/MOD7 reverse carry, circular
and bit-reversed modes, MOD15, extension fetch, opcode decode, buses, timing,
and emulator integration remain outside this slice.

The MOD13 address test sweeps both access kinds and widths across every
selected AR index and complete single-source dimensions, with independent
cross-source and exact signed-boundary vectors. The effect-application test
separately exercises the tagged source shape, LONG32 parity, mode/source
validation and precedence, unknown/stale selected-AR state, transactional
rejection, and one PRE_ACCESS commit. These are independent source-dimension
sweeps and representative combinations, not a full Cartesian claim over AR
and offset.

### Reverse-carry WORD16 slice

The reverse-carry foundation is isolated in `src/foundation/` and implements
the documented bit-propagation primitive without owning architectural state.
The Table 5-4 record `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD4-MOD7` is used
only for MOD4's old-AR then subtract-AR0 and MOD7's old-AR then add-AR0
ordering; it does not establish width scaling or access restrictions.
`E-TI-C54X-SPRU131G-S5.5.3.5-P5-18-5-19-REVERSE-CARRY` records AR0 as
2^(N-1), left-to-right (bit 15/MSB toward bit 0/LSB) carry propagation, and the
worked sequence. The planner admits WORD16 safe domains and carries value-owned
selected-AR and AR0 snapshots. The sequencer applies at most one post-access
selected-AR update.

The project-only records `E-PROJECT-REVERSE-CARRY-PRIMITIVE-CONTRACT`,
`E-PROJECT-INDIRECT-REVERSE-CARRY-PLANNER-CONTRACT`, and
`E-PROJECT-INDIRECT-REVERSE-CARRY-APPLICATION` own validation and transactional
behavior. Zero/non-one-hot and unsafe results are rejected by those contracts,
not presented as TI behavior. `E-TI-C54X-SPRU131G-S5.5.5-T5-9-P5-23-AR0-SYNTAX`
records `*AR0-0B` and `*AR0+0B` as legal syntax only; selected-AR0 alias
ordering, final propagation/wrap, and LONG32 applicability/update remain
`UNRESOLVED` and are intentionally untested.

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

The same private state owns checked field access for ARP, DP, CPL, and CMPT.
ARP is the three-bit ST0 field at bits 15–13, DP is ST0 bits 8–0, CPL is the
one-bit ST1 field at bit 14, and CMPT is ST1 bit 5. Valid writes replace only
their field bits and establish field knownness, while unknown or invalid reads
preserve caller outputs and rejected writes preserve the complete state. The
DP/CPL accessors are used by direct-source application but remain private state
contracts; they do not widen the common reset claim or add raw status-write,
timing, bus, or memory behavior.

The direct DP/SP branches of the shared project-owned address-effect application
seam consume those planner results. A planner result carries a value-owned
DP9/dma7 or SP16/dma7 source snapshot and the shared seam checks the tagged
source arm, canonical field ranges, recomputed WORD16 operand, and matching CPL
policy before reading state. Malformed payloads and non-OK planner results are
rejected first. Unreadable CPL, unreadable DP/SP, and mismatching known DP/SP
snapshots retain distinct statuses, and every rejection is bytewise
transactional. These checks prove interface ordering and state ownership only;
they do not claim direct long32, address wrap, buses, timing, or diagnostic
behavior.

The public `ti_c54x_config.reset_pc_word` remains a project bootstrap setting
owned by the CPU shell. It is not the TI hardware-reset value and is intentionally
not migrated or embedded by this work piece. A future integration must preserve
these two policies as separate contracts rather than making one field represent
both a caller-selected start address and the documented `FF80h` reset vector.

## Circular-buffer size register foundation

The common state foundation now carries BK as a private 16-bit architectural
value. TI `SPRU131G` §3.3.4.8 (printed p. 3-28) defines BK as the circular-buffer
size register used by the ARAUs; §3.3.4/Table 3-3 (printed p. 3-27) maps the CPU
memory-mapped register to data-memory address `0x19`, but MMR routing remains a
planned `src/bus/` responsibility. The §5.5.3.4 record (printed pp. 5-15–5-16)
is retained as future circular-addressing evidence only: no circular arithmetic,
base alignment, boundary, step, width, or timing behavior is implemented here.

`src/state/block_size_register.*` owns checked BK read/write accessors with
explicit knownness and rejection statuses. Reset leaves the physical BK storage
zeroed but unknown under the project contract; this work piece makes no TI reset
claim. Circular address planners and memory-mapped register routing remain
separate future ownership slices.

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
