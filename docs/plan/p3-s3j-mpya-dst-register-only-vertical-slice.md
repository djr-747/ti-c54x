### P3-S3j: MPYA-dst register-only vertical slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `MPYA dst` form from TI SPRU172C §MPYA
(printed pp. 4-104–4-105), dual-sourced against TI SPRU179C §MPYA
(printed pp. 4-106–4-107, algebraic `dst = T * hi(A)`), is implemented
as `MPYA_DST` with value `0xF48C`, mask `0xFEFF`, D bit 8, two
encodings, the reused `MAX_MIN_ACC` layout, resources
`ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`, and repeatability
`ELIGIBLE`. Word 0 is `1111_010D_1000_1100`: D=0 writes A, D=1 writes
B. A single `MASKED` row carries both encodings (two exact rows under
mask `0xFEFF` would collide on the unmasked D bit); the destination
rides the word through the reused single-destination-bit layout,
following the `MAX_ACC`/`MIN_ACC` compare-family precedent — no new
layout, resource, effect, address, state, CPU, public API, or
source-manifest seam is added. Execution is `(T) x (A(32-16))` into
`dst` through the existing multiply core with STORE select and no
rounding (`hi(A)` feeds the core exactly as the S3d Smem leaf does,
with T and multiplicand roles swapped); T and source A are unchanged,
C/TC are preserved, and `OVdst` follows the core overflow. The TI
Example 2 vector (`MPYA B` with `A` = `FF 8765 1111`, `T` = `4567`,
`FRCT` = 0 yielding `B` = `FF DF4D B2A3`) is the independent oracle.
Table 2-28 lists no MPYA-family instruction, so syntax 2 is repeatable
by omission on the same recorded basis as the S3d `ELIGIBLE`
classification. The four RED executable contracts are
`tests/unit/isa/catalog/test_mpya_dst_metadata.c`
(CTest `ti_c54x.mpya_dst_metadata`),
`tests/unit/isa/decode/test_mpya_dst_decode.c`
(CTest `ti_c54x.mpya_dst_decode`),
`tests/unit/isa/ops/multiply/test_mpya_dst_ops.c`
(CTest `ti_c54x.mpya_dst_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_mpya_dst.c`
(CTest `ti_c54x.cpu_mpya_dst`), plus the `SUBSET`
`FORM-T2-03-07` inventory binding. Projected closure is 197 catalog
rows, `ID_COUNT=197`, 13,799 lookup entries, 196 bindings, family
coverage 72/205 (35.122%), and 33/205 strict complete-form coverage;
363 evidence records, 575 requirements, and 459 tests (459 `PASSING`,
0 `PLANNED`). These numbers are projections until the slice lands.
