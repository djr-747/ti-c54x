### P3-S29: ADD/SUB src-ASM rows (implemented; local verification green)

This immutable anchor heads the P3-S29 evidence freeze. Two
catalog rows land the wave-5 src-ASM leaves over the shared
accumulator source/destination layout: `ADD_SRC_ASM`
(`0xF480/0xFCFF`) and `SUB_SRC_ASM` (`0xF481/0xFCFF`), both
`UNRESOLVED` repeatability with
`ACCUMULATORS_STATUS_SXM_OVM_ASM` inputs (the `LD_SRC_ASM`
sibling precedent), wiring the landed P3-WAVE5 plain plus
trusted `op_add_src_asm`/`op_sub_src_asm` handlers through the
catalog X-macro into dispatch. No handler, layout, decode, or
CPU change; the generated first-word lookup grows by 8 entries
with zero neighbor moves. `FORM-T2-01-10` and `FORM-T2-02-10`
bind at `COMPLETE` under `ALL` (the `LD_SRC_ASM` precedent;
the landed leaves cover all four S/D combinations) and flip
`UNRESOLVED` to `COMMON_CANDIDATE` with the rows. Projected
closure adds one evidence record, one requirement, and no new
test record on the extended ADD/SUB projection plus binding
suites: 453 evidence records, 698 requirements, and 556 tests
(556 `PASSING`, 0 `PLANNED`). These numbers are projections
until the slice lands.
