### P3-S111: POLY single-memory polynomial row and leaf (implemented; local verification green)

This immutable anchor heads the P3-S111 evidence freeze. One
single-word filter row lands on the multiply seam: `POLY
Smem` (`0x3600` under mask `0xFF00`, TI SPRU172C section
POLY, printed p. 4-126) with the shared `INDIRECT_ADDR7`
layout (I plus 7-bit indirect field), the new
`ACCUMULATORS_T_REGISTER_STATUS_SXM_FRCT_OVM` input
resources, and ELIGIBLE repeatability (absent from Table
2-28). Semantics (TI execution): the SXM-widened Smem word
shifted left 16 loads into B while A-high times live T plus
B-old rounds into A (FRCT-doubled product, OVM saturation,
OVA on destination overflow); IMR plays no role and the Smem
read never loads T. The `op_poly_smem` leaf (multiply home,
checked plus trusted entries, metadata cross-check,
transactional rejection) stages one single-memory DATA READ
(direct or standard-indirect MOD0--MOD3; exotic modes above
MOD3 defer as INVALID_OPERAND) with `load_t_with_read`
false, and the read-value completion publishes one
`ACCUMULATOR_WRITE_PAIR` (A first with the multiply-core
result and OVA delta, B second with the SXM-widened shift16
load that sets the guard bits on negative words). T is a
live validated input: unknown T rejects at stage time,
unlike the MACA family whose read loads T. Retirement flows
through the generic single-memory MEMORY route with no
sequencer change. The T2-06-12 (POLY) form flips to
`COMMON_CANDIDATE`/`COMPLETE` with one binding under ALL.
Landing adds one TI section record, three ARCHITECTURAL
requirements (leaf semantics, CPU retirement, catalog
registration), and three COMMON test records on the op, CPU,
and metadata suites: 510 evidence records, 847
requirements, and 701 tests (701 `PASSING`, 0 `PLANNED`); 383
bindings, 188/205 forms (91.707%), 55 strict-complete
(26.829%), 359 rows, 360 IDs, 44953 lookup slots.
