### P3-S56: MVDM Smem-to-MMR move slice (implemented; local verification green)

`MVDM Smem, MMRx` (FORM-T2-26-03) moves one DATA word into a
memory-mapped register. It is the first halt of the standalone
5110 boot probe (reset vector retires FF80→0F00, then `MVDM
0x0000, AR4` at `0xF07` stops UNIMPLEMENTED), and the census
weighs it at 205 occurrences, so it pays both the boot and the
205/205 threads.

Encoding (pinned during implementation from SPRU172C §MVDM with
a TI evidence entry): two words; the first word carries the MMR
destination with the Smem addressing mode, the second word the
16-bit Smem address for the absolute flavor (`0x7214` at the
halt reads DATA `0x0000` into AR4; the oracle shows `0x7212`
writing AR2). The Smem side needs the same direct/indirect
coverage question every Smem row answers; the absolute flavor
comes first because the boot halt uses it.

Root design (open): no read-then-write-different-address path
exists. The ordinary completion stages accumulator/status/T
only, never MEMORY; RMW writes back the same address; DELAY
targets Smem+1. The MMR write leg needs no endpoint traffic
(`mmr_route` serves it locally like STM), but it must publish
only after the Smem READ succeeds, across WAIT resumes. The
slice designs that two-phase commit (pair-leg extension versus
a dedicated MMR-store completion) before the leaf lands.

Out of scope: MVMD/MVMM/MVDK/MVKD/MVPD/MVDP/READA/WRITA (own
slices in census order), peripheral MMR modeling behind the
Smem reads (explicit faults stay), timing.
