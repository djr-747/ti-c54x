### P3-S71: MCU upload peer slice (planned)

The loader1 MBOX spin needs its MCU counterpart: a
hosted-side peer that plays the oracle-verified upload
protocol (boot-sequence doc sections 2-3, cross-checked
against the live `dct3_boot_trace` conversation log)
through the runner's DATA endpoint. The core stays
untouched; everything new lives in `tools/hosted/` plus
a unit test.

Protocol (DSP data addresses): header `0x87B-0x881`
(destA `0x100`, destB `0x300`, remaining hi:lo, MBOX0,
MBOX1, len 512), buffers A `0x900-0xAFF` and B
`0xB00-0xCFF` (512 words each). Polarity: 1 = MCU owns,
0 = buffer ready; DSP ACKs with 1. Peer init writes the
canonical header (`remaining = blocks * len`, both MBOX
1). Before the first DSP poll the peer fills buffer A
from flash block 0 and sets MBOX0 0; each DSP ACK (write
1 to a MBOX cell) advances the pump (fill the other
buffer from the next flash block, set its MBOX 0) until
the block limit, then the peer holds (MBOX stays 1).

Flash bytes (user decision): block n (0-based) is 512
words from flash offset `0x200040 + n * 0x4000`, verified
against the oracle doc; the staged `.fls` is gitignored
like the probe images. loader1 placement reads 186 words
at flash `0x2996F8` (first word must be `0xF7BB`,
cross-checked against block25). A block-count limit
bounds wall time (full 64 is ~5h at current core speed);
bounded runs finalize early with an explicit checksum
mismatch, which is honest: the peer models the MCU
protocol, and the DSP's own finalize is the final judge.

Runner form: `run-image ... --mcu-upload <flash>
<blocks>` (new literal-flag trailing form; existing
forms untouched). Stages 4-5 (rehome, warm reset,
doorbell, go-command) stay out: separate slice after
the DSP core executes the spin.

RED contracts: `tests/unit/hosted/test_mcu_upload.c`
(header init, polarity, ACK advance, flash slice bytes,
64-iteration ack counts 32+32, bounded hold, neighbors
untouched). Acceptance: peer unit tests green plus a
live run to the current core frontier (`0xF0F` now that
the S70 ST-absolute slice landed, MVDP absolute stops
the DSP before any MBOX traffic); end-to-end finalize
waits on the DSP ISA chain (MVDP absolute, LD/STL/STH/
ADDM absolute, CRC routine).
