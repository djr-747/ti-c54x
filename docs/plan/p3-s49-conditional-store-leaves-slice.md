### P3-S49: conditional-store leaves (implemented; local verification green)

Fan-out over the S48 seam: three row-less leaves, one worker
each, maintainer wires and merges. TI ground truth is
SPRU172C pp. 4-152/4-153 (SACCD), 4-165 (SRCCD), 4-186
(STRCD); registry assertions `E-TI-C54X-SPRU172C-S4-SACCD/
SRCCD/STRCD-*` restate them. Shared S48 seam pieces the
leaves consume: `ti_c54x_store_cond_evaluate` (12 codes,
`op_store_cond_common.h`), `OVERWRITE`/`WRITEBACK` RMW
(`memory_instruction_rmw.h`), `BRC_REGISTER` input
(`brc_register_raw16/known`), `SELECT_DUAL_XMOD_XAR_COND4`
layout (operands S@8, XMOD@7:6, XAR@5:4, COND@3:0).

Prescribed names (exact; maintainer wiring depends on them):

- SACCD: `src/isa/ops/store/op_saccd_xmem_cond.{c,h}`,
  `ti_c54x_op_saccd_xmem_cond` /
  `ti_c54x_trusted_op_saccd_xmem_cond`,
  `TI_C54X_OPCODE_ID_SACCD_XMEM_COND`, test
  `tests/unit/isa/ops/store/test_saccd_xmem_cond.c`.
  Catalog row (maintainer): value `0x9E00`, mask `0xFE00`
  (S live: 0=A, 1=B), COND4 layout, UNRESOLVED, resources
  `ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_SXM_ASM`.
- SRCCD: `op_srccd_xmem_cond`, `TI_C54X_OPCODE_ID_SRCCD_XMEM_COND`,
  test `test_srccd_xmem_cond.c`. Row: `0x9D00`/`0xFF00`
  (S fixed 1; leaf rejects S==0 as INVALID_OPERAND),
  resources `ACCUMULATORS_AUXILIARY_REGISTERS_BRC_REGISTER`.
- STRCD: `op_strcd_xmem_cond`, `TI_C54X_OPCODE_ID_STRCD_XMEM_COND`,
  test `test_strcd_xmem_cond.c`. Row: `0x9C00`/`0xFF00`
  (S fixed 0; leaf rejects S==1), resources
  `ACCUMULATORS_AUXILIARY_REGISTERS_T_REGISTER`.

Leaf contract (all three; mirror `op_sth_shft_xmem.c` structure
with `op_stl_smem_direct.c` checked/trusted discipline):

- Extract 4 operands (widths 1/2/2/4); XMOD 0/1/2 valid,
  XMOD 3 rejects INVALID_OPERAND (MOD3 indexed deferred
  boundary); XAR field +2 must land AR2..AR5 else
  INVALID_OPERAND; COND via `ti_c54x_store_cond_evaluate`
  (propagate UNKNOWN_INPUT/INVALID_OPERAND); NULL args
  INVALID_ARGUMENT; effect untouched on failure.
- Stage one MEMORY effect: address
  `DUAL_INDIRECT_WORD16_X_ONLY` with x=y=(x_mode, x_ar),
  access READ, complete NULL, rmw OVERWRITE with
  `value_word` = staged store word when taken, WRITEBACK
  (staged word ignored) when not taken. Pipeline note: the
  sequencer resolves RMW read legs with canonical zero and
  applies the frozen staged word on the write leg, so the AR
  update commits exactly once (`memory_is_rmw` already
  classifies the new operations; no sequencer change).
- Staged words: SACCD low-16 of the 40-bit shift of src by
  (ASM_signed - 16) with ASM_signed = asm<=15 ? asm : asm-32
  (always <= -1, i.e. a right shift; arithmetic iff SXM=1 —
  mirror `stl_asm_smem_direct_shift`; bounds-check asm<=31,
  sxm<=1, raw40<=mask). SRCCD: BRC raw16. STRCD: T raw16.
- TI vectors: SRCCD `*AR5-,AGT` word `0x9D76`
  (XMOD=1 dec, XAR=3->AR5, COND=0110) with A `00_70FF_FFFF`,
  BRC `0x4321`, mem `0x1234`->`0x4321`, AR5 `0202`->`0201`
  (address/update asserted at effect level: address AR5,
  update -1); same shape for STRCD with T `0x4321`.
  SACCD shift math: A `FF_FE00_4321`, ASM `01`, SXM=1, ALT
  taken, staged `0xFC00` (TI p.4-153; its `*AR3+0%` is
  MOD3-indexed, so test the shift at effect level plus a
  MOD1 analogue). Cond-false cases must stage WRITEBACK.

Worker protocol (bounded ownership, no commits):

- Create NEW files only (the two paths above). Write the RED
  test first, then implement.
- To validate locally, TEMPORARILY add in your worktree only:
  the one prescribed catalog row, the one resource-combo
  token plus KNOWN_MASK bit, and the two manifest lines per
  build system; run `make build`, your test binary, and the
  `sth_shft_xmem` + `store_cond_evaluate` + `rmw_overwrite`
  neighbors. Then REVERT every shared-file edit: final
  `git status` shows only your new files.
- Never touch another worker's files, never commit, never
  rebase. Report: new-file list, validation evidence
  (binaries run + outputs), and any TI/contract ambiguity.

Maintainer merge notes: all three handoffs merged with only
reviewer-side fixes. (1) SACCD staged bits 31-16 with an
unsigned ASM and no TI vector; TI p. 4-153 (`0xFC00`) proves
low-16 extraction with signed ASM, so the helper, the full
vector table (recomputed against an independent oracle), and
three stale comments were corrected at merge. (2) The brief
said reject XMOD 3, but both store workers staged 0-3
matching the landed `STH_SHFT_XMEM` sibling while the
address core owns the MOD3 boundary — accepted, brief
corrected. (3) STRCD S-mismatch returned INVALID_DECODE on
the checked path; moved into the values core as
INVALID_OPERAND on both paths (SRCCD parity), and the
resource token renamed to the canonical
`ACCUMULATORS_AUXILIARY_REGISTERS_T_REGISTER` in code and
test. Closure: 301 rows, 307 bindings, 34,825 lookup
entries, 149/205 forms COMPLETE-or-better (72.683%, strict
43/205), 782 requirements, 638 tests.
