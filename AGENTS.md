# Repository working rules

- This is a standalone TI C54x library. Production code and public headers must not
  include or depend on Nokia emulator, QEMU, Osmocom, firmware, or host-application
  sources.
- Build and test through this repository only. Do not invoke validation targets from
  another repository.
- Use test-driven development. A behavior is not implemented until its requirement,
  evidence, and executable test are present.
- Do not copy source, opcode tables, comments, or structure from the legacy emulator
  core. Treat it only as a later black-box differential oracle.
- Keep the main agent as the orchestrator and commit owner. Delegated agents must use
  bounded file ownership, must not revert other agents, and must not create commits.
- Keep commits atomic. Run the local fast test suite once when a coherent work piece
  is ready to commit.
- Log slice progress in `docs/plan/`, `docs/implementation-plan.md`, and
  `docs/development-ledger.md`. Each slice owns one `docs/plan/<id>-<slug>.md`
  doc whose `###` heading is the FIT/ownership anchor: never reword it, only
  advance its status parenthetical (`(planned)` →
  `(implemented; local verification green)`). The plan's slice link mirrors the
  same parenthetical. Implementation commits carry code, tests,
  registry/evidence, and generated files only. The follow-up ledger-only commit
  flips both parentheticals and appends the ledger entry recording the completed
  commit hash, validation evidence, decisions, knob changes, and follow-up work.
  Never rewrite an old ledger entry; append a correction.
- Keep all mutable runtime state instance-owned. No environment reads, file I/O,
  mutable process globals, hidden allocators, or logging in the core execution path.
- The library must never call `getenv`. Hosted tools may read environment once before
  CPU initialization and pass explicit immutable configuration. Optional observation
  state is a copied per-instance `uint64_t` mask containing `OBSERVE` bits only;
  `TEST` and `A/B` controls use separate typed cold configuration.
- All hot-path observation checks go through one compile-time-aware macro using
  short-circuit `&&`. An observer-disabled build must compile those sites out.
  Opcode handlers never format, print, or directly fan out diagnostic callbacks.
- Every non-architectural knob must be registered in `docs/knobs.md` with exactly one
  class (`TEST`, `OBSERVE`, or `A/B`), one disposition (`KEEP`, `TEMP`, or `RETIRE`),
  an owning module, default, scope, tests, and an exit/removal criterion. Unregistered
  string, environment, compile-time, or runtime knobs are forbidden.
- Every handwritten source and header must begin with a concise module contract that
  states what it owns and what it must not own. Public APIs must document copied,
  borrowed, and caller-owned objects and their required lifetimes.
- Avoid monoliths. Keep orchestration, arithmetic, addressing, decode, sequencing,
  devices, serialization, tracing, and opcode families in separate modules. Review
  any handwritten file approaching 500 lines and require an explicit split decision
  before it exceeds 800 lines. Generated tables are exempt and remain isolated.
- The directory tree must mirror the ownership hierarchy documented in
  `docs/implementation-plan.md`. Do not place new implementation or test files in a
  flat catch-all directory when an owning module directory exists.
- Use explicit units in names and interfaces: program words, data words,
  instructions, and machine cycles are not interchangeable.
- Machine-cycle APIs remain unavailable until timing is supported by evidence and
  tests. Early execution APIs use instruction budgets.
- Integration with an emulator is deferred until provenance and licensing are
  resolved and the standalone contracts are stable.
