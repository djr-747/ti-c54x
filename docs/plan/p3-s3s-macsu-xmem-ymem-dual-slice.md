### P3-S3s: MACSU Xmem,Ymem dual slice (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The one-word `MACSU Xmem, Ymem, src` syntax-2 form
from TI SPRU172C §MACSU (printed p. 4-91, word 0
`1010_011S_XXXX_YYYY`) is implemented as nine catalog rows (X field 7:4, Y field 3:0):
`MACSU_DUAL_Mx_My` with values `0xA800|xmode<<6|ymode<<2`
(`0xA800`, `0xA804`, `0xA808`, `0xA840`, `0xA844`, `0xA848`,
`0xA880`, `0xA884`, `0xA888`) under mask `0xFECC` (bits 15:9
`1010011`, S bit 8, X/Y MOD bits 7:6/3:2 fixed per row, Xar/Yar
bits 5:4/1:0 free; 32 S/Xar/Yar encodings per row, 288 total).
The rows reuse the `DUAL_DESTINATION_XAR_YAR` layout (D[8]
positionally is the S select; no new layout, no trusted-capacity
pressure: 3 operands of widths 1/2/2) with `UNRESOLVED`
repeatability and
`ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_FRCT_OVM` resources.
Checked and trusted leaves stage dual-indirect MOD0-MOD2 reads
with read-phase X-side T load (`T` takes the `Xmem` word) and,
on pair completion, one accumulator write of `(src)` plus
`unsigned(Xmem)` times `signed(Ymem)` into the S-selected
accumulator (S selects the shared src/dst accumulator) through
the multiply core with ADD select, FRCT/OVM handling, and no
rounding. The mixed-sign product needs a core seam extension:
one additive `unsigned_t_only` select on the multiply operands
(`T` unsigned, multiplicand signed; the existing
`unsigned_operands` both-unsigned select keeps precedence for
`MPYU`, all-signed leaves set the new select false explicitly)
with the FRCT shift, accumulate, saturation, and overflow
disposition unchanged. The oracle vector is recomputed
independently before pinning: `MACSU *AR4+, *AR5+, A` with `A` =
`00 0000 0000`, `FRCT` = 0, `AR4` = `0100`, `AR5` = `0200`,
`mem[0100]` = `4000`, `mem[0200]` = `4000` yields `A` =
`00 1000 0000` with `T` = `4000`, `AR4` = `0101`, `AR5` =
`0201` (`16384` times `16384` equals `0x10000000`); the FRCT
lane yields `A` = `00 2000 0000`; the mixed-sign lane
(`X` = `8765`, `Y` = `C000`) pins the unsigned-X/signed-Y
product (`34661` times `-16384`) and fails under either
uniform-signedness reading. The four RED executable contracts
are the extended core proof
`tests/unit/isa/ops/multiply/test_multiply_core.c` (CTest
`ti_c54x.multiply_core`, mixed-sign product lanes), the family
proofs `tests/unit/isa/ops/multiply/test_macsu_dual_ops.c`
(CTest `ti_c54x.macsu_dual_ops`) and
`tests/unit/cpu/isa/multiply/test_cpu_macsu_dual.c` (CTest
`ti_c54x.cpu_macsu_dual`), plus the `SUBSET` `FORM-T2-04-13`
inventory binding. Projected closure is 215 catalog rows,
`ID_COUNT=215`, 17,681 lookup entries, 214 bindings, family
coverage 82/205 (40.000%), and 33/205 strict complete-form
coverage; 381 evidence records, 627 requirements, and 500
tests (500 `PASSING`, 0 `PLANNED`). These numbers are
projections until the slice lands.
