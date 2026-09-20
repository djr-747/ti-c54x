### P2-W23: MAX/MIN accumulator vertical slice (implemented; local verification green)

State: implemented; this heading remains the frozen historical evidence anchor.

P2-W23 is a narrow accumulator-only ISA slice. The production rows, state/effect
owners, and public CPU proof now implement the frozen contract:

- `MAX dst` registers `MAX_ACC` as `0xF486/0xFEFF`; `MIN dst` registers
  `MIN_ACC` as `0xF487/0xFEFF`. Each mask leaves only destination bit D variable,
  yielding exactly four one-word encodings: D=0 targets A (`0xF486`/`0xF487`)
  and D=1 targets B (`0xF586`/`0xF587`). TI's operation text selects A on
  `A >= B` for MAX and `A <= B` for MIN, including ties; the selected value is
  written to `dst`, C is 0 for an A selection and 1 for a B selection, and C is
  the only status bit changed. The other accumulator is unchanged.
- A and B are signed, full 40-bit accumulator values including guard bits. Tests
  must cover zero, positive/negative ordering, ties, sign-bit and guard-bit
  boundaries, both destinations, and preservation of the non-destination value.
  The exact four encodings and all mask near-misses are decode-test vectors;
  semantic vectors are exhaustive over destination/operation/tie relation and
  representative full-width accumulator boundaries.
- Ownership mirrors the tree: `src/isa/catalog/` owns the two declarative rows;
  `src/isa/decode/` owns an accumulator destination layout and raw extraction;
  `src/isa/ops/compare/` owns thin MAX/MIN handlers that receive a copied,
  read-only A/B input snapshot, perform the signed compare/select, and return a
  value-owned effect; `src/state/accumulator/` owns canonical full-40-bit A/B
  storage, checked access, and knownness; `src/sequencer/accumulator/` validates
  only the effect payload structure and atomically applies the candidate
  accumulator plus C state once; `src/cpu/` owns checked snapshot acquisition,
  fetch/dispatch/retire integration. Tests mirror those owners
  under `tests/unit/isa/decode/`, `tests/unit/isa/ops/compare/`,
  `tests/unit/state/accumulator/`, `tests/unit/sequencer/accumulator/`, and
  `tests/unit/cpu/`.
- The effect is transactional: handlers do not mutate state and do not place
  source snapshots in the effect. The sequencer does not re-run MAX/MIN
  comparison; it validates only destination, a canonical 40-bit two's-complement
  selected value, and one-bit C before one retirement commit. Structurally
  invalid or rejected effects leave every named field unchanged. Before dispatch,
  the CPU obtains a checked, immutable A/B snapshot; unknown source state stops
  with `INVALID_STATE` without
  retirement or mutation. CPU tests may seed state through the accumulator
  owner's checked internal API before invoking the public execution path;
  no premature public set-state/bootstrap API is introduced. The CPU proof
  checks one attempt/retire, sequential PC +1, destination-only mutation, C
  result, and unknown-word stop.

Review boundary: `src/cpu/cpu.c` is nearing the 500-line review threshold. The
next CPU growth must extract effect routing/retirement transaction into its own
owner; no opcode family may add a new CPU opcode-identity branch.

Performance note: the initial P2-W23 implementation copied accumulator snapshots
and candidate state for every opcode, measuring a 13–25% NOP/LD regression. Catalog
input-resource masks now declare `NONE` for NOP/LD and `ACCUMULATORS` only for
MAX/MIN; CPU snapshot/candidate work follows that generic mask. Two local
bench-compare samples were noisy: one remained +11.7% to +13.7% for NOP/LD,
while the other was within -3.7% to +3.4% versus the hosted cohort; spread
remains high and needs follow-up.

TI `SPRU172C` §4 MAX/MIN (printed pp. 4-99–4-100) is the normative form and
effect evidence; TI `SPRU131G` §2.3 (printed p. 2-8) is the normative
40-bit-accumulator/guard-bit evidence. GNU
binutils 2.42 commit `c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e`,
`opcodes/tic54x-opc.c:358–359`, is nonnormative encoding corroboration only and
must not be copied or vendored. Table 2-28's preamble/list does not positively
authorize MAX/MIN repetition, so repeatability stays `UNRESOLVED`; no machine
cycle or pipeline timing claim is admitted. No memory/bus/device/emulator/RPT/IRQ
behavior is part of this work piece.
