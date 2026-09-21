### P3-S103: RESET software-reset row and leaf (implemented; local verification green)

This immutable anchor heads the P3-S103 evidence freeze. One
single-word control row lands on the misc-control seam: `RESET`
(`0xF7C0` exact, TI SPRU172C section RESET, printed pp.
4-137–4-138) with the `NONE` layout, `NONE` input resources,
and INELIGIBLE repeatability (TI: not repeatable). Semantics
(TI execution list): PC takes (IPTR) shifted left 7; OVA, OVB,
ARP, DP, ASM, BRAF, HM, C16, FRCT, CMPT, CPL, and OVM clear to
0; C, TC, SXM, XF, and INTM set to 1; IFR clears to 0. MP/MC
is not sampled and IPTR/peripheral initialization differs from
RS, both outside the common subset. The `op_reset` leaf (IDLE
sibling precedent: checked plus trusted entries, metadata
cross-check, transactional rejection) stages one new
`RESET` effect; the CPU retires it through a dedicated reset
applier that snapshots IPTR first (unknown IPTR stops
INVALID_STATE: no successor is provable), writes exactly the
manual's field list via the checked status writers, clears
IFR, sets PC to IPTR<<7, counts one retirement, cancels any
pending delayed-branch countdown, and clears the CPU-owned
repeat and take latches. Accumulators, T, ARs, SP, RC, SWWSR,
CLKMD, and unlisted PMST fields are preserved. No dispatch
change beyond the automatic X-macro arm. The T2-18-04 (RESET)
form flips to `COMMON_CANDIDATE`/`COMPLETE` with one binding
under ALL.
Projected closure adds one TI section record, two
ARCHITECTURAL requirements, and two COMMON test records on the
op and CPU suites: 504 evidence records, 829 requirements, and
683 tests (683 `PASSING`, 0 `PLANNED`); 377 bindings, 182/205
forms (88.780%), 49 strict-complete (23.902%), 353 rows, 354
IDs, 43865 lookup slots. These numbers are projections until
the slice lands.
