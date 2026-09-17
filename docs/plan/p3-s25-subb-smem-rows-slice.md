### P3-S25: SUBB Smem rows (implemented; local verification green)

This immutable anchor heads the P3-S25 evidence freeze. `SUBB Smem`
(TI SPRU172C §SUBB, p. 4-191: `0000_111D_IAAAAAAA`, class 3A/3B)
lands one catalog row over the shared `SELECT_INDIRECT_ADDR7`
layout: `SUBB_SMEM` (`0x0E00/0xFE00`, `UNRESOLVED` repeatability,
`ACCUMULATORS_STATUS_C_OVM` inputs) with the landed P3-S24 plain
plus trusted `op_subb_smem` handlers wired through the catalog
X-macro into dispatch. The slice registers the new
`ACCUMULATORS_STATUS_C_OVM` input combination (accumulators plus
C plus OVM, no SXM) in the catalog header and the schema
allowlist. No handler, layout, decode, or CPU change; the
generated first-word lookup grows by 512 entries with zero
neighbor moves. `FORM-T2-02-11` binds at `SUBSET` under
`DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3`; long-offset and
absolute Smem extension words stay undecoded. Projected closure
adds one evidence record, one requirement, and no new test record
on the extended ADD/SUB projection plus binding suites: 442
evidence records, 686 requirements, and 548 tests (548 `PASSING`,
0 `PLANNED`). These numbers are projections until the slice lands.
