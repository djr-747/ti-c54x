### P2-W30: two-word immediate logical accumulator vertical slice (implemented; local verification green)

The immutable `(planned)` heading preserves the evidence-registry anchor. P2-W30
is now implemented: the two-word immediate AND, OR, and XOR accumulator forms
have six catalog rows, eight implemented requirements, and eight passing tests.
The forms add 204 populated first words (AND 68, OR 68, XOR 68), taking the
catalog from 16 rows / 1083 populated words to 22 rows / 1287 populated words.
Because every form needs an initialized accumulator input, reset-runnable
coverage remains 4/22.

The central architecture seam is limited to two decode-owned two-word layouts:
`S` bit 9, `D` bit 8, unsigned `SHFT4` bits 3:0, and extension word 1 `lk16`
for the shifted forms; and the same `S`/`D` fields with extension `lk16` and an
implicit shift of 16 for the fixed forms. Existing extension fetch, complete
instruction bundles, the generic program-PC width bridge, `ACCUMULATORS` input,
`ACCUMULATOR_WRITE`, tagged effect transaction, and CPU retirement are reused.
The CPU proof must fetch the extension exactly once at `PC+1`, retire success at
`PC+2`, and roll back atomically on extension failure. At `PC=0xfffd`, retirement
to `0xffff` is valid; at `PC=0xfffe`, the extension may be fetched at `0xffff`
but candidate `PC+2` is rejected with no effect, PC, or retirement mutation.

Ownership mirrors the tree. `src/isa/ops/logical/immediate/common/` owns shared
decoded-operand validation, immediate construction, and parameterized logical-kind
computation; `src/isa/ops/logical/immediate/and/`, `/or/`, and `/xor/` own only their family
leaves. Pure tests mirror those leaves under
`tests/unit/isa/ops/logical/immediate/{and,or,xor}/`; CPU proofs mirror them
under `tests/unit/cpu/isa/logical/immediate/{and,or,xor}/`. The orchestrator
owns catalog metadata, generated lookup, dispatch, parent manifests, CPU shared
surfaces, and integration. A lane must stop and report a GAP if it needs new
memory, status, timing, IRQ/RPT, public API, knob, diagnostic, environment,
allocation, or emulator behavior.

The focused catalog, decode, family, and CPU tests prove the complete path and
its malformed-input rejection. The six new rows are not included in hosted
benchmark streams because initialized accumulator state is not exposed through
the public API; fixed scenarios and the four-row mixed semantic stream remain
unchanged. Semantics use only the selected source
accumulator. The prior destination is not an input even when `D != S`; an
unknown distinct destination is therefore valid because it is overwritten.
`lk16` is unsigned and zero-extended before the selected shift. Status is
preserved. Absence from Table 2-28's
nonrepeatable list is not positive eligibility evidence, so logical-immediate
repeatability remains unresolved; manual two-cycle summaries are evidence only,
not a timing API or claim.

Focused strict and ASan/UBSan tests, two independent reviews, the evidence and
ownership checks, and the full standalone `make test` gate are green. Review
found and closed one catalog-binding weakness: each generated row alias now
rejects its sibling form transactionally instead of accepting either family ID.
Remote CMake, Clang, install, consumer, and sanitizer matrix validation remains
assigned to post-push CI.
