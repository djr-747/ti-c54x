### P3-S115: dual store-compute seam with ST parallel-ADD/SUB rows (implemented; local verification green)

This anchor heads the P3-S115 evidence freeze. Table 2-24
(TI SPRU172C p. 2-17) lists the two parallel store-and-add
forms (`ST||ADD`, `ST||SUB`); the §ST||ADD diagram (p. 4-177,
`1100_00SD_XXXX_YYYY`) and the §ST||SUB diagram (p. 4-185,
`1100_01SD_XXXX_YYYY`) pin the rows at 0xC000/0xC400 under
mask 0xFC00 (S src-select bit 9, D dst-select bit 8, FIRS
dualxy 7:0). Table 2-28 carries no parallel entry, so
repeatability is ELIGIBLE by the absence precedent. The
§ST||ADD example (ST A,*AR3 || ADD *AR5+0%,B) and the
§ST||SUB example (ST A,*AR3- || SUB *AR5+0%,B) pin the
semantics: the ADD/SUB half computes dst_ +/- SXM(X)<<16
into the D-selected accumulator with carry/borrow-driven C
and OVM saturation (dual-arithmetic precedent), while the
ST half stores the STH-by-ASM value (S11 leaf shape:
40-bit shift by signed ASM, bits 31-16) of src into Ymem.
The §ST||ADD example prints A-after as FF 8021 1000
against an A-before of FF 8421 1000; the §ST||SUB,
§ST||MAC[R], §ST||MAS[R], and §ST||MPY examples all show
src unchanged, and the src==dst clause only orders the
store before the overwrite, so the ADD A-after is recorded
as a one-nibble TI typo with src preserved.

The current dual roots cannot host the ST half: the READ
root reads both sides and the MOVE root carries the X
observation with no completion. The slice therefore lands
a store-compute child root mirroring the dual-move root
(X READ leg, then Y WRITE leg with the leaf-frozen ST
value, then the single-read completion on the X word with
the accumulator effect, then shared AR/PC commit; reads
before writes, so an X/Y alias computes from the old
value). The root reuses the MOVE_PAIR spec kind and the
dual-move resolver with a disjoint predicate (WRITE plus
non-null `complete`; no effect-struct change, the
completion union already carries it), plus the shared AR
publisher. The leaf stages the frozen ST value via the
exported STH-ASM shift helper (proven identical on all
seven parallel-ST vectors) and the ADD/SUB half mirrors
the dual-arithmetic carry/borrow/OVM core. New
`SRC_DEST_DUALXY` layout ([S(1), D(1), DUALXY(8)]) and one
ASM-bearing input token (no new resource bit). T2-24-01/02
bind COMPLETE/ALL; the T2-25 MAC/MAS/MPY rows follow in
P3-S116 over this seam.
