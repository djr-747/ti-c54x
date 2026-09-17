### P3-S26: SUBS Smem rows (implemented; local verification green)

This immutable anchor heads the P3-S26 evidence freeze. `SUBS Smem`
(TI SPRU172C §SUBS, p. 4-194: `0000_101S_IAAAAAAA`, class 3A/3B)
lands one catalog row over the shared `SELECT_INDIRECT_ADDR7`
layout: `SUBS_SMEM` (`0x0A00/0xFE00`, `UNRESOLVED` repeatability,
`ACCUMULATORS_STATUS_OVM` inputs) with the landed P3-S23 plain
plus trusted `op_subs_smem` handlers wired through the catalog
X-macro into dispatch. The slice registers the new
`ACCUMULATORS_STATUS_OVM` input combination (accumulators plus
OVM; C is written but never read and SXM is never consumed, so
neither bit is set, matching the SUB Smem precedent) in the
catalog header and the schema allowlist. No handler, layout,
decode, or CPU change; the generated first-word lookup grows by
512 entries with zero neighbor moves. `FORM-T2-02-13` binds at
`SUBSET` under `DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3`;
long-offset and absolute Smem extension words stay undecoded.
Projected closure adds one evidence record, one requirement, and
no new test record on the extended ADD/SUB projection plus
binding suites: 443 evidence records, 687 requirements, and 548
tests (548 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.
