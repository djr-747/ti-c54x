### P3-S13: STH by-SHIFT Smem store slice (implemented; local verification green)

STH syntax 4 (`STH src,SHIFT,Smem`, two words) for the direct-DP word plus
the MOD4-family single-indirect words (MOD4, MOD5, MOD6, MOD7, MOD8, MOD10).
MOD0-MOD3 stay deferred: their first words collide with the LD Smem,SHIFT
range at 0x6F80 and need second-word disambiguation work. The seven rows are
`STH_SHIFT_DIRECT` (0x6F00 under mask 0xFF80, 7-bit dma) and
`STH_SHIFT_SMEM_MOD4` through `STH_SHIFT_SMEM_MOD10` (0x6FA0-0x6FD0 under
mask 0xFFF8, ARF in bits 2-0, MOD in bits 6-3). The second word carries the
0x0C60 fixed pattern (bits 15-9 carry 0x06, bits 7-5 fixed 011) with S at
bit 8 selecting accumulator A (0) or B (1) and the signed 5-bit SHIFT in
bits 4-0; the word-2 mask is 0xFEE0 so the source select and SHIFT stay
outside it.

Semantics: shift the S-selected accumulator arithmetically by the signed
SHIFT count (-16 to +15, negative counts shift right) and store bits 31-16
of the shifted value. The stored field is pure source bits, so SXM rides
hostile and unconsumed. The direct form addresses (DP << 7) | dma with DP
unknownness gated at the leaf through STATUS_DP; the MOD rows stage
MOD4-family indexed (MOD5/MOD6), circular (MOD8/MOD10), or reverse-carry
(MOD4/MOD7) WRITEs through the shared staging helpers with no ARF0 alias
rejection (the two-word form carries no ARP alias). No status,
accumulator, or T-register effect. The rows stay repeatability UNRESOLVED
on the absent TI repeatability note.

Seams: two new layouts (`ACCUMULATOR_DIRECT7_SHIFT5`,
`ACCUMULATOR_ARF_MOD4_SHIFT5`) with checked sets, trusted plans, and schema
tokens; the `ACCUMULATORS_STATUS_DP` input resource with snapshot
(`status_dp`/`status_dp_known`) and collect arms; the
`DIRECT_DP_WORD16` address kind reusing the shared `long_offset_word`
(DP page) and `direct_dma7` (offset) carrier with no ABI growth,
resolution through the shared DP planner, a candidate-validator shape arm,
and a `matches_live` arm that re-reads CPL==0 and DP==dp9 live (the generic
CMPT0/ARP0 gate does not apply). The input snapshot grows 60 to 64 bytes
on ILP32 for the DP fields (LP64 unchanged at 64); the frozen m32 root
pin moves 328 to 332.

RED contracts: `tests/unit/isa/ops/store/test_sth_shift_direct.c` (nine
accumulator/SHIFT vectors over both sources and four hostile SXM seeds,
transactional DP/unknown/malformed gates, trusted-operand equality),
`tests/unit/isa/ops/store/test_sth_shift_smem_mod.c` (all six MOD rows
over ARF0-ARF7 with indexed/circular/reverse-carry staging), and
`tests/unit/cpu/isa/store/test_cpu_sth_shift_direct.c` (TI vector plus
unknown-DP rejection, fault, and boundary cases through the generic
MEMORY route).

Evidence projection: two TI records (syntax-4 opcode and
semantics/example), one P3-S13 activation, three requirements
(SEMANTICS, CPU-RETIREMENT, INVENTORY), three test records (DIRECT, MOD,
CPU). FORM-T2-20-08 lands SUBSET (seven symbols); catalog-family bound
forms 93 to 94 of 205 (strict lower bound stays 34); catalog rows
239 to 246; lookup 21345 to 21521.
