### P3-S7: STM immediate-MMR-store vertical slice (planned)

This immutable `(planned)` heading is the durable S7 FIT/ownership
anchor. S7 composes the first family on the S1 seam: TI SPRU172C
§4 `STM #lk, MMR` (printed pp. 4-175–4-176) stores the second
program word into the MMR file or DP0 with the nine MSBs of the
effective address cleared regardless of `DP`. Word 0 is
`0111_011I_AAAAAAAA` (one row `STM` at `0x7700` under mask
`0xFF00`, 256 encodings, two program words): `I=0` selects the
7-bit direct form (`0x00`–`0x1F` route through S1, `0x20`–`0x7F`
reach DP0 endpoint RAM) while `I=1` selects the indirect form
and rejects as deferred at the op leaf, mirroring the
unevidenced-condition precedent. Status bits are none;
repeatability stays `UNRESOLVED` (`STM` is absent from Table
2-28); inputs are none. The new `DIRECT` memory address kind
carries the 7-bit address in an appended `direct_dma7` byte
(the ABI asserts guard the unchanged sizes) and resolves
through a new DP-independent page-0 planner (the `DIRECT_DP`
planner wrongly requires live `DP==dp9`), with a new
`DIRECT7_EXTENSION16` layout (`I` flag, `addr7`, `lk16`).
`FORM-T2-20-14` lands `SUBSET` with the `STM` symbol (the
deferred indirect form keeps it from `COMPLETE`).
The four RED executable contracts are
`tests/unit/isa/catalog/test_stm_metadata.c`
(CTest `ti_c54x.stm_metadata`),
`tests/unit/isa/decode/test_stm_decode.c`
(CTest `ti_c54x.stm_decode`),
`tests/unit/isa/ops/store/test_stm.c`
(CTest `ti_c54x.stm_ops`), and
`tests/unit/cpu/isa/store/test_cpu_stm.c`
(CTest `ti_c54x.cpu_stm`), plus the address-layer planner
lane and the `SUBSET` inventory binding. Realized closure
is 163 catalog rows, `ID_COUNT=163`, 7,620 lookup entries,
family coverage 62/205 (30.244%), and 33/205 strict
complete-form coverage; 339 evidence records, 527
requirements, and 418 tests (418 `PASSING`, 0 `PLANNED`).
Landing also added the page-0 `DIRECT` liveness arm to
candidate validation with a CMPT-independence CPU proof,
and re-pinned the two `ST #lk,Smem` near-miss flips
(`0x0100`) that now resolve into the `STM` row.
