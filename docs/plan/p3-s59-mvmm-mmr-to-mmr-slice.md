### P3-S59: MVMM MMR-to-MMR move slice (implemented; local verification green)

`MVMM MMRx, MMRy` (FORM-T2-26-07) moves one memory-mapped
register into another. It is the third 5110 census MMR move
(288 occurrences, the heaviest remaining move in the wave-5
group) and the first MMR-read leg that is not paired with a
DATA Smem operand.

Encoding (TI SPRU172C p. 4-116, vendored text): one word pins
`0xE700` with the 4-bit MMR source at bits 7:4 and the 4-bit
MMR destination at bits 3:0. Only nine codes per nibble are
legal (AR0-AR7, SP); codes 9-15 are rejected by the leaf. Table 2-28
(p. 2-20) lists MVMM as nonrepeatable.

Root design: the leaf stages an absolute DATA READ from
`0x10+MMRX` with a `STORE_WITH_READ` sidecar numbered
`0x10+MMRY`, reusing the MVDM read leg, the MMR-routed issue
path, and the read-phase MMR store commit block with no
sequencer changes. The leaf rejects any nibble above 8 with
`INVALID_DECODE`: codes 9-13 address routed BK/BRC/RSA/REA/
PMST cells the hardware form never touches, so router-decline
alone cannot prove them. A new one-word `MMR4_MMR4` operand
layout carries the two nibbles.

Out of scope: MVDK/MVKD/MVPD/MVDP/READA/WRITA (own slices in
census order), peripheral MMR modeling behind the Smem
accesses (explicit faults stay), timing.
