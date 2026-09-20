### P3-S83: STH absolute Smem slice (implemented; local verification green)

`STH *(abs),src` is TI SPRU172C section STH (store
with absolute addressing): first `0x82F8` with S
selecting the source accumulator (S=0 is A, STH_SMEM
precedent) and I=1 with the `0x78` absolute selector,
16-bit absolute DATA address word; execution stores
src(31:16) to the word at abs with no status-bit
change. The staged loader1 run stops
`UNIMPLEMENTED_OPCODE` on it at `0xF2D` (words
`82F8 087D`; the follower past it is not yet read):
no `0x82xx` absolute row exists yet. The STH `0xFEE0`
family mask never matches `0x82F8` (S75/S80 pattern),
so the row sits with the absolute group and no shadow
carve is needed; direct/indirect Smem forms stay
deferred. Absolute operands cannot repeat (section 2
rule, same evidence as S70/S72-S82), so the row marks
`INELIGIBLE`.

Leaf design: new `EXACT` row `STH_SMEM_ABSOLUTE`
(`0x82F8`, 2 words) reusing the absolute
ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 layout set
(source S[8:8], I[7:7], selector A[6:0], absolute
word; 4 operands keep the trusted packet simple). The
leaf gates source-accumulator knownness (STH_SMEM
precedent: the family consumes ACCUMULATORS only) and
stages the existing `ABSOLUTE_DATA` WORD16 WRITE arm
fire-and-forget (staged src high word rides
verbatim, sidecar NONE, complete NULL — ST #lk
precedent). Only the evidenced absolute selector
composes. New catalog ID moves `ID_COUNT` 338 to 339
with pins; `FORM-T2-20-05` binds the row `SUBSET`
with `ABSOLUTE_DATA`; no MMR, knob, or timing
changes. The cookie was uncovered, so populated first
words move 40272 to 40273 (S75/S80 pattern, unlike
S81/S82).

RED contracts: new decode/ops/metadata/CPU tests
(`test_sth_smem_absolute_decode`,
`test_sth_smem_absolute`,
`test_sth_smem_absolute_metadata`,
`test_cpu_sth_smem_absolute`) plus the catalog-contract
row pin. Acceptance is the hosted repro advancing past
`0xF2D` to the next explicit stop.

Out of scope: STH direct/indirect/long-offset Smem
family (own slices as the boot reaches them), STH
absolute changes (S83 frozen), timing.
