### P3-S67: SWWSR peripheral-MMR slice (implemented; local verification green)

`SWWSR` at data `0x28` is the software wait-state register
(TI SPRU131 Tables 8-1/8-2 C542 map, Figure 10-2 diagram,
Table 10-2 bit summary, all vendored in-repo): bits 14:0
are R/W wait-state fields, bit 15 is reserved (R, reset 0),
and every field bit resets to 1, so the reset value is
`0x7FFF`. The faithful loader1 cold run (`dsp_full.bin` +
block25 at `0xF00`) retires through `STM #0x1EC6,SP` and
`STM #0xFFA8,PMST`, then stops `BUS_FAULT` on
`STM #0x2000,0x28` at `0xF05` (`first_data_fault=0x0028`):
the S1 MMR window ends at `0x1F`, so `0x28` declines to the
unmapped endpoint. On silicon `0x28` is the register, never
RAM, so the fix routes it to a device cell with precedence
over the DP0 endpoint.

Leaf design: new `src/devices/` module owning the
instance-owned SWWSR cell (storage plus knownness, reset
`0x7FFF` known, bit-15 writes masked to 0 per the R
diagram marking), carried on `arch_state` and reset with
the documented common subset. The MMR router serves DATA
`0x28` from that cell both directions; `is_routed_address`
names `0x28` explicitly with the `0x1F` window untouched,
so the planner's WRITE-unconditional and READ-probe arms
need no changes. No opcode, catalog, inventory, knob, or
timing changes: wait-state function stays out of scope per
the machine-cycle rule; the register is modeled as
architecturally-visible R/W state only.

RED contracts: `tests/unit/devices/test_software_wait_state_register.c`
(cell reset/read/write/bit-15 mask), `tests/unit/bus/test_mmr_route.c`
(`0x28` serviced both directions, reset value, still
declined neighbors), and a CPU round trip (`STM #0x2000`
to `0x28` retires with zero endpoint calls, `LDM 0x28,A`
reads back `0x2000`, fresh reset reads `0x7FFF`).
Acceptance is the hosted repro advancing past `0xF05` to
the next explicit stop.

Out of scope: other peripheral MMRs (`0x58` CLKMD is the
predicted next stop), wait-state timing function, RAM
behind `0x00-0x5F` (none on silicon), bit-15 write-then-read
silicon behavior beyond the reserved-0 model.
