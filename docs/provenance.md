# Provenance and evidence ledger

Status: investigation in progress; distribution license not yet selected.

## Intent

`ti-c54x` is a newly structured standalone implementation. It is not a source split
or mechanical rewrite of an existing emulator core.

The Nokia DCT3 emulator currently contains a C54x implementation whose local README
attributes it to `bbaranoff/qemu`, licensed GPL-2.0-or-later, with substantial later
changes. The relationship of that work to Osmocom sources still needs to be pinned
from repository history and upstream records.

## Evidence policy

Behavioral requirements should be derived from:

1. identified TI architecture and silicon documentation;
2. reproducible physical-hardware test results where available;
3. independently constructed assembly/disassembly vectors; and
4. normalized black-box traces from other implementations, used only to find
   disagreements that are then resolved against stronger evidence.

Do not copy opcode maps, implementation code, comments, names, tests, or data tables
from the legacy core or GNU binutils into this repository.

Every future opcode and architectural rule must carry an evidence identifier that
maps to a document section, hardware-vector record, or explicitly unresolved issue.

## Open decisions

- Confirm the full lineage of the emulator's C54x source, including any Osmocom
  relationship.
- Select the repository license with the owner after the lineage review.
- Define the supported first silicon variant and the exact TI document set.
- Decide whether independent implementation/provenance review needs separate
  specification and coding roles.

Until these are resolved, this repository is not ready for public distribution or
emulator integration.
