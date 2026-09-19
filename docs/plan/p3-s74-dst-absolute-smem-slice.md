### P3-S74: DST absolute-Smem slice (planned)

`DST src,*(abs)` is TI SPRU172C section DST ("Store
Accumulator in Long Word", opcode `0100 111S IAAA AAAA`,
1 base word, "add 1 word for absolute addressing"):
first `0x4EF8` with S=0 (accumulator A, same polarity as
the MOD13 leaf: source 0 is A) and I=1 with the `0x78`
absolute selector, 16-bit absolute DATA address word;
execution `(A(31:0)) -> Lmem`, no status bits (TI SPRU179
`dbl(Lmem) = src`). The staged loader1 run stops
`UNIMPLEMENTED_OPCODE` on it at `0xF71` (words `4EF8
0060`, `0xFC00` opens the next instruction): no `0x4E00`
family row exists yet. The oracle disassembles the same
first word as `dbl(*(abs)) = A` (staged `dsp_full.bin`
`0x45BB: 4EF8 00B4`), confirming S=0 is A. Absolute
operands cannot repeat (section 2 rule, same evidence as
S70/S72/S73), so the row marks `INELIGIBLE`.

Leaf design: new `EXACT` row `DST_SMEM_ABSOLUTE`
(`0x4EF8`, 2 words) with a new first+S+abs16 layout set
(`ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16`: source S[8:8],
I[7:7], selector A[6:0], absolute word; 4 operands keep
the trusted packet simple). The leaf gates accumulator
knownness plus SXM/SST knownness and applies the MOD13
leaf's saturation (SST-gated SXM-signed/unsigned
32-bit clamp, low 32 bits), staging a new
`LONG32_ABSOLUTE_STORE` arm (kind 27: absolute DATA
address, MSW/LSW payload halves, WRITE access) through
the established long32 pair engine with an absolute-leg
snapshot branch. Only the evidenced absolute selector
composes. New catalog ID moves `ID_COUNT` 329 to 330
with pins; `FORM-T2-20-01` binds the row `SUBSET`
with `ABSOLUTE_DATA`; no MMR, knob, or timing changes.

RED contracts: new decode/ops/metadata/CPU tests
(`test_dst_smem_absolute_decode`,
`test_dst_smem_absolute`,
`test_dst_smem_absolute_metadata`,
`test_cpu_dst_smem_absolute`) plus the catalog-contract
row pin. Acceptance is the hosted repro advancing past
`0xF71` to the next explicit stop (`0xFC00` at `0xF73`
is the predicted follower).

Out of scope: DST direct/indirect/long-offset Smem
family (own slices as the boot reaches them), DLD
absolute (own slice), timing.
