### P3-S84: STL absolute Smem slice (implemented; local verification green)

`STL *(abs),src` is TI SPRU172C section STL (store
with absolute addressing): first `0x80F8` with S
selecting the source accumulator (S=0 is A, STL_SMEM
precedent) and I=1 with the `0x78` absolute selector,
16-bit absolute DATA address word; execution stores
src(15:0) to the word at abs with no status-bit
change. The staged loader1 run stops
`UNIMPLEMENTED_OPCODE` on it at `0xF2F` (words
`80F8 087E`; `0xF847` opens the next instruction):
no `0x80xx` absolute row exists yet. The STL `0xFEE0`
family mask never matches `0x80F8` (S75/S80/S83
pattern), so the row sits with the absolute group and
no shadow carve is needed; direct/indirect Smem forms
stay deferred. Absolute operands cannot repeat
(section 2 rule, same evidence as S70/S72-S83), so
the row marks `INELIGIBLE`.

Leaf design: new `EXACT` row `STL_SMEM_ABSOLUTE`
(`0x80F8`, 2 words) reusing the absolute
ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 layout set
(source S[8:8], I[7:7], selector A[6:0], absolute
word; 4 operands keep the trusted packet simple). The
leaf gates source-accumulator knownness (STL_SMEM
precedent: the family consumes ACCUMULATORS only) and
stages the existing `ABSOLUTE_DATA` WORD16 WRITE arm
fire-and-forget (staged src low word rides verbatim,
sidecar NONE, complete NULL — ST #lk precedent).
Only the evidenced absolute selector composes. New
catalog ID moves `ID_COUNT` 339 to 340 with pins;
`FORM-T2-20-09` binds the row `SUBSET` with
`ABSOLUTE_DATA`; no MMR, knob, or timing changes.
The cookie was uncovered, so populated first words
move 40273 to 40274 (S75/S80/S83 pattern).

RED contracts: new decode/ops/metadata/CPU tests
(`test_stl_smem_absolute_decode`,
`test_stl_smem_absolute`,
`test_stl_smem_absolute_metadata`,
`test_cpu_stl_smem_absolute`) plus the catalog-contract
row pin. Acceptance is the hosted repro advancing past
`0xF2F`: `0xF847` (BD, already implemented) opens at
`0xF31`, and the run cascades through the finalize
chain into the steady RPTB poll loop with no further
explicit stop inside a 100k budget.

Out of scope: STL direct/indirect/long-offset Smem
family (own slices as the boot reaches them), STL
absolute changes (S84 frozen), timing.
