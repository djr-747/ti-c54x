### P2-W19B: transactional extension fetch and width-aware program PC (active kickoff)

State: implemented and green. This work piece froze the two seams needed before
any multiword opcode, including `RPT`, is allowed to enter CPU retirement. It adds no
production opcode, handler, catalog row, or CPU end-to-end count-two integration.
The implementation and tests must remain standalone, allocation-free, and
project-contract-only; no TI conformance, machine-cycle, memory, IRQ, repeat, or
emulator behavior is claimed here.

`src/cpu/instruction_fetch.[ch]` owns private extension collection only after the
CPU has fetched and decoded the first word and learned `program_word_count`. Its
by-value result has helper capacity for exactly two program words (not an ISA-wide
maximum) and explicitly publishes status, the exact bus status, `program_words[2]`,
and the resulting count. The input copies the first word and exact `uint32_t`
`first_program_word_address` and borrows a bus descriptor/context for the call only;
it retains no pointer. Counts one and two are the only admitted widths: count zero
and every count above two are rejected before any callback. Count one publishes the
first word, makes zero callbacks, and reports `TI_C54X_BUS_OK` because no bus was
attempted. Validation and overflow paths that make zero callbacks—including a null
bus, unsupported width, and address overflow—report `TI_C54X_BUS_OK` in the result
bus-status field because no bus transaction was attempted. Count two checks
`first_program_word_address + 1` in the 32-bit carrier before exactly one callback
at that exact carrier; high carriers are allowed and are never truncated before
`UINT32_MAX` overflow is rejected. A null bus is invalid.
A missing callback for count two maps to a bus fault with
`TI_C54X_BUS_NO_CALLBACK`, while any callback failure preserves its exact non-OK
bus status and maps fetch status to bus fault. Overflow or unsupported width makes
zero callbacks. Every non-OK result publishes no partial bundle (zero count and
zero words), initializes all named result fields deterministically, and is not
retried. This owner does not fetch the first word, count
attempts or retirements, mutate CPU/PC/state, infer common 16-bit/XPC policy, model
timing/waits or memory effects, or emit observers/diagnostics.

`src/sequencer/sequential_pc.[ch]` and `program_pc_bridge.[ch]` generalize the
private staged PC effect to explicit `uint8_t program_word_count` values from 1
through 255. They convert both operands to `uint32_t` before addition for hosts
where `int` is 16 bits, reject count zero, reject any sum above `UINT16_MAX` as
unresolved, and never wrap or narrow before validation. The existing one-word
behavior remains exact: `0xfffe -> 0xffff` is admitted and a
one-word instruction at `0xffff` is unresolved. Valid/stale/malformed effects
remain transactional. The public bridge rejects carriers above `0xffff` without
XPC inference and delegates admitted width arithmetic; it performs no bus I/O.

Current CPU orchestration and P2-W18 NOP ordering remain unchanged. Because no
multiword opcode is registered, the CPU does not yet use count two end-to-end.
RPT work will decide the later orchestration order among bridge span validation,
extension reads, operand extraction, and semantics. `last_fetched_word` policy is
unchanged. Ownership follows the tree: `src/cpu/` owns fetch collection,
`src/sequencer/` owns PC arithmetic and transactional application, and tests live
under `tests/unit/cpu/` and `tests/unit/sequencer/` respectively.

The passing proofs cover count-one/no-read, exact low/high count-two carriers,
`UINT32_MAX` overflow, missing callbacks, every bus-fault status, callback scribble
on failure, no retry, complete-bundle-only publication, deterministic failures, no
pointer retention, exhaustive valid widths and boundaries 1..255, count-one
regression, explicit `0xfffd + 2 = 0xffff` admission and `0xfffe + 2`
unresolved-boundary rejection, high public carriers, malformed/stale effects, and
no mutation. Existing P2-W18 CPU tests remain regression gates. Both new tests are
`PROJECT_CONTRACT_ONLY` and passing under their owner-local runners.

The implementation commit is `3e57e316bdac5cbb469cba2cc5d5060cc45d1dea`. Focused
normal and ASan/UBSan tests passed for both new seams, and the full standalone
`make -B test` gate passed. The next work is evidence-first RPT/IRQ semantics;
this slice does not claim TI conformance, timing, repeat, IRQ, memory mutation,
XPC, or emulator integration.
