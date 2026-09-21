### P3-S114: LD parallel-MAC/MAS dual-memory vertical slice (implemented; local verification green)

This anchor heads the P3-S114 evidence freeze. Table 2-23
(TI SPRU172C p. 2-16) lists four parallel load-and-multiply
forms (`LD||MAC`, `LD||MACR`, `LD||MAS`, `LD||MASR`); the
§LD||MAC[R] diagram (p. 4-74, `1010_10RD_XXXX_YYYY`) and the
§LD||MAS[R] diagram (p. 4-76, `1010_11RD_XXXX_YYYY`) pin the
two shared rows at 0xA800/0xAC00 under mask 0xFC00 (R bit 9,
D bit 8, 8-bit dualxy 7:0 with the FIRS XMOD/XAR/YMOD/YAR
split). The R suffix rides bit 9 inside each row, so the
rounded variants bind to the shared rows (S99 precedent) and
no R-specific row lands. Table 2-28 carries no parallel
entry, so repeatability is ELIGIBLE by the absence
precedent. The TI worked example (p. 4-74: `LD *AR4+,A ||
MAC *AR5+,B`, Xmem 0x1234 → A `00 1234 0000`, T untouched,
both ARs post-incremented) pins the LD-half full-40
zeroing load, the live-T MAC half, and D=0 selecting
LD-dst A with MAC-dst_ B.

No new seam: the W50A/W51B dual root stages the pair read,
the multiply core proves the YxT+dst_ accumulation with
FRCT/OVM and optional rounding, and the generic effect
transaction retires the ACCUMULATOR_WRITE_PAIR. The LD half
reuses the shift16-load precedent (SXM-widen X, logical
left shift 16); the MAC half mirrors the MAC-dual
completion (C/TC preserved, OV written only on the dst_
channel). T is a live validated input and is never written
(no read-phase T load). The slice adds: two catalog rows,
one `ROUND_DESTINATION_DUALXY` layout ([RD(2), DUALXY(8)],
within the four-operand trusted capacity), one
AUX+T-combined input token (no new resource bit; the
16-bit mask is fully allocated), one shared leaf with
ADD/SUB select plus two wrapper pairs, op/CPU/metadata
suites, four inventory bindings (T2-23-01/02 → MAC row,
03/04 → MAS row, all COMPLETE), and the evidence triad.
In-word X/Y modes 0-3 ride the shared dual resolver with
no addressing change.
