### P3-S53: PSHD/POPD indirect Smem pair-leg slice (implemented; local verification green)

Admits the I-set half of the landed S52 rows (`PSHD`/`POPD`
single-indirect Smem, TI classes 16B/17B) through the S52 pair
carrier. TI semantics (SPRU172C pp. 4-127, 4-131, already
pinned): the `Smem` operand reads through single-indirect
addressing while the stack leg is unchanged (`--SP` then
`(Smem)` to `TOS` for `PSHD`; `(TOS)` to `Smem` then `++SP` for
`POPD`). No new rows: `PSHD_DIRECT`/`POPD_DIRECT` already claim
`0x4B00`/`0x8B00` under mask `0xFF00`, so the I-set halves
(512 encodings) go live in place. `PSHM`/`POPM` keep rejecting
I=1 in the leaf (TI lists no indirect MMR class: 16A/17A only).

Design (pinned): the shared indirect-addr7 cookie splits as
`mode4 = cookie >> 3`, `arf = cookie & 7` (LTD precedent);
indirect admits `mode4 <= 3` (MOD0--MOD3) and rejects above.
The leaves stage the same `STACK_PUSH_PAIR`/`STACK_POP_PAIR`
arm with `indirect = 1` plus the mode/arf fields the S52 arm
already carries; row input resources stay `STACK_POINTER`
(LTD precedent: AR unknownness rejects transactionally
downstream, never in the leaf). The pair root synthesizes a
`SINGLE_INDIRECT_WORD16` DATA-leg spec and resolves it through
the existing single-leg resolver; shape, template, and SP
liveness ride the snapshot unchanged, while DATA-leg liveness
binds the indirect mode/arf echo against the frozen pair arm
plus the live AR/ARP/CMPT state the generic indirect switch
pins. The TOS stack leg, MMR routing, commit order, and
cancel/reset paths are untouched.

Scope: indirect `PSHD`/`POPD` Smem only (two inventory subset
flips to `DIRECT_PAGE0_STANDARD_INDIRECT_MOD0_MOD3`); forms
stay SUBSET direct-or-better. No status-bit effects (Status
Bits None on both pages), no timing, no RPT interaction
(repeatability UNRESOLVED unless Table 2-28 positively lists a
form, per the W52A rule).

Projected closure: 307 rows, 313 bindings, 36,361 lookup
entries, 155/205 forms SUBSET-or-better with strict 43/205.
These numbers are projections until the slice lands.
