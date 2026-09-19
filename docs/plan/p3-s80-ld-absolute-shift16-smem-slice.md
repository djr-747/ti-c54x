### P3-S80: LD absolute-shift16 Smem slice (implemented; local verification green)

`LD *(abs),16,dst` is TI SPRU172C section LD (load
with shift, absolute addressing): first `0x44F8`
with D selecting the destination accumulator and
I=1 with the `0x78` absolute selector, 16-bit
absolute DATA address word; execution loads the
word at abs into dst(31:16) with SXM widening
first (SXM=0 clears dst(39:32), SXM=1 sign-extends
bit 15) then logical-left-shift 16, dst(15:0)=0,
no status-bit change. The staged loader1 run stops
`UNIMPLEMENTED_OPCODE` on it at `0xF27` (words
`44F8 087D`, `0x1AF8` opens the next instruction):
no `0x44xx` absolute row exists yet. The `0x44B8`
MOD7 mask never matches `0x44F8`, so no shadow
carve is needed; long-offset and direct/indirect
Smem forms stay deferred. Absolute operands cannot
repeat (section 2 rule, same evidence as S70/S72-
S78), so the row marks `INELIGIBLE`.

Leaf design: new `EXACT` row
`LD_SMEM_ABSOLUTE_SHIFT16` (`0x44F8`, 2 words)
reusing the S74/S75/S78 first+D+abs16 layout set
(`ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16`:
destination D[8:8], I[7:7], selector A[6:0],
absolute word; 4 operands keep the trusted packet
simple). The leaf gates SXM knownness (shift16 MOD
precedent: the family consumes STATUS_SXM only)
and stages the existing `ABSOLUTE_DATA` WORD16
READ arm with a shift16 widening completion
(SXM-widen then `<<16`, C/TC preserved). Only the
evidenced absolute selector composes. New catalog
ID moves `ID_COUNT` 335 to 336 with pins;
`FORM-T2-19-04` binds the row `SUBSET` with
`ABSOLUTE_DATA`; no MMR, knob, or timing changes.

RED contracts: new decode/ops/metadata/CPU tests
(`test_ld_smem_absolute_shift16_decode`,
`test_ld_smem_absolute_shift16`,
`test_ld_smem_absolute_shift16_metadata`,
`test_cpu_ld_smem_absolute_shift16`) plus the
catalog-contract row pin. Acceptance is the
hosted repro advancing past `0xF27` to the next
explicit stop (`0x1AF8` at `0xF29` is the
predicted follower).

Out of scope: LD direct/indirect/long-offset
shift16 Smem family (own slices as the boot
reaches them), LD absolute changes (S75 frozen),
timing.
