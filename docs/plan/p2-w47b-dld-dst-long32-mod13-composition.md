### P2-W47B: DLD/DST LONG32 MOD13 composition (implemented; local verification green)

This immutable `(planned)` heading is the FIT/ownership anchor for the W47B
activation. The TI DLD and DST entries define the long-word read/write families
and their one-extension-word long-offset forms. Composed with the pinned
CMPT=0 MOD13, LONG32 delta, word-pairing, and repeat-policy records, this slice
freezes exactly two two-word rows: `DLD_LONG32_MOD13` base `0x56E8`, mask
`0xFEF8`, and `DST_LONG32_MOD13` base `0x4EE8`, mask `0xFEF8`. Their full
patterns are `0101_011D_1110_1AAA` and `0100_111S_1110_1AAA`; D/S is bit 8,
ARF is bits 2:0, MOD13 is bits 6:3, and the second word is the copied `lk`
extension. Each admits sixteen ARF encodings, uses the new private
`ACCUMULATOR_ARF_EXTENSION16_MOD4` layout, and is ineligible for single-repeat
execution. The activation does not claim TI bus order, external pair atomicity,
rollback, or machine-cycle timing.

DLD consumes `STATUS_SXM` and reads first/MSW then second/LSW into `dst31:0`;
SXM=0 clears `dst39:32`, while SXM=1 sign-extends bit 31. C16 is not an input
resource; project vectors must prove the C16×SXM lane descriptions converge on
the same stored 40-bit pattern. DST consumes `ACCUMULATORS`, `STATUS_SXM`, and
`STATUS_SST`; the full source remains unchanged and SST/SXM saturation is
applied before first/MSW then second/LSW store. SST=0 stores source bits31:0;
SST=1 clamps according to the pinned raw-40/signed-40 rules. Unknown SST or
SXM rejects transactionally before endpoint issue.

W47B composes both rows over the private W47A reducer and generic MOD13
LONG32 MEMORY path. A fixed-size discriminated child union copies operands and
source snapshots, issues exactly two children, and never replans, refetches, or
recomputes source values. Pre-access AR/PC candidates commit only after pair
completion; per-word physical write visibility remains the W47A contract. Static
completion callbacks may remain process-lifetime function pointers but own no
mutable context, formatting, or fan-out. Root and validator helpers split before
file-size thresholds, and an observer-off full-CPU benchmark is required.

W47B1 is implementation-complete for the generic fixed-size LONG32 MEMORY
carrier, checked CMPT=0 MOD13 resolver, and discriminated root transaction.
Focused and sanitizer matrices are 10/10 GREEN. Native LP64 freezes the address
specification/effect/root at 24/40/336 bytes; supported ILP32 freezes them at
24/36/308. The root issues two copied requests directly through the pair reducer,
preserves active IDs and the sequence watermark across WAIT/fault/cancel/stale
paths, and commits AR/PC only after both words are READY. The established WORD16
path does not call the LONG32 preparation helper. The 494-line root and 782-line
cohesive lifecycle test are closed to further W47B growth; W47B2 owns its opcode,
semantic, CPU, quality, and benchmark work in their existing modules.

W47B1 accounting is 181 evidence records; 304 requirements (259 `IMPLEMENTED`,
16 `PLANNED`, 29 `UNRESOLVED`); 258 tests (242 `PASSING`, 16 `PLANNED`); and 7
sources. Catalog and inventory counts remain unchanged until W47B2 activates the
two production rows.

W47B2A completes the typed PMST.SST state/input prerequisite without widening
the 48-byte copied opcode-input snapshot on LP64 or ILP32. Common reset leaves
SST unknown; checked typed writes establish knownness; collection keeps SST and
SXM independently selectable and stages output transactionally. Unknown-status
rejection before endpoint issue remains in the DLD/DST semantic and CPU slices.
Accounting is 181 evidence records; 304 requirements (260 `IMPLEMENTED`, 15
`PLANNED`, 29 `UNRESOLVED`); 258 tests (243 `PASSING`, 15 `PLANNED`); and 7
sources. Catalog, lookup, binding, and ISA coverage counts remain unchanged.

W47B2B activates the exact two-word DLD/DST MOD13 rows and their four-field
layout. Checked semantics validate complete decode; trusted semantics consume
the single pre-extracted D/S, ARF, `lk`, and MOD13 packet without resolving or
extracting the layout again. DLD publishes canonical SXM zero/sign extension
only after its MSW/LSW completion. DST snapshots and saturates the selected
raw-40 accumulator once before staging copied MSW/LSW writes. Inventory now
binds both rows as bounded MOD13 subsets: 100 catalog rows, `ID_COUNT=101`,
2,711 lookup entries, 100 bindings, 40/205 family coverage, and unchanged
25/205 strict coverage. Accounting is 181 evidence records; 304 requirements
(264 `IMPLEMENTED`, 11 `PLANNED`, 29 `UNRESOLVED`); 258 tests (247 `PASSING`,
11 `PLANNED`); and 7 sources. CPU READY/WAIT/FAULT execution and the
observer-off benchmark remain W47B2C.

W47B2C closes the activation with full public-CPU execution proofs. DLD and DST
each cover READY, first/second WAIT and first/second FAULT, two-word fetch,
stable resume identity, fresh paired transaction IDs, no refetch, deferred
AR/PC publication, and parity-derived LONG32 word order. DLD additionally
proves D/S selection, SXM lanes, unknown-SXM rejection, and C16 independence;
DST proves copied raw-40 sources, the SST/SXM saturation boundary matrix,
unknown-source/status rejection, and first-write physical visibility when the
second write faults. The dedicated observer-off benchmark runs both exact rows
through the public CPU with semantic counts and ordering checked around every
timed window. Its capture schema records host/compiler/build provenance and
timing spread but intentionally freezes no host threshold or TI timing claim.
Structural tests retain single trusted-packet extraction, static dispatch,
allocation/I/O/environment exclusion, and LP64/ILP32 carrier sizes. Accounting
is 181 evidence records; 304 requirements (267 `IMPLEMENTED`, 8 `PLANNED`, 29
`UNRESOLVED`); 258 tests (250 `PASSING`, 8 `PLANNED`); and 7 sources. Catalog,
lookup, binding, and ISA-coverage counts remain 100, 2,711, 100, 40/205, and
25/205 respectively.

Inventory rows `FORM-T2-19-01` (DLD) and `FORM-T2-20-01` (DST) now bind the
production symbols as `LONG_OFFSET_MOD13_CMPT0` subsets. Strict complete
coverage remains unchanged because each row implements only part of its full
mnemonic family.
