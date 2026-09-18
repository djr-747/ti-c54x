# Chip profile: nokia-ti-lead-rom4 (sourced, unratified)

Status: **sourced** — map facts below were paraphrased from a
read-only review of the sibling legacy core integration (pointers
kept, no source/opcode tables/comments/structure copied). They are
harness *requirements*, not redistributable claims: treating them
as device truth still waits on the open P0-W4 owner/legal
follow-up. Identity: Nokia-customized TMS320C54x ('542-class,
single 64K program page, no XPC paging); 5110 (NSE-1, firmware
v5.30 A) is a ROM-4 reference alongside 6110/3210/7110.

## Program space (word addresses)

- Core sizes: 256K program words, 64K data words, 64K I/O words.
- Image format: raw big-endian 16-bit words, no header; word `i`
  loads at program address `i` (128 KB loader cap in the legacy
  integration). Runner flags: `ORDER=be`, `PROG_BASE=0`.
- On-chip ROM image covers roughly `0x9000-0xFF7F`; interrupt
  vectors live at `0xFF80-0xFFFF`.
- Reset: silicon forces PC to `0xFF80` (vector-table base); the
  effective reset target is `0x0F00` via that vector. Alternate
  cold entry `0x3000` (resident PROM cold-init) is selectable.
- Vectors the platform uses: reset `0xFF80`; INT1 `0xFFC4`; INT2
  `0xFFC8` (host-command handler near `0x3598`); timer TINT
  vector 19 at `0xFFCC` (DARAM dispatcher near `0x241A`);
  doorbell vector 25.
- No flash-on-DSP-bus: the DSP executes its on-chip image plus
  MCU-uploaded DARAM blocks; the MCU stages block uploads.
- DSP does not run until the MCU release edge; loader staging
  happens at that edge. Post-upload releases are warm boots
  (re-apply register reset, preserve DARAM `[0x80,0x2800)`).

## Data space (word addresses)

- MMR region `0x00-0x1F` (CPU: IMR `0x00`, IFR `0x01`, ST0/ST1,
  accumulators, T/TRN, AR0-7, SP/BK/BRC/RSA/REA, PMST `0x1D`,
  XPC `0x1E`); peripheral MMRs `0x20-0x5F` (timer `0x24-0x26`,
  BSCR `0x29`, HPIC `0x2C`, serial/codec `0x20-0x23`).
- Writable DATA window for smoke runs: `[0x800,0x2800)` (OVLY/HPI
  window; canonical host-visible 2K slice `0x800-0xFFF`).
  Runner flags: `DATA_BASE=0x800` with a caller-supplied RAM file
  (or none — unmapped DATA then faults explicitly).
- Sourcing audit 2026-09-19: the MMR/peripheral addresses above
  are pinned in NEITHER the vendored ISA text (SPRU172C carries
  no device memory map) NOR the core (its MMR file models CPU
  registers only; no timer). They stay legacy-sourced in full
  until device-datasheet evidence or the P0-W4 decision lands.
  Consequence for smoke runs: any access into `0x20-0x5F`
  currently resolves through the core's own MMR/device seams
  where they exist and faults explicitly elsewhere — the
  report's `first_data_fault` names exactly which address needs
  a map fact next.
- Data-ROM supplement (separate text file of address/value hex
  lines) loads at data `0xB000+`.
- Overlay run addresses uploaded by the MCU (`0x926`, `0xD00`,
  `0xD80`, `0xF00`) execute in place under OVLY.

## I/O space

- DSP port operand range below `0x100`. Runner faults all I/O
  today and logs the first/ongoing port traffic.
- Command surface: DSP port 1 reads the MCU command word, port 2
  the mask/ack word.
- Codec samples: port `0x27` data with port-`0x21` control, port
  `0x21` the sample source.
- Frame clock: port `0x0E` period, `0x0F` event scripts, `0x10`
  live readback; ports `0x00/0x03/0x04` latched handshakes.

## Reset state a faithful run must replicate

PMST=`0xFFA8`, PC=`0xFF80`, INTM set, IFR=0, SP=`0x1100`,
ST0=`0x181F`, ST1=`0x2900`, IMR mask family, timer free-running
from `TIM=PRD=0xFFFF`. The standalone core seeds its own
documented reset subset today; full replication is a harness
plus core-reset-coverage gap (SP is seedable in tests; PMST/IMR
timer paths are not yet drivable inputs).

TI-pinned subset (SPRU172C p. 4-138, RESET instruction): reset
loads `PC = IPTR<<7` and forces C=1, TC=1, SXM=1, XF=1, INTM=1
with OVA/OVB/ARP/DP/ASM/BRAF/HM/C16/FRCT/CMPT/CPL/OVM/IFR=0.
`RESET` itself is opcode `0xF7E0`, one word, Class 35, and
explicitly not repeatable (a future RESET slice takes
`INELIGIBLE` with evidence in hand). INTM-set and IFR=0 match
the legacy claims above, so those two graduate from
legacy-sourced to TI-pinned. Nuance: TI notes hardware-RS
initialization of IPTR/peripherals differs from software-RESET
init, which is why the legacy XF-low claim (hardware reset edge)
and the TI XF=1 claim (RESET instruction) can both be true.
Vector addresses (`0xFF80` et al.) are device-specific and stay
legacy-sourced.

## Canonical invocation (once the image path is supplied)

```sh
make run-image IMAGE=<prog.bin> PROG_BASE=0 ENTRY=0xFF80 \
  BUDGET=<n> ORDER=be DATA_BASE=0x800
```

Known gaps before a real ROM run means anything: block repeat
(`RPTB`/`RPTBD`), `MAR`, `POPM`/`PSHM`, `MVxx` moves, `TRAP`
vectors, timer-tick delivery, and the P0-W4 permitted-use
decision. A run that stops with `UNIMPLEMENTED_OPCODE` names the
next form; a `BUS_FAULT` on a plausible vector names the next
map fact to check.

Coverage limit (verified 2026-09-19): from a cold reset every
DATA/IO-touching leaf rejects on unknown address inputs (DP/ARs)
before reaching a bus, so the report's `data_txn`, `data_faults`,
`first_data_fault`, `io_calls`, and `first_io` fields are live but
currently unreachable — only `prog_reads`, `prog_unmapped`, and
`prog_covered` can be nonzero. Reaching them needs harness-side
register seeding (no public setters exist yet) or broader reset
coverage, tracked as a harness gap, not a report bug.
