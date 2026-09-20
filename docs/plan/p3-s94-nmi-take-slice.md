### P3-S94: NMI take slice (implemented; local verification green)

The loader2 hop's first half lands here. S93 proved the
receive layer and re-derived the consistent two-step:
NMI (ignores INTM) takes to the NMI `RETF` stub, our
`RETF` clears INTM (ST1 bit 11, implemented), the
resumed `B` loop re-idles, and the pending HPINT then
takes to `0x3772`. This slice proves the NMI half plus
the take machinery the HPINT half will reuse: an NMI
source, predecode arbitration, the take itself (push
parked PC, INTM set, PC to vector), and the `RETE`
round trip back to the parked program.

Evidence basis (no new E-TI row: S93 precedent reuses
pinned rows). TI SPRU131G §6.10.8 (pinned row
`E-TI-C54X-SPRU131G-S6-10-3-S6-10-8-P6-31-6-36-IFR`):
a requested nonmaskable interrupt is acknowledged
immediately, INTM is set to 1, the PC is saved on the
stack, the CPU branches to the ISR, and the return
pops the address. TI SPRU131G §6.7 (pinned row
`E-TI-C54X-SPRU131G-S6-7-P6-20-6-22-RPT-IRQ`): once a
repeat instruction is decoded, all interrupts
including NMI but not RS are disabled until the
repeat loop completes — so a pending NMI defers
while single or block repeat is ACTIVE. NMI may
additionally delay past multicycle instructions, so
the take fires at the predecode instruction boundary
(after root resume, before fetch), and defers while
a delayed-branch countdown is armed.

Scope: one public CPU API,
`ti_c54x_cpu_request_nmi(cpu, vector_word)` — latches
one pending NMI take naming the device-owned vector
address (the program word the CPU branches to; the
slot conventionally holds a branch to the ISR). The
core treats the vector as opaque: table base (IPTR,
MP/MC) and the PMST/IMR gaps stay recorded open, so
no profile constant is baked in. Null CPU,
in-run calls, and a second request while one is
pending reject with `INVALID_ARGUMENT` and mutate
nothing (lossless: the first edge is never
overwritten). INTM is irrelevant to NMI by TI
construction. The take is the root's second
producer: a `STACK_PUSH_TAKE` WORD16 WRITE of the
parked PC through the existing push/commit seams
with a take completion staging the INTM set
(`RETE`-completion mirror, WRITE_1 to ST1 bit 11),
PC redirected to the frozen vector, no RTN load
(takes are not calls), counted as one retirement.
Pending clears only on RETIRED commit, so WAIT and
FAULT keep the edge for resume/retry exactly like
opcode transactions.

RED contracts:
`tests/unit/sequencer/interrupt/test_take_arbitration.c`
(pure decision truth table: pending + quiet →
TAKE; pending + single-ACTIVE / block-ACTIVE /
delay-armed → DEFER; idle → FETCH; INTM either way
never matters);
`tests/unit/sequencer/interrupt/test_take_complete.c`
(direct completion call stages exactly the ST1-bit-11
set with everything else ignored);
`tests/unit/cpu/isa/status/test_cpu_nmi_take.c`
(arrival rejections; take pushes parked PC to
SP-1, sets INTM, lands PC on the vector with full
arch preservation outside SP/PC/INTM and no RTN
move; WAIT-then-READY spans run calls with a stable
vector; DATA FAULT stops with the edge preserved;
unknown SP stops with no mutation; repeat-ACTIVE
and delay-armed defer with the edge preserved;
IDLE park + request + run takes; ISR `RETE`
round trip resumes the parked PC with INTM clear).
Acceptance is the focused tests plus the full gate,
with the park run byte-identical absent an NMI
request.

Out of scope: maskable arbitration and take (needs
the IMR write seam — STM wave 2 — plus priority;
the HPINT half reuses this take core later),
RS/reset, INTR/TRAP, IACK signaling, EALLOW-style
protection, timing.

Findings (GREEN): `ti_c54x_cpu_request_nmi` latches one
edge with its vector (null/in-run/second-while-pending
reject losslessly); a quiet pending edge takes at the
predecode boundary with no program fetch, one TOS WRITE
of the parked PC at old SP - 1, INTM set, PC on the
frozen vector, no RTN move, and full preservation
outside SP/PC/INTM; WAIT spans run calls with a stable
vector, FAULT and unknown SP stop with the edge and
state preserved, repeat-ACTIVE and delay-armed defer
with the edge preserved, the IDLE park wakes into the
take pushing the IDLE address, and an ISR `RETE`
round trip resumes the parked PC with INTM clear.
No new E-TI row: the §6.10.3/§6.10.8 take sequence
and §6.7 NMI-disabled-during-repeat rows already
pinned the behavior (S93 precedent). The take is the
root's second producer (`STACK_PUSH_TAKE` WORD16 with
a frozen vector, sentinel instruction, and take-only
liveness); the maskable take reuses this core once
the IMR write seam lands.

Validation evidence: `make test` exit 0 (three new
RED-first tests green via both make and ctest
harnesses; evidence registry PASS with the new REQ
plus three T rows); fresh CMake Debug `ctest`
full-suite green; park run byte-identical absent an
NMI request.
