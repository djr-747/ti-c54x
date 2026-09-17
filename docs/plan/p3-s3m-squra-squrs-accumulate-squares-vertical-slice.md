### P3-S3m: SQURA/SQURS accumulate-squares vertical slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `SQURA Smem,src` and `SQURS Smem,src`
forms from TI SPRU172C §SQURA (printed p. 4-163) and §SQURS (printed
p. 4-164) are implemented as two catalog rows sharing one leaf file
with an ADD/SUBTRACT select: `SQURA_SMEM` with value `0x3800`, mask
`0xFE00` (bits 15:9 `0011100`, S bit 8, I bit 7, Smem field 6:0, 512
encodings) and `SQURS_SMEM` with value `0x3A00`, mask `0xFE00`
(bits 15:9 `0011101`, same field shape, 512 encodings). Both reuse
the S3k `DESTINATION_INDIRECT_ADDR7` layout (S select rides field 0),
resources `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`, and
repeatability `ELIGIBLE`. Word 0 is `0011_100S_IAAA_AAAA` /
`0011_101S_IAAA_AAAA`: the Smem word loads T in the read phase and
the pair completion runs `(src)` plus/minus `(Smem) x (Smem)` through
the existing multiply core with ADD/SUBTRACT select and no rounding;
C/TC are preserved and `OVdst` follows the core. The TI SQURA
Example 1 vector (`SQURA 30, B` with `mem[031E]` = `000F`, `B` =
`00 0320 0000`, `FRCT` = 0 yielding `B` = `00 0320 00E1` and `T` =
`000F`) and SQURS Example 1 vector (`SQURS 9, A` with `mem[0309]`
= `1234`, `A` = `00 014B 5DB0`, `FRCT` = 0 yielding `A` = `00 0000
0320` and `T` = `1234`; both vectors recomputed independently
before pinning) are the oracles. Table 2-28 lists no SQURA/SQURS
instruction, so both rows are repeatable by omission on the recorded
S3 omission basis. The four RED executable contracts are
`tests/unit/isa/catalog/test_squr_acc_metadata.c`
(CTest `ti_c54x.squr_acc_metadata`),
`tests/unit/isa/decode/test_squr_acc_decode.c`
(CTest `ti_c54x.squr_acc_decode`),
`tests/unit/isa/ops/multiply/test_squr_acc_ops.c`
(CTest `ti_c54x.squr_acc_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_squr_acc.c`
(CTest `ti_c54x.cpu_squr_acc`), plus the `SUBSET`
`FORM-T2-04-21`/`FORM-T2-04-22` inventory bindings. Projected closure
is 201 catalog rows, `ID_COUNT=201`, 15,337 lookup entries, 200
bindings, family coverage 76/205 (37.073%), and 33/205 strict
complete-form coverage; 370 evidence records, 592 requirements, and
472 tests (472 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.
