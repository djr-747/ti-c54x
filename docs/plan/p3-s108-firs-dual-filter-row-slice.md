### P3-S108: FIRS dual-memory filter row and leaf (implemented; local verification green)

This immutable anchor heads the P3-S108 evidence freeze. One
class-7 dual-memory filter row lands on the triple seam: `FIRS
Xmem,Ymem,pmad` (`0xE000` under mask `0xFF00`, TI SPRU172C
section FIRS, printed p. 4-59) with the
`DUAL_XMOD_XAR_YMOD_YAR_PMAD16` layout,
`ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_SXM_FRCT_OVM`
input resources, and ELIGIBLE repeatability (absent from Table
2-28). Semantics (TI execution): the Xmem and Ymem DATA words
read in order with both ARs post-updated, the second program
word carries the absolute PROGRAM coefficient address with no
post-update, `A` stages the SXM-widened `(X + Y) << 16`, and
`B` accumulates the signed old-`A(32-16)` times the signed
coefficient (doubled under FRCT) as one 40-bit add with OVM
saturation; SXM, FRCT, and OVM affect the result and T is
never written. The `op_firs_dual` leaf (arithmetic home,
checked plus trusted entries, metadata cross-check,
transactional rejection) stages one
`DUAL_PROGRAM_TRIPLE_WORD16` READ effect over the frozen dual
X/Y fields plus the pmad; the shared dual resolver plans the
DATA legs from a re-frozen dual-kind view while the new triple
root issues X, then Y, then the PROGRAM coefficient leg, the
FIRS completion stages the accumulator-write pair through the
shared 40-bit foundation, and both AR updates commit across
all four XMOD/YMOD codes including MOD3 indexed and circular
commits. The root grows one triple union member (576/516).
The T2-06-06 (FIRS) form flips to `COMMON_CANDIDATE`/`COMPLETE`
with one binding under ALL.
Projected closure adds one TI section record, three
ARCHITECTURAL requirements (leaf semantics, CPU retirement,
catalog registration), and three COMMON test records on
the op, CPU, and metadata suites: 508 evidence records, 841
requirements, and 695 tests (695 `PASSING`, 0 `PLANNED`); 381
bindings, 186/205 forms (90.732%), 53 strict-complete
(25.854%), 357 rows, 358 IDs, 44665 lookup slots. These
numbers are projections until the slice lands.
