### P2-W26: SFTL shifted-accumulator vertical slice (planned)

State: implemented; the immutable `(planned)` heading remains the historical
evidence anchor. Catalog, decode, shift-operation, CPU, full local repository,
and remote implementation proofs are green.

P2-W26 is a narrow SFTL accumulator slice. It adds no public API, architectural
state, input-resource bit, effect type, sequencer seam, memory behavior, RPT/IRQ
behavior, device behavior, or machine-timing contract. SFTL consumes the
existing `ACCUMULATOR_WRITE` plus `C_WRITE` effect path.

The evidence-backed form and ownership contract is:

- TI `SPRU172C` §4 SFTL (printed p. 4-158, PDF p. 271) defines syntax
  `SFTL src, SHIFT [,dst]`, A/B operands, and a signed shift range of `-16..15`.
  The bit format `1111 00SD 111S HIFT` yields value `0xF0E0` with mask
  `0xFCE0`, one program word, and 128 encoded first words. The catalog row is
  `SFTL_ACC`/`sftl_acc` with `ACCUMULATORS` input only.
- `src/isa/decode/` owns the generic
  `TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5` layout:
  source bit 9 is operand 0, destination bit 8 is operand 1, and raw shift bits
  4:0 are operand 2. The handler sign-extends the five-bit two's-complement
  field; no SFTL-named bit parser is permitted.
- `src/isa/ops/shift/` owns pure semantics. For `q < 0`, let `n = -q`, shift
  selected low32 right by `n`, and set C to source bit `n-1`; for `q == 0`,
  preserve low32 and set C to zero; for `q > 0`, shift low32 left by `q` and
  set C to source bit `32-q`. Guard bits 39:32 clear, and C is the only status
  field changed. Pre-C is not an input. The destination is selected by D;
  source and destination may be A or B.
- `src/cpu/` routes SFTL through the existing generic input/effect/retirement
  path without a production change or opcode branch. Unknown selected source or
  noncanonical input/decode rejects transactionally; unknown unselected state is
  allowed. Non-C status, failed effects, and the PC-`0xffff` boundary remain
  unchanged on rejection. Ownership-mirrored tests live under
  `tests/unit/isa/catalog/`, `tests/unit/isa/decode/`,
  `tests/unit/isa/ops/shift/`, and `tests/unit/cpu/`.

The TDD proof must exhaust all 128 encodings and cover q boundaries `-16`, `-1`,
`0`, `1`, and `15`, low32 zero/one/high-bit/all-ones vectors, guard-only and
mixed raw40 values, carry endpoint cases, source/destination combinations,
known and unknown pre-C values that must not affect execution,
canonical input and decode rejection with output nonmutation, unknown selected
source rollback, allowed unselected unknownness, preserved non-C status, and
one attempt/one retirement/PC+1 for admitted forms. The catalog's post-slice
quality result is expected to be ten rows and 693 populated first-word
encodings; that is not a claim of 693 distinct instructions. The mixed
reset-runnable workload remains four of ten rows.

TI `SPRU172C` §4 SFTL (printed p. 4-158, PDF p. 271) is normative for the
encoding, execution/carry cases, guard clearing, and its statement that C is the
only affected status field. §2.5/Table 2-28 (printed pp. 2-19–2-21) supplies
only a repeatability boundary: SFTL is not positively authorized as repeatable,
so repeatability remains `UNRESOLVED`. Any GNU binutils 2.42 reference at pinned
commit `c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e` is nonnormative corroboration
only and must not be copied or vendored. Summary cycle fields are not admitted
as machine-timing evidence.

Closeout: the catalog now contains ten declarative rows and 693 populated
encoded first words, not 693 distinct instructions; the mixed reset-runnable
coverage remains four of ten rows. No `cpu.c` change, public API, memory,
RPT/IRQ, or timing claim was added.
