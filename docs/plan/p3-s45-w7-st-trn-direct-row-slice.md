### P3-S45: W7 batch-7 ST TRN direct row (implemented; local verification green)

Seventh managed fan-out landing: the direct slice of TI syntax-2 ST
TRN,Smem on a fixed-zero I bit — `FORM-T2-22-02` (ST TRN direct,
`0x8D00/0xFF80` from opcode diagram `1000_1101_IAAAAAAA`, TI
SPRU172C section ST pp. 4-166–4-168). Execution is `(TRN)` into
Smem with no status-bit effects; the worked direct example is
`ST TRN,5` storing `TRN=0x1234` at `0205h`. The 7-bit page-0
address rides bits 6:0 through the shared indirect/addr7 layout;
a set I bit rejects as INVALID_OPERAND, mirroring the ST T
direct leaf. The row stays `UNRESOLVED` repeatability (family
precedent) with TRN_REGISTER inputs (P3-S44 seam) and stages a
page-0 DATA WRITE of the TRN register with a null completion.
The leaf is built row-less by an isolated worker against this
brief and merged by the maintainer, who wires the row, dispatch,
manifests, inventory, evidence, and count bumps. Projected
closure from the S44 freeze (293 rows, 299 bindings, 32,649
lookup entries, 141/205 forms at 68.780%, 762 requirements, 618
tests): 294 rows, 300 bindings, 32,777 lookup entries (+128
pure), 142/205 forms at 69.268% SUBSET-or-better, 765
requirements, 621 tests, reusing the ST TI evidence record.
These numbers are projections until the slice lands.
