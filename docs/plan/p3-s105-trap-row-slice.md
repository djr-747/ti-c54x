### P3-S105: TRAP software-interrupt row and leaf (planned)

This immutable anchor heads the P3-S105 evidence freeze. One
single-word control row lands on the trap seam: `TRAP K`
(`0xF4C0` under mask `0xFFE0`, TI SPRU172C section TRAP,
printed p. 4-195) with the new `TRAP_IMMEDIATE_K` layout
(5-bit K at word bits 4:0), `STACK_POINTER` input resources,
and INELIGIBLE repeatability (TI: not repeatable). Semantics
(TI execution): SP decrements by one, PC+1 pushes to TOS, the
K vector loads into PC, and no status bit changes (not
affected by INTM, does not affect INTM, no IFR effect). The
`op_trap` leaf (control home, checked plus trusted entries,
metadata cross-check, transactional rejection) stages one
`STACK_PUSH_TRAP_WORD16` MEMORY effect (new address kind 32,
all-zero arm, WRITE access, zero staged value, null
completion); K rides the frozen bundle word to the root. The
root freezes the sequential successor (PC+1) into the TOS
WRITE like the immediate pushes, freezes the K entry address
`(IPTR << 7) + (K << 2)` into the LONG32-MSW carrier (unknown
IPTR rejects; the RESET unknown-IPTR precedent), and retires
through the shared take-commit redirect with no RTN load and
no INTM/IFR mutation. No run.c change beyond the generic
MEMORY route with the redirect flag keyed off the TRAP kind.
The T2-14-02 (TRAP) form flips to `COMMON_CANDIDATE`/
`COMPLETE` with one binding under ALL. INTR stays
UNIMPLEMENTED: its IFR-bit clearing needs a K-to-bit mapping
decision (device-dependent) that TRAP never touches.
Projected closure adds one TI section record, three
ARCHITECTURAL requirements (leaf semantics, CPU retirement,
catalog registration), and three COMMON test records on
the op, CPU, and metadata suites: 506 evidence records, 835
requirements, and 689 tests (689 `PASSING`, 0 `PLANNED`); 379
bindings, 184/205 forms (89.756%), 51 strict-complete
(24.878%), 355 rows, 356 IDs, 44153 lookup slots. These
numbers are projections until the slice lands.
