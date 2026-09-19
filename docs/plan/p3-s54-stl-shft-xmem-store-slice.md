### P3-S54: STL SHFT Xmem store slice (implemented; local verification green)

This immutable `(planned)` heading is the durable S54 FIT/ownership
anchor. S54 closes the Table 2-20 STL Xmem gap (`FORM-T2-20-12`,
the only unbound STL form) as an exact S12 (`STH_SHFT_XMEM`,
`0x9A00/0xFE00`) mirror: one catalog row `STL_SHFT_XMEM`
(`0x9800/0xFE00`, TI SPRU172C §STL syntax 3 opcode `1001_10_0S_XXXX_S_H_F_T`,
pp. 4-171–4-172) over the shared
`ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4` layout, wiring new plain plus
trusted `op_stl_shft_xmem` handlers through the catalog X-macro
into dispatch. Semantics: `Xmem = low 16 bits of (src << SHFT)`;
the stored field is pure source bits, so SXM rides hostile and
unconsumed exactly as in S12. No handler, layout, decode, or CPU
change beyond the new leaf; the generated first-word lookup
absorbs 512 `UNKNOWN` words with zero neighbor moves.
`FORM-T2-20-12` binds at `COMPLETE` under `ALL` (the
`STH_SHFT_XMEM` precedent; the landed leaf covers all four XMOD
modes). Landed closure adds three evidence records (two TI plus
the activation), two requirements, and two test records on the new
op plus binding suites: 491 evidence records, 801 requirements,
and 654 tests (654 `PASSING`, 0 `PLANNED`). Per the S32 precedent
there is no per-slice PROJECT inventory requirement; the binding
is covered by the shared inventory-catalog-binding sweep.
Non-goals: far/XPC, timing, any other STL
syntax, READA/WRITA, the T2-05 doubles, and parallel forms.
