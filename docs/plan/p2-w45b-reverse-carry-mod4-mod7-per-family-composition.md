### P2-W45B: reverse-carry MOD4/MOD7 per-family composition (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for W45B. The
ten existing Smem families compose exactly twenty private MOD4/MOD7 rows:

`LD_SMEM_MOD4/7` `0x10A0/0x10B8` mask `0xFEF8`;
`LD_SMEM_TS_MOD4/7` `0x14A0/0x14B8` mask `0xFEF8`;
`STL_SMEM_MOD4/7` `0x80A0/0x80B8` mask `0xFEF8`;
`LDU_SMEM_MOD4/7` `0x12A0/0x12B8` mask `0xFEF8`;
`LD_SMEM_SHIFT16_MOD4/7` `0x44A0/0x44B8` mask `0xFEF8`;
`LD_T_SMEM_MOD4/7` `0x30A0/0x30B8` mask `0xFFF8`;
`ST_IMMEDIATE_SMEM_MOD4/7` `0x76A0/0x76B8` mask `0xFFF8` (two words);
`LD_DP_SMEM_MOD4/7` `0x46A0/0x46B8` mask `0xFFF8`;
`LD_ASM_SMEM_MOD4/7` `0x32A0/0x32B8` mask `0xFFF8`; and
`ST_T_SMEM_MOD4/7` `0x8CA0/0x8CB8` mask `0xFFF8`. Handler tokens use
`_mod4`/`_mod7` suffixes and the existing MOD4 layouts/resources/word counts.

W45B reuses the typed W45A reverse-carry seam with CMPT=0/ARP=0, known
nonzero one-hot AR0, old selected-AR address, and POST selected-AR update.
ARF1..7 executes; ARF0 encodings decode but handlers reject transactionally
before endpoint issue because alias semantics remain unresolved. No BK or
wrap inference is admitted, and no operand-layout, root-transaction, CPU
branch, public ABI, timing, device, or RPT change is included. Inventory uses
`STANDARD_INDIRECT_MOD0_MOD3_MOD4_MOD5_MOD6_MOD7_MOD8_MOD10`; family and strict
coverage remain 35/205 (17.073%) and 25/205 (12.195%).

Closure accounting is 170 evidence records; 281 requirements (244
IMPLEMENTED, 8 PLANNED, 29 UNRESOLVED); 235 tests (227 PASSING, 8 PLANNED);
and 7 sources. W45B's six owner-local RED lanes are GREEN: the authoritative
standalone `make test` is GREEN, quality validation covers 215 tests, fresh
post-fix ASan/UBSan validation is 5/5 without diagnostics, and independent
review is clean. No new knob was added; the family and strict coverage remain
35/205 (17.073%) and 25/205 (12.195%).
