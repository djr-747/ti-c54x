### P3-S72: MVDP absolute-Smem slice (implemented; local verification green)

`MVDP *(abs),pmad` is TI SPRU172C section MVDP (p. 4-111)
with absolute Smem addressing (3 words: first `0x7DF8`
with I=1 and the `0x78` absolute selector, 16-bit
absolute DATA address, 16-bit pmad; "add 1 word for
absolute addressing", execution `(Smem) -> Pmem[pmad]`,
no status bits, all vendored in-repo). The staged
loader1 run stops `INVALID_STATE` on it at `0xF0F`
(words `7DF8 0803 FF87`): the `MVDP_SMEM` row owns only
the I=1 indirect half. Absolute operands cannot repeat
(section 2 rule, same evidence as S70), so the row marks
`INELIGIBLE`.

Leaf design: new `EXACT` row `MVDP_SMEM_ABSOLUTE`
(`0x7DF8`, 3 words) with a new first+abs16+pmad16 layout
set (`INDIRECT_ADDR7_ABSOLUTE16_PMAD`), beside the
indirect leaf's `op_mvdp` pair (same fire-and-forget
DATA-read/PROGRAM-write pair shape, no-T/no-completion).
The Smem leg rides a new `SMEM_ABSOLUTE_PROGRAM_PAIR`
effect arm (absolute DATA address + pmad) through the
established pair engine: same route and child kind, with
an absolute-leg snapshot branch synthesizing the
`ABSOLUTE_DATA` first-leg spec the MVDM/MVMD rows
established. New catalog ID moves `ID_COUNT` 327 to 328
with pins; `FORM-T2-26-04` binds the row `SUBSET` with
`ABSOLUTE_DATA` (same token as the S70 absolute row);
no MMR, knob, or timing changes.

RED contracts: new decode/ops/metadata/CPU tests
(`test_mvdp_smem_absolute_decode`,
`test_mvdp_smem_absolute`,
`test_mvdp_smem_absolute_metadata`,
`test_cpu_mvdp_smem_absolute`) plus the catalog-contract
row pin. Acceptance is the hosted repro advancing past
`0xF0F` to the next explicit stop (`MVPD` absolute at
`0xF12` is the predicted follower).

Out of scope: long-offset indirect, MVPD absolute (own
slice), LD/STL/STH/ADDM absolute (own slices as the boot
reaches them), timing.
