### P3-S1: MMR/device routing seam (planned)

This immutable `(planned)` heading is the durable S1 FIT/ownership
anchor. S1 is a sealed architectural seam with no catalog rows, no
opcode identities, no handlers, and no CPU opcode branch, following
the W43A pattern. TI SPRU131G §3.3.4/Table 3-3 (printed p. 3-27)
maps the CPU memory-mapped registers to DATA-space addresses
`0x00`–`0x1F`: `IMR 00`, `IFR 01`, reserved `02`–`05`, `ST0 06`,
`ST1 07`, `AL 08`, `AH 09`, `AG 0A`, `BL 0B`, `BH 0C`, `BG 0D`,
`T 0E`, `TRN 0F`, `AR0`–`AR7 10`–`17`, `SP 18`, `BK 19`, `BRC 1A`,
`RSA 1B`, `REA 1C`, `PMST 1D`, `XPC 1E` (far/XPC deferred per the
arch criterion, declined), reserved `1E`–`1F`. TI SPRU172C §4
`STM #lk, MMR` (printed pp. 4-175–4-176) stores the second
program word into the MMR file or DP0 with the nine MSBs of the
effective address cleared, so DATA-space WORD16 traffic at
`0x00`–`0x1F` must route to register state, never to plain
endpoint memory.

Scope: the `src/bus/mmr/` MMR file owns the Table 3-3 map with
per-address owner dispatch versus private instance-owned
16-bit storage plus knownness for the five registers with no
architectural owner (`IMR`, `TRN`, `BRC`, `RSA`, `REA`);
reserved `02`–`05` and `XPC` decline and fall through to the
endpoint exactly as today. Registers with live owners route to
checked whole-word access: `T`/`ARn`/`SP`/`BK`/`IFR` through
existing accessors, `ST0`/`ST1`/`PMST` and accumulator words
`AL`/`AH`/`AG`/`BL`/`BH`/`BG` through new whole-word owner
accessors (single-sourced knownness stays with the owner:
whole-word status knownness requires the full known mask,
accumulator-word knownness follows the accumulator known
bit). The `src/sequencer/memory/` resolver gates MMR READ
knownness at prepare time (transactional reject on unknown,
mirroring the `STACK_POP_RTN` precedent) and the
`src/sequencer/instruction/` root transaction serves routed
requests locally with no endpoint traffic, preserving atomic
WAIT/READY/FAULT/cancel behavior. No timing, IRQ/RPT, wrap,
machine-cycle, public-ABI, or new public CPU snapshot claim
is admitted. No knob is added.

Ownership: `REQ-PROJECT-P3-S1-MMR-FILE` (`src/bus/mmr/`),
`REQ-PROJECT-P3-S1-MMR-RESOLVER` (`src/sequencer/memory/`),
`REQ-PROJECT-P3-S1-MMR-TRANSACTION`
(`src/sequencer/instruction/`), and
`REQ-PROJECT-P3-S1-MMR-OWNER-ACCESS` (`src/state/`) carry
the project contract over TI `Table 3-3` map evidence plus
the reused `BK`/`T` MMR records. The four RED lanes are
`tests/unit/bus/test_mmr_route.c` (CTest
`ti_c54x.mmr_route`), `tests/unit/sequencer/memory/test_mmr_address_resolution.c`
(CTest `ti_c54x.mmr_resolver`),
`tests/unit/sequencer/instruction/test_mmr_memory_transaction.c`
(CTest `ti_c54x.mmr_transaction`), and owner-local accessor
cases under `tests/unit/state/` (CTest
`ti_c54x.mmr_owner_access`). Exit: sealed seam with all four
lanes green, existing endpoint behavior preserved for
declined addresses, and slice 7 (`STM`) unblocked.
