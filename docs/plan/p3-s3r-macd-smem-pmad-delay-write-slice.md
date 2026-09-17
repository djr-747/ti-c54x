### P3-S3r: MACD Smem,Pmad delay-write slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The two-word `MACD Smem, pmad [, dst]` syntax-1 form
from TI SPRU172C §MACD (printed p. 4-87, word 0
`0111_101S_IAAA_AAAA` with word 1 `pmad`) is implemented as one
catalog row: `MACD_SMEM` with value `0x7A00`, mask `0xFE00`
(bits 15:9 `0111101`, S bit 8, I bit 7, Smem field 6:0, 512
encodings) and `program_word_count` 2. The row reuses the S3q
`DESTINATION_INDIRECT_ADDR7_PMAD16` layout (no new layout),
resources `ACCUMULATORS_STATUS_FRCT_OVM`, and repeatability
`UNRESOLVED` (two-word STM precedent). Word 0 is
`0111_101S_IAAA_AAAA`: the Smem word is read from DATA memory,
the `pmad` word names a PROGRAM-memory coefficient read, T takes
the Smem word in the read phase, the pair completion runs
`(dst)` plus `(Smem)` times `(pmad)` into the S-selected
accumulator with ADD select and no rounding, and then the delay
sidecar writes the Smem word to DATA `Smem+1` (16-bit wraparound)
before the Smem post-access commit and PC publish. The sidecar
rides a new `write_smem_delay` flag on the MEMORY effect carrier
(a second boolean in the existing padding; the 48-byte ABI is
unchanged and only the DATA+PROGRAM pair kind may set it): the
pair root issues the delay WRITE through the endpoint with a
third transaction ID after both reads complete, reusing `PENDING`
with the pair reducer reset and the reused child driven directly,
so no new phase and no CPU resume-path change; WAIT resumes the
delay child, FAULT publishes nothing, and cancel/reset route by
pair-versus-delay child state. Pair traffic (reads and delay
write) always uses the endpoint by the dual precedent; there is
no MMR branch. The oracle vector is recomputed independently
before pinning: `MACD *AR5+, #0100h` with `B` = `00 0000 0000`,
`FRCT` = 0, `AR5` = `0100`, `mem[0100]` = `4000`,
`pmem[0100]` = `4000`, `mem[0101]` = `0000` yields `B` =
`00 1000 0000` with `T` = `4000`, `AR5` = `0101`, and
`mem[0101]` = `4000` (`0x4000` times `0x4000` equals
`0x10000000`); the FRCT lane yields `B` = `00 2000 0000`.
The five RED executable contracts are the four family proofs
`tests/unit/isa/catalog/test_macd_smem_metadata.c`
(CTest `ti_c54x.macd_smem_metadata`),
`tests/unit/isa/decode/test_macd_smem_decode.c`
(CTest `ti_c54x.macd_smem_decode`),
`tests/unit/isa/ops/multiply/test_macd_smem_ops.c`
(CTest `ti_c54x.macd_smem_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_macd_smem.c`
(CTest `ti_c54x.cpu_macd_smem`), plus the seam proof
`tests/unit/sequencer/instruction/test_data_program_delay_write.c`
(CTest `ti_c54x.data_program_delay_write`), plus the
`SUBSET` `FORM-T2-04-11` inventory binding. Projected
closure is 206 catalog rows, `ID_COUNT=206`, 17,393 lookup
entries, 205 bindings, family coverage 81/205 (39.512%), and
33/205 strict complete-form coverage; 379 evidence records, 623
requirements, and 498 tests (498 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.
