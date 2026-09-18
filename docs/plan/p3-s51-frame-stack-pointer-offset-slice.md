### P3-S51: FRAME stack-pointer-offset slice (implemented; local verification green)

Opens table 2-17 (`FORM-T2-17-01`, first of its table):
`FRAME K` (SPRU172C p. 4-60) adds the signed 8-bit short-immediate
offset K (-128..127) to SP with no status-bit effects, no memory
traffic, and no latency claim. Opcode `1110_1110_KKKKKKKK`
(`0xEE00` under mask `0xFF00`, one word, Class 1): no neighbor
row claims the `0xEE00` slot (`RPT_K` sits at `0xEC00`,
`LD_IMMEDIATE_ASM` at `0xED00/0xFFE0`), so the row projects a
clean 256 first-word encodings.

Design (pinned): the leaf (`src/isa/ops/stack/op_frame_k.c`,
new stack-pointer family owner beside `branch/`) validates the
copied bundle against the new `FRAME_K8` layout (low 8-bit K
field, mirroring the `RPT_IMMEDIATE_K` piece shape without
reusing the repeat-owned token), requires the landed W64
`STACK_POINTER` input snapshot, sign-extends K, and stages one
new `TI_C54X_OP_EFFECT_SP_WRITE` carrying the copied
`(sp + K) mod 65536` value-after. 16-bit wrap is an explicit
project-interpretation pin (the manual states no stack-overflow
status and status bits are None), mirroring the W65A push-wrap
pin; C/TC/OV and unrelated state are preserved by construction
(the effect carries no status payload). Unknown SP rejects
transactionally with no effect staged. A new sequencer leaf
(`src/sequencer/stack/sp_write_apply.c`) commits the staged
value through `arch_state_write_sp` (which marks SP known);
run.c routes `SP_WRITE` through it on the ordinary path ahead
of the effect-transaction stage, mirroring the repeat-route
block placement (no effect-transaction signature change, so
the six memory-commit call sites are untouched). Repeatability
stays `UNRESOLVED`: FRAME is not positively listed in the
Table 2-28 restriction record, and eligibility is never
inferred (W52A rule).

No second word, memory leg, MMR routing, status effect,
timing, or IRQ/RPT change is admitted. `PSHD`/`POPD`/`PSHM`/
`POPM` (the remaining table 2-17 forms) defer to a later
pair-leg slice over the landed W65A/W66A roots; the FRAME
leaf shares no code path with them beyond the SP input
snapshot.

Projected closure: 303 rows, 309 bindings, 35,337 lookup
entries, 151/205 forms SUBSET-or-better with strict 43/205,
791 requirements, 647 tests. These numbers are projections
until the slice lands.
