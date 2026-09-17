### P3-S3l: SQUR-Adst register-only vertical slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `SQUR A,dst` form from TI SPRU172C §SQUR
(printed p. 4-162), dual-sourced against TI SPRU179C §Square
(printed p. 4-164, algebraic `B = square(hi(A))`), is implemented as
`SQUR_ADST` with value `0xF48D`, mask `0xFEFF`, D bit 8, two
encodings, the reused `MAX_MIN_ACC` layout, resources
`ACCUMULATORS_STATUS_FRCT_OVM`, and repeatability `ELIGIBLE`. Word 0
is `1111_010D_1000_1101`: D=0 writes A, D=1 writes B. A single
`MASKED` row carries both encodings (two exact rows under mask
`0xFEFF` would collide on the unmasked D bit), following the S3j
`MPYA_DST` precedent — no new layout, resource, effect, address,
state, CPU, public API, or source-manifest seam is added. Execution
is `(A(32-16)) x (A(32-16))` into dst through the existing multiply
core with STORE select and no rounding; T is unaffected, C/TC are
preserved, and `OVdst` follows the core. The SPRU179C Example 2
vector (`B = square(hi(A))` with `A` = `00 000F 0000`, `FRCT` = 1
yielding `B` = `00 0000 01C2`, i.e. 15 squared FRCT-shifted) is the
independent oracle. Table 2-28 lists no SQUR-family instruction, so
syntax 2 is repeatable by omission on the recorded S3 omission basis.
The four RED executable contracts are
`tests/unit/isa/catalog/test_squr_adst_metadata.c`
(CTest `ti_c54x.squr_adst_metadata`),
`tests/unit/isa/decode/test_squr_adst_decode.c`
(CTest `ti_c54x.squr_adst_decode`),
`tests/unit/isa/ops/multiply/test_squr_adst_ops.c`
(CTest `ti_c54x.squr_adst_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_squr_adst.c`
(CTest `ti_c54x.cpu_squr_adst`), plus the `SUBSET`
`FORM-T2-03-10` inventory binding. The `test_mpya_dst_decode.c`
UNKNOWN lane that pins `0xF48D`/`0xF58D` today moves to the new row
as the recorded W57→W58 cross-slice update. Projected closure is 199
catalog rows, `ID_COUNT=199`, 14,313 lookup entries, 198 bindings,
family coverage 74/205 (36.098%), and 33/205 strict complete-form
coverage; 367 evidence records, 586 requirements, and 468 tests (468
`PASSING`, 0 `PLANNED`). These numbers are projections until the
slice lands.
