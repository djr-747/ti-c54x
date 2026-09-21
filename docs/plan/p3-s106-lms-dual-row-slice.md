### P3-S106: LMS dual-memory row and leaf (implemented; local verification green)

This immutable anchor heads the P3-S106 evidence freeze. One
class-7 dual-memory row lands on the shared dual seam: `LMS
Xmem,Ymem` (`0xC100` under mask `0xFF00`, TI SPRU172C section
LMS, printed p. 4-80) with the `DUAL_XMOD_XAR_YMOD_YAR`
layout, `ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_SXM_FRCT_OVM`
input resources, and ELIGIBLE repeatability (the LMS loop
instruction; absent from Table 2-28). Semantics (TI
execution): `A = A + Xmem << 16 + 2^15` and
`B = B + Xmem * Ymem` in parallel with no T update; SXM,
FRCT, and OVM affect the result and C, OVA, OVB report it.
The TI worked example (`LMS *AR3+, *AR4+`, X=0x0055,
Y=0x00AA) pins the exact math: A gains `(X << 16) + 0x8000`
as one 40-bit add, B gains the signed product (doubled under
FRCT). The `op_lms` leaf (arithmetic home, checked plus
trusted entries, metadata cross-check, transactional
rejection) stages one `DUAL_INDIRECT_WORD16` READ effect over
the frozen dual X/Y fields; the shared dual READ root issues
both legs, the LMS completion stages the accumulator-write
pair (A-channel carry sets C, B-channel carry sets C, OVA/OVB
with OVM saturation, TC preserved, per the ADD-SH16 and
SQDST precedents), and both AR updates commit with the shared
dual resolver across all four XMOD/YMOD codes including MOD3
indexed and circular commits. No run.c change: retirement
flows through the generic MEMORY route. The T2-06-07 (LMS)
form flips to `COMMON_CANDIDATE`/`COMPLETE` with one binding
under ALL.
Projected closure adds one TI section record, three
ARCHITECTURAL requirements (leaf semantics, CPU retirement,
catalog registration), and three COMMON test records on
the op, CPU, and metadata suites: 507 evidence records, 838
requirements, and 692 tests (692 `PASSING`, 0 `PLANNED`); 380
bindings, 185/205 forms (90.244%), 52 strict-complete
(25.366%), 356 rows, 357 IDs, 44409 lookup slots. These
numbers are projections until the slice lands.
