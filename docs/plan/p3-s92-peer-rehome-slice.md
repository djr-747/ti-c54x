### P3-S92: peer rehome presentation slice (planned)

S91 left the unblock crisp: phase 2 polls
`MBOX0=1` forever, and the peer must present
`MBOX0=0` with a filled buffer and a coherent
header. The oracle boot-sequence doc (§4,
two-phase reset cycle; §6 signal table) grounds
the move: after the pump the MCU warm-resets
the DSP (PC→`0xFF80`, DARAM preserved —
already our `--warm-boot`), loader1's 2nd pass
sees the preserved upload, and control reaches
loader2 (`0xD80`) via interrupt out of the idle
and/or the branch table. The rehome content is
byte-identical to a genuine 1-block re-upload
of the last stride block (buffer A already
holds it, so the peer skips a redundant fill —
no invention). Loader2/doorbell-take stays out:
with INTM=1 no maskable take can land, and the
take machinery does not exist yet.

Scope: peer gains `present_rehome` (armed +
holding only; writes `MBOX0=0` with
`remaining=512`, buffer and all other header
cells untouched; idempotent; the following ACK
falls into the existing hold branch, so no new
peer state), and the runner gains bare
`--mcu-rehome` (strippable anywhere, single
use, requires `--mcu-upload` and `--warm-boot`;
at phase-2 start only, the peer presents once —
phase 1 is untouched with or without the flag).
No core, MMR, knob, timing, or flash changes.

RED contracts: `test_mcu_upload` gains
present-after-hold (MBOX0/remaining set, buffer
untouched, ACK holds without refill),
present-mid-pump no-op, and present-disarmed
safety. Acceptance is the falsifiable
prediction: a bounded 1-block pump run parks
`IDLE` at `0xF6A` in 6628 retirements today, so
the rehome phase 2 must take the same finalize
path (same park or a named divergence).

Out of scope: loader2 content/staging, the
doorbell take (arrival + arbitration + take +
`RETE` wiring, S93 candidate), OVLY
program-fetch mapping, timing.

Findings (GREEN): the rehome phase 2 takes the
predicted finalize path — `mcu_rehome
presented=1`, then 6612 retirements to the
`IDLE` park at `0xF6A` with zero faults, 4 new
port calls (same handshake), overlay
`writes=1` to `2`, and 9 header accesses shaped
exactly like the bounded 1-block run's 9
(`R MBOX0=0, R destA, R len, W ACK, R
remaining=512, R len, W remaining=0`). Phase-2
DATA traffic is byte-identical in count to the
bounded run (561 txns); it retires 16 fewer
instructions with no memory/port/overlay
difference — same path, minor setup divergence,
not chased. The 2nd-pass gate is answered:
MBOX-presented + remaining-driven, finalize on
`remaining<=0`. The loader2 hop therefore
needs the interrupt out of the idle (doorbell
take), which does not exist yet — S93.

Validation evidence: `make test` exit 0 (new
peer `present_rehome` tests RED-first green;
selftest 4 vectors still PASS; registries and
checkers PASS); fresh CMake Debug `ctest`
756/756.
