### P3-S14: SUB Smem slice (planned)

SUB Smem (TI SPRU172C section Sec4 SUB p.4-187), form FORM-T2-02-01:
syntax 1 (`src -= Smem`), syntax 2 (`src -= Smem << TS`), and syntax 3
(`dst = src - (Smem << 16)`) over direct page-0 and standard-indirect
MOD0--MOD3 reads through the memory effect.

Semantics: the Smem word is SXM-extended to 40 bits exactly like the ADD
Smem sibling (sign extension when SXM=1 and bit 15 is set, zero extension
otherwise), then shifted per syntax (unshifted; T-register shift with the
established 0x00--0x1F left / 0x30--0x3F arithmetic-right bands, 0x20--0x2F
rejected; fixed logical <<16), then subtracted from the S-selected
accumulator via `ti_c54x_u40_sub_checked`. Syntaxes 1-2 write C from the
bit-32 borrow (C set means no borrow), mirroring the SUB_ACC_SHIFT leaf;
syntax 3 sets C on no-borrow and otherwise preserves it, mirroring the
explicit TI rule for the fixed <<16 form on the ADD side. OVM saturation
pins to 0x007FFFFFFF / 0xFF80000000 with sticky OVdst, TC is preserved,
and exotic indirect modes (mode4 above MOD3) defer as INVALID_OPERAND
with the caller effect untouched.

No-catalog decision: `opcode_catalog.def` carries ADD_SMEM / ADD_SMEM_TS /
ADD_SMEM_SH16 but no SUB Smem rows, and slice ownership forbids touching
the catalog, dispatch, or build manifests. The new leaf therefore keys
its checked entry points on the shared operand layouts
(SELECT_INDIRECT_ADDR7 for syntaxes 1-2,
SOURCE_DESTINATION_INDIRECT_ADDR7 for syntax 3) instead of opcode
metadata, with trusted-operand entry points alongside. Catalog rows plus
dispatch/dispatch-binding coverage are follow-up work for the slice that
owns the inventory.

RED contract: `tests/unit/isa/ops/arithmetic/test_sub_smem.c` (16 cases:
direct/B-select subtracts, SXM sign/zero extension, borrow clears C,
OVM negative and positive saturation, MOD2 indirect staging, exotic-mode
deferral with effect preservation, TS left and arithmetic-right shifts,
T unknown/out-of-band gates, SH16 set-on-no-borrow versus
borrow-preserve C, cross-accumulator SH16, unknown-input stops, invalid
raw40/SXM rejection, trusted completions for all three syntaxes,
malformed trusted operands, crossed-layout decode rejection, null
rejection including the read completion).

Evidence: standalone leaf build plus focused test binary, both green
under the repo's strict warning set with sanitizers (see verification
commands in the slice report).
