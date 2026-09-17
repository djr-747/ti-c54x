### P3-S3w: MASA Smem and T slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
evidence freeze. `MASA Smem` (TI SPRU179C §MASA syntax 1, opcode
`0011_0011_IAAAAAAA`, pp. 4-99–4-100) lands one `MASA_SMEM` row at
`0x3300` under mask `0xFF00` (256 encodings, one program word)
through the shared `INDIRECT_ADDR7` layout: B minus Smem times A-high
into fixed B with read-phase Smem T load. `MASA T` (syntax 2,
opcode `1111_01SD_1000_101R`) lands one `MASA_T` row at `0xF48A`
under mask `0xFCFE` (S/D/R ride the word, 8 encodings, one program
word, no memory traffic) through the shared
`SOURCE_DESTINATION_ROUND` layout: dst minus T times A-high into
dst, with R=1 carrying the future MASAR T variant by the recorded
R-sharing convention. Both handlers reuse the multiply core with the
SUBTRACT select and the A-high multiplicand, FRCT/OVM handling, and
C/TC preservation, mirroring the `MACA_SMEM`/`MACA_T` leaves. Both
rows land `UNRESOLVED` repeatability: no Table 2-28 MASA fact is on
record, and absence from the evidenced negative lists is not
`ELIGIBLE` authorization (P2-W48C precedent). The RED executable
contracts pin the TI vectors end to end — the Example 1 Smem lane
(`0x56 78` read, B `00 0002 0000` to `FF F9DB FFA0` with T load),
the Example 2 T lane (T `0x0444`, FRCT=1, B to `FF FF66 B460`),
and the Example 3 rounding lane (R=1, B to `FF FF67 0000`) — each at
the leaf, through the trusted entry point, and retired on the CPU.
`FORM-T2-04-18`/`FORM-T2-04-19` land `SUBSET` with the `MASA_SMEM`/
`MASA_T` symbols; `FORM-T2-04-20`/`FORM-T2-04-21` (MASAR) stay
`UNIMPLEMENTED` with empty symbol lists. Projected closure adds two
catalog rows, two inventory bindings, one TI source record, one
activation record, four requirements, and four test records: 386
evidence records, 637 requirements, and 510 tests (510 `PASSING`,
0 `PLANNED`). These numbers are projections until the slice lands.
