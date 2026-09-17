### P2-W17: masked opcode catalog foundation (active kickoff)

State: implemented. The completed work piece establishes the shared catalog
schema, deterministic lookup generator, checked-in lookup artifact, and their
exhaustive and synthetic quality proofs. Production still registers only the
exact NOP row; no masked instruction is enabled by this slice.

This work piece extends the P2-W16 catalog infrastructure without adding an
operand-bearing or masked production instruction. The exact NOP row, its `0xF495`
encoding, behavior, and public/private APIs remain unchanged. The sole
`src/isa/catalog/` inventory now has an `EXACT` plus `MASKED` schema with these
project-contract rules:

- Every encoding is canonical 16-bit `value`/`mask` data. `EXACT` is represented
  by a full mask; `MASKED` must have a nonzero mask other than `0xffff`, and the
  value may not set any bit outside its mask.
- Widths are one through 255 program words. IDs and handler tokens are unique.
  Pairwise pattern overlap is rejected deterministically; there is
  no priority or declaration-order tie-breaker.
- A deterministic repository-owned generator emits the checked-in immutable
  65,536-entry `uint16_t` direct lookup at
  `generated/isa/catalog/opcode_first_word.inc`. The
  ordinary Make, CMake, and installed-consumer builds consume only checked-in C
  artifacts and remain compiler-only/Python-free. Quality checks regenerate the
  artifact and reject stale output, but runtime has no initialization,
  allocation, or mutable global-table update.
- Operand-field layouts/extraction and extension-word fetch remain separate
  future owners/work pieces. This foundation does not infer variable operand
  fields, multiword fetch semantics, CPU fetch/decode/dispatch/retire
  integration, timing, memory, bus, device, observer, XPC, or external-table
  behavior.

Ownership follows the tree: `src/isa/catalog/` owns the declarative rows and
runtime lookup, `tools/isa/catalog/generate_opcode_lookup.py` owns deterministic
generation, `generated/isa/catalog/` owns the checked-in derivative, and
`tests/contract/quality/isa/catalog/` owns the Python schema, overlap, generator,
and stale-artifact checks. No
implementation file outside the catalog owner may add an opcode row or duplicate
the generated table. Existing TI field/encoding evidence remains architectural
context for later instruction work; the catalog and generator evidence is
`PROJECT_CONTRACT_ONLY` and does not claim TI conformance.

The TDD proof now covers malformed schema, invalid masks/widths, duplicate IDs
or handlers, deterministic pairwise overlap rejection, stale lookup output,
exhaustive 65,536-word mapping, and generated-artifact immutability. The two
Python tests run through `make test` via CTest
`ti_c54x.quality.python`; normal Make, CMake, and installed-consumer builds
consume only checked-in C artifacts and remain compiler-only/Python-free.
