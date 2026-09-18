### P3-S33: MPY syn3 / MAC syn4 Smem-lk rows (implemented; local verification green)

This immutable anchor heads the P3-S33 evidence freeze. It
lands Seam 3 end to end: two new shared two-word operand
layouts, `DESTINATION_INDIRECT_ADDR7_EXTENSION16`
(D[8:8]/I[7:7]/A[6:0] from word 0 with the copied 16-bit lk
from word 1) and
`SOURCE_DESTINATION_INDIRECT_ADDR7_EXTENSION16`
(S[9:9]/D[8:8]/one 8-bit Smem cookie from word 0 with lk from
word 1, the cookie keeping trusted packets within the
four-operand capacity); two T-loading memory leaves
(`op_mpy_smem_lk` with plain plus trusted entries staging a
read-value STORE completion of Smem times signed lk, and
`op_mac_smem_lk` staging a read-value ADD completion of the
S-selected source plus Smem times signed lk, both through the
multiply core with FRCT shift, OVM saturation, sticky OVdst,
and preserved C/TC); and two `MASKED` catalog rows
`MPY_SMEM_LK` (`0x6200/0xFE00`, TI SPRU172C section MPY
syntax 3) and `MAC_SMEM_LK` (`0x6400/0xFC00`, section MAC
syntax 4), both `ELIGIBLE` (Table 6-18 omits every MPY/MAC
form) with `STATUS_FRCT_OVM` and
`ACCUMULATORS_STATUS_FRCT_OVM` inputs, wired through the
catalog X-macro into dispatch. TI pins no R bit in either
opcode space, so the unrounded leaves cover both rows fully.
`FORM-T2-03-04` and `FORM-T2-04-03` bind at `SUBSET` under
`DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3` and flip
`UNRESOLVED`/`UNIMPLEMENTED` to
`COMMON_CANDIDATE`/`SUBSET` with the rows. The generated
first-word lookup grows by 1,536 entries with zero neighbor
moves (no established rows live in 0x60--0x67). Projected
closure adds two evidence records, six requirements, and six
test records on the new semantics suites plus the binding
suites: 459 evidence records, 711 requirements, and 568
tests (568 `PASSING`, 0 `PLANNED`). These numbers are
projections until the slice lands.
