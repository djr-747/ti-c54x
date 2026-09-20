### P3-S93: interrupt-receive seam slice (implemented; local verification green)

The loader2 hop needs a doorbell take that cannot
land today: the park holds INTM=1, and TI
SPRU131G §6.11.1 pins that an IDLE1 wake with
INTM=1 resumes after the `IDLE` (our `B 0x0F6A`
loop) while only INTM=0 enters the ISR — with
IMR-gated wake-ups either way (RS/NMI excepted).
Finalize decode (`0xF8F` sub: `PORTW`, delay,
`PORTR`-poll, `RET`) shows no `RSBX INTM`, no
loader2 branch, and no take path anywhere in
loader1, and the NMI slot (`0xFF84`) is a `RETF`
stub: a take while parked is impossible by
construction, so S93 builds the receive layer
it will need later rather than a take that
advances nothing.

Scope: one public CPU API,
`ti_c54x_cpu_request_interrupt(cpu, mask)` —
TI SPRU131G §6.10 phase 1 (a maskable request
sets its IFR bit on receipt). Exactly one bit
must be set (the state helper's contract);
null CPU, zero, and multi-bit masks reject with
`INVALID_ARGUMENT`. Latch only: no
arbitration, no take, no IDLE/run-loop change,
no runner or peer wiring. NMI/RS have no IFR
bit and stay out (S94: acknowledge + take).

RED contracts: `test_cpu_interrupt_arrival`
(fresh CPU: INT1-bit request latches visible
via inspect with INTM undisturbed; a second
bit accumulates; null/zero/multi-bit reject;
no run needed). Acceptance is the focused test
plus the full gate, with the park run
byte-identical (no caller exists yet).

Out of scope: acknowledgment/arbitration
(IMR/INTM/priority), the take (vector fetch,
context, `RETE` wiring), IDLE-wake behavior
change, NMI/RS, doorbell injection, OVLY
fetch, timing.

Findings (GREEN): `ti_c54x_cpu_request_interrupt`
latches one IFR bit per call (INT1 then INT2
accumulate to `0x0006`), visible via the S89
inspect path with INTM held set; null/zero/
multi-bit reject without mutation. New
evidence rows (TI receive + TI IDLE1-wake +
COMMON receive REQ + test row) keep the
registry green.

Hop mechanism (oracle black-box + TI primary,
no emu execution): the doorbell is DSPINT bit2
(`[0x30000] |= 4`) driving HPINT — TI Table
6-28 #25 pins HPINT/SINT9 at vector `0x64`,
i.e. our live `0xFFE4 → 0x3772` slot ("doorbell
vector 25" grounded; NMI at `0xFF84` and
INT0-2 at `0xFFC0-0xFFC8` are confirmed `RETF`
stubs). A loader1 static scan finds no `RSBX`,
no `TRAP`, and no branch to `0xD80`, so the hop
is exclusively interrupt-driven — yet TI
§6.11.1 forbids the take with INTM=1. The
consistent two-step: NMI (ignores INTM) takes
to the NMI `RETF` stub, and our `RETF` clears
INTM (ST1 bit 11, implemented); the resumed
`B` loop re-idles, and the pending HPINT then
takes to `0x3772`. The emu's own docs admit
the hop is untraced there (plus a warm-reset
modeling artifact at `ret @0xf73`), so a live
emu run could only show its approximation —
the two-step above is the falsifiable story
S94 proves: NMI source plus arbitration plus
take plus `RETE` wiring, with this slice's
arrival API already covering the HPINT half
(mask `0x0200`).

Validation evidence: `make test` exit 0 (new
arrival test RED-first green via both make and
ctest harnesses; evidence registry PASS at
802 requirements / 655 tests); fresh CMake
Debug `ctest` 757/757; park run untouched
(no runner change).
