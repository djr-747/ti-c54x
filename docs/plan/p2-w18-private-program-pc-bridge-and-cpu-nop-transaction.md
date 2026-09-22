### P2-W18: private program-PC bridge and CPU NOP transaction (implemented; local verification green)

State: implemented. This work piece is the first cross-owner execution slice:
it connects the existing public CPU shell to the existing exact NOP decoder and
dispatch seam without adding another instruction or changing the catalog.

The contract is intentionally explicit about what is and is not architectural:

- `src/cpu/` owns instruction-budget orchestration, public-carrier handling,
  caller-bus fetch, decode/dispatch sequencing, stop reasons, and cumulative
  attempted/retired counters. A zero budget performs no fetch or attempt.
- `src/sequencer/` owns the private common 16-bit program-PC bridge. It stages a
  sequential successor as value-owned transaction data; it never truncates a
  public 32-bit carrier or infers XPC/page state. `0xfffe -> 0xffff` is admitted;
  staging a one-word successor from `0xffff` returns an explicit unresolved
  boundary and cannot retire.
- The execution order starts an attempt immediately before the program-bus read,
  passes the exact public 32-bit carrier to that callback, fetches exactly one
  16-bit program word, classifies it, and stops on fetch failure or unknown
  decode while preserving committed state. Bus faults and unknown words count as
  one attempted instruction; zero budget and cumulative-counter preflight failure
  count zero. A known NOP at a carrier above `0xffff` reaches bridge validation
  and stops there without truncation or XPC inference. For an admitted word, the
  CPU dispatches NOP through the generated function-pointer seam, stages the
  private PC, commits PC transactionally, then increments `retired` once. The
  CPU does not expose a partial PC commit.
- `run(N)` is an instruction-retirement budget: it returns after `N` successful
  retirements or the first explicit stop and never overshoots. Cumulative counter
  overflow is an explicit no-mutation stop; silent unsigned wrap is forbidden.
- The public result uses distinct
  `TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS`,
  `TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY`, and
  `TI_C54X_STOP_COUNTER_OVERFLOW` reasons for the three new policy stops;
  malformed internal dispatch or stale-transaction states remain
  `TI_C54X_STOP_INVALID_STATE`.

Ownership mirrors the tree. The implementation seam belongs under
`src/sequencer/program_pc_bridge.[ch]`, CPU orchestration remains under
`src/cpu/`, and the passing tests are split between
`tests/unit/sequencer/test_program_pc_bridge.c` and the CPU-owned execution,
budget, and common-NOP files. The project-contract evidence for this boundary is
anchored by this heading; TI evidence is used only for the existing NOP word,
one-word width, no status effects, and sequential-flow context.

This completed slice excludes operand-field extraction, extension-word fetch, timing or
machine-cycle accounting, shared-memory mutation, data-memory buses, interrupts,
repeat state, new observer/diagnostic behavior, device profiles, emulator
adapters, and all other ISA families. Existing observer behavior is unchanged;
direct Make linkage uses an observer-enabled archive derived from the canonical
source inventory, matching the existing CMake variant and avoiding a second
handwritten source list.
