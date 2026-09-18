### P3-S44: TRN input seam (implemented; local verification green)

Maintainer arch seam that unblocks the deferred ST TRN direct row
(`FORM-T2-22-02`): the copied input snapshot carries no TRN field
today. Mirroring the T_REGISTER (P2-W39), SST, and stack-pointer
(P2-W64) seam precedents, S44 adds one input resource bit
`TI_C54X_INPUT_RESOURCES_TRN_REGISTER` (8192, fourteenth
resource) with `KNOWN_MASK` inclusion, snapshot fields
`trn_register_raw16` plus `trn_register_known`, a `collect.c`
arm reading the architectural TRN through the MMR file cell
(`ti_c54x_mmr_file_read_trn`: OK copies exact WORD16 as known,
UNKNOWN leaves zero/unknown, anything else rejects the
transaction), and a borrowed `mmr_file` source pointer wired
once in `run.c` from `cpu->arch_state.mmr_file`. Unrequested
TRN is not read. The eleven `0x2000` invalid-resource probes
across the `cpu/input` collect suites flip to `0x4000`, and the
pinned 64-byte snapshot ABI asserts (LP64/ILP32 plus the perf
contract) move to the verified new size. No catalog row,
layout, handler, effect, inventory form, or CPU retirement
change is admitted; the ST TRN leaf belongs to S45.

Ownership: `REQ-PROJECT-P3-S44-TRN-INPUT-SEAM`
(`src/cpu/input/`, `src/isa/ops/`, `src/isa/catalog/`,
`src/cpu/execution/`) carries the project contract. The RED
executable contract is
`tests/unit/cpu/input/test_collect_trn_register.c` mirroring
the T collection proof (exact copies, unknown canonicalization,
selectivity, transactional rejection). Projected closure from
the S43 freeze (293 rows, 299 bindings, 32,649 lookup entries,
141/205 forms at 68.780%, 761 requirements, 617 tests):
catalog, lookup, bindings, and family coverage unchanged, 762
requirements, 618 tests. These numbers are projections until
the slice lands.
