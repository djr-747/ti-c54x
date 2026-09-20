### P3-S82: SUB absolute Smem slice (implemented; local verification green)

`SUB *(abs),src` is TI SPRU172C section SUB syntax 1
(subtract with absolute addressing): first `0x08F8`
with S selecting the source accumulator (S=0 is A,
SUB_SMEM precedent) and I=1 with the `0x78` absolute
selector, 16-bit absolute DATA address word; execution
computes src minus the SXM-widened word at abs with
OVM saturation on signed-32 overflow, C written from
the bit-32 borrow (set means no borrow), TC preserved,
OV set on overflow. The staged loader1 run stops
`INVALID_STATE` on it at `0xF2B` (words `08F8 0881`,
`0x82F8` opens the next instruction): the `0x08F8`
first word classifies into the 1-word SUB_SMEM masked
row, whose leaf rejects mode 15. Only SUB_SMEM covers
`0x08F8`; the exact row sits before it (its `0xFE00`
mask spans the absolute cookie, S81 precedent) and
wins by generation, so no shadow carve is needed;
direct/indirect Smem forms stay frozen. Absolute
operands cannot repeat (section 2 rule, same evidence
as S70/S72-S81), so the row marks `INELIGIBLE`.

Leaf design: new `EXACT` row `SUB_SMEM_ABSOLUTE`
(`0x08F8`, 2 words) reusing the absolute
ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 layout set
(select S[8:8], I[7:7], selector A[6:0], absolute
word; 4 operands keep the trusted packet simple). The
leaf gates source-accumulator, SXM, and OVM knownness
(SUB_SMEM precedent: the family consumes
ACCUMULATORS_STATUS_SXM_OVM) and stages the existing
`ABSOLUTE_DATA` WORD16 READ arm with the SUB syntax-1
completion (SXM-widened subtrahend, OVM saturation,
C from no-borrow, TC preserved, OV on overflow).
Only the evidenced absolute selector composes. New
catalog ID moves `ID_COUNT` 337 to 338 with pins;
`FORM-T2-02-01` (SUB_SMEM family form) binds the row
`SUBSET` with `ABSOLUTE_DATA`; no MMR, knob, or timing
changes. Like S81, the exact word reclaims its lookup
pin, so populated first words stay 40272.

RED contracts: new decode/ops/metadata/CPU tests
(`test_sub_smem_absolute_decode`,
`test_sub_smem_absolute`,
`test_sub_smem_absolute_metadata`,
`test_cpu_sub_smem_absolute`) plus the catalog-contract
row pin. Acceptance is the hosted repro advancing past
`0xF2B` to the next explicit stop (`0x82F8` at `0xF2D`
is the predicted follower, currently UNKNOWN).

Out of scope: SUB direct/indirect/long-offset Smem
family (own slices as the boot reaches them), SUB
absolute changes (S82 frozen), timing.
