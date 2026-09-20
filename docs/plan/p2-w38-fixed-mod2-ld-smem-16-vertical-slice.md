### P2-W38: fixed-MOD2 LD Smem,16 vertical slice (implemented; local verification green)

This immutable `(planned)` heading is the FIT/ownership anchor for this
evidence freeze. The bounded one-word `LD Smem,16,dst` form from TI SPRU172C
§4.66 (printed pp. 4-65–4-69) is implemented as
`LD_SMEM_SHIFT16_MOD2` with unique handler token `ld_smem_shift16`, value
`0x4490`, mask `0xFEF8`, D bit 8, ARF bits 2:0, sixteen encodings, the existing
`ACCUMULATOR_ARF` layout, `STATUS_SXM` input, and repeatability `UNRESOLVED`.
The exact SXM transform is WORD16 zero-extension when SXM=0 or sign-extension
when SXM=1 to canonical 40 bits, followed by logical left shift 16 within 40
bits: `0000` is zero in both modes, `7fff` is `0x007fff0000`, `8000` is
`0x0080000000`/`0xff80000000`, `ffff` is `0x00ffff0000`/`0xffffff0000`, and
the printed `FEDC`/SXM=1 vector is `0xfffedc0000`. C, TC, OV, and unrelated
state remain unchanged. The unique handler extends the shared `op_ld_smem`
core; existing MEMORY read/completion, ACCUMULATOR_WRITE, fixed-MOD2
addressing, and generic CPU WAIT/READY/FAULT sequential-PC paths are reused
under CMPT=0/ARP=0. No new layout, resource, effect, address, state, CPU,
public API, or source-manifest seam was added. The TDD fixture corrections
record that D bit 8 is an operand (not invalid), `0x4490 ^ 0x0200 = 0x4690`
collides with the W37 DP sibling so `0x0208`/`0x4698` are used, and stale W37
ID-count and aggregate fixed-MOD2 row-count expectations were updated from
33→34 and 2→3. STH, LDR, LD#K, ST T, and TS remain deferred for their recorded
gaps. Focused strict, ASan/UBSan, quality-186, dual-review, and independent
CMake full CTest 178/178 and final clean `make test` gates are green. The
`op_ld_smem.c` leaf is 357 lines and its
header is 44 lines, both below 500; `operand_layout.c` remains 535 lines with
no split. Realized coverage is 33 rows and 1479 populated first-word encodings
with 6 reset-runnable. W31P/W31B performance follow-ups remain open and
nonblocking.
