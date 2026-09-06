# Configuration knob registry

This registry is mandatory for every non-architectural compile-time or runtime
switch. Architectural state and documented silicon-variant selection are not knobs;
they belong in the architecture requirements ledger.

## Labels

- Class: `TEST`, `OBSERVE`, or `A/B`.
- Disposition: `KEEP`, `TEMP`, or `RETIRE`.

## Registry

State is `ACTIVE` when call sites/build behavior exist and `PLANNED` when the
identifier is reserved by an approved work piece but not implemented.

| Identifier | State | Class | Disposition | Owner | Default | Scope | Architectural effect | Tests | Exit/removal criterion |
|---|---|---|---|---|---|---|---|---|---|
| `TI_C54X_BUILD_TESTS` | ACTIVE | TEST | KEEP | `tests/` | On | Build | None; selects test targets | Configure-on/off and CI test discovery | Retain while CTest is supported |
| `TI_C54X_BUILD_EXAMPLES` | ACTIVE | TEST | KEEP | `examples/` | Off | Build | None; selects example targets | Minimal consumer build/run | Retain while examples ship |
| `TI_C54X_BUILD_BENCHMARKS` | ACTIVE | TEST | KEEP | `bench/` | Off | Hosted benchmark build/capture | None; selects observer-off performance tooling only | Make bench/bench-smoke/bench-record/bench-compare, CMake smoke, hosted CI tool contract | Retain while hosted performance tracking is supported |
| `TI_C54X_ENABLE_SANITIZERS` | ACTIVE | TEST | KEEP | `cmake/` | Off | Build | None; instruments local targets | ASan/UBSan CI job and non-leakage from installed target | Retain while supported toolchains provide sanitizers |
| `TI_C54X_WARNINGS_AS_ERRORS` | ACTIVE | TEST | KEEP | `cmake/` | On | Build | None; changes diagnostic severity | GCC/Clang strict build | Retain; downstream opt-out remains supported |
| `TI_C54X_ENABLE_OBSERVERS` | ACTIVE | OBSERVE | KEEP | `src/debug/` | Off | Build | None | Observer-off symbol/hot-path audit; observer-on event tests | Retain while the structured observer API is supported |
| `trace.event_mask` | ACTIVE | OBSERVE | KEEP | `src/debug/` | `UINT64_C(0)` | Per CPU instance, copied at init | None | Mask isolation and event non-interference tests | Retain while the structured observer API is supported |
| `TI_C54X_ENABLE_TEST_HOOKS` | PLANNED | TEST | TEMP | `tests/support/` | Off | Test build only | None in production builds | Installed-header and production-symbol absence tests | Remove if all required injection can be expressed through public buses/events |

## Observer-bit allocations

Observer bits are stable within schema ABI major `0`. The machine-readable registry
at [`config/knobs.json`](../config/knobs.json) is authoritative; this table is the
human review surface and keeps the logical identifier, C identifier, and mask visible.

| Logical identifier | Kind C identifier | Mask C identifier | State | Bit | Mask | Meaning |
|---|---|---|---|---:|---|---|
| `trace.event.reset` | `TI_C54X_TRACE_RESET` | `TI_C54X_TRACE_MASK_RESET` | ACTIVE | 0 | `0x0000000000000001` | CPU reset lifecycle event |
| `trace.event.fetch` | `TI_C54X_TRACE_FETCH` | `TI_C54X_TRACE_MASK_FETCH` | ACTIVE | 1 | `0x0000000000000002` | Instruction fetch event |
| `trace.event.stop` | `TI_C54X_TRACE_STOP` | `TI_C54X_TRACE_MASK_STOP` | ACTIVE | 2 | `0x0000000000000004` | Instruction-budget or fault stop event |

There are currently no observer-bit tombstones. A retired bit remains reserved in
this ABI major and must be recorded in `observer_bit_tombstones` before removal;
its mask and C identifier may not be reused.

## Registration rules

- Environment variable names are never library knobs. A hosted tool may map its own
  configuration into a registered typed field once before initialization.
- `OBSERVE` knobs must pass a state, memory-transaction, stop-reason, and timing
  equivalence test with observation off and on.
- `A/B` entries must name both implementations, a comparison corpus, a decision
  owner, and the criterion that changes the disposition to `KEEP` or `RETIRE`.
- `TEMP` entries require a concrete exit criterion before their first call site is
  committed.
- `RETIRE` entries accept no new call sites and must identify the replacement or why
  removal is behavior-neutral.
- The `uint64_t` hot event mask contains `OBSERVE` bits only. `TEST` and `A/B`
  controls use separate typed configuration owned outside opcode execution.
- Event-bit identifiers are not reused within an ABI major. Removed bits stay in a
  tombstone table until the next explicitly incompatible ABI.
- The future machine-readable `config/` registry is authoritative for identifiers,
  bit uniqueness, allowed build definitions, and lifecycle state; this document is
  checked against it.

## Lifecycle review

At each release boundary:

1. Review every `TEMP` entry and either retain its exit criterion or move it to
   `KEEP`/`RETIRE`.
2. Reject new call sites for every `RETIRE` entry.
3. Verify `OBSERVE` equivalence tests and compile-out checks.
4. Verify `TEST` knobs are absent from installed headers and production symbols.
5. Require a comparison report and decision owner for each `A/B` knob.
6. Record removed IDs in the ABI-major tombstone list.
