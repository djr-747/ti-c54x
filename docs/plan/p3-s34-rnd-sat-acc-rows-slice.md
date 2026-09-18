### P3-S34: RND/SAT accumulator rows (implemented; local verification green)

This immutable anchor heads the P3-S34 evidence freeze. It
lands the Wave-7 pilot end to end: two worker-built pure
accumulator leaves (`op_rnd_acc` staging dst40 = src40 plus
8000h with 40-bit wrap and OVM-gated saturation of a
signed-32 overflowing result with sticky OVdst, and
`op_sat_acc` clamping src40 to signed 32 bits regardless of
OVM with OVsrc recomputed, both with preserved C/TC and
checked plus trusted entries); one shared-contract
extension adding `WRITE_0` recompute-to-clear to the
destination-overflow action with a sequencer commit branch
that clears the destination OV flag while preserving its
sibling; and two `MASKED` catalog rows `RND_ACC`
(`0xF49F/0xFCFF`, TI SPRU172C section RND p. 4-142,
`INELIGIBLE` by explicit section note and Table 2-28 with
`ACCUMULATORS_STATUS_OVM` inputs) and `SAT_ACC`
(`0xF483/0xFEFF`, section SAT p. 4-154, `UNRESOLVED` like
its MAX/MIN/EXP/CMPL accumulator-unary neighbors with
`ACCUMULATORS` inputs), wired through the catalog X-macro
into dispatch. `FORM-T2-06-13` and `FORM-T2-06-14` bind at
`SUBSET` under `REGISTER_DST` and flip
`UNRESOLVED`/`UNIMPLEMENTED` to
`COMMON_CANDIDATE`/`SUBSET` with the rows. The generated
first-word lookup grows by 6 entries with zero neighbor
moves. Projected closure adds two evidence records, seven
requirements, and six test records on the new semantics
suites plus the binding suites: 461 evidence records, 718
requirements, and 574 tests (574 `PASSING`, 0 `PLANNED`).
These numbers are projections until the slice lands.
