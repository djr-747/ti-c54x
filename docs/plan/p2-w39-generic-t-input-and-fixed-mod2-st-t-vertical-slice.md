### P2-W39: generic T input and fixed-MOD2 ST T vertical slice (planned)

This immutable `(planned)` heading remains the FIT/ownership anchor for the
evidence freeze; the W39 implementation is now complete. TI SPRU172C Store
T/TRN/immediate (printed pp. 4-166–4-168)
defines the one-word `ST T,*ARx+` form with syntax-1 opcode diagram
`1000_1100_IAAAAAAA`, which composes with the registered SPRU131G §5.5.3
fixed-MOD2 `*ARx+` rule as value `0x8C90`, mask `0xFFF8`, fixed bit 8, ARF bits
2:0, eight encodings, and the existing `OPERAND_LAYOUT_ARF`. The operation
copies the old 16-bit T value to the old selected-AR WORD16 address before
postincrement and has no status-bit effects; its printed `*AR7-` example stores
`T=0x4210` at old `AR7=0x0321` then leaves `AR7=0x0320`.

The project contract adds a `T_REGISTER` input resource (bit 32) carrying copied
`t_register_raw16` and `t_register_known` fields from private T state. T is not
read when unrequested; malformed or unknown input is transactional, and unknown
T remains `known=false` for semantic rejection. The `st_t_smem` leaf emits a
pure MEMORY WORD16 WRITE with exact copied T, `complete=NULL`, old-AR address
before postincrement, and no status effects. Generic fixed-MOD2 address
resolution and CPU WAIT/READY/FAULT/sequential-PC retirement are reused without
a CPU opcode branch. No new layout, effect, address, architectural-state,
public API, source-manifest, timing, or machine-cycle seam was added.

Five RED lanes established the input, catalog, decode, effects, and CPU
retirement contracts. Focused strict plus legacy input/store regressions,
focused ASan/UBSan, quality-186, semantic review, integration review,
independent fresh CMake CTest 183/183, and authoritative clean `make test` are
green; integration corrected two stale `ID_COUNT34` fixtures to `ID_COUNT35`.
Realized coverage is 34 rows, 1487 populated first-word encodings, ID_COUNT 35,
6 reset-runnable rows, and four aggregate fixed-MOD2 memory rows. Repeatability
remains `UNRESOLVED` because Table 2-28 absence is not positive eligibility
evidence. STH, LDR, LD#K, and TS remain deferred for their recorded gaps;
W31P/W31B performance follow-ups remain open and nonblocking.
