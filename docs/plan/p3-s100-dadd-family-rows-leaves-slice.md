### P3-S100: DADD-family long-word rows and leaves (planned)

This immutable anchor heads the P3-S100 evidence freeze. Five
double-precision/dual-16-bit long-word rows land on the
established kind-21 seam through the third managed fan-out (one
isolated-worktree worker for the five-leaf family,
maintainer-sourced TI briefs, maintainer merge): `DADST`
(`0x5A00/0xFE00`, TI SPRU172C section DADST p. 4-39),
`DRSUB` (`0x5800/0xFE00`, section DRSUB p. 4-43), `DSADT`
(`0x5E00/0xFE00`, section DSADT p. 4-45), `DSUB`
(`0x5400/0xFE00`, section DSUB p. 4-48), and `DSUBT`
(`0x5C00/0xFE00`, section DSUBT p. 4-50), each with the shared
source/destination/indirect/addr7 layout and UNRESOLVED
repeatability. The T-reading rows (DADST/DSADT/DSUBT) carry the
new `ACCUMULATORS_T_REGISTER_STATUS_SXM_OVM_C16` input token.
Each leaf stages one kind-21 LONG32 READ with a C16-gated
completion (40-bit arithmetic with OVM/C/OV at C16=0; dual
independent 16-bit halves with no status effects at C16=1),
plus checked and trusted entries: DADST adds Lmem+(T+T<<16)
(dual halves Lmemhi+T and Lmemlo-T), DRSUB completes the
reversed Lmem-src, DSADT subtracts (T+T<<16) (dual halves
Lmemhi-T and Lmemlo+T), DSUB completes src-Lmem, and DSUBT
subtracts (T+T<<16) (dual Lmem-T halves). No dispatch,
sequencing, or CPU change beyond the automatic X-macro arms;
the five T2-05 forms flip to `COMMON_CANDIDATE`/`SUBSET` with
one binding each under the DADD addressing token. Projected
closure adds five TI section records, ten ARCHITECTURAL
requirements, and ten COMMON test records on the fifteen new
suites: 500 evidence records, 821 requirements, and 675 tests
(675 `PASSING`, 0 `PLANNED`); 373 bindings, 178/205 forms
(86.829%), 349 rows, 350 IDs, 43096 lookup slots. These
numbers are projections until the slice lands.
