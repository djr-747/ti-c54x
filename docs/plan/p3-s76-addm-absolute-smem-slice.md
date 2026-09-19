### P3-S76: ADDM absolute-Smem slice (implemented; local verification green)

`ADDM #lk,*(abs)` is TI SPRU172C section ADDM (`0110
1011 I addr7`, add long immediate to Smem, nonrepeatable):
first `0x6BF8` with I=1 and the `0x78` absolute selector,
16-bit immediate word, 16-bit absolute DATA address word;
execution `*(abs) += lk` with OVM/SXM saturation and
sticky C/OVA commit. The staged loader1 run stops
`UNIMPLEMENTED_OPCODE` on it at `0xF1F` (words `6BF8
0013 0800`). The `ADDM_SMEM_MOD0_3` mask (`0x6B80 /
0xFFE0`) never matches `0x6BF8`, so no shadow carve is
needed. Absolute operands cannot repeat (section 2 rule,
same evidence as S70/S72-S76), so the row marks
`INELIGIBLE`.

Leaf design: new `EXACT` row `ADDM_LK_SMEM_ABSOLUTE`
(`0x6BF8`, 3 words) reusing the S70 first+I/abs/lk
layout set (`INDIRECT_ADDR7_ABSOLUTE16_LK`), beside the
indirect leaf's atomic-logical pair. The leaf gates
OVM/SXM knownness (ADDM precedent) and stages a new
`ATOMIC_ABSOLUTE` arm (kind 28: absolute DATA address,
staged `#lk` immediate, RMW ADD, no completion): kind
22 forbids RMW transforms and staged READ values, so
the pair/root reuse stops at the layout. The
established WORD16 RMW engine drives the
read/transform/writeback from the resolved absolute
address with the frozen OVM/SXM modes (new resolver
module plus candidate shape/liveness, mirroring the
S74 LONG32 arm). Only the evidenced absolute selector
composes. New catalog ID moves `ID_COUNT` 331 to 332
with pins; `FORM-T2-01-12` binds the row `SUBSET`
with `ABSOLUTE_DATA`; populated first words 40012 to
40013
(third exact row on a previously unmapped word); no
MMR, knob, or timing changes.

RED contracts: new decode/ops/metadata/CPU tests
(`test_addm_lk_smem_absolute_decode`,
`test_addm_lk_smem_absolute`,
`test_addm_lk_smem_absolute_metadata`,
`test_cpu_addm_lk_smem_absolute`) plus the
catalog-contract row pin. Acceptance is the hosted
repro advancing past `0xF1F` to the next explicit
stop.

Out of scope: ADDM direct/indirect/long-offset Smem
(own slices as the boot reaches them), ANDM/ORM/XORM
absolute (own slices), timing.
