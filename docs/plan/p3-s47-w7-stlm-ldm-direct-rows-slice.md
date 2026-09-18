### P3-S47: W7 batch-9 STLM/LDM direct rows (implemented; local verification green)

Ninth managed fan-out landing: the direct slices of TI STLM
syntax (`STLM src,MMR`, `FORM-T2-20-13`) and LDM syntax
(`LDM MMR,dst`, `FORM-T2-19-18`) on a fixed-zero I bit —
`STLM_SMEM_DIRECT` (`0x8800/0xFE80`, TI SPRU172C section STLM
p. 4-175) and `LDM_SMEM_DIRECT` (`0x4800/0xFE80`, TI SPRU172C
section LDM p. 4-73). Both ride the existing
SELECT_INDIRECT_ADDR7 layout (S/D@8 select, I@7, 7-bit
address, no layout seam) with `UNRESOLVED` repeatability.
STLM stages a DP-independent page-0 DATA WRITE of the
selected accumulator low word with a null completion (worked
example `STLM A,BRC` stores `0x4321`; MMR-register routing
happens downstream at the bus, as for the landed STM row).
LDM stages a DP-independent page-0 DATA READ whose static
completion zero-extends the WORD16 into dst(15:0) and clears
dst(39:16) (worked examples `LDM AR4,A` and `LDM 060h,B`;
mirror the `op_ldu_smem` completion shape; not affected by
SXM). A set I bit rejects as INVALID_OPERAND on both (indirect
MMR deferred). Two isolated workers build the leaves row-less
in parallel against this brief (W9A STLM, W9B LDM,
self-contained cores); the maintainer merges, wires rows,
dispatch, manifests, inventory, evidence, and count bumps.
Projected closure from the S46 freeze (296 rows, 302
bindings, 33,289 lookup entries, 144/205 forms at 70.244%,
771 requirements, 627 tests): 298 rows, 304 bindings, 33,801
lookup entries (+512 pure), 146/205 forms at 71.220%
SUBSET-or-better, 777 requirements, 633 tests, adding the
two STLM/LDM TI evidence records at merge. These numbers are
projections until the slice lands.
