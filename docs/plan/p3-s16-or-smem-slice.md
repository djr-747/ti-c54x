### P3-S16: OR Smem slice (implemented; local verification green)

`OR Smem, dst` (TI SPRU172C Sec4 OR, p. 4-123: `dst |= Smem`) lands
one catalog-free pure leaf: `ti_c54x_op_or_smem_values` plus
`ti_c54x_trusted_op_or_smem` in
`src/isa/ops/logical/op_or_smem.c/.h`, proved by
`tests/unit/isa/ops/logical/test_or_smem.c`.

Contract (derived from in-repo siblings only):

- Operand shape mirrors the in-repo `DESTINATION_INDIRECT_ADDR7`
  layout used by single-destination Smem forms (D@8, I@7, addr@6:0):
  three copied values `(destination, indirect, addr7)` with widths
  1/1/7. No catalog row exists for this form yet, so the leaf takes
  fields directly and catalog/decode/dispatch binding is explicitly
  deferred to a later slice; no encoding constant is introduced here.
- Read staging reuses `src/isa/ops/memory/memory_effect.c` through
  `src/isa/ops/op_effect.h`: a single WORD16 READ with
  `value_word = 0` and `load_t_with_read = false`. Direct (`I = 0`)
  stages `DIRECT_WORD16`; indirect splits `addr7` into mode/ARF
  exactly like the sibling Smem leaves, stages
  `SINGLE_INDIRECT_WORD16` for MOD0--MOD3, and defers exotic mode
  bits as `INVALID_OPERAND`. Every staged effect satisfies
  `ti_c54x_op_memory_effect_is_valid`.
- Read-value completion ORs the zero-extended unshifted 16-bit word
  full-width with the destination accumulator
  (`dst_new = dst_raw40 | word`), following the zero-extension
  precedent of the in-repo logical-immediate family
  (`src/isa/ops/logical/immediate/common/op_logical_immediate.c`).
  The completion is destination-bound at stage time (one static
  completion per accumulator), keeping the standard
  `ti_c54x_op_memory_complete_fn` shape with no globals.
- Status: C and TC preserved (as in `src/isa/ops/logical/op_logical.c`);
  no SXM/OVM input is consumed; destination-overflow preserved.
- Rejections: NULL borrowers `INVALID_ARGUMENT`; destination > 1,
  indirect > 1, addr7 > 0x7F, and out-of-range accumulator content
  `INVALID_OPERAND`; unknown destination accumulator
  `UNKNOWN_INPUT` at both stage and completion. Failed calls leave
  the caller-owned effect unchanged.

Verification: the focused test compiles standalone against the new
leaf plus `memory_effect.c` (no build-file changes, per bounded file
ownership) and all 13 checks pass; `git status --short` shows only
the four assigned new files.

Follow-up (out of scope): catalog row plus decode/dispatch binding
for FORM-T2-08-01, AND/XOR-neighbor rows, and exotic-mode expansion.
