### P2-W35: fixed-MOD2 LD-to-T vertical slice (planned)

This implemented evidence-fit slice is bounded by the TI `SPRU172C` §4.70 LD
entry (printed pp. 4-70–4-71), the registered standard-indirect MOD2 rule in
`SPRU131G` §5.5.3 (printed p. 5-13), and the registered Table 2-28
repeatability boundary (printed pp. 2-19–2-21). The one-word `LD Smem,T` form
uses opcode diagram `0011_0000_IAAAAAAA`; after fixed-MOD2 composition its row
is value `0x3090`, mask `0xFFF8`, with bit 8 fixed zero, ARF bits 2:0, and
exactly eight encodings. It requires a private generic
`TI_C54X_OPERAND_LAYOUT_ARF` decode layout and no input resources. Repeatability
is `UNRESOLVED`: absence from Table 2-28's forbidden list is not positive
eligibility evidence, and the separately forbidden fixed long-offset or
absolute forms are excluded.

The project fit reuses the W31B fixed-MOD2 MEMORY effect/completion and the
existing `T_REGISTER_WRITE` effect. Under CMPT=0 and ARP=0, the old selected AR
WORD16 is transferred into T before its postincrement; the checked planner's
AR=0xffff boundary remains in force. The `*AR3+` example transfers data
`0xFEDC` from the old AR3 address `0x0300`, then advances AR3 to `0x0301`. No
public T/MMR API, new resource,
address kind, CPU opcode branch, timing, IRQ/RPT, device, environment,
diagnostic, or allocation behavior is claimed.

TDD ownership is split across catalog, decode, pure load MEMORY completion, and
generic CPU retirement tests under mirrored paths; all four executable contracts
are passing. `src/isa/decode/` owns the new private layout contract; no family
workaround may hide a shared seam. The current `operand_layout.c` is 518 lines
and remains cohesive, so the reviewed no-split decision keeps it well below the
800-line threshold. Realized coverage is 29 rows, 1439 populated first words,
and 6/29 reset-runnable. Repeatability remains unresolved. W31P hosted stability
and W31B immediate-READY benchmark follow-ups remain open and nonblocking.
