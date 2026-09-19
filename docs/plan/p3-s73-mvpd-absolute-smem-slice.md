### P3-S73: MVPD absolute-Smem slice (planned)

`MVPD pmad,*(abs)` is TI SPRU172C section MVPD (p. 4-117)
with absolute Smem addressing (3 words: first `0x7CF8`
with I=1 and the `0x78` absolute selector, 16-bit
absolute DATA address, 16-bit pmad; execution
`Pmem[pmad] -> (Smem)`, no status bits, all vendored
in-repo). The staged loader1 run stops `INVALID_STATE`
on it at `0xF12` (words `7CF8 0802 FF87`): the
`MVPD_SMEM` row owns only the I=1 indirect half. Absolute
operands cannot repeat (section 2 rule, same evidence as
S70/S72), so the row marks `INELIGIBLE`.

Leaf design: new `EXACT` row `MVPD_SMEM_ABSOLUTE`
(`0x7CF8`, 3 words) reusing the S72
first+abs16+pmad16 layout set
(`INDIRECT_ADDR7_ABSOLUTE16_PMAD`), beside the
indirect leaf's `op_mvpd` pair. The Smem leg stages the
established `SMEM_ABSOLUTE_PROGRAM_PAIR` arm (kind 26)
with `WRITE` access (the access names the Smem-leg
direction, MVPD convention): no new effect kind, no
sequencer changes — the pair prepare/root already run
the PROGRAM leg first and the Smem leg second for
`WRITE` access with the PROGRAM observation replacing
the zero template. New catalog ID moves `ID_COUNT` 328
to 329 with pins; `FORM-T2-26-04` binds the row
`SUBSET` with `ABSOLUTE_DATA` (same token as the S70/S72
absolute rows); no MMR, knob, or timing changes.

RED contracts: new decode/ops/metadata/CPU tests
(`test_mvpd_smem_absolute_decode`,
`test_mvpd_smem_absolute`,
`test_mvpd_smem_absolute_metadata`,
`test_cpu_mvpd_smem_absolute`) plus the catalog-contract
row pin. Acceptance is the hosted repro advancing past
`0xF12` to the next explicit stop.

Out of scope: long-offset indirect, LD/STL/STH/ADDM
absolute (own slices as the boot reaches them), timing.
