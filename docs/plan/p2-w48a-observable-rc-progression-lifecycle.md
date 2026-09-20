### P2-W48A: observable RC progression lifecycle (implemented; local verification green)

This heading is the FIT/ownership anchor for the observable repeat-counter
progression seam. Implementation commit
`148c811c77c250ca60c88e764258b79957a86827` adds checked typed RC access and
a fixed-size repeat-progress reducer under the existing state and repeat
owners. It consumes the value-owned `REPEAT_START` effect, loads exact RC,
widens before forming the operand-plus-one remaining-body count, and mutates
RC only after the caller reports a successfully retired body.

Evidence basis (`E-PROJECT-P2-W48A-RC-PROGRESSION-LIFECYCLE`, anchored in
`p2-w20a-rpt-evidence-and-instruction-transaction-architecture.md`): the
earlier W20F conclusion stands but is narrowed by three new TI SPRU172C Rev C
records. READA (printed pp. 4-136–4-137) and WRITA (pp. 4-196–4-197) perform
their transfer, increment PAR, and write `RC-1` while RC is nonzero, then
perform the terminal transfer at RC zero without another RC update. FIRS
(p. 4-59) independently corroborates the nonzero decrement loop. Together
with RPT's load and operand-plus-one count, these passages prove
operation-level RC progression and final zero; they do not prove the exact
internal decrement ordering.

The progress reducer validates canonical IDLE/ACTIVE/COMPLETE phase shape,
count bounds, RC knownness, and the exact `RC == remaining-1` invariant
before publication. Unknown, stale, malformed, unsupported-effect,
active-restart, and extra-completion paths preserve architectural state and
progress. A new repeat may overwrite RC after a completed repeat, including
after an intervening known RC state write through the typed owner.

Ledger-recorded validation (2026-09-09 entry): three focused Make
executables pass; independent CMake build and exact CTest selection pass
3/3; evidence checker passes; authoritative `make test` gate passes. The
body-retirement hot path performs one typed RC read, one non-failing typed
RC write, and one small progress update; `run.c`, the CPU instance, public
ABI, catalog, trusted packet, and benchmark binaries are unchanged. No
allocator, I/O, callback, environment, observer, diagnostic, mutable global,
or knob was added. Repeated-body PC flow and predecode request arbitration
still gate runtime RPT activation.
