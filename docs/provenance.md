# Provenance and evidence ledger

Status: legacy lineage pinned; distribution license and evidence-use review remain
open.

## Intent

`ti-c54x` is a newly structured standalone implementation. It is not a source split
or mechanical rewrite of an existing emulator core.

The Nokia DCT3 emulator contains a C54x interpreter imported from
`bbaranoff/qemu`, licensed `GPL-2.0-or-later`, followed by substantial project-local
changes. The imported base and its relationship to Osmocom are now pinned below.

## Legacy interpreter lineage

The first local import is emulator commit
`404ee94c410544e660e44b3be88ae10295a411d5`, dated 2026-06-06, which added
`spike/dsp54/calypso_c54x.c` and `.h` under the commit subject “recovered 5110/MAD1
C54x DSP image runs + mailbox bridge works.” Its Git blob IDs are:

- C source: `11ad34bf222ff53752bd6d4aeefdf2fe82fe8b80`
- header: `9d86da5cdffac9b38d0b2249975e2fcd392773c6`

The exact upstream base is `bbaranoff/qemu` commit
[`94c7def317ae6aac6d40aeda60acfa60bccf6c0d`](https://github.com/bbaranoff/qemu/commit/94c7def317ae6aac6d40aeda60acfa60bccf6c0d),
dated 2026-06-02. A complete comparison found the imported header byte-identical;
the imported C source differs only by the initial Nokia port-hook globals, PORTR and
PORTW handlers, and low-port routing additions. The upstream file is available at
the [pinned source path](https://github.com/bbaranoff/qemu/blob/94c7def317ae6aac6d40aeda60acfa60bccf6c0d/hw/arm/calypso/calypso_c54x.c).

Local commit `b7c1324a1b3fafb2f233351cc48f48366fcef25e` copied the implementation into
`third_party/c54x/`. Commit
`0f42d1e190c99a0629a5d65333d3b23e9176724a` later removed the duplicate and made
that directory canonical. Later emulator commits are project modifications and must
be treated as part of the legacy GPL implementation when used or redistributed.

The bbaranoff repository begins at root commit
[`0771c20fe9bd6874e49c04f3b04b3d99c11a8eb7`](https://github.com/bbaranoff/qemu/commit/0771c20fe9bd6874e49c04f3b04b3d99c11a8eb7)
with no Git parent. Its README describes applying the tree to QEMU 9.2.4, but Git
ancestry does not prove an upstream QEMU base. The C54x file is a bbaranoff addition,
not an upstream QEMU C54x interpreter. Its file-level SPDX identifier is
`GPL-2.0-or-later`; the repository also carries [QEMU licensing
notices](https://github.com/bbaranoff/qemu/blob/main/COPYING).

## Osmocom relationship

No inspected history indicates that Osmocom authored the C54x interpreter. OsmocomBB
provides the target firmware environment, DSP mailbox/API structures, and a
firmware-side DSP upload/communication driver:

- [Osmocom DSP API header](https://gitea.osmocom.org/phone-side/osmocom-bb/src/branch/master/src/target/firmware/include/calypso/dsp_api.h)
- [Osmocom firmware DSP driver](https://gitea.osmocom.org/phone-side/osmocom-bb/src/branch/master/src/target/firmware/calypso/dsp.c)

Those are platform/firmware inputs, not evidence that the interpreter originated in
Osmocom. Their layouts, source, and binaries have their own provenance and licensing
requirements and are not copied into `ti-c54x`.

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

- Select the repository license with the owner after the lineage review.
- Define the supported first silicon variant and the exact TI document set.
- Decide whether independent implementation/provenance review needs separate
  specification and coding roles.
- Review permitted use of TI manual material, DSP images, GNU binutils metadata, and
  any generated test-vector corpus before distribution.

Until these are resolved, this repository is not ready for public distribution or
emulator integration.
