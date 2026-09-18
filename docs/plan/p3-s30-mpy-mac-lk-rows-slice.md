### P3-S30: MPY/MAC #lk rows (implemented; local verification green)

This immutable anchor heads the P3-S30 evidence freeze. Two
catalog rows land the wave-6 #lk leaves over the shared 2-word
accumulator layouts: `MPY_LK` (`0xF066/0xFEFF`) and `MAC_LK`
(`0xF067/0xFCFF`), both `UNRESOLVED` repeatability with
`ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM` inputs (the
`MPY_SMEM`/`MAC_SMEM` sibling precedent), wiring the landed
P3-WAVE6 plain plus trusted `op_mpy_lk`/`op_mac_lk` handlers
through the catalog X-macro into dispatch. No handler, layout,
decode, or CPU change; the generated first-word lookup grows
by 6 entries with zero neighbor moves. `FORM-T2-03-05` and
`FORM-T2-04-02` bind at `COMPLETE` under `ALL`: the TI opcode
tables pin no R bit in either #lk space (MPY syntax 4 and MAC
syntax 3 are unrounded-only encodings), so the unrounded
leaves cover them fully. Both forms flip `UNRESOLVED` to
`COMMON_CANDIDATE` with the rows. Projected closure adds one
evidence record, one requirement, and no new test record on
the binding suite (the landed wave-6 semantics suites already
cover the leaves; the scope gate forbids linking those
`COMMON_TI` suites to this `PROJECT_CONTRACT` requirement):
454 evidence records, 699 requirements, and 556 tests (556
`PASSING`, 0 `PLANNED`). These numbers are projections until
the slice lands.
