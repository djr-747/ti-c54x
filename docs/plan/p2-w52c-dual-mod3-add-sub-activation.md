### P2-W52C: dual MOD3 ADD/SUB activation (planned)

This immutable `(planned)` heading is the durable W52C FIT/ownership anchor.
W52C activates the fourteen MOD3-involving syntax-6 dual-memory ADD/SUB
rows at `0xA000|(m<<6)|(n<<2)` and `0xA200|(m<<6)|(n<<2)` for `(m,n)` in
`{0,1,2,3}^2` minus the nine MOD0–MOD2 combinations, each with mask
`0xFECC`, one program word, thirty-two encodings, the existing
`DUAL_DESTINATION_XAR_YAR` layout, `UNRESOLVED` repeatability, and
`AUXILIARY_REGISTERS_STATUS_C_SXM_OVM` resources. Checked and trusted
leaves share the operation/mode-parameterized core with per-row identity
validation and completion validators; macro-driven dispatch binds all rows
with no opcode-specific CPU branch and no public ABI change. MOD3 sides
retire through the generic W52B root with AR0/BK snapshots, atomic per-row
address publication, single X-owned commits for same-AR aliases, and
destination routing. The fourteen neighborhoods were probed CLEAN before
activation; none matched a positively listed Table 2-28 nonrepeatable
form, so all fourteen stay `UNRESOLVED`.

Ownership: `REQ-COMMON-P2-W52C-DUAL-MOD3-CATALOG` (`src/isa/catalog/`),
`REQ-COMMON-P2-W52C-DUAL-MOD3-DECODE` (`src/isa/decode/`), and
`REQ-COMMON-P2-W52C-DUAL-MOD3-EFFECTS` (`src/isa/ops/arithmetic/`) carry
the TI architectural evidence;
`REQ-PROJECT-P2-W52C-DUAL-MOD3-TRUSTED-BINDING`
(`src/isa/ops/dispatch/`),
`REQ-PROJECT-P2-W52C-DUAL-MOD3-CPU-RETIREMENT` (`src/cpu/`), and
`REQ-PROJECT-P2-W52C-DUAL-MOD3-INVENTORY` (`tools/quality/`) carry the
project contract. The RED executable contracts are
`tests/unit/isa/catalog/test_add_sub_dual_mod3_metadata.c` with CTest name
`ti_c54x.add_sub_dual_mod3_metadata`,
`tests/unit/isa/decode/test_add_sub_dual_mod3_decode.c` with CTest name
`ti_c54x.add_sub_dual_mod3_decode`,
`tests/unit/isa/ops/arithmetic/test_add_sub_dual_mod3.c` with CTest name
`ti_c54x.add_sub_dual_mod3`, and
`tests/unit/cpu/isa/arithmetic/test_cpu_add_sub_dual_mod3.c` with CTest
name `ti_c54x.cpu_add_sub_dual_mod3`; all four are green. The W52C closure
registry is 219 immutable evidence records, 353 requirements (320
`IMPLEMENTED`, 2 `PLANNED`, 31 `UNRESOLVED`), and 299 tests (297
`PASSING`, 2 `PLANNED`). Coverage is 134 catalog rows, `ID_COUNT=135`,
3,991 lookup entries, 134 trusted bindings, family coverage above 44/205,
and unchanged 27/205 strict complete-form coverage. The next documented
work is reset/MMR edges.
