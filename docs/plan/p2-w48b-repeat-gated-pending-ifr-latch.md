### P2-W48B: repeat-gated pending IFR latch (implemented; local verification green)

This heading is the FIT/ownership anchor for the maskable-request
receive/defer/take lifecycle. Implementation commit
`d06f12dddbcd81a2926318022eb35c2bd535a0f4` adds checked typed IFR access
and one fixed-size, caller-owned maskable pending-request reducer under the
new interrupt sequencer owner. Receipt validates one caller-selected IFR
bit, sets it before consulting the existing repeat gate, and preserves both
the bit and latch while an active repeat defers take. Explicit normal take
at remaining count zero clears only the matching bit and latch.

Evidence basis (`E-PROJECT-P2-W48B-PENDING-REQUEST-LIFECYCLE`, anchored in
`p2-w20a-rpt-evidence-and-instruction-transaction-architecture.md`): new TI
SPRU131G §6.10.1 printed pp. 6-27–6-28 evidence records all IFR clear
paths. Normal interrupt take is not exclusive: reset, write-one-to-IFR, and
`INTR` also clear flags. The live common pending-request requirement is
narrowed to a maskable request received after the repeat gate is active.
Exact bit assignment, NMI representation, priority selection, interrupt
entry, and already-pending predecode arbitration remain outside it.

The state owner provides knownness-aware IFR reads, full writes, and
exact-one-bit set/clear operations. The reducer uses the existing repeat
request gate and rejects noncanonical EMPTY/LATCHED values, duplicate
receipt, zero/multi-bit masks, counts above 65,536, unknown validation
state, and stale latches without partial publication. IFR-write-one and
`INTR` clear matching pending state; authoritative reset establishes
known-zero IFR and an empty latch even when prior IFR storage is unknown.
Unrelated IFR flags and architectural fields are preserved.

Ledger-recorded validation (2026-09-09 entry): focused Make and
CMake/CTest execution passes all three new state/interrupt tests;
independent semantic review found no production defect; authoritative `make
test` gate passes. This slice changes no CPU, `run.c`, catalog, trusted
packet, public ABI, or benchmark binary. All work is fixed-size and
allocation-free with no callback, environment, I/O, logging, observer,
diagnostic, or mutable-global path. No knob was added or changed.
Repeated-body PC flow and predecode request arbitration still gate runtime
RPT activation.
