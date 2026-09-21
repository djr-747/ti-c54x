### P3-S116: ST parallel-MAC/MAS/MPY rows over the store-compute seam (implemented; local verification green)

Table 2-25 (TI SPRU172C p. 2-17) parallel store-and-multiply forms
`ST||MAC[R]`, `ST||MAS[R]`, `ST||MPY` land as three shared catalog rows:
`ST_PARALLEL_MAC` at 0xD000, `ST_PARALLEL_MAS` at 0xD800 (each under
mask 0xF800 with R at bit 10 riding inside the row per the S99 shared-row
precedent), and `ST_PARALLEL_MPY` at 0xCC00 under mask 0xFC00. S bit 9
selects the store source, D bit 8 the compute destination, dualxy 7:0 the
FIRS-split X/Y modes; all rows ELIGIBLE by Table 2-28 absence. MAC/MAS use
a new ROUND_SRC_DEST layout [R(1)@10, S(1)@9, D(1)@8, DUALXY(8)] with an
ASM+T+FRCT input token; MPY reuses the S115 SRC_DEST_DUALXY layout. The new
`op_st_parallel_mac` leaf (arithmetic home, checked plus trusted entries,
transactional rejection) stages the same dual WORD16 WRITE with the
leaf-frozen STH-by-ASM value and single-read completion, no T load; the
completion runs the multiply core over live-T times X with the DST
accumulator as addend base (MAC: dst+T*X, MAS: dst-T*X, MPY: T*X replacing
dst, R rounding per the S114 shape) with C and TC preserved
(multiply-core MAC precedent: the core reports no carry) and
OV written on 32-bit overflow with OVM saturation. The S115 store-compute root, child-route
entry, and Y-template echo are reused with no sequencer change. T2-25-01/02
bind the MAC row, 03/04 the MAS row, 05 the MPY row, all COMPLETE/ALL.
TI vectors: MAC Ex1 (ST A,*AR4- || MAC *AR5,B; B to 00 010C 9511, Y 0222,
AR4 to 00FF) and MAS Ex1/Ex2 (B to FF FEF3 8D11 / FF FEF4 0000, Y 0222 /
0022) plus MPY (ST A,*AR3+ || MPY *AR5+,B; B to 00 2000 0000, Y 8421,
AR3/AR5 +1); T=0x0400/0x4000 preserved, FRCT as printed. Table 2-24's
ST||ADD expression prints `ASM*16` against its own section Description
(`ASM-16`) and every worked ST vector; the seam follows the Description
and vectors (S115 ledger correction).
