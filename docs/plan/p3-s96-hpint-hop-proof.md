### P3-S96: HPINT hop proof slice (implemented; local verification green)

The loader2 hop end to end. S93 latched the HPINT edge
(mask `0x0200`), S94 proved the NMI take, and S95 proved
the maskable take plus the `RETE` round trip; this slice
proves the composed two-step the doorbell story needs:
a parked INTM-masked program with HPINT pending takes an
NMI to a `RETF` stub, the stub clears INTM, and the still
pending HPINT then takes to the routed loader2 entry,
whose `RETE` resumes the parked program with no re-take.

Evidence basis (no new E-TI row: S93/S94/S95 precedent
reuses pinned rows). TI SPRU131G §6.10.8 (pinned row
`E-TI-C54X-SPRU131G-S6-10-3-S6-10-8-P6-31-6-36-IFR`):
each take pushes the PC, acknowledges its source, sets
INTM, and branches to the ISR. TI SPRU172C p. 4-141
(RETF precedent from its vertical slice): `RETF` takes
PC from RTN and clears INTM, so the stub's return path
is the CALL-staged RTN convention the `RETF` slice
already proved, not the pushed slot (discarded, no bus
traffic). No new seams: every step reuses S94/S95
production paths.

Scope: no new production API. The proof is one CPU
integration test scripting the hop with the public
arrival APIs: park post-`CALL` (RTN staged, INTM set,
HPINT routed to `0x3772` and pending but masked),
`ti_c54x_cpu_request_nmi` to the `RETF` stub, `RETF`
(pops nothing, INTM clears, HPINT edge preserved),
maskable take to `0x3772` (IFR acknowledges, INTM
sets), `RETE` at the entry (pops the parked PC, INTM
clears), then one parked step with no re-take.

RED contracts:
`tests/unit/cpu/isa/status/test_cpu_hpint_hop.c`
(the six-retirement hop: `CALL`, NMI take, `RETF`,
HPINT take, `RETE`, parked step; SP/PC/INTM/IFR/RTN
asserted at every boundary; exactly four data-endpoint
transactions with no stub-ward program fetch on either
take). Acceptance is the focused test plus the full
gate, with the park run byte-identical absent requests.

Out of scope: vector-table fetch (routing stays
caller-supplied), multi-source priority, RS/reset,
INTR/TRAP, IACK signaling, timing, the real image hop
(stub and loader2 contents stay scripted).

Findings (GREEN): the six-retirement hop passes with no
production change — every step reuses the S94/S95 paths
(`CALL` stages RTN, NMI take, `RETF`, maskable take,
`RETE`, parked step). Ordering matters: the NMI must
arrive after the park is established because a pending
NMI preempts decode at once (the first draft requested
it up front and the `CALL` never ran). `RETF` takes PC
from the CALL-staged RTN and discards the pushed slot
with no bus traffic; the HPINT edge survives both the
NMI take and the stub. `result.opcode_valid` stays
sticky from the last fetch across takes (run-support
reports `last_fetch_valid`), so the proof asserts no
new program fetch across each take instead. No new
E-TI row: the §6.10.3/§6.10.8 and §6.7 rows already
pin the behavior (S93/S94/S95 precedent); no new
seams, no new knobs.

Validation evidence: `make test` exit 0 (new RED-first
test green via the make harness; evidence registry
PASS at 807 requirements / 661 tests, source
boundaries, knobs, manifests, observer-off artifact);
fresh CMake Debug `ctest` 763/763 including
`ti_c54x.cpu_hpint_hop`; hosted runner `--selftest`
PASS (4 vectors). The S90 canonical park is not
re-runnable here (probe `nokia-5110-v530a.fls` absent
from the worktree; a substitute upload image fails
peer init), but parity holds by construction: the
slice touches no production file (tests, docs, and
registry only).
