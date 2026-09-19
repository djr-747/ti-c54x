### P3-S57: 5110 boot-probe first light (implemented; local verification green)

First hosted run of the recovered 5110 DSP image (`dsp_full.bin`,
the image DSP_COSIM uses, staged in `/tmp` only — no firmware
bytes enter the repo) through `tools/hosted/ti_c54x_run_image`
under the `nokia-ti-lead-rom4` profile. Two entry vectors,
instruction budget 10 and 50 respectively, `ORDER=be`:

- Reset `ENTRY=0xFF80`: `STM #0xFFA8,PMST` retires, `B 0xFF85`
  retires, `B 0x0F00` retires (attempted=4, retired=3,
  `prog_reads=7`, no unmapped reads). PC `0x0F00` holds the
  zero word where the MCU-uploaded overlay belongs; it decodes
  `ADD_SMEM` and rejects `INVALID_STATE` on unknown DP. This is
  correct cold-reset behavior, and it matches the profile's
  predicted coverage limit (DATA-touching leaves reject before
  any bus traffic). No DATA/IO transactions occur.
- PROM `ENTRY=0x3000`: `CALL_PMAD 0x23CC` attempts first and
  rejects `INVALID_STATE` on unknown SP (call push needs a known
  stack pointer). Same root cause, same correct behavior.

What this names next: (1) the MCU overlay upload map (which
sibling `re/dsp/block*.bin` lands at `0x926`/`0xD00`/`0xD80`/
`0xF00` — `block01` et al. are DATA uploads, not the `0xF00`
code); (2) harness-side DP/AR/SP seeding or broader core reset
coverage (profile gap, unchanged); (3) the `MVDM 0x0000,AR4` at
`0xF07` the S56 doc predicted appears only once the overlay is
staged. Stops are architectural, not implementation defects:
nothing in either report indicates a wrong retire.

Out of scope: overlay staging, register seeding, new ISA forms,
timing, device models.
