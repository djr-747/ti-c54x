### P3-S68: CLKMD peripheral-MMR slice (implemented; local verification green)

`CLKMD` at data `0x58` is the clock-mode register (TI
SPRU131 §8 clock-generator module: Figure 8-4 diagram,
Tables 8-18/8-19, all vendored in-repo; the RE loader1
symbol names the address `clkmd`). Bits 15:1 are R/W PLL
configuration, bit 0 (PLLSTATUS) is R status. The
faithful loader1 cold run retires through `STM #0x2000`
to SWWSR, then stops `BUS_FAULT` on `STM #0x3146,0x58`
at `0xF07` (`first_data_fault=0x0058`): no peripheral
window exists past `0x28`. The written value enables the
PLL (PLLMUL=3, PLLDIV=0, PLLCOUNT=0x28, PLLON=1,
PLLNDIV=1) with the status bit 0.

TI reset values are pin-dependent (Table 8-16; the C542
table marks `0x58` reserved), so no reset value is
adopted: the cell resets UNKNOWN per standing
zero-plus-unknown policy, and loader1's write makes it
known. Bit 0 masks to zero on write (diagram R marking);
PLL lock dynamics that would raise it are out of scope,
as is all wait/clock timing function per the
machine-cycle rule.

Leaf design: second `src/devices/` cell beside SWWSR
(same struct shape and checked access, reset UNKNOWN,
writable mask `0xFFFE`), carried on `arch_state` and
reset with the common subset. The MMR router serves
`0x58` from that cell both directions; the planner arms
again need no changes (WRITE-unconditional,
READ-probe). No opcode, catalog, inventory, knob, or
timing changes.

RED contracts: `tests/unit/devices/test_clock_mode_register.c`
(reset-unknown, write/read round trip, bit-0 mask, NULL),
`tests/unit/bus/test_mmr_route.c` (`0x58` write then
read `0x3146`, reset read UNKNOWN, `0x57`/`0x59`
declined), and `tests/unit/cpu/isa/store/test_cpu_clkmd.c`
(`STM #0x3146` to `0x58` retires with zero endpoint
calls). Acceptance is the hosted repro advancing past
`0xF07` to the next explicit stop.

Out of scope: other peripheral MMRs, PLL lock/status
dynamics, clock timing function, reset-value adoption
without board-strapping evidence.
