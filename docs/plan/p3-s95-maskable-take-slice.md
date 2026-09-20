### P3-S95: maskable take slice (planned)

The loader2 hop's second half. S93 latched the HPINT edge
(mask `0x0200`) and S94 proved the NMI take plus the take
machinery; this slice reuses that core for one unambiguous
maskable source: IMR-gated arbitration, the take itself
(push parked PC, INTM set, PC to the routed entry, IFR
acknowledge), and the `RETE` round trip.

Evidence basis (no new E-TI row: S93/S94 precedent reuses
pinned rows). TI SPRU131G §6.10.8 (pinned row
`E-TI-C54X-SPRU131G-S6-10-3-S6-10-8-P6-31-6-36-IFR`): a
maskable take needs INTM clear with its IMR bit set, and on
acknowledge the CPU clears the IFR flag, sets INTM, pushes
the PC, and branches to the ISR. TI SPRU131G §6.7 (pinned
row `E-TI-C54X-SPRU131G-S6-7-P6-20-6-22-RPT-IRQ`): pending
defers while a repeat loop is ACTIVE. TI SPRU131G §6.10
(priorities live in the per-device tables of §6.10.10):
hardware-interrupt priority is device-specific, so the
common core takes only an unambiguous single enabled
source; multiple enabled sources defer with edges
preserved. NMI keeps architectural priority over every
maskable (priority 2 after RS).

Scope: one public CPU API,
`ti_c54x_cpu_request_interrupt_vector(cpu, mask, vector)` —
S93 validation (null/in-run/zero/multi-bit reject
losslessly) plus latching the caller-selected entry
address for that source; re-routing a pending source
overwrites (last wins, edge preserved). The S93
vectorless latch stays receive-only: a take needs a
routed bit. The take fires at the predecode boundary
when INTM is known-clear, IMR is known with the bit
set, exactly one enabled bit is pending, and its vector
is routed; the IFR bit clears atomically with the
retirement (before the next bus effect), reusing the
take core (frozen bit stored at issue; WAIT/FAULT keep
the edge for resume/retry exactly like S94). IMR has
no production writer yet (STM is wave 2): tests seed
it through the internal MMR cell API as white-box
setup, INTM through seeded ST1.

RED contracts:
`tests/unit/sequencer/interrupt/test_take_arbitration_maskable.c`
(pure decision table: quiet + single routed enabled
source takes naming its bit; INTM set/unknown, IMR
unknown/masked, none enabled, or vector missing
fetch; multiple enabled or busy repeat/delay defer;
INTM/IMR/IFR knownness strictly required);
`tests/unit/cpu/isa/status/test_cpu_maskable_take.c`
(vector-API rejections and last-wins re-route; take
pushes parked PC, clears exactly its IFR bit, sets
INTM, lands PC on the routed entry with full arch
preservation outside SP/PC/INTM/IFR and no RTN move;
INTM/IMR/vector/priority deferrals preserve edges
with no bus traffic; NMI preempts a pending maskable
and survives a maskable WAIT span; ISR `RETE`
round trip resumes with INTM clear and no re-take).
Acceptance is the focused tests plus the full gate,
with the park run byte-identical absent requests.

Out of scope: multi-source priority (device-owned;
defers), maskable vector fetch from the table
(IPTR/PMST gaps stay open; routing stays caller
supplied), RS/reset, INTR/TRAP, IACK signaling,
EALLOW-style protection, timing.

Findings (GREEN): `ti_c54x_cpu_request_interrupt_vector`
latches one bit with its entry (null/zero/multi/in-run
reject losslessly; re-routing overwrites with the edge
preserved); a quiet single routed enabled source takes
at the predecode boundary with no program fetch, one TOS
WRITE of the parked PC, exactly its IFR bit cleared,
INTM set, PC on the routed entry, and full preservation
outside SP/PC/INTM/IFR with no RTN move. INTM/IMR/
vector/priority deferrals preserve edges with no bus
traffic; NMI preempts immediately and survives a
maskable WAIT span (frozen-bit acknowledge hits only
the taken source); an ISR `RETE` round trip resumes
with INTM clear and no re-take. No new E-TI row: the
§6.10.3/§6.10.8 and §6.7 rows already pin the behavior
(S93/S94 precedent); one E-PROJECT seam anchor added.
Hardware-interrupt priority stays device-owned per TI
§6.10 (per-device tables), hence the single-source
rule with multi-enabled deferral.

Validation evidence: `make test` exit 0 (two new
RED-first tests green via both make and ctest
harnesses; evidence registry PASS with the new REQ
pair plus two T rows); fresh CMake Debug `ctest`
762/762; S90 canonical park run byte-identical
absent requests.
