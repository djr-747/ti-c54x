### P3-S90: MCU wake seam slice (implemented; local verification green)

The `0xF6A` IDLE1 park cannot be exited by doorbell.
S89 showed the stop holds INTM=1 with IFR=0; the
on-chip vector table (`0xFF80-0xFFE7`) decodes with
the repo catalog as `RETF`+NOP stubs in every slot
but two: `0xFFE0` branches to `0x3598` (the
INT2 host-command handler) and `0xFFE4`
(vector 25) to `0x3772` (doorbell), and both
targets open with genuine prologues (`CALL`+`PSHM`
saves at `0x3598`, `PSHM` saves at `0x3772`).
A maskable take is therefore real code but
unreachable: INTM=1 blocks it, and an IDLE wake
without take resumes the `B 0x0F6A` loop. The only
control-flow-changing wake is warm reset (profile
`nokia-ti-lead-rom4`: post-upload releases are
warm boots — register reset re-applied, DARAM
`[0x80,0x2800)` preserved; S71 stages 4-5).

Scope: one hosted-runner second phase,
`--warm-boot <budget2>` (trailing literal-flag
form, strippable anywhere like `--port-peer`).
Phase 2 de-inits and re-inits the CPU with the
fixed silicon entry `0xFF80` while the entire bus
survives: program image, DATA window (DSP and
peer writes intact), both peers (the MCU does not
re-upload; sticky port lines hold), the program
overlay, and coverage. Each phase prints the
established stop block; a `warm_boot phase=2`
marker separates them; cumulative bus counters
continue while the first-fault latches reset per
phase. No core, MMR, knob, or timing changes:
warm reset differs from cold init only in
host-owned memory preservation, and the core's
documented reset subset seeds both. The
cold/warm register-equivalence assumption and the
open PMST/IMR/timer gaps stay recorded.

RED contracts: runner `--selftest` vector 3
(two phases over one wired bus: phase 1
`ST #0xA5,*(0x800)` retires; after re-init phase 2
`ST #0x5A,*(0x801)` retires; the vector asserts
both cells hold — preservation plus rerun).
Acceptance is the canonical park repro stopping
`IDLE` in phase 1 with the phase-2 frontier
recorded in the ledger.

Out of scope: MCU stimulus between phases
(driving ports/doorbell/go-command, S91),
interrupt arrival/take machinery, `RETE`,
faithful power modeling, timing.

Canonical park repro (S88's invocation was never
recorded; this one is byte-exact for the
implementer):
`ti_c54x_run_image /tmp/dsp_cold_loader1.bin 0
0xFF80 1000000 be /tmp/ram_big.bin 0x0
--mcu-upload probe-images/nokia-5110-v530a.fls 64
--port-peer` stops `IDLE` at `pc=0xF6A`
(`attempted=394728 retired=394727`,
`io_calls=4`, zero faults, full 64-block pump).
The staged program image (loader1 overlaid at
`0xF00` per the ledger recipe) and the zeroed
`0xC000`-word DATA window are gitignored
`/tmp` scratch, not deliverables.

Findings (GREEN): phase 1 stops `IDLE` at
`pc=0xF6A` byte-identical to the no-flag run
(`attempted=394728 retired=394727`,
`io_calls=4`, zero faults, full 64-block pump,
`irq intm=1 ifr=0x0000`). After the
`warm_boot phase=2 entry=0xFF80` marker, phase 2
runs the full budget (`attempted=1000000
retired=1000000`) and spins at `pc=0xF17`
(the MBOX0 poll) with zero faults: memory
preserved (phase-1 uploads intact, peer holds at
64/64, overlay grows `writes=1` to `2` on the
rerun MVDP), per-phase fault latches reset
(`first_io=none`), coverage unions. A
`--dump-prog 0xFFE0 8` after phase 2 confirms
the runtime vector slots live (`0xFFE0:
0xF273/0x3598`, `0xFFE4: 0xF273/0x3772`).
Warm reset alone re-waits rather than advances:
phase 2 needs what the MCU has not yet staged,
so S91 is peer stages-4-5 behavior (rehome
upload, doorbell, go-command) to unblock the
`0xF17` MBOX wait.

Validation evidence: `make test` exit 0 (new
`--selftest` vector 3 RED-first green: phase-1
`ST` survives re-init, phase-2 `ST` proves
rerun; registries and checkers PASS); fresh
CMake Debug `ctest` 756/756.
