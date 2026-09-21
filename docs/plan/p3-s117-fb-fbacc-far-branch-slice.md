### P3-S117: FB/FBACC far-branch rows with XPC commit (implemented; local verification green)

Table 2-12 (TI SPRU172C p. 2-10) far-branch forms `FB[D]`,
`FBACC[D]` land as four catalog rows following the B/BD
separate-row precedent (delayed variants are own rows sharing
the family form with NONDELAYED/DELAYED addressing subsets):
`FB_PMAD` (2 words; first word 1111_1000_1PPP_PPPP, value
0xF880 under mask 0xFF80 with page PPPPPPP at bits 6:0;
extension word pmad(15-0)), `FBD` (value 0xFA80 under mask
0xFF80), `FBACC` (1 word, 1111_01ZS_1110_0110, value 0xF4E6
under mask 0xFEFF with Z bit 9 pinned per row and S bit 8 src
select riding inside), `FBACCD` (value 0xF6E6 under mask
0xFEFF). New 2-word
FAR_PMAD layout [PAGE(7)@w0[6:0], PMAD(16)@w1]; FBACC reuses
ACCUMULATOR_SOURCE. All rows INELIGIBLE by Table 2-28
membership (FB[D], FBACC[D] listed); RPTB exclusion rides the
existing ineligibility gate. New FAR_PC_WRITE /
DELAYED_FAR_PC_WRITE effects carry pmad(15-0) plus page
pmad(22-16); the sequencer target planner and CPU route commit
XPC through the S113 write accessor. No status bits, no stack,
src preserved. TI vectors: FB 012000h (PC 1000 to 2000, XPC 00
to 01) and FBD 7F1000h with the ANDM delay slot (PC 2000 to
1000, XPC 00 to 7F); FBACC A with A 00 0001 3000 (PC 1000 to
3000, XPC 00 to 01) and FBACCD B with the ANDM delay slot
(B 00 007F 2000, XPC 01 to 7F). T2-12-05 binds FB/FBD,
06 binds FBACC/FBACCD, all COMPLETE with NONDELAYED/DELAYED
subsets. First-word overlap note: FB (0xF880-0xF8FF) and FBD
(0xFA80-0xFAFF) sit inside the settled BC/BCD free-cond-byte
patterns (masks 0xFF00). Every TI condition code has bit 7
clear and OR-combination preserves that, so no valid BC/BCD
encoding sets bit 7; the slice tightens BC/BCD first-word
masks to 0xFF80 (128 valid conds each) and the far rows claim
the bit-7 halves. The BC/BCD 256-cond sweeps narrow to
0x00-0x7F with the 0x80-0xFF halves pinned as FB/FBD
siblings; CC/CCD keep their masks until S118 claims their
bit-7 halves for FCALL/FCALLD.
