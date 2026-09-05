# Evidence index

The canonical machine-readable registry is
[`config/evidence.json`](../config/evidence.json). This index mirrors every
source, evidence, requirement, and test ID in that file for review. The primary
target is `C54X_CORE_COMMON_V1`; the only other target is the planned
`C54X_VC5402_PROFILE_V1`.

## Targets

| Target ID | Kind | State | Parent | Allowed claim kinds | Boundary |
|---|---|---|---|---|---|
| `C54X_CORE_COMMON_V1` | COMMON | ACTIVE | — | ARCHITECTURAL, PROJECT_CONTRACT | Device-neutral common CPU foundations |
| `C54X_VC5402_PROFILE_V1` | PROFILE | PLANNED | `C54X_CORE_COMMON_V1` | PROFILE | VC5402 maps, vectors, peripherals, and extensions later |

XPC, extended pages, device maps, vectors, peripherals, and machine-cycle timing
are exclusions of the common target, not hidden project targets. The common
program-address carrier is 32-bit with semantic scope `CARRIER_ONLY`; its
scalar typedef is owned by `include/ti_c54x/address.h`.

## Sources

All TI links below are official Texas Instruments documents. ISO dates, revision
dates, normative scope, and access date are authoritative in the JSON registry.

| Source ID | State | Kind | Normative scope | Applicable target IDs | Literature / revision | Official source |
|---|---|---|---|---|---|---|
| `SRC-TI-SPRU131G` | ACTIVE | TI_PRIMARY | COMMON_ALLOWED | `C54X_CORE_COMMON_V1` | SPRU131G / G, 2001-03 | [Volume 1: CPU and Peripherals](https://www.ti.com/lit/ug/spru131g/spru131g.pdf) |
| `SRC-TI-SPRU172C` | ACTIVE | TI_PRIMARY | COMMON_ALLOWED | `C54X_CORE_COMMON_V1` | SPRU172C / C, 2001-03 | [Volume 2: Mnemonic Instruction Set](https://www.ti.com/lit/ug/spru172c/spru172c.pdf) |
| `SRC-TI-SPRU179C` | ACTIVE | TI_PRIMARY | COMMON_ALLOWED | `C54X_CORE_COMMON_V1` | SPRU179C / C, 2001-03 | [Volume 3: Algebraic Instruction Set](https://www.ti.com/lit/ug/spru179/spru179.pdf) |
| `SRC-TI-SPRU307A` | ACTIVE | TI_PRIMARY | COMMON_ALLOWED | `C54X_CORE_COMMON_V1` | SPRU307A / A, 1998-09; revised 2000-05 | [Functional Overview](https://www.ti.com/lit/ug/spru307/spru307.pdf) |
| `SRC-TI-SPRS079G` | PLANNED | TI_PRIMARY | PROFILE_ONLY | `C54X_VC5402_PROFILE_V1` | SPRS079G / G, 1998-10; revised 2008-10 | [TMS320VC5402 datasheet](https://www.ti.com/lit/ds/symlink/tms320vc5402.pdf) |
| `SRC-TI-SPRU302B` | PLANNED | TI_PRIMARY | PROFILE_ONLY | `C54X_VC5402_PROFILE_V1` | SPRU302B / B, 2007-03 | [Volume 5: Enhanced Peripherals](https://www.ti.com/lit/ug/spru302/spru302.pdf) |
| `SRC-PROJECT-CONTRACT` | ACTIVE | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `C54X_CORE_COMMON_V1` | repository state | Local contracts only; no external URL |

## Immutable evidence

`PINNED` evidence is immutable: each record has a source, exact section/page
locator, scope, classification, and a short independently phrased assertion.

| Evidence ID | State | Scope | Classification | Source | Applicable target IDs | Locator |
|---|---|---|---|---|---|---|
| `E-TI-C54X-SPRU131G-S6.1-P6-3` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §6.1, `6-3` |
| `E-TI-C54X-SPRU131G-S6.2-P6-4` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §6.2, `6-4` |
| `E-TI-C54X-SPRU131G-S6.9-P6-25` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §6.9, `6-25` |
| `E-TI-C54X-SPRU131G-S4.1.1-P4-2-4-5` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §4.1.1, `4-2`–`4-5` |
| `E-TI-C54X-SPRU131G-S4.1.2-P4-6-4-8` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §4.1.2, `4-6`–`4-8` |
| `E-TI-C54X-SPRU131G-S3.3.4.6-P3-28` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §3.3.4.6, `3-28` |
| `E-TI-C54X-SPRU131G-S3.3.4.7-P3-28` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §3.3.4.7, `3-28` |
| `E-TI-C54X-SPRU131G-S6.2-T6-2-P6-4` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §6.2, `6-4` |
| `E-TI-C54X-SPRU131G-S6.7-P6-20` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §6.7, `6-20` |
| `E-TI-C54X-SPRU131G-S6.10.1-P6-27-6-28` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §6.10.1, `6-27`–`6-28` |
| `E-TI-C54X-SPRU172C-S4.NOP-P4-121` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §4 NOP, `4-121` |
| `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9-R1` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.4, `5-7`–`5-9` |
| `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.1, `5-10`–`5-11` |
| `E-TI-C54X-SPRU131G-S5.5.2-P5-11` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.2, `5-11` |
| `E-TI-C54X-SPRU131G-S5.5.3-P5-13` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3, `5-13` |
| `E-TI-C54X-SPRU131G-S5.5.3.1-P5-14` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3.1, `5-14` |
| `E-TI-C54X-SPRU131G-S5.5.5-P5-24` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.5, `5-24` |
| `E-PROJECT-ADDRESS-EFFECT-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/address/address_effect.h` anchor `TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY` |
| `E-PROJECT-CPU-SHELL-API` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `include/ti_c54x/cpu.h` anchor `ti_c54x_cpu_init` |
| `E-PROJECT-U40-FOUNDATION` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/foundation/arithmetic40.h` anchor `ti_c54x_u40_t` |
| `E-PROJECT-OBSERVER-NONINTERFERENCE` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `tests/unit/debug/test_observer.c` anchor `test_observation_does_not_change_architectural_result` |
| `E-PROJECT-EVIDENCE-REGISTRY` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `config/evidence.json` anchor `ti-c54x-evidence-registry` |
| `E-PROJECT-ARCH-STATE-KNOWNNESS` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/state/arch_state.h` anchor `TI_C54X_ARCH_STATE_UNKNOWN` |
| `E-PROJECT-ADDRESS-EFFECT-APPLICATION` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/sequencer/address_effect_apply.h` anchor `ti_c54x_sequencer_apply_address_result` |

The reset assertion enumerates PC, IPTR, IFR, RC, ARP, ASM, AVIS, BRAF, C16, C,
CLKOFF, CMPT, CPL, DP, DROM, FRCT, HM, INTM, OVA, OVB, OVM, OVLY, SXM, TC, and
XF. SP and remaining status bits are unresolved. NOP is `0xF495`, one word, with
no status-bit effects and only a PC increment. Corrected direct-addressing
evidence records a 7-bit dma; the implemented formation requirement covers the
valid CPL=0 concatenation of the 9-bit DP field and dma into a 16-bit
data-memory word address. A separate planned requirement and test cover the
source's statement that direct addressing leaves DP and SP unchanged; this
register-effect behavior is not folded into the implemented formation claim.
The source does not specify AR effects. SP-boundary overflow/wrap remains
unresolved.

The sequencer application slice consumes value-owned planner effects. It checks
the intrinsic NONE/PRE/POST shape and effective-address relation, then requires
the selected known AR to match the effect's expected value before performing at
most one checked write. Unknown and stale AR state have distinct statuses;
rejected calls preserve the whole private state. This is a project sequencing
contract and deliberately adds no ARP, bus, memory, timing, arithmetic, or
address-boundary behavior.

The private state seed is narrower than the full sequencer reset requirement:
`REQ-COMMON-ARCH-STATE-RESET-SEED` proves the fixed-width PC/IFR/RC and raw
ST0/ST1/PMST reset values and known masks using
`T-COMMON-ARCH-STATE-RESET-SEED`. AR0–AR7 and SP remain physically stored but
unknown after reset. `REQ-PROJECT-ARCH-STATE-KNOWNNESS` proves that checked
accessors preserve outputs on invalid or unknown reads, reset stale storage and
knownness deterministically, and establish known values only through writes;
this is a project contract and does not claim CPU-shell reset integration.

## Requirements

Requirement state and test state are separate. Requirements use exactly
`PLANNED`, `IMPLEMENTED`, or `UNRESOLVED`; `claim_kind` is `ARCHITECTURAL`,
`PROJECT_CONTRACT`, or `PROFILE`. Every implemented requirement links to an
existing `PASSING` test. The project rows below target the common architecture
but are explicitly `PROJECT_CONTRACT`, not TI conformance.

| Requirement ID | State | Target | Claim kind | Owner | Evidence IDs | Test IDs |
|---|---|---|---|---|---|---|
| `REQ-COMMON-RESET-DOCUMENTED-SUBSET` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S6.9-P6-25` | `T-COMMON-RESET-SUBSET` |
| `REQ-COMMON-ARCH-STATE-RESET-SEED` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/state/` | `E-TI-C54X-SPRU131G-S6.9-P6-25`, `E-TI-C54X-SPRU131G-S4.1.1-P4-2-4-5`, `E-TI-C54X-SPRU131G-S4.1.2-P4-6-4-8`, `E-TI-C54X-SPRU131G-S3.3.4.6-P3-28`, `E-TI-C54X-SPRU131G-S3.3.4.7-P3-28`, `E-TI-C54X-SPRU131G-S6.2-T6-2-P6-4`, `E-TI-C54X-SPRU131G-S6.7-P6-20`, `E-TI-C54X-SPRU131G-S6.10.1-P6-27-6-28` | `T-COMMON-ARCH-STATE-RESET-SEED` |
| `REQ-COMMON-RESET-UNSPECIFIED-FIELDS` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S6.9-P6-25` | — |
| `REQ-COMMON-NOP-DECODE` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/decode/` | `E-TI-C54X-SPRU172C-S4.NOP-P4-121` | `T-COMMON-NOP-DECODE` |
| `REQ-COMMON-NOP-SEMANTICS` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/ops/` | `E-TI-C54X-SPRU172C-S4.NOP-P4-121` | `T-COMMON-NOP-SEMANTICS` |
| `REQ-COMMON-NOP-PC-SEQUENTIAL` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S6.1-P6-3`, `E-TI-C54X-SPRU131G-S6.2-P6-4`, `E-TI-C54X-SPRU172C-S4.NOP-P4-121` | `T-COMMON-NOP-PC-SEQUENTIAL` |
| `REQ-COMMON-DIRECT-DP-ADDRESS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9-R1` | `T-COMMON-DIRECT-DP` |
| `REQ-COMMON-DIRECT-REGISTER-NONMUTATION` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9-R1` | `T-COMMON-DIRECT-REGISTER-NONMUTATION` |
| `REQ-COMMON-DIRECT-SP-WRAP` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9-R1` | — |
| `REQ-COMMON-INDIRECT-MOD0-WORD` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.3-P5-13` | `T-COMMON-INDIRECT-SINGLE-WORD` |
| `REQ-COMMON-INDIRECT-MOD1-MOD2-WORD-POSTUPDATE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3-P5-13`, `E-TI-C54X-SPRU131G-S5.5.3.1-P5-14` | `T-COMMON-INDIRECT-SINGLE-WORD` |
| `REQ-COMMON-INDIRECT-MOD3-WORD-PREINCREMENT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3-P5-13`, `E-TI-C54X-SPRU131G-S5.5.3.1-P5-14` | `T-COMMON-INDIRECT-SINGLE-WORD` |
| `REQ-COMMON-INDIRECT-CMPT0-ARF-SELECTION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.5-P5-24` | `T-COMMON-INDIRECT-SINGLE-WORD` |
| `REQ-COMMON-INDIRECT-CMPT0-ARP-STATE` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/state/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.5-P5-24` | `T-COMMON-INDIRECT-ARP-STATE` |
| `REQ-COMMON-INDIRECT-BOUNDARY` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.2-P5-11` | — |
| `REQ-COMMON-INDIRECT-LONG-DELTA` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3-P5-13` | `T-COMMON-INDIRECT-SINGLE-LONG` |
| `REQ-COMMON-INDIRECT-EFFECT-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3-P5-13`, `E-TI-C54X-SPRU131G-S5.5.3.1-P5-14` | `T-COMMON-INDIRECT-EFFECT-APPLICATION` |
| `REQ-PROJECT-ADDRESS-EFFECT-CONTRACT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/address/` | `E-PROJECT-ADDRESS-EFFECT-CONTRACT` | `T-PROJECT-ADDRESS-EFFECT-CONTRACT` |
| `REQ-PROJECT-CPU-SHELL` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `include/ti_c54x/` | `E-PROJECT-CPU-SHELL-API` | `T-PROJECT-CPU-SHELL` |
| `REQ-PROJECT-U40-FOUNDATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/foundation/` | `E-PROJECT-U40-FOUNDATION` | `T-PROJECT-U40-FOUNDATION` |
| `REQ-PROJECT-OBSERVER-NONINTERFERENCE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/debug/` | `E-PROJECT-OBSERVER-NONINTERFERENCE` | `T-PROJECT-OBSERVER-NONINTERFERENCE` |
| `REQ-PROJECT-EVIDENCE-REGISTRY` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `tools/quality/` | `E-PROJECT-EVIDENCE-REGISTRY` | `T-PROJECT-EVIDENCE-REGISTRY` |
| `REQ-PROJECT-ARCH-STATE-KNOWNNESS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/state/` | `E-PROJECT-ARCH-STATE-KNOWNNESS` | `T-PROJECT-ARCH-STATE-KNOWNNESS` |
| `REQ-PROJECT-ADDRESS-EFFECT-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-ADDRESS-EFFECT-APPLICATION` | `T-PROJECT-ADDRESS-EFFECT-APPLICATION` |

## Tests

Tests use exactly `PLANNED` or `PASSING`. Every test declares a scope and reverse
links to its requirements. Planned paths may not exist yet; passing paths must
exist in the repository.

| Test ID | State | Scope | Path | Runner | Requirement IDs |
|---|---|---|---|---|---|
| `T-COMMON-RESET-SUBSET` | PLANNED | COMMON_TI | `tests/unit/sequencer/test_reset.c` | `null` | `REQ-COMMON-RESET-DOCUMENTED-SUBSET` |
| `T-COMMON-ARCH-STATE-RESET-SEED` | PASSING | COMMON_TI | `tests/unit/state/test_arch_state_reset.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.arch_state_reset"}` | `REQ-COMMON-ARCH-STATE-RESET-SEED` |
| `T-COMMON-NOP-DECODE` | PLANNED | COMMON_TI | `tests/unit/isa/decode/test_nop_decode.c` | `null` | `REQ-COMMON-NOP-DECODE` |
| `T-COMMON-NOP-SEMANTICS` | PLANNED | COMMON_TI | `tests/unit/isa/ops/test_nop.c` | `null` | `REQ-COMMON-NOP-SEMANTICS` |
| `T-COMMON-NOP-PC-SEQUENTIAL` | PLANNED | COMMON_TI | `tests/unit/sequencer/test_nop_flow.c` | `null` | `REQ-COMMON-NOP-PC-SEQUENTIAL` |
| `T-COMMON-DIRECT-REGISTER-NONMUTATION` | PLANNED | COMMON_TI | `tests/unit/address/test_direct_register_effects.c` | `null` | `REQ-COMMON-DIRECT-REGISTER-NONMUTATION` |
| `T-COMMON-DIRECT-DP` | PASSING | COMMON_TI | `tests/unit/address/test_direct_dp.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.direct_dp"}` | `REQ-COMMON-DIRECT-DP-ADDRESS` |
| `T-COMMON-INDIRECT-SINGLE-WORD` | PASSING | COMMON_TI | `tests/unit/address/test_indirect_single_word.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_single_word"}` | `REQ-COMMON-INDIRECT-MOD0-WORD`, `REQ-COMMON-INDIRECT-MOD1-MOD2-WORD-POSTUPDATE`, `REQ-COMMON-INDIRECT-MOD3-WORD-PREINCREMENT`, `REQ-COMMON-INDIRECT-CMPT0-ARF-SELECTION` |
| `T-COMMON-INDIRECT-EFFECT-APPLICATION` | PASSING | COMMON_TI | `tests/unit/sequencer/test_address_effect_apply.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.address_effect_apply"}` | `REQ-COMMON-INDIRECT-EFFECT-APPLICATION` |
| `T-COMMON-INDIRECT-ARP-STATE` | PLANNED | COMMON_TI | `tests/unit/state/test_indirect_arp_state.c` | `null` | `REQ-COMMON-INDIRECT-CMPT0-ARP-STATE` |
| `T-COMMON-INDIRECT-SINGLE-LONG` | PLANNED | COMMON_TI | `tests/unit/address/test_indirect_single_long.c` | `null` | `REQ-COMMON-INDIRECT-LONG-DELTA` |
| `T-PROJECT-ADDRESS-EFFECT-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/address/test_address_effect_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.address_effect_contract"}` | `REQ-PROJECT-ADDRESS-EFFECT-CONTRACT` |
| `T-PROJECT-CPU-SHELL` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/cpu/test_cpu_api.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.cpu_api"}` | `REQ-PROJECT-CPU-SHELL` |
| `T-PROJECT-U40-FOUNDATION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/foundation/test_arithmetic40.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.arithmetic40"}` | `REQ-PROJECT-U40-FOUNDATION` |
| `T-PROJECT-OBSERVER-NONINTERFERENCE` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/debug/test_observer.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.observer"}` | `REQ-PROJECT-OBSERVER-NONINTERFERENCE` |
| `T-PROJECT-EVIDENCE-REGISTRY` | PASSING | PROJECT_CONTRACT_ONLY | `tests/contract/quality/test_evidence_registry.py` | `{"kind":"PYTHON_UNITTEST_DISCOVERY","make_goal":"test","ctest_name":"ti_c54x.quality.python"}` | `REQ-PROJECT-EVIDENCE-REGISTRY` |
| `T-PROJECT-ARCH-STATE-KNOWNNESS` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/state/test_arch_state_knownness.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.arch_state_knownness"}` | `REQ-PROJECT-ARCH-STATE-KNOWNNESS` |
| `T-PROJECT-ADDRESS-EFFECT-APPLICATION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_address_effect_apply_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.address_effect_apply_contract"}` | `REQ-PROJECT-ADDRESS-EFFECT-APPLICATION` |

## Correction and tombstone policy

Source and evidence corrections receive new IDs; Git review enforces history, but
the registry does not overclaim that a checker can detect historical edits.
Requirement and test IDs are also stable. The evidence tombstone array records
the correction below; the target, source, requirement, and test tombstone arrays
remain empty. A retired ID is retained for the ABI-major lifetime and is never
silently reused.

| Category | ID | Retired ABI | Reason | Replacement |
|---|---|---|---|---|
| targets | — | — | No tombstones currently recorded | — |
| sources | — | — | No tombstones currently recorded | — |
| evidence | `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9` | 0 | The original assertion overclaimed AR effects; the corrected record narrows the source to its printed field and address-construction statements. | `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9-R1` |
| requirements | — | — | No tombstones currently recorded | — |
| tests | — | — | No tombstones currently recorded | — |
