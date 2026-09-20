### P3-S86: PORTW port-peer seam slice (implemented; local verification green)

P3-S85 stages `PORTW *(0x0008),0` at `0xF57` but the
hosted runner owns no port peer: IO-space legs
fault at the data endpoint (`first_data_fault=
0x0000`, `io_calls=0`), so boot stops BUS_FAULT on
the first hardware-port access while the CPU test
proves retirement. The core never calls the bus
`io_read_word`/`io_write_word` callbacks — all IO
traffic rides the data endpoint — so the seam is
hosted-only: a port peer under `tools/hosted`
(MCU-peer precedent, P3-S71) serving IO-space
requests at the runner endpoint. No library change;
no MMR, knob, or timing changes.

Peer design: fixed 32-entry write journal (port
plus value, counts, overflow flag; no allocation,
no I/O, caller-owned lifetimes). Reads are sticky:
the last value the DSP wrote to that port, else
`0x0000` (zeroed-window philosophy — staged
simplification, documented, never hardware truth).
Runner form: optional trailing `--port-peer`
stripped before the existing argc dispatch, so the
6/8/9/11 forms are untouched; detached behavior is
byte-identical (IO still faults, `io_calls=0`).
Attached endpoint IO is served, journaled, and
counted in `io_calls`/`first_io`; a `port_peer`
journal line joins the stop report beside the MCU
fills line.

RED contracts: `tests/unit/hosted/test_port_peer.c`
(peer init, write journal order, sticky read,
unknown-port zero, journal overflow flag, null
rejection) plus the existing selftest untouched.
Acceptance is the hosted repro retiring the
`0xF57` PORTW with the peer attached and recording
the new frontier past `0xF5A` (whose `0xEC0E`
already retires standalone).

Out of scope: a faithful hardware port model
(oracle seam stays unresolved), PORTR-specific
semantics, library or timing changes.

Implementation record (GREEN): `tools/hosted/
port_peer.c` journals writes (32 entries, counts,
overflow flag) and serves sticky reads; the
runner strips one optional trailing `--port-peer`
before the 6/8/9/11 dispatch, serves attached
endpoint IO through the peer (counted in
`io_calls`/`first_io`), and prints a `port_peer`
journal report. Detached runs are byte-identical
(IO still faults, `io_calls=0`); the bus
`io_read/io_write` callbacks stay UNMAPPED — the
core never calls them.

Validation evidence: `make test` exit 0 (new
`test_port_peer` RED-first green; selftest PASS).
Targeted `ENTRY=0xF57` run with the peer retires
5/5 to pc=`0xF5B` (`port_write port=0x0000
value=0x0000`, zeroed-window DATA). BUDGET=
1000000 widened-window run with the 64-block MCU
peer plus `--port-peer` retires 394718 with zero
data faults past three served PORTWs (ports
`0x0000`, `0x000C`, `0x002C`) to the new frontier
`INVALID_STATE` at pc=`0xF97`, opcode `0x74F8`
(PORTR family — next library slice).
