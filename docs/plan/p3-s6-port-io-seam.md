### P3-S6: port I/O seam (implemented; local verification green)

This immutable heading is the durable S6 FIT/ownership
anchor (status parenthetical advanced at exit review; title text unchanged).
Superseded FIT anchor text retained for the PINNED row locators:
P3-S6: port I/O seam (planned). S6 is a sealed architectural seam with no catalog rows, no
opcode identities, no handlers, and no CPU opcode branch, following
the S1/W43A pattern. TI SPRU172C Table 2-26 (p. 2-17) lists `PORTR
PA,Smem` and `PORTW Smem,PA` (2 words, 2 cycles, classes 27A/27B and
28A/28B); §4 pp. 4-129–4-130 give word 1 as `0xE8` (PORTR) /
`0xE9` (PORTW) with an I-bit plus 7-bit Smem cookie and word 2
carrying the 16-bit PA (0–65,535), with one word added for
long-offset indirect or absolute Smem. The same I-plus-7-bit Smem
cookie appears on `MVDK Smem,dmad` (§4 p. 4-108) with word 2 carrying
its 16-bit constant, corroborating the shape. Status bits: none. No
timing claim is admitted (wave-8 note), and repeatability stays
UNRESOLVED on the absent TI note.

Scope: `TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16 = 18` with a `port` arm
reusing the pair-arm shape (`indirect` select, `smem_mode`/`smem_arf`
vs `smem_dma7`, 16-bit `port_address`, reserved words; the 24-byte
address ABI stays pinned). The direct cookie is I=0 with 7-bit
direct dma (TI examples: `PORTR 05,INDAT`, `PORTW OUTDAT,5h`); the
indirect cookie is I=1 reusing the landed ARF-plus-4-bit-MOD
extension shape (MOD bits 6:3, ARF bits 2:0), proved by vectors in
the family lane. The `src/sequencer/memory/` resolver maps PORT
READ/WRITE to IO-space requests with PA bounds and transactional
reject; the generic commit root serves the `complete` chain
(PORTR: IO READ then DATA Smem WRITE; PORTW: DATA Smem READ then IO
WRITE) preserving atomic WAIT/READY/FAULT/cancel behavior. The
endpoint owns IO state; the seam adds no device model. No timing,
IRQ/RPT, wrap, machine-cycle, public-ABI, or new public CPU snapshot
claim is admitted. No knob is added.

Ownership: `REQ-PROJECT-P3-S6-PORT-RESOLVER`
(`src/sequencer/memory/`) and `REQ-PROJECT-P3-S6-PORT-TRANSACTION`
(`src/sequencer/instruction/`) carry the project contract over the
Table 2-26 and §4-129/130 plan evidence. The two RED lanes are
`tests/unit/sequencer/memory/test_port_address_resolution.c` (CTest
`ti_c54x.port_resolver`) and
`tests/unit/sequencer/instruction/test_port_memory_transaction.c`
(CTest `ti_c54x.port_transaction`). Exit: sealed seam with both lanes
green, existing DATA behavior preserved for all current kinds, and
the PORTR/PORTW direct family lane unblocked. Follow-ups: direct
rows plus handlers, indirect Smem rows, FORM-T2-26-09/10 registry
binding with TI §4-129/130 evidence records.
