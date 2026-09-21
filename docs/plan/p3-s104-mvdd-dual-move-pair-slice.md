### P3-S104: MVDD dual-move pair row and leaf (planned)

This immutable anchor heads the P3-S104 evidence freeze. One
class-14 dual-memory row lands on the shared dual seam: `MVDD
Xmem, Ymem` (`0xE500` under mask `0xFF00`, TI SPRU172C section
MVDD, printed p. 4-107) with the `DUAL_XMOD_XAR_YMOD_YAR`
layout, `AUXILIARY_REGISTERS` input resources, and ELIGIBLE
repeatability (absent from Table 2-28). Semantics (TI
execution): the Xmem word copies to Ymem with no status
effect; the manual's `*AR3+, *AR5+` example pins MOD
post-update on both legs. The `op_mvdd` leaf (store home,
checked plus trusted entries, metadata cross-check,
transactional rejection) stages one `MEMORY` effect over a new
`DUAL_INDIRECT_MOVE_PAIR_WORD16` address kind carrying the
frozen dual X/Y fields; the pair root issues the X READ and
the Y WRITE, publishes the moved word through the read-value
completion shape with null follow-on, and commits both AR
updates with the shared dual resolver across all four
XMOD/YMOD codes including MOD3 indexed and circular commits.
No run.c change: retirement flows through the generic MEMORY
route. The T2-26-01 (MVDD) form flips to `COMMON_CANDIDATE`/
`COMPLETE` with one binding under ALL.
Projected closure adds one TI section record, three
ARCHITECTURAL requirements (leaf semantics, CPU retirement,
catalog registration), and three COMMON test records on
the op, CPU, and metadata suites: 505 evidence records, 832
requirements, and 686 tests (686 `PASSING`, 0 `PLANNED`); 378
bindings, 183/205 forms (89.268%), 50 strict-complete
(24.390%), 354 rows, 355 IDs, 44121 lookup slots. These
numbers are projections until the slice lands.
