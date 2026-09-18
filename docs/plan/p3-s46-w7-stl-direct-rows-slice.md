### P3-S46: W7 batch-8 STL direct rows (implemented; local verification green)

Eighth managed fan-out landing: the direct slices of TI STL
syntax 1 (`STL src,Smem`, `FORM-T2-20-10`) and syntax 2
(`STL src,ASM,Smem`, `FORM-T2-20-11`) on a fixed-zero I bit —
`STL_SMEM_DIRECT` (`0x8000/0xFE80`) and `STL_ASM_SMEM_DIRECT`
(`0x8400/0xFE80`) from the STL opcode diagrams (TI SPRU172C
section STL pp. 4-172–4-173). Both ride the existing
SELECT_INDIRECT_ADDR7 layout (S@8 src select, I@7, 7-bit
page-0 address, no layout seam): S=0 reads accumulator A,
S=1 reads B. Syntax 1 stages bits 15:0 unshifted; syntax 2
shifts the 40-bit source by the signed ASM count (0–15 left,
16–31 right by 32−ASM, mirroring `op_add_src_asm`/`op_sth_asm_smem`)
with SXM-gated fill and stages bits 15:0. Worked examples:
`STL A,11` stores `0x4321` (A=`FF87654321`); `STL B,−8,*AR7−`
stores `0x2112` (B=`FF84211234`, SXM=0). A set I bit rejects
as INVALID_OPERAND (indirect STL deferred); unknown src/ASM
(and SXM for syntax 2) rejects as UNKNOWN_INPUT. Both rows
stay `UNRESOLVED` repeatability (family precedent) with a
page-0 DATA WRITE and null completion. Two isolated workers
build the leaves row-less in parallel against this brief
(W8A syntax 1, W8B syntax 2, self-contained cores); the
maintainer merges, wires rows, dispatch, manifests,
inventory, evidence, and count bumps. Projected closure from
the S45 freeze (294 rows, 300 bindings, 32,777 lookup
entries, 142/205 forms at 69.268%, 765 requirements, 621
tests): 296 rows, 302 bindings, 33,289 lookup entries (+512
pure), 144/205 forms at 70.244% SUBSET-or-better, 771
requirements, 627 tests, reusing the STL TI evidence record.
These numbers are projections until the slice lands.
