### P2-W42B: standard-indirect MOD0–3 Smem family expansion (planned)

This immutable `(planned)` heading is the FIT/ownership anchor for the W42B
activation. W42B broadens the ten existing fixed-MOD2 standard-indirect Smem
families by replacing, rather than adding, their `_MOD2` catalog rows with
neutral symbols: `LD_SMEM`, `LD_SMEM_TS`, `STL_SMEM`, `LDU_SMEM`,
`LD_T_SMEM`, `ST_IMMEDIATE_SMEM`, `LD_DP_SMEM`, `LD_ASM_SMEM`,
`LD_SMEM_SHIFT16`, and `ST_T_SMEM`. Their exact MOD0 bases and masks are,
respectively, `0x1080/0xFEE0`, `0x1480/0xFEE0`, `0x8080/0xFEE0`,
`0x1280/0xFEE0`, `0x3080/0xFFE0`, `0x7680/0xFFE0`, `0x4680/0xFFE0`,
`0x3280/0xFFE0`, `0x4480/0xFEE0`, and `0x8C80/0xFFE0`.

The broadened forms use explicit neutral layouts without changing existing
operand indices: `ACCUMULATOR_ARF_MOD` carries D/source, ARF, MOD;
`ARF_MOD` carries ARF, MOD; and `ARF_EXTENSION16_MOD` carries ARF,
EXTENSION16, MOD. Mode bits are [4:3], ARF is [2:0], and D is bit 8.
Handlers consume decoded mode fields and never parse raw instruction words.
Loads admit MOD0/MOD1/MOD2 and return `INVALID_OPERAND` for MOD3 before any
MEMORY effect or endpoint request; stores admit MOD0–MOD3 through the W42A
transaction seam. No new effect kind, address kind, CPU branch, public ABI,
device/MMR route, timing contract, or architectural state is added.

Status: COMPLETE. The five W42B requirements and executable tests are closed;
the implementation retains the project-contract boundary and makes no new
public ABI or timing promise.

The expansion retains `ID_COUNT=36` and projects the direct first-word lookup
from 1503 to 1863 (+360): 480 broadened encodings comprise 392 legal forms and
88 known-but-rejected load MOD3 forms, for +272 legal and +88 rejected words.
Inventory symbols become neutral while all ten remain `SUBSET` with
`addressing_subset=STANDARD_INDIRECT_MOD0_MOD3`; family progress remains
35/205 (17.073%) and strict complete-form coverage remains 25/205 (12.195%)
through W42B because other Smem addressing forms remain outside this slice.
The load implementation is split across `op_ld_smem.c` (~393 lines) and
`op_ld_smem_common.c` (~145 lines), keeping each module below the review
threshold. `operand_layout.c` is ~577 lines and remains cohesive for now; it
must be split before further growth approaches 800 lines. W42A completion and
lookup projections are preserved, and no duplicate/conflicting row semantics
are admitted.
