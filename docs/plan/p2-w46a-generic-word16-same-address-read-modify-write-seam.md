### P2-W46A: generic WORD16 same-address read-modify-write seam (implemented; local verification green)

This immutable `(planned)` heading is the FIT/ownership anchor for W46A. The
seam remains private and value-owned inside the existing
`TI_C54X_OP_EFFECT_MEMORY` carrier: a compact typed transform enum (`AND`,
`OR`, `XOR`) and copied WORD16 operand describe one same-address read followed
by one write. No indirect callback, allocation, new effect kind, CPU opcode
branch, or public ABI is admitted. Address resolution runs once; the cached old
address is reused for both bus phases, each phase receives a fresh transaction
ID, and the transform runs exactly once on the copied read word.

The transaction must preserve WAIT/FAULT/cancel and stale semantics. Resume
paths validate direct live snapshots (CMPT/ARP, selected AR, and AR0/BK when
the selected mode needs them, plus PC) instead of rerunning the planner or
rereading the endpoint. Architectural address/PC effects commit only after
WRITE READY. This is core architectural commit atomicity; exclusion against
an external bus master between the read and write is explicitly unsupported
until separately evidenced. Helpers are split before the 500-line review
threshold.

Performance is part of the seam contract: keep the carrier/candidate fixed-size
where feasible, retain an allocation-free hot path, and add a structural
one-read/one-write gate. W46B owns the later executable observer-off RMW
benchmark. That benchmark is non-history and must not redefine the five frozen
performance scenarios or claim machine-cycle timing.

Implementation preserves the 40-byte MEMORY carrier and 48-byte generic effect
on supported 64-bit ABIs (36/40 bytes on supported 32-bit ABIs). The root
transaction remains 464 lines after splitting value transforms and paired-write
construction into a private helper. Address resolution runs once; READY/WAIT,
FAULT, cancel, stale-state, candidate-forgery, endpoint-callback mutation, and
second-ID exhaustion paths are executable. Direct live validation never calls
the address resolver, and AR/PC publish only after WRITE READY. The cohesive RMW
transaction test is about 600 lines; it has been reviewed and remains unsplit
for this single lifecycle contract, with a mandatory split before 800 lines.

Closure accounting is 175 evidence records; 290 requirements (253
IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 244 tests (236 PASSING, 8 PLANNED);
and 7 sources. W46A's two focused C runners and six-test structural performance
lane are GREEN. W46B's five focused C/Python runners, dedicated benchmark quality
contract, clean baseline artifact, and baseline comparison are GREEN. No
external-master exclusion or TI timing claim is added.
