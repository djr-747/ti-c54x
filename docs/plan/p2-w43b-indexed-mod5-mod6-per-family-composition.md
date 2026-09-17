### P2-W43B: indexed MOD5/MOD6 per-family composition (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W43B
activation. TI SPRU131G Figure 5-7 and Table 5-3 (printed pp. 5-10–5-11)
establish the single-indirect composition of family opcode fields with D where
present, ARF bits [2:0], and MOD bits [4:3] under CMPT=0/ARP=0; Table 5-4
(printed p. 5-13) defines MOD5 as old ARx for the operand followed by ARx−AR0
and MOD6 as old ARx followed by ARx+AR0. The composition is pinned only for
the ten existing SPRU172C family diagrams/pages: `LD Smem,dst`, `LD
Smem,TS,dst`, `STL src,Smem`, `LDU Smem,dst`, `LD T,Smem`, `ST immediate,Smem`,
`LD Smem,DP`, `LD Smem,ASM`, `LD Smem,16,dst`, and `ST T,Smem`.

W43B freezes exactly twenty private IDs and unique wrapper tokens, one MOD5
and one MOD6 row per family: `LD_SMEM_MOD5/6`, `LD_SMEM_TS_MOD5/6`,
`STL_SMEM_MOD5/6`, `LDU_SMEM_MOD5/6`, `LD_T_SMEM_MOD5/6`,
`ST_IMMEDIATE_SMEM_MOD5/6`, `LD_DP_SMEM_MOD5/6`, `LD_ASM_SMEM_MOD5/6`,
`LD_SMEM_SHIFT16_MOD5/6`, and `ST_T_SMEM_MOD5/6`, with handler tokens using
the corresponding `_mod5`/`_mod6` suffix. Their exact base/mask pairs, in
the same family order, are `0x10A8/0xFEF8` and `0x10B0/0xFEF8`,
`0x14A8/0xFEF8` and `0x14B0/0xFEF8`, `0x80A8/0xFEF8` and `0x80B0/0xFEF8`,
`0x12A8/0xFEF8` and `0x12B0/0xFEF8`, `0x30A8/0xFFF8` and `0x30B0/0xFFF8`,
`0x76A8/0xFFF8` and `0x76B0/0xFFF8`, `0x46A8/0xFFF8` and `0x46B0/0xFFF8`,
`0x32A8/0xFFF8` and `0x32B0/0xFFF8`, `0x44A8/0xFEF8` and `0x44B0/0xFEF8`,
and `0x8CA8/0xFFF8` and `0x8CB0/0xFFF8`.

The three appended private decode layouts are `ACCUMULATOR_ARF_MOD4`,
`ARF_MOD4`, and `ARF_EXTENSION16_MOD4`; they preserve existing operand
indices while carrying the indexed mode discriminator. Each family wrapper
consumes decoded fields and delegates its existing pure transform to the W43A
`INDEXED_AR0_WORD16` MEMORY READ/WRITE seam, including old-AR addressing,
selected-AR post-update, and atomic WAIT/READY/FAULT/cancel behavior. No new
effect/address/state kind, public ABI, opcode-specific CPU branch, timing,
device/MMR, IRQ/RPT, wrap, or architectural-state claim is admitted.

Inventory remains audit metadata only: the ten subset bindings use the
multi-addressing token `STANDARD_INDIRECT_MOD0_MOD3_MOD5_MOD6`, with family
and strict coverage percentages unchanged until complete-form evidence exists.
The planned catalog projection is `ID_COUNT=56` and 2,103 populated first-word
lookup entries (+240); these are implementation targets, not realized coverage.
The six RED lanes are owner-local project tests for catalog metadata/lookup,
decode/layout, indexed load effects, indexed store effects, CPU integration,
and inventory multi-binding quality. Planned CTest runners are
`ti_c54x.indexed_standard_indirect_metadata`,
`ti_c54x.indexed_standard_indirect_decode`,
`ti_c54x.indexed_standard_indirect_load_effects`,
`ti_c54x.indexed_standard_indirect_store_effects`,
`ti_c54x.cpu_indexed_standard_indirect`, and the existing
`ti_c54x.quality.python` discovery runner for the inventory lane; each test
path and owning module is recorded in the evidence registry.

Implementation closure: all six W43B RED lanes are GREEN. The catalog,
decode/layout, indexed load, indexed store, CPU integration, and inventory
multi-binding fixtures passed their recorded runners; a fresh ASan+UBSan
configure/build passed the five focused CTests (5/5) with no diagnostics, and
the inventory Python lane passed 3/3. The catalog projection
is ID_COUNT=56 with 2,103 populated first-word lookup entries and 55 bindings;
the ten indexed form bindings retain the
`STANDARD_INDIRECT_MOD0_MOD3_MOD5_MOD6` addressing token, so inventory family
coverage remains 35/205 (17.073%) and strict complete-form coverage remains
25/205 (12.195%). The implementation fixed the stale W42B/legacy ID_COUNT
fixtures and added the TS pure prevalidation path before the memory endpoint;
these corrections are regression fixtures, not new ISA scope. Operand
ownership review retains `src/isa/decode/operand_layout.c` at 620 lines as a
cohesive owner and requires a split before 800 lines. The current load/store
owners remain below that threshold: `op_ld_smem.c` 393 lines,
`op_ld_smem_common.c` 308, `op_ld_smem_indexed.c` 317, and
`op_store_indexed_common.c` 243. No public ABI, new effect/address/state
kind, CPU branch, timing, device/MMR, or IRQ/RPT expansion was added by this
closure; sanitizer output is validation evidence only.

Closure accounting is 165 evidence records; 263 requirements (226
IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 217 tests (209 PASSING, 8 PLANNED);
and 7 sources. Inventory coverage remains 35/205 = 17.073% catalog-family
progress and 25/205 = 12.195% strict complete-form coverage. W43A seam reuse
is the only address/effect integration admitted; no new ABI, timing, device,
CPU branch, or architectural state is planned.
