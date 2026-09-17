### P3-S3q: MACP Smem,Pmad program-coefficient slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
evidence freeze. The two-word `MACP Smem, pmad [, dst]` syntax-1 form
from TI SPRU172C §MACP (printed p. 4-89, word 0
`0111_100S_IAAA_AAAA` with word 1 `pmad`) is implemented as one
catalog row: `MACP_SMEM` with value `0x7800`, mask `0xFE00`
(bits 15:9 `0111100`, S bit 8, I bit 7, Smem field 6:0, 512
encodings) and `program_word_count` 2. The row uses a new
`DESTINATION_INDIRECT_ADDR7_PMAD16` layout (S/I/addr7 from word 0
plus verbatim `pmad16` from word 1, mirroring the S3k
destination/indirect/addr7 set with the S3 branch `pmad16` second
word), resources `ACCUMULATORS_STATUS_FRCT_OVM` (the MPYA_SMEM
precedent: T is written, not read, and there is no R rounding
bit), and repeatability `UNRESOLVED` (two-word forms stay
unresolved by the STM precedent). Word 0 is
`0111_100S_IAAA_AAAA`: the Smem word is read from DATA memory,
the `pmad` word names a PROGRAM-memory coefficient read, T takes
the Smem word in the read phase, and the pair completion runs
`(dst)` plus `(Smem)` times `(pmad)` into the S-selected
accumulator through the existing multiply core with ADD select
and no rounding; C/TC are preserved and `OVdst` follows the
core. The S polarity follows MAC_SMEM (`0` selects A,
`1` selects B). Smem covers direct page-0 and standard-indirect
MOD0--MOD3 with exotic-mode deferral; the PROGRAM leg is an
absolute 16-bit program-word address with no post-update. The new
DATA+PROGRAM pair seam exists for this slice: address kind
`DATA_PROGRAM_PAIR_WORD16` stages the Smem leg through the
generic single-leg resolver and the PROGRAM leg as an absolute
read, the `data_program_word16_read_transaction` reducer issues
the DATA read then the PROGRAM read through one reused child
transaction with the shared `complete_dual` shape (`x` is Smem,
`y` is the coefficient), and the `memory_instruction_data_program`
root (with its commit and candidate-validate owners) publishes
the single Smem post-access commit with a zeroed PROGRAM commit
plus the read-phase X-side T load. The MACD delay sidecar
(Smem+1 write after pair completion) stays a later slice, so
`FORM-T2-04-11` stays `UNIMPLEMENTED`. The oracle vector is
recomputed independently before pinning: `MACP *AR5+, #0100h`
with `B` = `00 0000 0000`, `FRCT` = 0, `AR5` = `0100`,
`mem[0100]` = `4000`, `pmem[0100]` = `4000` yields `B` =
`00 1000 0000` with `T` = `4000` and `AR5` = `0101`
(`0x4000` times `0x4000` equals `0x10000000`); the FRCT lane
yields `B` = `00 2000 0000`. The seven RED executable contracts
are the five family proofs
`tests/unit/isa/decode/test_destination_indirect_addr7_pmad16_layout.c`
(CTest `ti_c54x.destination_indirect_addr7_pmad16_layout`),
`tests/unit/isa/catalog/test_macp_smem_metadata.c`
(CTest `ti_c54x.macp_smem_metadata`),
`tests/unit/isa/decode/test_macp_smem_decode.c`
(CTest `ti_c54x.macp_smem_decode`),
`tests/unit/isa/ops/multiply/test_macp_smem_ops.c`
(CTest `ti_c54x.macp_smem_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_macp_smem.c`
(CTest `ti_c54x.cpu_macp_smem`), plus the two seam proofs
`tests/unit/sequencer/instruction/test_data_program_word16_read_transaction.c`
(CTest `ti_c54x.data_program_word16_read_transaction`) and
`tests/unit/sequencer/instruction/test_data_program_memory_instruction_transaction.c`
(CTest `ti_c54x.data_program_memory_instruction_transaction`),
plus the `SUBSET` `FORM-T2-04-12` inventory binding. Projected
closure is 205 catalog rows, `ID_COUNT=205`, 16,881 lookup
entries, 204 bindings, family coverage 80/205 (39.024%), and
33/205 strict complete-form coverage; 377 evidence records, 617
requirements, and 493 tests (493 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.
