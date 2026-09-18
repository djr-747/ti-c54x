### P3-S40: W7 batch-4 LTD Smem row (implemented; local verification green)

Fourth managed fan-out landing: one single-word Smem row on
the existing `INDIRECT_ADDR7` layout, no new seam —
`FORM-T2-19-21` (LTD Smem, `0x4C00/0xFF00`, TI SPRU172C
section LTD p. 4-81). I rides bit 7 and the address rides bits
6:0; the row stays `UNRESOLVED` repeatability and promotes its
form to `COMMON_CANDIDATE` at `SUBSET`. LTD stages a Smem read
with the MACD-style delay write-back (Smem word to DATA
Smem+1) and completes a T-register write of the read word; no
accumulator or status inputs, no status effects. The leaf was
built row-less by an isolated worker against a maintainer TI
brief and merged with an id flip, no fixes needed. Projected
closure from the S39 freeze (290 rows, 296 bindings, 32,009
lookup entries, 138/205 forms at 67.317%, 751 requirements,
607 tests): 291 rows, 297 bindings, 32,265 lookup entries
(+256 pure), 139/205 forms at 67.805% SUBSET-or-better, 754
requirements, 610 tests. These numbers are projections until
the slice lands.
