### P3-S18: STH MOD0-MOD3 second-word disambiguation seam (implemented; local verification green)

This immutable heading is the durable S18 FIT/ownership
anchor (status parenthetical advanced at exit review; title text unchanged). S18 is a sealed decode seam with no handlers of its own: STH
syntax-4 single-indirect words with MOD0–MOD3 address modes carry first
words inside the `LD_SMEM_SHIFT` slot (`0x6F80` under mask `0xFFE0`,
`src/isa/catalog/opcode_catalog.def`), which the first-word-only lookup
(`ti_c54x_decode_first_word`, table
`generated/isa/catalog/opcode_first_word.inc`) cannot discriminate. The
second words are provably disjoint under mask `0xFEE0`: LD requires
`(w2 & 0xFEE0) == 0x0C40` (checked in
`ti_c54x_trusted_op_ld_smem_shift`, `src/isa/ops/load/op_ld_smem.c`),
while STH syntax-4 carries the `0x0C60` fixed pattern (bits 15–9 carry
`0x06`, bits 7–5 fixed `011`; S13 record
`E-TI-C54X-SPRU172C-S4-STH-SHIFT-SEMANTICS-P4-168-4-171`). The two
patterns differ in bit 5, so no TI input is needed to separate them.

Scope: extend the catalog generator
(`tools/isa/catalog/generate_opcode_lookup.py`) and the decode contract
so a first-word slot can qualify rows on the second program word; add
the four `STH_SHIFT_SMEM_MOD0/MOD1/MOD2/MOD3` rows reusing the
`ACCUMULATOR_ARF_MOD4_SHIFT5` layout and the landed S13 checked/trusted
handlers in `src/isa/ops/store/op_sth_shift_smem.c`; keep every existing
`LD_SMEM_SHIFT` decode bit-identical. S18 owns no memory effects, no
handlers, no CPU branches, and no timing.

RED lanes: generator unit coverage for a word-2-qualified slot;
decode lookup coverage proving `0x6F80/FFE0` resolves LD for `0x0C40`
words and STH for `0x0C60` words with all other second words declined;
LD Smem,SHIFT regression proving existing vectors unchanged; STH
MOD0–MOD3 execution coverage through the shared MEMORY effect with
exotic-mode deferral preserved. Exit: FORM-T2-20-08 advances past
SUBSET with four bound symbols, and the S13 deferral note is retired.
Non-goals: far/XPC, timing, any other overlapping family.
