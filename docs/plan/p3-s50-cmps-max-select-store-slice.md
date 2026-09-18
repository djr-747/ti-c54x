### P3-S50: CMPS max-select-store slice (implemented; local verification green)

Closes table 2-21 (`FORM-T2-21-01`, the last of its table):
`CMPS src,Smem` (SPRU172C p. 4-35) stores
max(src(31:16), src(15:0)) as signed 16-bit compares into
Smem, shifts `(TRN << 1) | taken` back into TRN, and writes
TC = taken. Opcode `1000_111S_IAAAAAAA` (S selects A/B, I
direct/indirect, 7-bit Smem): the ST_T direct/indirect
split shape, so this slice takes the direct half (I=0,
DP-relative, SELECT_INDIRECT_ADDR7-family layout) and
defers indirect (I=1) exactly like S43/S47.

Design (pinned): the architecture allows two commits per
leaf (one primary + one ordinary follow-on), but CMPS needs
three (Smem, TRN, TC). The follow-on is therefore one
compound effect, `TI_C54X_OP_EFFECT_TRN_SHIFT_WRITE`
carrying the copied new TRN word and TC bit (BRANCH_AUXILIARY
is the multi-target precedent; the leaf computes, the
sequencer only copies). Primary: Smem WRITE of the selected
half; completion stages the compound. New ordinary-transaction
arms (mirror T_REGISTER_WRITE; apply helper beside
`t_register_effect_apply`) with `mmr_file` threaded through
stage/commit (six call sites). TC target is ST0 bit 12
(CMPR precedent). Inputs: source accumulator + TRN
(AUX only if the deferred indirect half is attempted — it
is not). The "read phase" pipeline note is a no-timing-model
observation: the snapshot is the pre-execution value. TI
example `CMPS A, *AR4+` is indirect (deferred);
direct-slice vectors use DP-relative Smem analogues plus
the max-select truth table (hi>lo, hi<lo, hi==lo, negative
halves).

No catalog row, handler, inventory form, or CPU retirement
change is admitted until the slice lands. The indirect half
(I=1) defers to a later MOD slice; the leaf rejects it
transactionally while staying decodable so the checked and
trusted paths agree.

Closure: 302 rows, 308 bindings, 35,081 lookup entries,
150/205 forms SUBSET-or-better (73.171%, strict 43/205 at
20.976%), 786 requirements, 642 tests, 617 unit binaries.
The RED vector table needed one correction at landing: the
0x00FF tie expected taken, but ties are untaken under the
strict-wins rule (matching the 0x1234 tie vector), so the
expectation was fixed to false with the accumulator kept.
Trusted/dispatch parity (binding identity, packet operands,
four-path effect memcmp plus follow-on memcmp) was added to
the effects suite at landing. Landing also closed the
TRN-cell completion gap the seam left open: the memory
pipeline admitted no WRITE-with-completion shape, so CMPS
retired INVALID_STATE at the CPU. Root cause, not probe
error: the validator required a null completion on WRITE
and the single-leg WRITE branch retired without invoking
completions. Fixed by admitting WRITE + NONE + completion
(the staged value feeds the completion; CMPS ignores it)
and invoking it in the single-leg WRITE branch with the
ordinary follow-on commit before the architecture commit;
RMW paired-write legs still never carry completions. A
committed CPU retirement suite (taken/untaken/B-select,
unknown-TRN rejection, DP-independence) and a validator
shape test cover the fix. The CPU suite links a new
src/cpu/-owned CPU-RETIREMENT requirement (MPYA
precedent). Landing also fixed a `check_evidence` hang:
`repo_files` post-filtered a whole-tree rglob across
gigabytes of per-slice `build-*` trees; it now prunes
ignored trees during traversal (0.08 s).
