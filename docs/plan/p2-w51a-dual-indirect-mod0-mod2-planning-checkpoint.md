### P2-W51A: dual-indirect MOD0-MOD2 planning checkpoint (implemented; local verification green)

This immutable `(planned)` heading is the durable W51A FIT/ownership anchor.
W51A extends the pure dual-indirect WORD16 planner and the sequencer resolution
seam without activating an opcode row or changing the CPU, catalog, inventory,
root carrier, or public API. The primary evidence is SPRU131G §5.5.4/Table 5-8
(printed pp. 5-19–5-22): modes 0, 1, and 2 retain the old address, stage a
post-access decrement of one, or stage a post-access increment of one, and the
encoded Xar/Yar fields directly select AR2–AR5. SPRU179C ADD Example 4
(printed p. 4-7) independently demonstrates that `*AR2+` paired with `*AR2−`
leaves the X-side increment effective. MOD3 stays deferred because its AR0/BK
circular dependency and the existing CMPT1 dual boundary are not yet resolved.

`src/address/` owns the value-only mode planner. It accepts modes 0, 1, and 2,
uses copied old AR values for both operands, stages at most one post-access AR
update for a same-selector alias, and gives Xmod precedence. Directly encoded
Xar/Yar selectors do not read CMPT or ARP; those controls belong to other
addressing families. Boundary rejection remains transactional, with no partial
X/Y effect on error. The new COMMON requirement is exercised by
`tests/unit/address/test_indirect_dual_modes.c`.

`src/sequencer/memory/` resolves the pair against one frozen architectural
snapshot and publishes two canonical DATA READ requests plus value-owned commit
candidates. W51A deliberately preserves the requests-only persistent snapshot
at exactly 32 bytes (`sizeof(ti_c54x_dual_memory_address_snapshot) == 32`);
atomic commit/publication of the staged updates is the next W51B seam. Existing
`tests/unit/sequencer/memory/test_dual_address_resolution.c` now covers the
MOD1/MOD2 and same-AR behavior while retaining W50A's reverse-linked project
resolution requirement.

W51A's RED→GREEN focused evidence covers all nine MOD0/MOD1/MOD2 X/Y mode pairs,
old-address request order, decrement/increment candidates, same-AR Xmod
precedence, transactional boundary rejection, and independence from CMPT/ARP.
The observer-off W50B smoke remains green with semantic hash
`b40823092c0aa605`. A dirty-tree W51A sample measured 733.988–882.744
ns/instruction; this is volatile host characterization only, with no regression
threshold, release gate, TI-cycle claim, or benchmark-history artifact.

W51A closure accounting is 206 immutable evidence records; 337 requirements
(299 `IMPLEMENTED`, 7 `PLANNED`, 31 `UNRESOLVED`); and 287 tests (280 `PASSING`,
7 `PLANNED`). Catalog, lookup, binding, family, and strict-complete coverage
remain 104, 3,031, 104, 44/205, and 27/205. The next documented seam is W51B:
persist both prevalidated update candidates and publish the root transaction
atomically, after which broader dual-mode opcode activation can proceed.
