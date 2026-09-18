### P3-S27: ADD/SUB #lk rows (planned)

This immutable anchor heads the P3-S27 evidence freeze. Four
catalog rows land the wave-5 #lk leaves over the shared 2-word
accumulator layouts: `ADD_LK_SHIFT` (`0xF000/0xFCF0`),
`ADD_LK_16` (`0xF060/0xFCFF`), `SUB_LK_SHIFT`
(`0xF010/0xFCF0`), and `SUB_LK_16` (`0xF061/0xFCFF`), all
`UNRESOLVED` repeatability with `ACCUMULATORS_STATUS_SXM_OVM`
inputs, wiring the landed P3-WAVE5 plain plus trusted
`op_add_lk`/`op_sub_lk` handlers through the catalog X-macro
into dispatch. No handler, layout, decode, or CPU change; the
generated first-word lookup grows with zero neighbor moves.
`FORM-T2-01-07`, `FORM-T2-01-08`, `FORM-T2-02-07`, and
`FORM-T2-02-08` bind at `SUBSET` under `IMMEDIATE_LK` (the RPT_LK
precedent; the schema forbids SUBSET bindings at `ALL`).
Projected closure
adds one evidence record, one requirement, and no new test
record on the extended ADD/SUB projection plus binding suites:
448 evidence records, 694 requirements, and 554 tests (554
`PASSING`, 0 `PLANNED`). These numbers are projections until
the slice lands.
