### P2-W51B: dual-memory root commit/publication contract (implemented; local verification green)

This immutable `(planned)` heading is the durable W51B FIT/ownership anchor.
W51B owns the generic root transaction seam only; it does not activate an opcode
row, alter CPU wiring, widen the public API, or change catalog/inventory
projection. The W51A architectural mode/update evidence remains the source for
MOD0/MOD1/MOD2 old-address behavior, direct Xar/Yar selector mapping to AR2–AR5,
and same-AR Xmod precedence. All lifecycle and ABI assertions in this slice
are project contracts.

The persistent dual snapshot must carry exactly two value-owned commit
descriptors alongside its copied requests. Before endpoint issue and again at
the commit boundary, the root validates the copied mode/selector/address
shadows against the live architectural state; malformed modes, selectors,
addresses, or drift reject transactionally without partial output. Distinct
selected ARs publish both post-access updates atomically, and only after both
DATA reads are READY and the ordinary effect staging succeeds. If X and Y
select one AR, Xmod precedence yields one commit. WAIT, FAULT, cancellation,
and other nonterminal paths remain non-publishing. Native LP64/ILP32 root
sizes stay frozen at 336/308 bytes, and no broader dual-mode opcode activation
is part of W51B.

The executable contract lives at
`tests/unit/sequencer/instruction/test_dual_memory_instruction_w51b_contract.c`
with CTest name `ti_c54x.dual_memory_instruction_w51b_contract` and is green:
the 48-byte snapshot, atomic distinct-AR publication, single X-owned alias
commit, WAIT/FAULT non-publication, drift rejection, and frozen 336/308 root
sizes are all proven. The dual effect carrier admits MOD0/MOD1/MOD2 with MOD3
still deferred, and the READY commit lives in the split
`memory_instruction_dual_commit` owner after the 500-line quality bound forced
`memory_instruction_dual.c` back to 448 lines. The W51B closure registry is
208 immutable evidence records, 338 requirements (300 `IMPLEMENTED`, 7
`PLANNED`, 31 `UNRESOLVED`), and 288 tests (281 `PASSING`, 7 `PLANNED`).
Catalog, lookup, binding, family, and strict-complete coverage remain 104,
3,031, 104, 44/205, and 27/205. The next documented seam is broader dual-mode
opcode activation through this root.
