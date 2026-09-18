### P3-S43: W7 batch-6 ST T direct row (implemented; local verification green)

Sixth managed fan-out landing: the direct slice of TI syntax-1 ST
T,Smem on a fixed-zero I bit — `FORM-T2-22-01` (ST T direct,
`0x8C00/0xFF80`, TI SPRU172C section ST pp. 4-167–4-168). The
7-bit page-0 address rides bits 6:0 through the shared
indirect/addr7 layout; a set I bit rejects as INVALID_OPERAND
because indirect ST T composes through the established ST_T MOD
rows (which start at `0x8C80`, so the slice is disjoint by
construction). The row stays `UNRESOLVED` repeatability (T
precedent of the LD_T/LTD/ST_T family) with T_REGISTER inputs
and stages a page-0 DATA WRITE of the T register with a null
completion. The leaf was built row-less by an isolated worker
against a maintainer TI brief and merged with no fixes. ST TRN
(`FORM-T2-22-02`) stays deferred: the input snapshot carries no
TRN field, so it needs a TRN input seam first. Projected closure
from the S42 freeze (292 rows, 298 bindings, 32,521 lookup
entries, 140/205 forms at 68.293%, 758 requirements, 614
tests): 293 rows, 299 bindings, 32,649 lookup entries (+128
pure), 141/205 forms at 68.780% SUBSET-or-better, 761
requirements, 617 tests. These numbers are projections until
the slice lands.
