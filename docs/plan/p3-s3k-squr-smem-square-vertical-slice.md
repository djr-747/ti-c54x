### P3-S3k: SQUR-Smem square vertical slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `SQUR Smem,dst` form from TI SPRU172C
§SQUR (printed pp. 4-160–4-161), dual-sourced against the TI SPRU179C
algebraic restatement, is implemented as `SQUR_SMEM` with value
`0x2600`, mask `0xFE00`, D bit 8, I bit 7, Smem field 6:0, 512
encodings, a new `DESTINATION_INDIRECT_ADDR7` layout (D plus
indirect/addr7 with no round or select bits — the shared
round/select/indirect/addr7 layout would mislabel fixed bit 9 as
rounding), resources `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`, and
repeatability `ELIGIBLE`. Word 0 is `0010_011D_IAAA_AAAA`: the Smem
word loads T in the read phase and feeds both multiplier inputs, so
pair completion runs `(Smem) x (Smem)` into dst through the existing
multiply core with STORE select and no rounding; C/TC are preserved
and `OVdst` follows the core. The TI Example 1 vector (`SQUR 30, B`
with `mem[031E]` = `000F`, `FRCT` = 0 yielding `B` = `00 0000 00E1`
and `T` = `000F`; 15 squared is 225) is the independent oracle. Table 2-28 lists no
SQUR-family instruction, so syntax 1 is repeatable by omission on the
recorded S3 omission basis. The five RED executable contracts are
`tests/unit/isa/catalog/test_squr_smem_metadata.c`
(CTest `ti_c54x.squr_smem_metadata`),
`tests/unit/isa/decode/test_squr_smem_decode.c`
(CTest `ti_c54x.squr_smem_decode`),
`tests/unit/isa/ops/multiply/test_squr_smem_ops.c`
(CTest `ti_c54x.squr_smem_ops`),
`tests/unit/cpu/isa/multiply/test_cpu_squr_smem.c`
(CTest `ti_c54x.cpu_squr_smem`), and the layout proof
`tests/unit/isa/decode/test_destination_indirect_addr7_layout.c`
(CTest `ti_c54x.destination_indirect_addr7_layout`), plus the `SUBSET`
`FORM-T2-03-09` inventory binding. Syntax 2 (`SQUR A,dst`,
`0xF48D`/`0xF58D`) stays `UNIMPLEMENTED` for its own register-only
slice, which will update the `test_mpya_dst_decode.c` UNKNOWN lane
that pins those words today. Projected closure is 198 catalog rows,
`ID_COUNT=198`, 14,311 lookup entries, 197 bindings, family coverage
73/205 (35.610%), and 33/205 strict complete-form coverage; 365
evidence records, 581 requirements, and 464 tests (464 `PASSING`, 0
`PLANNED`). These numbers are projections until the slice lands.
