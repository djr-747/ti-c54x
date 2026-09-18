### P3-S38: W7 batch-2 ADDC/ADDS/SUBC Smem rows (implemented; local verification green)

Second managed fan-out landing: three single-word Smem rows on
the existing `SELECT_INDIRECT_ADDR7` layout, no new seam —
`FORM-T2-01-11` (ADDC Smem,src, `0x0600/0xFE00`, TI SPRU172C
section ADDC p. 4-8), `FORM-T2-01-13` (ADDS Smem,src,
`0x0200/0xFE00`, section ADDS p. 4-10), and `FORM-T2-02-12`
(SUBC Smem,src, `0x1E00/0xFE00`, section SUBC pp. 4-192--4-193).
S/I/addr7 ride bits 8/7/6:0; all three rows stay `UNRESOLVED`
repeatability and promote their forms to `COMMON_CANDIDATE` at
`SUBSET`. ADDC stages `dst = src + Smem + C` with
SXM-suppressed zero extension, bit-32 carry-out, and OVM-gated
saturation with sticky OVdst; ADDS stages `dst = src +
unsigned(Smem)` with the same saturation rule; SUBC stages the
conditional divide step (`Smem << 15`, double-with-1 on
positive) reading SXM (SXM==1 with Smem MSB set rejects) and
ignoring OVM with sticky OV on signed-32 misfit. Leaves were
built row-less by isolated workers against maintainer TI
briefs and merged with id flips plus one test-vector fix (the
ADDC SXM-suppression vector forgot its C=1 input). A new
`ACCUMULATORS_STATUS_SXM` input-resource composite covers the
SUBC row. Projected closure from the S37 freeze (286 rows, 292
bindings, 30,345 lookup entries, 134/205 forms at 65.366%, 739
requirements, 595 tests): 289 rows, 295 bindings, 31,881 lookup
entries (+1,536 pure), 137/205 forms at 66.829% SUBSET-or-better,
748 requirements, 604 tests. These numbers are projections until
the slice lands.
