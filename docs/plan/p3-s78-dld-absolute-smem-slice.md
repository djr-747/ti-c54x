### P3-S78: DLD absolute-Smem slice (implemented; local verification green)

`DLD *(abs),dst` is TI SPRU172C section DLD ("Load
Accumulator with Long Word", opcode `0101 011D IAAA
AAAA`, 1 base word, "add 1 word for absolute
addressing"): first `0x56F8` with D=0 (accumulator A,
same polarity as the MOD13 leaf: destination 0 is A)
and I=1 with the `0x78` absolute selector, 16-bit
absolute DATA address word; execution loads the
32-bit long word (MSW at abs, LSW above it) into
dst(31:0) with SXM widening (SXM=0 clears dst(39:32),
SXM=1 sign-extends bit 31; TI SPRU179 `dst =
dbl(*(abs))`) and no status-bit change. The staged
loader1 run stops `UNIMPLEMENTED_OPCODE` on it at
`0x8024` (words `56F8 0060`, `0xF074` opens the next
instruction): no `0x56xx` row exists yet. The `0x56E8`
MOD13 mask never matches `0x56F8`, so no shadow carve
is needed; long-offset and direct/indirect Smem forms
stay deferred. Absolute operands cannot repeat
(section 2 rule, same evidence as S70/S72-S76), so
the row marks `INELIGIBLE`.

Leaf design: new `EXACT` row `DLD_SMEM_ABSOLUTE`
(`0x56F8`, 2 words) reusing the S74/S75
first+D+abs16 layout set
(`ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16`: destination
D[8:8], I[7:7], selector A[6:0], absolute word; 4
operands keep the trusted packet simple). The leaf
gates SXM knownness (MOD13 DLD precedent: DLD
consumes STATUS_SXM only, no accumulator/SST gate)
and stages a new `LONG32_ABSOLUTE_LOAD` arm (kind 29:
absolute DATA address, READ access, staged-zero
halves, non-null `complete_long32` accumulator-write
widening completion) through the established long32
pair engine with an absolute-load-leg snapshot
branch. Only the evidenced absolute selector
composes. New catalog ID moves `ID_COUNT` 333 to 334
with pins; `FORM-T2-19-01` binds the row `SUBSET`
with `ABSOLUTE_DATA`; no MMR, knob, or timing
changes.

RED contracts: new decode/ops/metadata/CPU tests
(`test_dld_smem_absolute_decode`,
`test_dld_smem_absolute`,
`test_dld_smem_absolute_metadata`,
`test_cpu_dld_smem_absolute`) plus the catalog-contract
row pin. Acceptance is the hosted repro advancing
past `0x8024` to the next explicit stop (`0xF074` at
`0x8026` is the predicted follower).

Out of scope: DLD direct/indirect/long-offset Smem
family (own slices as the boot reaches them), DST
absolute changes (S74 frozen), timing.

Split decision (recorded, not deferred): the new
`LONG32_ABSOLUTE_LOAD` case tipped
`src/isa/ops/memory/memory_effect.c` to 502 lines,
tripping the 500-line hot-path bound. The eight
absolute-addressed arms (absolute DATA, Smem
absolute/pair arms, LONG32 absolute store/load,
atomic absolute) now dispatch through the new
`memory_effect_absolute_dispatch.[ch]` (63 lines);
`memory_effect.c` falls to 483 lines with grouped
case labels. No semantic change: existing absolute
ops tests cover every arm through the generic gate.
