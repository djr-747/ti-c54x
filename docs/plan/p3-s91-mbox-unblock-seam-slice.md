### P3-S91: MBOX-unblock seam slice (implemented; local verification green)

Phase 2 of the S90 warm boot spins at `0xF17`
(the MBOX0 poll) with the MCU peer holding at
64/64. The
spin decodes (repo catalog + repo `COND_` codes)
as a buffer-A/B pump with header `0x87B-0x881`
(destA `0x100`, destB `0x300`, remaining hi:lo,
MBOX0 `0x87F`, MBOX1 `0x880`, len 512):
`LD *(MBOX)` then `BC ANEQ` (wait for 0), per
iteration `MVDM`/`ADDM`/`CALL 0x8023` (verify),
`ST #1` (ACK), `remaining -= len`, and `BC ALEQ`
to the `0xF4F` finalize once `remaining <= 0`
(`BC AGT` re-tops otherwise). The peer can only
hold: flash blocks past 64 do not exist at its
stride, so the next MCU move (rehome upload,
doorbell, go-command) is not another pump round
and its encoding is unknown. Inventing it would
break the no-invention rule, so S91 diagnoses
the exact wait before S92 models the move.

Scope: one hosted-runner windowed DATA journal,
`--watch-data <base> <count>` (strippable
anywhere, single use). The DATA leg of
`run_image_advance` taps in-window accesses
after serving: reads journal the served value,
writes the written value; the first 32 entries
keep address/access/value with total and
overflow, mirroring the port peer. The journal
resets at each phase start (like the fault
latches) and prints per phase after the `irq`
line, only when requested. No core, MMR, knob,
timing, or peer changes.

RED contracts: `--selftest` vector 4 (one phase:
`ST` in-window then `ST` out-of-window asserts
total 1, journaled 1, entry address/value, no
overflow; a 35-`ST` run asserts the 32-cap with
overflow). Acceptance is the phase-2 MBOX/header
poll pattern (addresses, values, order) recorded
in the ledger with the S92 move defined.

Out of scope: the peer's post-pump move itself
(S92), interrupt arrival/take, timing, and any
new flash-content source.

Findings (GREEN): watching `0x878 count 16`
through the canonical two-phase repro shows
phase 1 opening with `R MBOX0=0`, `R destA`,
`R len`, `W MBOX0=1`, `R remaining`, `W
remaining-=512`, `R MBOX1=0`, `R destB`, `W
MBOX1=1` (576 watched, cap overflow, first-32
kept) — the live handshake exactly as
specified. Phase 2 is pure `R 0x87F=1`
(99831 of 99840 phase-2 DATA txns, zero
faults): the DSP re-ran loader1 setup and
waits on MBOX0 with `remaining=0`, so the
unblock is crisp — the peer must present
`MBOX0=0` with a filled buffer and a coherent
header (`remaining` = new count). A second
watch (`0x100 count 512`, whole phase 1)
records zero accesses: loader1 verifies the 64
blocks in place and never stages dest, so
rehome is MCU-side or later-DSP-side, never in
the pump. The DSP never reads ports 1-2 before
the park, so the port go-command sits strictly
downstream of the MBOX unblock. Uploaded code
cannot execute yet either: the runner has no
OVLY program-fetch mapping (a future seam —
program fetches serve image+overlay only).

Validation evidence: `make test` exit 0 (new
`--selftest` vectors 4/4b RED-first green:
window filtering plus 32-cap overflow;
registries and checkers PASS); fresh CMake
Debug `ctest` 756/756.
