### P3-S119: FRET/FRETE far-return rows with stack double-pop (implemented; local verification green)

Scope: Table 2-15 (TI SPRU172C pp. 4-60–4-63) far-return
forms `FRET[D]`, `FRETE[D]` land as four exact catalog rows:
`FRET` (0xF4E4), `FRETD` (0xF6E4), `FRETE` (0xF4E5), `FRETED`
(0xF6E5), 1 word each. The delay bit Z (bit 9) is pinned per
row following the B/BD, RET/RETD, FB/FBD, FCALL/FCALLD
separate-row precedent; E bit 0 selects interrupt-enable
(FRETE/FRETED clear INTM at retirement through the RETE
completion callback). All rows INELIGIBLE by Table 2-28
membership (TI: "not repeatable"). Layout NONE (like RET);
inputs STACK_POINTER.

Execution pops the return extension page (XPC) from the first
TOS at old SP, then the return address (PC) from the second
TOS at old SP + 1, publishing old SP + 2 in one commit — the
mirror of the S118 double-push. New STACK_POP_FAR address
kinds (immediate, delayed, int-enable immediate, int-enable
delayed) stage through the memory root with two DATA READ
legs: the root freezes nothing (both legs read live stack),
issues the second READ after first-leg completion with resume
semantics, and commits SP+2 plus the popped PC+XPC
atomically. Delayed forms (FRETD/FRETED) commit nothing
except SP+2 with the arming retirement, arm the two-word
countdown with the popped target deferred, and take effect
after the slots. T2-15-01 binds FRET/FRETD, 02 binds
FRETE/FRETED, all SUBSET with NONDELAYED/DELAYED subsets.

TI vectors (to be pinned from SPRU172C §FRET pp. 4-60–4-63):
FRET/FRETE examples with exact SP/PC/XPC/stack words.

Evidence: E-TI records for the FRET/FRETE opcode diagrams and
stack order; REQ-COMMON-P3-S119-FRET-SEMANTICS (leaves),
-CATALOG (rows), -RETIREMENT (root + CPU). Tests: 2 ops
leaves, 1 catalog metadata, 2 CPU retirement (immediate +
delayed, incl. INTM for FRETE, WAIT resume, fault/unknown
transactionality, MMR TOS rejection).

Knobs: none expected (follows the S118 no-knob pattern).

Follow-up: closes Wave-7 far control; remaining work is
timing/profiles/devices per the plan.
