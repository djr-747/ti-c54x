### P3-S81: OR absolute Smem slice (implemented; local verification green)

`OR *(abs),dst` is TI SPRU172C section OR (logical OR
with absolute addressing): first `0x1AF8` with S
selecting the destination accumulator (S=0 is A, OR_SMEM
precedent) and I=1 with the `0x78` absolute selector,
16-bit absolute DATA address word; execution ORs the
zero-extended word at abs full-width into dst with no
status-bit change. The staged loader1 run stops
`INVALID_STATE` on it at `0xF29` (words `1AF8 087E`,
`0x08F8` opens the next instruction): the `0x1AF8`
first word classifies into the 1-word OR_SMEM masked
row, whose leaf rejects mode 15. Only OR_SMEM covers
`0x1AF8`; the exact row sits before it (its `0xFE00`
mask spans the absolute cookie, unlike the LD `0xFEE0`
family mask) and wins by generation, so no shadow carve
is needed; direct/indirect Smem forms stay frozen. Absolute operands cannot repeat (section 2 rule,
same evidence as S70/S72-S80), so the row marks
`INELIGIBLE`.

Leaf design: new `EXACT` row `OR_SMEM_ABSOLUTE`
(`0x1AF8`, 2 words) reusing the S75/S78/S80 first+S+abs16
layout set (`ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16`:
destination S[8:8], I[7:7], selector A[6:0], absolute
word; 4 operands keep the trusted packet simple). The
leaf gates destination-accumulator knownness (OR_SMEM
precedent: the family consumes ACCUMULATORS only, no
SXM/OVM) and stages the existing `ABSOLUTE_DATA` WORD16
READ arm with a zero-extending OR completion (C/TC
preserved). Only the evidenced absolute selector
composes. New catalog ID moves `ID_COUNT` 336 to 337
with pins; `FORM-T2-08-01` binds the row `SUBSET` with
`ABSOLUTE_DATA`; no MMR, knob, or timing changes.
Unlike S80 (whose cookie was uncovered), the exact row
reclaims `0x1AF8` from the masked row, so populated
first words stay 40272.

RED contracts: new decode/ops/metadata/CPU tests
(`test_or_smem_absolute_decode`,
`test_or_smem_absolute`,
`test_or_smem_absolute_metadata`,
`test_cpu_or_smem_absolute`) plus the catalog-contract
row pin. Acceptance is the hosted repro advancing past
`0xF29` to the next explicit stop (`0x08F8` at `0xF2B`
is the predicted follower, SUB_SMEM-shaped).

Out of scope: OR direct/indirect/long-offset Smem family
(own slices as the boot reaches them), OR absolute
changes (S81 frozen), timing.
