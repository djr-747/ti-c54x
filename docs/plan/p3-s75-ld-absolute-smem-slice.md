### P3-S75: LD absolute-Smem slice (implemented; local verification green)

`LD *(abs),dst` is TI SPRU172C section LD (`LD Smem,dst`,
opcode `0001 000D IAAA AAAA`, 1 base word, "add 1 word
for absolute addressing"): first `0x10F8` with D=0
(accumulator A, same polarity as the LD_SMEM leaf) and
I=1 with the `0x78` absolute selector, 16-bit absolute
DATA address word; execution loads the 16-bit word into
dst(15:0) with SXM sign extension and no status-bit
change. The staged loader1 run stops
`UNIMPLEMENTED_OPCODE` on it at `0xF17` (words `10F8
087F`, the MBOX0 poll: abs `0x87F` is the upload-header
MBOX the S71 peer fills). The `0x1080/0xFEE0` and
`0x10A8/0xFEF8` masks never match `0x10F8`, so no shadow
carve is needed. Absolute operands cannot repeat
(section 2 rule, same evidence as S70/S72-S74), so the
row marks `INELIGIBLE`.

Leaf design: new `EXACT` row `LD_SMEM_ABSOLUTE`
(`0x10F8`, 2 words) reusing the S74
first+D+abs16 layout set
(`ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16`), beside the
indirect leaf's `op_ld_smem` pair. The leaf gates SXM
knownness (LD_SMEM precedent) and stages the
established `ABSOLUTE_DATA` arm (kind 22, READ, sidecar
NONE, staged-zero value) with an accumulator-write
read-value completion (SXM widening plus C/TC
preserve, LD_SMEM completion precedent). Only the
evidenced absolute selector composes. New catalog ID
moves `ID_COUNT` 330 to 331 with pins;
`FORM-T2-19-02` binds the row `SUBSET` with
`ABSOLUTE_DATA`; populated first words 40011 to 40012
(second exact row on a previously unmapped word); no
MMR, knob, or timing changes.

RED contracts: new decode/ops/metadata/CPU tests
(`test_ld_smem_absolute_decode`,
`test_ld_smem_absolute`,
`test_ld_smem_absolute_metadata`,
`test_cpu_ld_smem_absolute`) plus the catalog-contract
row pin. Acceptance is the hosted repro advancing past
`0xF17` to the next explicit stop (`0xF844` at `0xF19`
is the predicted follower).

Out of scope: LD direct/indirect/long-offset family
(own slices as the boot reaches them), LD shift/TS
absolute (own slices), timing.
