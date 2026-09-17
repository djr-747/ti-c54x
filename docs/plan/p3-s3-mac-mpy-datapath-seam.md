### P3-S3: MAC/MPY datapath seam (planned)

This immutable `(planned)` heading is the durable S3 FIT/ownership
anchor. S3 builds the multiply-accumulate datapath on the S2 repeat
seam: TI SPRU172C §4 `MAC[R]` (printed pp. 4-81–4-83) syntax 1
`MAC[R] Smem, src` executes `(Smem) x (T) + (src)` into `src`
(or `dst`), one program word, with opcode bits 15:10 `001010`
(R bit 9, S bit 8, I bit 7, Smem field 6:0); the operation is
affected by `FRCT` and `OVM` and affects `OVdst` (or `OVsrc`
when `dst` is unspecified); the `R` suffix rounds by adding
`0x8000` and clearing bits 15:0. Table 2-28 (p. 2-20) lists no
MAC/MPY-family instruction, so the Smem syntaxes are repeatable
by omission — the recorded basis for the future `ELIGIBLE`
classification and the S2 `HOLD`-tail replay. The TI Example 1
vector (`MAC *AR5+, A` with `A` = `00 0000 1000`, `T` =
`0400`, `mem[0100]` = `1234`, `FRCT` = 0 yielding `A` = `00
0048 E000`) is the seam's independent oracle.

The seam is a new pure module under `src/isa/ops/multiply/`
owning the checked/trusted multiply-accumulate core over a
copied raw40 accumulator, `u16` T, `u16` multiplicand, `FRCT` /
`OVM` snapshot bits, a rounding flag, and an add/sub/store
select: it stages accumulator plus overflow effects and owns no
addressing, catalog rows, CPU sequencing, or timing. Slices: S3a
lands the seam with no rows; S3b lands the `MAC` Smem row
(`FORM-T2-04-01`, Table 2-4 ordinal 1) with metadata, decode,
ops, CPU retirement, `ELIGIBLE` classification, and an `RPT
MAC` end-to-end replay; S3c lands `MPY` Smem
(`FORM-T2-03-01`); `MPYA`/`MAS`/`MACR` then follow in census
order. The RED executable contract for the seam is
`tests/unit/isa/ops/multiply/test_multiply_core.c` (CTest
`ti_c54x.multiply_core`), proving the TI Example 1 vector,
the `FRCT` shift, `R` rounding, `OVM` saturation with `OVdst`,
and unknown-T/unknown-src transactional stops. Deferred: dual
Xmem/Ymem syntax 2 (needs dual-operand addressing), `#lk`
syntaxes 3–4 (long-constant words), `MACA`/`MACD`/`MACP`
(accumulator-A/program-memory operands plus the delay seam),
the remaining wave-4 families, and any PMST product-shift
claim beyond the `FRCT` behavior the `MAC` section states.
