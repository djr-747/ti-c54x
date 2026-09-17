### P2-W28: TC-aware status foundation and ROLTC/SFTC vertical slice (planned)

State: both stages are implemented; P2-W28A is fully green locally and remotely,
and P2-W28B targeted, independent review, and full local checks are green. The
work piece is intentionally two-stage because
the opcode forms expose a shared TC status input/output seam that the earlier
C-only accumulator effect could not represent. The first stage is the smallest
demand-driven generalization; the second stage is the opcode proof that consumes
it. No speculative overflow, memory, interrupt, repeat, parallel-issue, device,
public-API, or timing surface is admitted.

#### P2-W28A — shared TC/status foundation (implemented; local and remote green)

The state/status owner added private typed TC read/write accessors with explicit
knownness, backed by the existing ST0 TC bit. The input path added a distinct
`STATUS_TC` resource and a composite `ACCUMULATORS_STATUS_TC` resource, copies
`status_tc/status_tc_known` into `op_input`, and collects TC only when requested.
The accumulator-sequencer owner replaced the C-only output shape with a
value-owned status delta containing independent C and TC actions:
`PRESERVE`, `WRITE_0`, or `WRITE_1`. Existing MAX/MIN, ROL/ROR, SFTL, CMPL, and
logical handlers migrated to that shape without changing their established
semantics. The accumulator transaction validates the candidate accumulator and
the complete status delta before committing both atomically; rejected effects
leave all copied state unchanged.

Closeout: the new seams are private APIs only. The C+TC delta is value-owned,
all prior accumulator handlers are migrated, and `src/cpu/cpu.c`, the public
API, memory, RPT/IRQ, timing, and OVA/OVB behavior are unchanged. Local targeted
tests and the full local `make test` gate are green; remote CI is green at
[run 34058489923](https://github.com/djr-747/ti-c54x/actions/runs/34058489923).

Ownership is explicit and mirrors the tree: `src/state/status/` owns TC access;
`src/isa/catalog/` owns the resource-mask identity; `src/isa/ops/op_input.h`
owns the copied snapshot fields; `src/cpu/input/` owns collection;
`src/isa/ops/op_effect.h` owns the value-only delta type; and
`src/sequencer/accumulator/` owns its validation and atomic application.
Existing family handlers remain in their family subtrees, and
`src/cpu/cpu.c` remains generic orchestration only. The RED tests
are exactly `tests/unit/state/status/test_tc_field.c`,
`tests/unit/cpu/input/test_collect_tc.c`, and
`tests/unit/sequencer/accumulator/test_accumulator_status_delta.c`. Phase A
GREEN also requires the existing accumulator-family tests to pass after
migration. No public header, new effect kind, CPU opcode branch, global state,
allocation, observer, environment read, or hot-path diagnostic is allowed.

#### P2-W28B — ROLTC/SFTC vertical slice

State: implemented; targeted checks, independent review, and the full local
`make test` gate are green.

P2-W28A was GREEN before the rotate and shift family lanes implemented pure
handlers and family-local tests in parallel. Catalog/decode metadata and the
generated lookup were RED-first; production registration and all shared
integration remained serialized by the orchestrator. Both forms are one program
word and use the existing accumulator-source layout. The only reported shared
tooling gap was the catalog schema's missing TC resource-token allowlist entries;
that owner was extended centrally under a focused RED/GREEN schema test, without
changing CPU or execution architecture.

TI `SPRU172C` §4 ROLTC (printed p. 4-144) gives value `0xF492`, mask `0xFEFF`:
the selected accumulator's pre-TC enters bit 0, low32 bits 30:0 move to
bits 31:1, old bit 31 becomes C, guard bits 39:32 clear, and TC is preserved.
The selected accumulator and TC must therefore be known; C is written through
the status delta and TC is preserved. TI §4 SFTC (printed p. 4-157) gives value
`0xF494`, mask `0xFEFF`: if low32 is zero, the accumulator is unchanged and
TC becomes 1; otherwise, when bit31 xor bit30 is zero, low32 shifts left one
with guards preserved and TC becomes 0; when that predicate is nonzero, the
accumulator is unchanged and TC becomes 1. SFTC preserves C and does not read
pre-TC. Guard-only and low32-zero combinations remain explicitly unresolved
where the source does not distinguish them; no inference from summary fields is
permitted.

The catalog result is 15 rows and 1081 populated first-word encodings
(two additional rows with two encodings each), while reset-runnable coverage is
four of 15. Required ownership tests are family-local ROLTC and SFTC handler
tests under `tests/unit/isa/ops/rotate/` and `tests/unit/isa/ops/shift/`,
catalog/decode metadata tests under their owning subtrees, and CPU retirement
proofs under `tests/unit/cpu/`. Repeatability for both forms is unresolved;
summary cycle fields are not timing evidence. If either lane needs a new layout,
status resource beyond P2-W28A, opcode-specific CPU logic, memory, RPT/IRQ,
parallel issue, timing, or public API, it reports a hard GAP with the evidence
locator, missing contract, owner, tests, and hot-path consequence before any
shared edit. Neither family lane reported such an architecture gap; targeted
opcode/CPU tests, catalog quality checks, independent review, and the full
local gate are green.
