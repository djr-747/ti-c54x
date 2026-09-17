### P3-S3p: MACA T,src register-only slice (implemented; local verification green)

This immutable `(implemented; local verification green)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `MACA[R] T, src [, dst]` syntax-2 form
from TI SPRU172C §MACA[R] (printed p. 4-85, opcode
`1111 01SD 1000 100R`) is implemented as one catalog row:
`MACA_T` with value `0xF488`, mask `0xFCFE` (bits 15:10
`111101`, S bit 9, D bit 8, R bit 0, 8 encodings). The R bit
carries the MACAR T,src rounding variant on the shared row by the
recorded S3c/S3f R-sharing convention, so `FORM-T2-04-10` stays
`UNIMPLEMENTED`. The row uses a new `SOURCE_DESTINATION_ROUND`
layout (S/D/R selects), resources
`ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM`, and repeatability
`ELIGIBLE` (Class 3A/3B single-repeatable classes with no Table 2-28
entry). Word 0 is `1111_01SD_1000_100R`: register-only, no memory
effect and no T load; the completion runs `(src)` plus `(T)` times
`A(32-16)` into the D-selected accumulator through the existing
multiply core with ADD select and R rounding; T, A, and the
unselected accumulator are preserved, C/TC ride through unchanged,
and `OVdst` follows the core for dst (which is src when dst is not
specified). T, A, and src must be known. The TI MACA Example 2
vector (`MACA T, B, B` with `A` = `00 1234 0000`, `B` =
`00 0002 0000`, `T` = `0444`, `FRCT` = 1 yielding `B` =
`00 009D 4BA0` with `T` unchanged; `0x0444` times `0x1234`
equals `0x4DA5D0`, FRCT-doubled to `0x9B4BA0`, plus `0x00020000`
equals `0x009D4BA0`, recomputed independently before pinning) and
the MACAR Example 4 rounding lane (R = 1 yields `B` =
`00 009D 0000`) are the oracles. The existing TI MACA §MACA[R]
record spans both syntaxes, so no new TI record is needed by the
recorded S3f convention — only the activation, six owner-split
requirements, and five test records. The five RED executable
contracts are
`tests/unit/isa/decode/test_source_destination_round_layout.c`
(CTest `ti_c54x.source_destination_round_layout`),
`tests/unit/isa/catalog/test_maca_t_metadata.c`
(CTest `ti_c54x.maca_t_metadata`),
`tests/unit/isa/decode/test_maca_t_decode.c`
(CTest `ti_c54x.maca_t_decode`),
`tests/unit/isa/ops/multiply/test_maca_t_ops.c`
(CTest `ti_c54x.maca_t_ops`), and
`tests/unit/cpu/isa/multiply/test_cpu_maca_t.c`
(CTest `ti_c54x.cpu_maca_t`), plus the `SUBSET`
`FORM-T2-04-08` inventory binding. MACD and MACP remain later
slices (program-memory operands plus the delay seam). Projected
closure is 204 catalog rows, `ID_COUNT=204`, 16,369 lookup
entries, 203 bindings, family coverage 79/205 (38.537%), and
33/205 strict complete-form coverage; 375 evidence records, 609
requirements, and 486 tests (486 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.
