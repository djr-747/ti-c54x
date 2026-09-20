### P2-W51C: dual ADD/SUB MOD0–MOD2 activation (implemented; local verification green)

This immutable `(planned)` heading is the durable W51C FIT/ownership anchor.
W51C activates the eight non-MOD0/MOD0 dual-indirect MOD0/MOD1/MOD2
combinations for syntax-6 `ADD Xmem,Ymem,dst` and `SUB Xmem,Ymem,dst`
through the W51B root; it adds no addressing mode, alters no CPU wiring,
widens no public API, and changes no timing or machine-cycle claim. The
W50B syntax-6 first-word evidence remains the source for the D[8],
Xar[5:4], and Yar[1:0] positions and the ADD/SUB bit-9 discriminator; the
W51A mode evidence remains the source for MOD0/MOD1/MOD2 old-address and
post-access update behavior with same-AR Xmod precedence. A clean-decode
probe proves the sixteen new first-word neighborhoods are currently
unclaimed, and MOD3 neighborhoods stay undecoded and deferred. All
lifecycle, eligibility, and ABI assertions in this slice are project
contracts; repeatability remains `UNRESOLVED` for every new row.

Each new row keeps mask `0xFECC`, one program word, thirty-two encodings
(D by XAR by YAR), the existing `DUAL_DESTINATION_XAR_YAR` layout,
`UNRESOLVED` repeatability, and the `AUXILIARY_REGISTERS_STATUS_C_SXM_OVM`
resources. ADD rows are
`ADD_DUAL_M{m}_{n}` at `0xA000|(m<<6)|(n<<2)` and SUB rows are
`SUB_DUAL_M{m}_{n}` at `0xA200|(m<<6)|(n<<2)` for `(m,n)` in `{0,1,2}^2`
minus `(0,0)`; the existing `ADD_DUAL_MOD0`/`SUB_DUAL_MOD0` rows keep their
names. Checked and trusted leaves share one core parameterized by
operation and both modes; per-row wrappers and completion validators prove
the exact decoded identity and stage the row's modes without an
opcode-specific CPU branch. Inventory binds all nine combos per form as
`SUBSET` under per-row `DUAL_INDIRECT_MOD{m}_MOD{n}` addressing; both forms
remain `SUBSET` while MOD3 is deferred. The MOD0 benchmark roster is frozen,
so no new hosted scenarios are added: the unchanged MOD0 smoke re-runs as
the shared-path characterization.

Ownership follows the W50B split: `REQ-COMMON-P2-W51C-DUAL-MODES-CATALOG`
(`src/isa/catalog/`), `REQ-COMMON-P2-W51C-DUAL-MODES-DECODE`
(`src/isa/decode/`), and `REQ-COMMON-P2-W51C-DUAL-MODES-EFFECTS`
(`src/isa/ops/arithmetic/`) carry the TI architectural evidence, while
`REQ-PROJECT-P2-W51C-DUAL-MODES-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`), `REQ-PROJECT-P2-W51C-DUAL-MODES-CPU-RETIREMENT`
(`src/cpu/`), and `REQ-PROJECT-P2-W51C-DUAL-MODES-INVENTORY`
(`tools/quality/`) carry the project plan anchor. Dispatch and inventory
are already green through their shared generic proofs (the all-ID dispatch
test and the nine-combo binding projection); catalog, decode, effects, and
retirement stay planned.

The RED executable contracts are
`tests/unit/isa/catalog/test_add_sub_dual_modes_metadata.c` with CTest name
`ti_c54x.add_sub_dual_modes_metadata`,
`tests/unit/isa/decode/test_add_sub_dual_modes_decode.c` with CTest name
`ti_c54x.add_sub_dual_modes_decode`,
`tests/unit/isa/ops/arithmetic/test_add_sub_dual_modes.c` with CTest name
`ti_c54x.add_sub_dual_modes`, and
`tests/unit/cpu/isa/arithmetic/test_cpu_add_sub_dual_modes.c` with CTest
name `ti_c54x.cpu_add_sub_dual_modes`; all four are green, and dispatch and
inventory were already green through their shared generic proofs. The W51C
closure registry is 210 immutable evidence records, 344 requirements (306
`IMPLEMENTED`, 7 `PLANNED`, 31 `UNRESOLVED`), and 292 tests (285 `PASSING`,
7 `PLANNED`). Catalog, lookup, binding, family, and strict-complete
coverage are 120, 3,543, 120, 44/205, and 27/205. The MOD0 benchmark smoke
re-runs unchanged as the shared-path characterization with semantic hash
`b40823092c0aa605`; the next documented work is the repeat-machinery phase.
