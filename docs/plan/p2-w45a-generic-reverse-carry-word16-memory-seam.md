### P2-W45A: generic reverse-carry WORD16 MEMORY seam (implemented; local verification green)

This immutable `(planned)` heading is the FIT/ownership anchor for the W45A
activation. W45A is now implementation-complete. Existing registered TI
MOD4/MOD7 and reverse-carry primitive, planner, and application records are reused. W45A freezes a private typed
`REVERSE_CARRY_WORD16` memory address kind for MOD4/MOD7; ARF admits only 1..7
and AR0 aliasing remains unresolved/rejected. Unrelated fields are canonical
zero and prevalidation completes before architectural reads.

Resolution is ordered CMPT, ARP, selected AR, then AR0; AR0 must be known,
nonzero, and one-hot. The planner uses the old selected AR as the operand
address and stages a POST update to the selected AR only. No modulo or wrap
inference is made; boundary propagation that escapes the documented domain is
rejected. Candidate validation requires exact live snapshots, with WAIT,
FAULT, cancel, stale, and no-refetch behavior preserved and READY committing
the selected-AR update, sequential PC, and MEMORY effect atomically.

W45A owns only the private memory effect, address-resolution, and candidate
validation seams. It adds no BK, root transaction, CPU, catalog, opcode, public
ABI, timing, device, or RPT behavior. Three project requirements/tests
cover effect shape, address resolution, and memory transaction ownership. RED
replay covered an absent kind/field compile failure, valid effect rejection by
the resolver, and transaction prepare rejection; all three are now GREEN.
The production sizes are `memory_effect.c` 95 lines, `memory_effect.h` 76,
`address_resolution.c` 293, `memory_instruction_candidate_validate.c` 354,
its header 29, and tests 219/411/426 lines. Review closure strengthened
rejection output zeroing, forged commit rejection, and module-contract wording.
No root transaction, CPU,
catalog/opcode, public ABI, timing, device, or RPT change was made.
W45B remains a later twenty-row MOD4/MOD7 fan-out, projected at ID_COUNT=96,
lookup 2,583, and 95 bindings; it is not activated here.

Closure accounting is 169 evidence records; 275 requirements (238
IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 229 tests (221 PASSING, 8 PLANNED);
and 7 sources. Fresh ASan/UBSan validation passed 15/15, independent reviews
found no production defect after their test/contract findings were closed, and
the authoritative standalone `make test` gate is GREEN. W45B is the next
audited but inactive reverse-carry fan-out.
