### P3-S32: ADDM RMW-ADD seam (implemented; local verification green)

This immutable anchor heads the P3-S32 evidence freeze. It
lands Seam 2 end to end: a new `RMW_OPERATION_ADD` carrier
value (the lk addend rides the existing `value_word`; OVM/SXM
travel in the already-copied frozen input snapshot, so the
carrier keeps its 48-byte shape), a value-owned 16-bit
reducer (`rmw_transform_add`: wrapped sum, bit-16 carry,
SXM-selected overflow, OVM saturation), sequencer staging of
the carry/overflow beside the write word in the root
transaction's scalar padding (root size unchanged on
LP64/ILP32), and a sticky C/OVA commit (`WRITE_1` on the
frozen flags, otherwise preserved) shared by the bus and MMR
RMW-WRITE paths. The `op_addm_smem_mod0_3` leaf (plain plus
trusted entries) reuses the neutral two-word
`ARF_EXTENSION16_MOD` shape with `STATUS_SXM_OVM` inputs and
OVM/SXM knownness gating, and the `MASKED` catalog row
`ADDM_SMEM_MOD0_3` (`0x6B80/0xFFE0`, TI SPRU172C section
ADDM `0110 1011 I AAAAAAA`, nonrepeatable) sits at the next
disjoint first-word slot after XORM. `FORM-T2-01-12` binds
at `SUBSET` under `STANDARD_INDIRECT_MOD0_MOD3` and flips
`UNRESOLVED`/`UNIMPLEMENTED` to `COMMON_CANDIDATE`/`SUBSET`
with the row. The generated first-word lookup grows by 32
entries with zero neighbor moves. There is no SUBM
instruction in the TI source, so the seam covers ADDM only.
Projected closure adds one evidence record, three
requirements, and four test records on the new semantics
suites plus the binding suites: 457 evidence records, 705
requirements, and 562 tests (562 `PASSING`, 0 `PLANNED`).
These numbers are projections until the slice lands.
