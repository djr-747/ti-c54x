### P3-S3n: MPYU unsigned-multiply vertical slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `MPYU Smem,dst` form from TI SPRU172C
§MPYU (printed p. 4-106, opcode `0010 010D IAAA AAAA`,
dual-sourced against the TI SPRU307A functional-overview opcode
summary `0010 010D IAAA AAAA`) is implemented as one catalog row:
`MPYU_SMEM` with value `0x2400`, mask `0xFE00` (bits 15:9
`0010010`, D bit 8, I bit 7, Smem field 6:0, 512 encodings). The
row reuses the S3k `DESTINATION_INDIRECT_ADDR7` layout (D select
rides field 0, the same position as the S3m S select),
resources `ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`, and
repeatability `ELIGIBLE`. Word 0 is `0010_010D_IAAA_AAAA`: T is
preset and read (never loaded — the TI example leaves T unchanged),
and the completion runs `uns(T)` times `uns(Smem)` into the
D-selected accumulator through the multiply core with a new
unsigned-operands select, STORE select, and no rounding; C/TC are
preserved and `OVdst` follows the core. The unsigned select lives
in the shared core (not the leaf) so the FRCT shift and the OVM
saturation path stay single-sourced; the product is non-negative,
so saturation keeps the existing positive direction. The TI MPYU
Example 1 vector (`MPYU *AR0-, A` with `A` = `FF 8000 0000`,
`T` = `4000`, `FRCT` = 0, `AR0` = `1000`, `mem[1000]` = `FE00`
yielding `A` = `00 3F80 0000` with `T` = `4000` and `AR0` =
`0FFF`; `0x4000` times `0xFE00` equals `0x3F800000`, recomputed
independently before pinning) is the oracle. Table 2-28 lists no
MPYU instruction, so the row is repeatable by omission on the
recorded S3 omission basis. The four RED executable contracts are
`tests/unit/isa/catalog/test_mpyu_smem_metadata.c`
(CTest `ti_c54x.mpyu_smem_metadata`),
`tests/unit/isa/decode/test_mpyu_smem_decode.c`
(CTest `ti_c54x.mpyu_smem_decode`),
`tests/unit/isa/ops/multiply/test_mpyu_smem_ops.c`
(CTest `ti_c54x.mpyu_smem_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_mpyu_smem.c`
(CTest `ti_c54x.cpu_mpyu_smem`), plus unsigned-mode lanes in
`tests/unit/isa/ops/multiply/test_multiply_core.c` and the
`SUBSET` `FORM-T2-03-08` inventory binding. Projected closure
is 202 catalog rows, `ID_COUNT=202`, 15,849 lookup entries, 201
bindings, family coverage 77/205 (37.561%), and 33/205 strict
complete-form coverage; 372 evidence records, 597 requirements, and
476 tests (476 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.
