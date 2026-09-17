### P2-W19A: value-owned operand-field extraction (active kickoff)

State: implemented. P2-W19 is deliberately split into two atomic contracts:
P2-W19A proves value-owned operand-field extraction first; completed P2-W19B
proves extension-word fetch and width-aware program-PC span/commit. Both precede
repeat instructions, including `RPT`, so operand shape and multiword sequencing are
not smuggled into an opcode handler. Bus reads belong to `src/cpu/` (the
instruction-fetch owner); `src/sequencer/` owns only the resulting width-aware
program-PC span and commit policy.

The 19A contract is a pure, allocation-free checked extractor under
`src/isa/decode/operand_field.[ch]`. Its `ti_c54x_program_words_view` borrows a
caller-owned `program_words` array whose `program_word_count` is the extent; at
least that many `uint16_t` elements must remain alive and unchanged for the call.
The immutable layout similarly borrows a piece array for its declared
`piece_count`, and the output is caller-owned. No pointer is retained after
return, and writes occur only after validation succeeds. A conceptual piece
contains a program-word index, source LSB, width from 1 through 16, and
destination/value LSB from 0 through 63. A field has one or more pieces and a
declared raw width from 1 through 64. Every piece remains inside one source word
and the declared destination width; destination pieces do not overlap and cover
the declared value exactly; source slices for a field do not overlap within a
source word.

The extractor assembles first-word, extension-word, split/non-contiguous, and
64-bit raw values without sign extension or semantic interpretation. Null or
invalid views/layouts/outputs, out-of-range word indexes, invalid widths, overlap,
and gaps leave the output byte-for-byte unchanged. It owns no bus, CPU, state, PC,
effect, timing, memory, interrupt, repeat, observer, diagnostic, file, environment,
heap, or mutable-global behavior. The production opcode inventory, catalog macro
arity, layout binding, handlers, public API, and fetch path remain unchanged.

Ownership mirrors the hierarchy: `src/isa/decode/operand_field.[ch]` owns the
implementation contract; `tests/unit/isa/decode/test_operand_field.c` owns its
executable proof; owner-local Make/CMake registration now owns the test target.
The passing test exhausts valid one-word slice positions and widths, then covers
multiword, extension-word, split/non-contiguous, 64-bit, borrowed-input, and
complete invalid/transactional cases. This is project-contract mechanics evidence
only; it makes no TI encoding or instruction-conformance claim and fetches no
words itself.

Performance boundary: this checked general extractor proves descriptor mechanics
and remains allocation-free, but it is not a requirement that every hot-path
instruction revalidate a descriptor. A future production catalog/layout binding
may validate immutable layouts offline and emit or directly use specialized
extraction code, so descriptor revalidation is not structurally required in the
hot path. No such generator or specialization exists in this work piece.
