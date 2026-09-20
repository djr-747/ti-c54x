### P2-W52B: dual MOD3 AR0/BK planning checkpoint (implemented; local verification green)

This immutable `(planned)` heading is the durable W52B FIT/ownership anchor.
W52B admits dual-indirect MOD3 (`*ARx+0%`) through the address planner, the
sequencer resolution seam, and the root commit validator without activating
an opcode row or changing the CPU, catalog, inventory, or public API. The
primary evidence is TI SPRU131G Table 5-8 (MOD3 function `addr = ARx` with
post-access `ARx = circ(ARx + AR0)`), §5.5.4.2 (BK=0 gives plain indexed
`ARx + AR0` addition), §5.5.4.3 (nonzero BK gives §5.5.3.4 circular
addressing), §5.5.3.4 step rules (signed AR0 step with magnitude at most BK
and first access inside the buffer), and Tables 5-6/5-7 plus §5.5.5 (Xar/Yar directly select
AR2–AR5 in a format with no ARF field, bounding the compatibility-mode
question). Same-selector aliases keep the evidenced Xmod precedence with
one X-owned update; indexed overflow and out-of-buffer circular starts
reject as boundaries with no wrap inference; unknown AR0/BK sources reject
at resolution; identical specs resolve identically under any CMPT/ARP
state. The persistent dual snapshot widens 48 to 52 bytes with copied
AR0/BK shadows so the root validator can recheck MOD3 commits, while the
root carriers hold at 336/308 because the candidate union already dominates
the dual snapshot arm.

Ownership: `REQ-COMMON-P2-W52B-DUAL-MOD3-PLAN` (`src/address/`),
`REQ-COMMON-P2-W52B-DUAL-MOD3-RESOLVE` (`src/sequencer/memory/`), and
`REQ-COMMON-P2-W52B-DUAL-MOD3-ROOT` (`src/sequencer/instruction/`) carry
the TI architectural evidence. The RED executable contracts are
`tests/unit/address/test_indirect_dual_mod3.c` with CTest name
`ti_c54x.indirect_dual_mod3`,
`tests/unit/sequencer/memory/test_dual_mod3_resolution.c` with CTest name
`ti_c54x.dual_mod3_resolution`, and
`tests/unit/sequencer/instruction/test_dual_mod3_root.c` with CTest name
`ti_c54x.dual_mod3_root`; all three are green. The W52B closure registry
is 217 immutable evidence records, 347 requirements (314 `IMPLEMENTED`, 2
`PLANNED`, 31 `UNRESOLVED`), and 295 tests (293 `PASSING`, 2 `PLANNED`).
Catalog, lookup, binding, family, and strict-complete coverage are
unchanged at 120, 3,543, 120, 44/205, and 27/205. The next documented work
is W52C MOD3 dual ADD/SUB activation, then reset/MMR edges.
