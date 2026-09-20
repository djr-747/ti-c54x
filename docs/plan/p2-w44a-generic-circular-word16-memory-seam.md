### P2-W44A: generic circular WORD16 MEMORY seam (implemented; local verification green)

This immutable `(planned)` heading remains the FIT/ownership anchor for the
W44A activation. W44A is now implementation-complete. The private typed
`CIRCULAR_WORD16` specification admits CMPT=0 WORD16
MOD8/MOD10 reads and writes without catalog rows or opcode handlers. The
operation-layer spec carries only copied ARF and typed circular-mode inputs;
the resolved address effect carries copied AR/BK snapshots. Resolution reads
dependencies in strict CMPT, ARP, selected AR, then BK order, admits selected
AR0–AR7, preserves BK=0 as a valid NONE/no-update result, and applies the
registered positive-BK wrap domain. Unknown or stale dependencies reject.
The root candidate and existing generic transaction preserve WAIT, READY,
FAULT, and cancel transactionality, with READY atomically committing any
selected-AR update, sequential PC, and completed effect; no root transaction
source change was needed.

The three owner-local RED contracts are separated across
`src/isa/ops/memory/`, `src/sequencer/memory/`, and
`src/sequencer/instruction/`, with tests mirrored under those owners. RED then
GREEN validation passed the three recorded C executable runners:
`ti_c54x.circular_word16_memory_effect`,
`ti_c54x.circular_word16_address_resolution`, and
`ti_c54x.circular_word16_memory_transaction` (each via `make_goal=test`).
The implementation does not claim LONG32 scaling, MOD9/MOD11, MOD14,
CMPT=1/dual-indirect, initial-boundary policy beyond the existing planner, BK
MMR routing, timing, device/MMR, IRQ/RPT, public ABI, or catalog scope.

Current owner sizes are `memory_effect.c` 78 lines,
`address_resolution.c` 222 lines, `memory_instruction_candidate_validate.c`
316 lines (confirmed current count), and
`memory_instruction_transaction.c` 335 lines unchanged. No split is needed;
the candidate validator remains under the 500-line review threshold and must
be reviewed before crossing it.

W44B remains the later twenty-row MOD8/MOD10 family fan-out, projected at
`ID_COUNT=76`, 2,343 populated first-word lookup entries, and 75 catalog
bindings. Those projection values are intentionally excluded from W44A's
completed implementation and coverage accounting.

Closure accounting is 166 evidence records; 266 requirements (229
IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 220 tests (212 PASSING, 8 PLANNED);
and 7 sources. Catalog identity remains ID_COUNT=56 with 2,103 populated
first-word lookup entries and 55 bindings; inventory coverage remains 35/205 =
17.073% catalog-family progress and 25/205 = 12.195% strict complete-form
coverage.
