### P2-W44B: circular MOD8/MOD10 per-family composition (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W44B
activation. Figure 5-7 and Table 5-3 establish the common single-indirect
field composition, while Table 5-4 fixes MOD8 as old ARx followed by circular
ARx−1 and MOD10 as old ARx followed by circular ARx+1 for fixed WORD16 steps.
Composed only with the ten existing SPRU172C Smem diagrams, the bounded rows
are exactly:

`LD_SMEM_MOD8/10` `0x10C0/0x10D0` mask `0xFEF8`;
`LD_SMEM_TS_MOD8/10` `0x14C0/0x14D0` mask `0xFEF8`;
`STL_SMEM_MOD8/10` `0x80C0/0x80D0` mask `0xFEF8`;
`LDU_SMEM_MOD8/10` `0x12C0/0x12D0` mask `0xFEF8`;
`LD_T_SMEM_MOD8/10` `0x30C0/0x30D0` mask `0xFFF8`;
`ST_IMMEDIATE_SMEM_MOD8/10` `0x76C0/0x76D0` mask `0xFFF8`;
`LD_DP_SMEM_MOD8/10` `0x46C0/0x46D0` mask `0xFFF8`;
`LD_ASM_SMEM_MOD8/10` `0x32C0/0x32D0` mask `0xFFF8`;
`LD_SMEM_SHIFT16_MOD8/10` `0x44C0/0x44D0` mask `0xFEF8`; and
`ST_T_SMEM_MOD8/10` `0x8CC0/0x8CD0` mask `0xFFF8`. Handler tokens use the
corresponding `_mod8`/`_mod10` suffixes.

W44B is now implementation-complete. RED-first catalog and decode contracts
initially failed on absent MOD8/MOD10 identities, and the inventory contract
reported 55 rather than 75 bindings; the corrected catalog, generated lookup,
inventory, load/store wrappers, and CPU integration are GREEN. The catalog
remains at the exact 76 IDs with 2,343 populated lookup entries and 75
bindings. Family coverage remains 35/205 (17.073%) and strict complete-form
coverage remains 25/205 (12.195%).

W44B reuses the existing `ACCUMULATOR_ARF_MOD4`, `ARF_MOD4`, and
`ARF_EXTENSION16_MOD4` layouts and the completed W44A CIRCULAR_WORD16 MEMORY
effect, resolver, and transaction seams. Selected AR0–AR7 remain admitted;
resolution orders CMPT, ARP, selected AR, then BK, with BK=0 as NONE/no-update
and BK>0 as the positive-BK POST-access wrap update. Inventory remains audit
metadata only: the ten stable forms use
`STANDARD_INDIRECT_MOD0_MOD3_MOD5_MOD6_MOD8_MOD10`, with family and strict
coverage unchanged at 35/205 (17.073%) and 25/205 (12.195%). No
`operand_layout.c` edit is planned; a new status-load circular file and new
store circular common file are preferred owner splits, while central catalog
and inventory edits remain serialized.

Unsupported and out of scope are MOD9/MOD11, MOD14, CMPT=1/dual-indirect,
LONG32 circular scaling, initial-boundary policy beyond the registered planner,
timing/repeatability, BK MMR/device routing, public ABI, new architectural
kinds, IRQ/RPT, and broader catalog scope. Six project requirements and
owner-local tests cover catalog, decode, seven load effects, three store
effects, CPU integration, and inventory multi-binding; all six now have
passing runners. The corrected near-miss fixtures replace stale 0x0040
unknown-word assumptions with explicit circular-sibling expectations. The common load and LD_T load
implementation is 472 lines and must not grow further without a split; the
original status-load file remains 482 lines and the circular status path is
split into its dedicated file. `operand_layout.c` remains 620 lines with the
prior split-before-800 decision.

Closure accounting is 168 evidence records; 272 requirements (235
IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 226 tests (218 PASSING, 8 PLANNED);
and 7 sources. The authoritative standalone `make test` runner is GREEN after
the full masked-row near-miss audit.
