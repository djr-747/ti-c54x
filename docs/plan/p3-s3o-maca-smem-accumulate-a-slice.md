### P3-S3o: MACA Smem accumulate-A slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `MACA[R] Smem[, B]` syntax-1 form from TI
SPRU172C §MACA[R] (printed p. 4-85, opcode `0011 01R1 IAAA AAAA`)
is implemented as one catalog row: `MACA_SMEM` with value `0x3500`,
mask `0xFD00` (bits 15:10 `001101`, bit 8 fixed 1, R bit 9, I bit 7,
Smem field 6:0, 512 encodings). The R bit carries the MACAR rounding
variant on the shared row by the recorded S3c/S3f R-sharing
convention, so `FORM-T2-04-09` stays `UNIMPLEMENTED`. The row uses a
new `ROUND_INDIRECT_ADDR7` layout (R/I/addr7, mirroring the S3k
destination/indirect/addr7 set), resources
`ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`, and repeatability
`ELIGIBLE` (Class 3A/3B single-repeatable classes with no Table 2-28
entry). Word 0 is `0011_01R1_IAAA_AAAA`: the Smem word loads T in
the read phase and the completion runs `(B)` plus `(Smem)` times
`A(32-16)` into fixed B through the existing multiply core with ADD
select and R rounding; C/TC are preserved and `OVdst` follows the
core for B. A-high is the multiplicand, so input A must be known
(and B known for the accumulate); T arrives via the read-phase load
and needs no prior knownness. The TI MACA Example 1 vector
(`MACA *AR5+` with `A` = `00 1234 0000`, `B` = `00 0000 0000`,
`FRCT` = 0, `AR5` = `0100`, `mem[0100]` = `5678` yielding `B` =
`00 0626 0060` with `T` = `5678` and `AR5` = `0101`;
`0x1234` times `0x5678` equals `0x6260060`, recomputed
independently before pinning) and the MACAR Example 3 rounding lane
(R = 1 yields `B` = `00 0626 0000`) are the oracles. The five RED
executable contracts are
`tests/unit/isa/decode/test_round_indirect_addr7_layout.c`
(CTest `ti_c54x.round_indirect_addr7_layout`),
`tests/unit/isa/catalog/test_maca_smem_metadata.c`
(CTest `ti_c54x.maca_smem_metadata`),
`tests/unit/isa/decode/test_maca_smem_decode.c`
(CTest `ti_c54x.maca_smem_decode`),
`tests/unit/isa/ops/multiply/test_maca_smem_ops.c`
(CTest `ti_c54x.maca_smem_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_maca_smem.c`
(CTest `ti_c54x.cpu_maca_smem`), plus the `SUBSET`
`FORM-T2-04-07` inventory binding. The MACA T,src syntax-2 row,
MACD, and MACP remain later slices (program-memory operands and
the delay seam). Projected closure is 203 catalog rows,
`ID_COUNT=203`, 16,361 lookup entries, 202 bindings, family
coverage 78/205 (38.049%), and 33/205 strict complete-form
coverage; 374 evidence records, 603 requirements, and 481 tests
(481 `PASSING`, 0 `PLANNED`). These numbers are projections until
the slice lands.
