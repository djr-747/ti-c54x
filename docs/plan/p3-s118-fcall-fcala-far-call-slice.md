### P3-S118: FCALL/FCALA far-call rows with stack double-push (implemented; local verification green)

Table 2-13 (TI SPRU172C p. 2-11) far-call forms `FCALL[D]`,
`FCALA[D]` land as four catalog rows following the B/BD
separate-row precedent (delayed variants are own rows sharing
the family form with NONDELAYED/DELAYED addressing subsets):
`FCALL_PMAD` (2 words; first word 1111_10Z1_1PPP_PPPP, value
0xF980 under mask 0xFF80 with page PPPPPPP at bits 6:0 and Z
bit 9 delay select; extension word pmad(15-0)), `FCALLD`
(value 0xFB80 under mask 0xFF80), `FCALA` (1 word,
1111_01ZS_1110_0111, value 0xF4E7 under mask 0xFEFF with Z
bit 9 pinned per row and S bit 8 src select riding inside),
`FCALAD` (value 0xF6E7 under mask 0xFEFF, S1 form 0xF7E7).
FCALL reuses the FAR_PMAD layout; FCALA reuses
ACCUMULATOR_SOURCE. All rows INELIGIBLE per the section 4
no-repeat notes (FCALL[D], FCALA[D] cannot repeat or join
RPTB); RPTB exclusion rides the existing ineligibility gate.
Leaves stage MEMORY far-push effects gated on known SP (plus
known selected accumulator for FCALA): the root pushes the
return address (PC+2 FCALL, PC+4 FCALLD, PC+1 FCALA, PC+3
FCALAD, derived from word count plus delay select) then the
live XPC, then commits the far PC+XPC target (immediate) or
arms the countdown with the sequential advance (delayed). No
status bits, src preserved. TI vectors: FCALL 013333h (PC
0025 to 3333, XPC 00 to 01, stack 1110h 0027 then 110Fh
0000) and FCALLD 301000h with the ANDM delay slot (PC 3001
to 1000, XPC 7F to 30, stack 1110h 3005 then 110Fh 007F);
FCALA A with A 00 007F 3000 (PC 0025 to 3000, XPC 00 to 7F,
stack 1110h 0026 then 110Fh 0000) and FCALAD B with the ANDM
delay slot (B 00 0020 2000, XPC 7F to 20, stack 1110h 0028
then 110Fh 007F). T2-13-05 binds FCALL/FCALLD, 04 binds
FCALA/FCALAD, all SUBSET with NONDELAYED/DELAYED subsets.
First-word overlap note: FCALL (0xF980-0xF9FF) and FCALLD
(0xFB80-0xFBFF) sit inside the settled CC/CCD cond-byte
patterns (masks 0xFF00). Every TI condition code has bit 7
clear and OR-combination preserves that, so no valid CC/CCD
encoding sets bit 7; the slice tightens CC/CCD first-word
masks to 0xFF80 (128 valid conds each) and the far rows claim
the bit-7 halves. The CC/CCD 256-cond sweeps narrow to
0x00-0x7F with the 0x80-0xFF halves pinned as FCALL/FCALLD
siblings.

Split decision (P3-S118): src/isa/ops/memory/memory_effect.c
reaches 506 lines on the four far-push validity arms. The
file is one central effect-validity switch (one arm per
address kind, exhaustiveness reviewed in one place) plus five
small shape helpers; splitting the switch would break the
dispatch locality the validity gate depends on. No split:
track the repository <800 rule instead of the 500 module
bound (test_dual_word16_hot_path carries the exemption, same
as the P3-S64 ADDRESS_SPEC_HEADER precedent).

Split record (P3-S118): root prepare extraction
`ti_c54x_memory_instruction_transaction_prepare` moves verbatim
to the new `memory_instruction_prepare.c` owner (declaration
stays in `memory_instruction_transaction.h`). The advance root
keeps issue/resume/commit; the dual hot-path contract test now
reads both roots. No behavior change: the move is a pure
relocation plus the far-push prepare call.

Split record (P3-S118): stack-push resolution extraction
The stack-push TOS arm (SP observe, first-TOS plan, far SP - 2
step after validation, WRITE build) moves to the new
`address_resolution_stack_push` owner, matching the
one-family-per-module layout of `sequencer/memory`. The far
SP - 2 step runs after intrinsic validation because the
intrinsic PRE-push rule pins operand == post-update register
for the single first leg; the root single-leg check re-pins
the stepped SP - 2 update and commit.

Shared kind classifiers (P3-S118): `memory_effect.h` owns five
static-inline families (`is_stack_push` for push-prepare,
`is_bundle_push`/`is_accumulator_push` for address planning,
`is_far_push`, `is_delayed_push`). Take stays out of the
prepare family (pair-routed); accumulator far kinds stay out
of the arf-zero planning arm. All converted sites are
exact-family matches, verified by the green suite plus direct
probes.
