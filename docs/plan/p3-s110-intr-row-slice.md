### P3-S110: INTR software-interrupt row and leaf (implemented; local verification green)

This immutable anchor heads the P3-S110 evidence freeze. One
single-word control row lands on the trap seam: `INTR K`
(`0xF7C0` under mask `0xFFE0`, TI SPRU172C section INTR,
printed p. 4-65) with the shared `TRAP_IMMEDIATE_K` layout
(5-bit K at word bits 4:0), `STACK_POINTER` input resources,
and INELIGIBLE repeatability (TI: not repeatable). Semantics
(TI execution): SP decrements by one, PC+1 pushes to TOS, the
K vector loads into PC, 1 loads into INTM, and the
corresponding IFR bit clears (IMR has no effect; INTR
executes regardless of INTM). The `op_intr` leaf (stack home,
checked plus trusted entries, metadata cross-check,
transactional rejection) stages one `STACK_PUSH_INTR_WORD16`
MEMORY effect (new address kind 34, all-zero arm, WRITE
access, zero staged value, the INTM-setting take completion);
K rides the frozen bundle word to the root. The root freezes
the sequential successor (PC+1) into the TOS WRITE like the
trap push, freezes the K entry address `(IPTR << 7) + (K <<
2)` into the LONG32-MSW carrier through the shared
software-interrupt freeze dispatcher (unknown IPTR rejects;
the K-indexed IFR knownness is proved at prepare whenever K
names a 16-bit IFR position), and retires through the shared
take-commit redirect with no RTN load; the completion sets
INTM and the retirement tail clears the K-indexed IFR bit (K
at 16 or above names no 16-bit IFR position and skips the
clear). The T2-14-01 (INTR) form flips to
`COMMON_CANDIDATE`/`COMPLETE` with one binding under ALL.
Landing adds one TI section record, three ARCHITECTURAL
requirements (leaf semantics, CPU retirement, catalog
registration), and three COMMON test records on the op, CPU,
and metadata suites: 509 evidence records, 844
requirements, and 698 tests (698 `PASSING`, 0 `PLANNED`); 382
bindings, 187/205 forms (91.220%), 54 strict-complete
(26.341%), 358 rows, 359 IDs, 44697 lookup slots.
