### P3-S42: W7 batch-5 DELAY Smem row (implemented; local verification green)

Fifth managed fan-out landing: one single-word Smem row on the
existing `INDIRECT_ADDR7` layout, no new seam — `FORM-T2-06-04`
(DELAY Smem, `0x4D00/0xFF00`, TI SPRU172C section DELAY p. 4-41).
I rides bit 7 and the address rides bits 6:0; the row stays
`UNRESOLVED` repeatability (LTD precedent) with no input
resources and no status effects. DELAY stages a Smem read with the
S41 single-word delay write-back (Smem word to DATA Smem+1) and a
null completion: the delay WRITE is the only publication. The leaf
is built row-less by an isolated worker against a maintainer TI
brief (Z-delay: Smem word to DATA Smem+1 with Smem unchanged, TI
worked example `DELAY *AR3` hand-verified) and merged with an id
flip if needed. The LTD decode neighbor probe at `0x4D00` flips to
the pinned DELAY sibling. Projected closure from the S41 freeze
(291 rows, 297 bindings, 32,265 lookup entries, 139/205 forms at
67.805%, 755 requirements, 611 tests): 292 rows, 298 bindings,
32,521 lookup entries (+256 pure), 140/205 forms at 68.293%
SUBSET-or-better, 758 requirements, 614 tests. These numbers are
projections until the slice lands.
