### P3-S19: SUB Smem rows (planned)

This immutable anchor heads the P3-S19 evidence freeze. `SUB Smem`
syntaxes 1-3 (TI SPRU172C §SUB, pp. 4-186-4-187) land three catalog
rows over the shared ADD-Smem layouts with the landed P3-S14 plain
plus trusted `op_sub_smem` handlers: `SUB_SMEM` (`0000_100S_IAAAAAAA`,
`0x0800/0xFE00`, syntax 1 `src -= Smem`), `SUB_SMEM_TS`
(`0000_110S_IAAAAAAA`, `0x0C00/0xFE00`, syntax 2 `src -= Smem << TS`),
and `SUB_SMEM_SH16` (`0100_00SD_IAAAAAAA`, `0x4000/0xFC00`, syntax 3
`dst = src - (Smem << 16)`), all `UNRESOLVED` repeatability with
`ACCUMULATORS_STATUS_SXM_OVM` inputs (plus `T_REGISTER` for syntax 2).
No handler, layout, decode, or CPU change: dispatch binds by the
catalog X-macro, and the generated first-word lookup grows by 2048
entries with zero neighbor moves. `FORM-T2-02-01/02/03` bind at
`SUBSET` under `DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3`; syntax 4
(two-word SHIFT), Xmem/dual/`#lk`/accumulator syntaxes, `SUBB`,
`SUBC`, and `SUBS` stay unimplemented. Projected closure adds two
evidence records, one requirement, and no new test record on the
extended projection plus landed handler suites: 431 evidence records,
680 requirements, and 546 tests (546 `PASSING`, 0 `PLANNED`). These
numbers are projections until the slice lands.
