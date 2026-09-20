### P2-W43A: generic indexed WORD16 MEMORY seam (implemented; local verification green)

This immutable `(planned)` heading is the FIT/ownership anchor for the W43A
activation. W43A is a generic transaction prerequisite only; it does not add
catalog rows, opcode identities, handlers, or a CPU opcode branch. The private
address kind is `INDEXED_AR0_WORD16`, with an explicit MOD5/MOD6 discriminator.
The resolver reads CMPT and ARP, then the selected AR and AR0 from the copied
private `arch_state`; it reuses the existing indexed planner and candidate/
commit structures. MOD5 uses the old selected ARx as the WORD16 address and
schedules ARx−AR0 after access, rejecting ARx<AR0; MOD6 uses ARx+AR0 and
rejects unsigned overflow. Selected AR0 aliasing is rejected, and AR0 never
mutates. Both READ and WRITE are admitted by this seam.

The root transaction remains atomic: WAIT, FAULT, and cancel preserve all
architectural state; READY validates both selected-AR and AR0 snapshots and
commits the selected AR, program PC, and MEMORY effect together. No timing,
device/MMR, IRQ, RPT, wrap, machine-cycle, public-ABI, or new public CPU
snapshot claim is admitted. No new `op_input` or CPU snapshot field is planned.
The three RED lanes are project-contract tests for the MEMORY effect shape,
resolver, and root transaction, owned respectively by `src/isa/ops/memory/`,
`src/sequencer/memory/`, and `src/sequencer/instruction/`, with owner-local
proposed test files `test_indexed_word16_memory_effect.c`,
`test_indexed_word16_memory_resolver.c`, and
`test_indexed_word16_memory_transaction.c`; implementation should register
the CTest runners `ti_c54x.indexed_word16_memory_effect`,
`ti_c54x.indexed_word16_memory_resolver`, and
`ti_c54x.indexed_word16_memory_transaction` when each lane closes.

The existing `memory_instruction_transaction.c` root owner was 523 lines before
implementation; W43A split it into a 335-line root owner and a 268-line
candidate validator. The indexed resolver is 169 lines. Existing
indexed-addressing TI and project-contract evidence is reused; no new evidence
is admitted by activation.

Status: COMPLETE. RED→GREEN focused strict and ASan/UBSan CTest execution,
independent semantic/integration review, the authoritative standalone `make
test`, the evidence checker, and repository diff validation are green. No
public ABI, CPU snapshot, timing, device/MMR, IRQ, RPT, wrap, catalog,
opcode-handler, or new architectural-state claim was added; inventory coverage
is unchanged. The 335-line root, 268-line validator, and 169-line resolver are
retained as cohesive modules and will be reviewed before approaching the
repository's 500/800-line thresholds.
