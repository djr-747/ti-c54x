# P2-W16-W20A kickoff-label resolution

### P2-W16-W20A kickoff-label resolution (implemented; local verification green)

Scope: resolve the open W16-W20A label question. Seven plan
anchors still carry the September 6 kickoff status while the work
they name is long landed: the `###` anchors in
`p2-w17`, `p2-w18`, `p2-w19a`, `p2-w19b`, `p2-w20a`, the `####`
P2-W20E anchor in `p2-w20a`, and the `####` P2-W21A anchor in
`p2-w21`. Contemporary ledger completion entries exist for
W17, W18, W19A, W19B, W20E, and W21A; W20A's scope was
partitioned into W20B–W20E (all completed) and its RPT
evidence/transaction architecture is exercised by today's green
suite. TI vectors: none — docs/registry-only status advance, no
opcode, sequencing, or timing semantics change.

Approach: advance exactly the seven stale parentheticals to
`(implemented; local verification green)` — no heading
reworded — and mirror the new status into the five
implementation-plan link rows, the seven evidence registry
anchors, the seven evidence-index rows, and the two
architecture-baseline prose headings that quote the old labels.
The bare P2-W16/P2-W21 parent headings, their links, and the
bare P2-W16 evidence anchor are intentionally untouched: nothing
stale to correct there, and they are recorded here as accepted.

Gates: `make test` exit 0 (evidence registry/index parity
included), zero remaining kickoff labels in the main tree
outside ledger history, heading/link/anchor agreement audit
clean. No evidence registry additions (anchor text sync only).
