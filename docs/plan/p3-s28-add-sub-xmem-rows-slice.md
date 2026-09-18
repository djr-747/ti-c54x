### P3-S28: ADD/SUB Xmem rows (implemented; local verification green)

This immutable anchor heads the P3-S28 evidence freeze. Two
catalog rows land the wave-5 Xmem leaves over the shared
accumulator/dual-XMOD/XAR/shift-4 layout: `ADD_XMEM`
(`0x9000/0xFE00`) and `SUB_XMEM` (`0x9200/0xFE00`), both
`UNRESOLVED` repeatability with
`AUXILIARY_REGISTERS_STATUS_SXM_OVM` inputs (the
`LD_XMEM_SHIFT`/`STH_SHFT_XMEM` sibling precedent), wiring the
landed P3-WAVE5 plain plus trusted `op_add_xmem`/`op_sub_xmem`
handlers through the catalog X-macro into dispatch. No handler,
layout, decode, or CPU change; the generated first-word lookup
grows by 1024 entries with zero neighbor moves.
`FORM-T2-01-05` and `FORM-T2-02-05` bind at `COMPLETE` under
`ALL` (the `LD_XMEM_SHIFT` precedent; the landed leaves cover
all four XMOD modes). Projected closure adds one evidence
record, one requirement, and no new test record on the extended
ADD/SUB projection plus binding suites: 452 evidence records,
697 requirements, and 556 tests (556 `PASSING`, 0 `PLANNED`).
These numbers are projections until the slice lands.
