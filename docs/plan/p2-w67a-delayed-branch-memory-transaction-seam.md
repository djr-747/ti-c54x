### P2-W67A: delayed-branch MEMORY transaction seam (planned)

This immutable `(planned)` heading is the durable W67A FIT/ownership
anchor. W67A is a generic project-contract seam only, mirroring the
W65A/W66A precedent: it adds no catalog row, opcode handler,
layout, CPU opcode branch, `run.c` change, inventory form, or TI
claim. It gives the W67B `RETD` and W67C `CALLD` rows a stack
transaction that defers only the PC redirect while committing the
stack update immediately.

TI semantics (SPRU172C §4 `RET[D]`, printed p. 4-139: TOS→PC with
SP+1 while the two following words execute; §4 `CALL[D]`,
printed p. 4-27: delayed execution pushes `(PC)+4`, i.e. the
address after the two delay-slot words, with the §4-27 worked
example pushing `0029` for `CALLD` at `0025`; SPRU131G §6.3.1
states the two following words execute and forbids a
PC-discontinuity inside them) split the commit: the SP decrement
or increment and the TOS WRITE publish at the delayed branch's
own retirement, and only the PC transfer waits for two program
words (one 2-word instruction or two 1-word instructions). The
§4-27 Execution block and its worked example are normative for
the `PC+4` return address; the Example 7-5 pipeline narrative in
SPRU131G §7 labels the early return register `a3` loosely and is
not adopted for the pushed value.

The seam adds two address kinds,
`TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16` and
`TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16`, with the
same all-zero spec and access shapes as the immediate kinds
(WRITE-only with the staged return address; READ-only with zero
value and null completion). Planning and resolution are shared
with the immediate arms: the pop observes old SP with a
`POST_ACCESS` update, the push observes old SP with a
`PRE_ACCESS` update. The root prepare proves the sequential
successor plus, for pushes, the `PC+4` return address: the staged
TOS WRITE carries successor+2, rejected as `INVALID_CANDIDATE`
when it exceeds `0xFFFF` (a delayed call whose return address
leaves 16-bit program space is malformed; TI code always has two
slot words after the call). The pop prepare proves TOS-target
plannability as before. Commit sets SP plus the TOS value
atomically at READY exactly like the immediate path, but writes
PC to the sequential successor instead of the branch target and
reports the deferred target (TOS for pops, the frozen pmad for
pushes) in the transaction result. The result carries
`deferred_branch_armed` plus the 16-bit deferred PC; ordinary
retirements leave it disarmed. The pure value
`src/sequencer/delayed_branch/delayed_branch.[ch]` owns the
two-word countdown with saturating consume (a slot fill wider
than the remaining words still takes the branch: malformed code
gets a total rule, not a halt) and replace-on-arm. The CPU
instance owns one such value, cleared by init and reset; no
`run.c` change is admitted (the W67B slice wires adopt/advance).

Ownership: `REQ-PROJECT-P2-W67A-DELAYED-BRANCH-VALUE`
(`src/sequencer/delayed_branch/`),
`REQ-PROJECT-P2-W67A-DELAYED-BRANCH-VALIDATE`
(`src/sequencer/` intrinsic shape),
`REQ-PROJECT-P2-W67A-DELAYED-BRANCH-MEMORY`
(`src/isa/ops/memory/`, `src/sequencer/memory/`), and
`REQ-PROJECT-P2-W67A-DELAYED-BRANCH-ROOT`
(`src/sequencer/instruction/`) carry the project contract. The
three RED executable contracts are
`tests/unit/sequencer/delayed_branch/test_delayed_branch.c`
(CTest `ti_c54x.delayed_branch`),
`tests/unit/sequencer/instruction/test_delayed_stack_pop_root.c`
(CTest `ti_c54x.delayed_stack_pop_root`), and
`tests/unit/sequencer/instruction/test_delayed_stack_push_root.c`
(CTest `ti_c54x.delayed_stack_push_root`), driven with hand-built
effects and bundles; no catalog identity is consumed. The root
proofs split pop/push into one file per kind (each under 500
lines), mirroring the W65A/W66A layout. Projected
closure is 291 evidence records, 445 requirements (414
`IMPLEMENTED`, 0 `PLANNED`, 31 `UNRESOLVED`), and 362 tests
(362 `PASSING`, 0 `PLANNED`) with catalog, lookup, bindings,
and family coverage unchanged. These numbers are projections
until the slice lands.
