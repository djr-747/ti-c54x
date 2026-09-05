# Configuration knob registry

This registry is mandatory for every non-architectural compile-time or runtime
switch. Architectural state and documented silicon-variant selection are not knobs;
they belong in the architecture requirements ledger.

## Labels

- Class: `TEST`, `OBSERVE`, or `A/B`.
- Disposition: `KEEP`, `TEMP`, or `RETIRE`.

## Registry

| Identifier | Class | Disposition | Owner | Default | Scope | Architectural effect | Tests | Exit/removal criterion |
|---|---|---|---|---|---|---|---|---|
| `TI_C54X_ENABLE_OBSERVERS` | OBSERVE | KEEP | `src/debug/` | Off | Build | None | Observer-off symbol/hot-path audit; observer-on event tests | Retain while the structured observer API is supported |
| `trace.event_mask` | OBSERVE | KEEP | `src/debug/` | `UINT64_C(0)` | Per CPU instance, copied at init | None | Mask isolation and event non-interference tests | Retain while the structured observer API is supported |
| `TI_C54X_ENABLE_TEST_HOOKS` | TEST | TEMP | `tests/support/` | Off | Test build only | None in production builds | Installed-header and production-symbol absence tests | Remove if all required injection can be expressed through public buses/events |

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
