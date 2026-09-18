### P3-S39: W7 batch-3 SFTA accumulator row (implemented; local verification green)

Third managed fan-out landing: one single-word accumulator row
on the existing `ACCUMULATOR_SOURCE_DESTINATION_SHIFT5`
layout, no new seam — `FORM-T2-10-04` (SFTA src, SHIFT [,dst],
`0xF460/0xFCE0`, TI SPRU172C section SFTA pp. 4-155--4-156).
S/D ride bits 9:8, SHIFT rides bits 4:0 sign-extended
(-16..+15); the row stays `UNRESOLVED` repeatability and
promotes its form to `COMMON_CANDIDATE` at `SUBSET`. Negative
SHIFT arithmetically shifts right with C taking the last bit
out and SXM-gated sign fill; zero SHIFT is identity with C
taking the sign bit; positive SHIFT shifts left with C taking
src(39-SHIFT) and no-sign-loss overflow detection (sticky
OVdst, OVM-gated saturation). The leaf was built row-less by
an isolated worker against a maintainer TI brief and merged
with an id flip, no fixes needed. Projected closure from the
S38 freeze (289 rows, 295 bindings, 31,881 lookup entries,
137/205 forms at 66.829%, 748 requirements, 604 tests):
290 rows, 296 bindings, 32,009 lookup entries (+128 pure),
138/205 forms at 67.317% SUBSET-or-better, 751 requirements,
607 tests. These numbers are projections until the slice lands.
