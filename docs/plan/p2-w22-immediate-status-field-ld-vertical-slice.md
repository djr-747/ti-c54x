### P2-W22: immediate status-field LD vertical slice (implemented; local verification green)

P2-W22 is one atomic end-to-end ISA vertical slice, not a new foundation phase.
It batches three one-word immediate status-field forms:

- `LD #k9,DP`: value `0xEA00`, mask `0xFE00`, unsigned 9-bit field, `DP := k9`,
  no other status-bit side effects, and repeatability `INELIGIBLE`.
- `LD #k5,ASM`: value `0xED00`, mask `0xFFE0`, raw 5-bit field representing
  `-16..15`, `ASM := raw field`, no other status-bit side effects, and
  repeatability `UNRESOLVED`.
- `LD #k3,ARP`: value `0xF4A0`, mask `0xFFF8`, 3-bit field, `ARP := k3`, no
  other status-bit side effects, and repeatability `INELIGIBLE`.

TI SPRU172C Table 2-19 (printed pages 2-14–2-15) and LD §4.70 (printed pages
4-70–4-71) are the primary form/effect evidence. Table 2-28 (printed pages
2-19–2-21) supplies DP/ARP nonrepeatability; ASM repeatability remains
unresolved. Summary cycle columns are deliberately not adopted as timing
evidence. GNU binutils tag `c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e`,
`opcodes/tic54x-opc.c` lines 316–319, is a nonnormative corroboration only;
no implementation, table, comment, or code is copied or vendored, and any
mismatch escalates to TI evidence.

Ownership is deliberately minimal. `src/isa/catalog/` owns exact masked rows,
`src/isa/decode/` owns raw-field extraction, and `src/isa/ops/load/` owns the
three readable semantic handlers and their aggregate target effects;
`src/state/status/` owns only the status-register contract; `src/sequencer/status/`
owns transactional effect application; and `src/cpu/` observes and atomically
retires the effect. `ti_c54x_state` intentionally grows observation fields before
API stability; those fields are not a raw snapshot or persistence ABI and no
stable persistence guarantee is made. The mirrored tests are
`tests/unit/isa/decode/test_ld_status_fields.c`,
`tests/unit/isa/ops/load/test_ld_status_fields.c`,
`tests/unit/state/status/test_status_registers.c`,
`tests/unit/sequencer/status/test_status_field_effect_apply.c`, and
`tests/unit/cpu/test_cpu_ld_status_fields.c`. Local C_EXECUTABLE runners are
`ti_c54x.ld_status_fields_decode`, `ti_c54x.ld_status_fields_ops`,
`ti_c54x.status_registers`, `ti_c54x.status_field_effect_apply`, and
`ti_c54x.cpu_ld_status_fields`; the full local and remote gates are green.
No memory/bus transaction, timing, RPT/IRQ, accumulator, device, or emulator work
is part of this slice; any status-bank split and generic effect commit exist only
because these opcodes require them. The local registry is 91 immutable evidence
records, 133 requirements (107 `IMPLEMENTED`, 9 `PLANNED`, 17 `UNRESOLVED`), and
96 tests (87 `PASSING`, 9 `PLANNED`); the full local repository gate is green and
remote P2-W24 validation is green, with the latest clean hosted performance
record captured at `7cad341`.
