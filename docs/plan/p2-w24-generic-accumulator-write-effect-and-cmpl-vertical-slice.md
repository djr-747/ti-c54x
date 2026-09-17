### P2-W24: generic accumulator-write effect and CMPL vertical slice (planned)

State: implemented; this heading remains the frozen historical evidence anchor.

P2-W24 is a two-step, CMPL-driven accumulator slice. The first step generalizes
the P2-W23 sequencer payload from a select-named effect to a reusable
`ACCUMULATOR_WRITE` effect. The second step registers and executes CMPL through
that seam. The work is deliberately small enough for a family-local ISA lane,
but the shared effect and CPU retirement surfaces remain integration-owned.

Step 1 freezes the generic payload and ownership contract:

- Rename the select-named effect to `ACCUMULATOR_WRITE`; do not add a CMPL-only
  effect or retain an `ACCUMULATOR_SELECT` semantic type. The payload owns a
  destination (A or B), one canonical full-40-bit value, and an explicit C
  action: `PRESERVE` or `WRITE`.
- Canonical validation is strict and value-owned. `PRESERVE` requires
  `c_value == 0`; `WRITE` requires `c_value` to be exactly 0 or 1. Unknown
  action values, noncanonical 40-bit payloads, invalid destinations, and any
  rejected call leave both accumulators and C unchanged. The sequencer applies
  one validated effect atomically and preserves all unrelated status state.
- The handler remains pure: source snapshots are copied into the input bundle,
  never into the effect. The CPU requests the A/B snapshot only when catalog
  metadata carries the generic `ACCUMULATORS` input-resource mask. No opcode
  identity branch is permitted, and this slice must not grow `src/cpu/cpu.c`;
  effect routing/retirement stays on the existing generic seam.

Step 2 implements CMPL as the first consumer:

- `CMPL` registers value `0xF493` with mask `0xFCFF`. S is bit 9 and D is bit 8,
  producing exactly four one-word forms for source A/B and destination A/B. A
  generic `ACCUMULATOR_SOURCE_DESTINATION` layout exposes S as operand 0 and D
  as operand 1; no CMPL-named field extractor or handler-side bit parsing is
  permitted.
- CMPL writes the full 40-bit one's complement of the selected source
  accumulator to the selected destination. It has no status effects, so its
  effect uses `C PRESERVE` with the canonical `c_value == 0` payload. The
  non-destination accumulator and all unrelated status fields are preserved.
- `src/isa/catalog/` owns the declarative row and `ACCUMULATORS` resource
  metadata; `src/isa/decode/` owns S/D extraction; `src/isa/ops/accumulator/`
  owns the pure CMPL handler; `src/sequencer/accumulator/` owns generic effect
  validation/application; and `src/cpu/` only consumes the generic effect with
  no new opcode-specific path. Tests mirror those owners under
  `tests/unit/isa/decode/`, `tests/unit/isa/ops/accumulator/`,
  `tests/unit/sequencer/accumulator/`, and `tests/unit/cpu/`.
- Decode coverage is exhaustive over all 65,536 first words: the four CMPL
  forms must classify exactly, every fixed-bit near-miss must reject, and
  no unrelated catalog row may be shadowed. Semantic vectors cover all source
  and destination combinations, zero, sign/guard boundaries, and C-preserve
  atomicity.

TI `SPRU172C` §4 CMPL (printed p. 4-32) is the normative encoding and effect
source. `SPRU131G` §2.3 (printed p. 2-8) remains the full 40-bit accumulator and
guard-bit source; CMPL applies one's complement across all forty bits. GNU
binutils 2.42 commit `c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e`,
`opcodes/tic54x-opc.c`, is nonnormative encoding corroboration only and must not
be copied or vendored. Table 2-28 does not positively authorize CMPL repetition,
so repeatability remains `UNRESOLVED`; summary cycle/pipeline columns are not
timing evidence and timing remains `UNRESOLVED`.

Exit condition: satisfied. The generic `ACCUMULATOR_WRITE` contract and its
adversarial payload tests pass, and CMPL's four forms pass exhaustive decode,
pure one's-complement semantics, transactional effect application, and public
CPU retirement tests without a CPU file-growth or opcode-identity exception.
No memory, bus, device, emulator, RPT, IRQ, repeatability, or machine-cycle
claim is added by this work piece.
