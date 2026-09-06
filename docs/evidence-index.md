# Evidence index

The canonical machine-readable registry is
[`config/evidence.json`](../config/evidence.json). This index mirrors every
source, evidence, requirement, and test ID in that file for review. The primary
target is `C54X_CORE_COMMON_V1`; the only other target is the planned
`C54X_VC5402_PROFILE_V1`.

Current registry counts are 2 targets, 7 sources, 91 immutable evidence records,
133 requirements (107 `IMPLEMENTED`, 9 `PLANNED`, 17 `UNRESOLVED`), and 96 tests
(87 `PASSING`, 9 `PLANNED`).

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
| `E-TI-C54X-SPRU131G-S4.1.1-P4-2-STATUS-FIELD-WRITES` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §4.1.1, `4-2` |
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
| `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD5-MOD6` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3, `5-13` |
| `E-TI-C54X-SPRU131G-S5.5.3.3-P5-15-AR0-INDEX` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3.3, `5-15` |
| `E-TI-C54X-SPRU131G-S5.5.3.1-P5-14` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3.1, `5-14` |
| `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-5-14-LONG32-DELTA` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3, `5-13`–`5-14` |
| `E-TI-C54X-SPRU131G-S5.8-P5-28-5-29-LONG32-WORD-PAIRING` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.8, `5-28`–`5-29` |
| `E-TI-C54X-SPRU131G-S5.5.5-P5-24` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.5, `5-24` |
| `E-PROJECT-ADDRESS-EFFECT-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/address/address_effect.h` anchor `TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY` |
| `E-PROJECT-OPCODE-CATALOG-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W16: NOP-seeded opcode architecture` |
| `E-PROJECT-CPU-SHELL-API` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `include/ti_c54x/cpu.h` anchor `ti_c54x_cpu_init` |
| `E-PROJECT-U40-FOUNDATION` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/foundation/arithmetic40.h` anchor `ti_c54x_u40_t` |
| `E-PROJECT-OBSERVER-NONINTERFERENCE` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `tests/unit/debug/test_observer.c` anchor `test_observation_does_not_change_architectural_result` |
| `E-PROJECT-EVIDENCE-REGISTRY` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `config/evidence.json` anchor `ti-c54x-evidence-registry` |
| `E-PROJECT-ARCH-STATE-KNOWNNESS` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/state/arch_state.h` anchor `TI_C54X_ARCH_STATE_UNKNOWN` |
| `E-PROJECT-ADDRESS-EFFECT-APPLICATION` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/sequencer/address_effect_apply.h` anchor `ti_c54x_sequencer_apply_address_result` |
| `E-PROJECT-ARCH-ADDRESS-FIELD-ACCESS` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/state/arch_state.h` anchor `ti_c54x_arch_state_read_arp` |
| `E-PROJECT-ADDRESS-STATE-POLICY` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/address/address_effect.h` anchor `TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0` |
| `E-PROJECT-DATA-OPERAND-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/address/data_operand.h` anchor `ti_c54x_data_operand_is_valid` |
| `E-PROJECT-ADDRESS-OPERAND-APPLICATION` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/sequencer/address_effect_apply.h` anchor `ti_c54x_sequencer_apply_address_result` |
| `E-PROJECT-DP-CPL-FIELD-ACCESS` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/state/arch_state.h` anchor `ti_c54x_arch_state_read_dp` |
| `E-PROJECT-DIRECT-SOURCE-VALIDATION` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/sequencer/address_effect_apply.h` anchor `source-arm domains` |
| `E-PROJECT-DIRECT-SOURCE-APPLICATION` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/sequencer/address_effect_apply.h` anchor `A rejected call leaves the complete` |
| `E-PROJECT-DIRECT-PLANNER-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/address/direct_sp.c` anchor `ti_c54x_direct_sp_result_with_status` |
| `E-PROJECT-INDIRECT-INDEXED-PLANNER-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/address/indirect_indexed.h` anchor `ti_c54x_indirect_indexed_word` |
| `E-PROJECT-INDIRECT-INDEXED-APPLICATION` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/sequencer/address_effect_apply.h` anchor `ti_c54x_sequencer_apply_address_result` |
| `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-14-MOD12-MOD13` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3, `5-14` |
| `E-TI-C54X-SPRU131G-S5.5.3.2-P5-14-5-15-LONG-OFFSET-CONTEXT` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3.2, `5-14`–`5-15` |
| `E-TI-C54X-SPRU172C-S3-9B-P3-21-LONG-OFFSET-READ` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §class 9B, `3-21` |
| `E-TI-C54X-SPRU172C-S3-13B-P3-29-LONG-OFFSET-WRITE` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §class 13B, `3-29` |
| `E-PROJECT-INDIRECT-LONG-OFFSET-PLANNER-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/address/indirect_long_offset.h` anchor `ti_c54x_indirect_long_offset_effect_is_valid` |
| `E-PROJECT-INDIRECT-LONG-OFFSET-APPLICATION` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/sequencer/address_effect_apply.h` anchor `ti_c54x_sequencer_apply_address_result` |
| `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD4-MOD7` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3, `5-13` |
| `E-TI-C54X-SPRU131G-S5.5.3.5-P5-18-5-19-REVERSE-CARRY` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3.5, `5-18`–`5-19` |
| `E-TI-C54X-SPRU131G-S5.5.5-T5-9-P5-23-AR0-SYNTAX` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.5, `5-23` |
| `E-PROJECT-REVERSE-CARRY-PRIMITIVE-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/foundation/reverse_carry.h` anchor `ti_c54x_reverse_carry_apply16` |
| `E-PROJECT-INDIRECT-REVERSE-CARRY-PLANNER-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/address/indirect_reverse_carry.h` anchor `ti_c54x_indirect_reverse_carry_word16` |
| `E-PROJECT-INDIRECT-REVERSE-CARRY-APPLICATION` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/sequencer/address_effect_apply.c` anchor `ti_c54x_sequencer_apply_address_result` |
| `E-TI-C54X-SPRU131G-S3.3.4.8-P3-28-BK-WIDTH` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §3.3.4.8, `3-28` |
| `E-TI-C54X-SPRU131G-S3.3.4-T3-3-P3-27-BK-MMR-ADDRESS` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §3.3.4, `3-27` |
| `E-TI-C54X-SPRU131G-S5.5.3.4-P5-15-5-16-CIRCULAR-BK` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3.4, `5-15`–`5-16` |
| `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD8-MOD10-WORD16` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3, `5-13` |
| `E-TI-C54X-SPRU131G-S5.5.3.4-P5-15-5-16-CIRCULAR-ALGORITHM` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §5.5.3.4, `5-15`–`5-16` |
| `E-PROJECT-ARCH-STATE-BK-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/state/block_size_register.h` anchor `ti_c54x_arch_state_read_bk` |
| `E-PROJECT-ADDRESS-EFFECT-VALIDATION-SEAM` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/sequencer/address_effect_validate.h` anchor `ti_c54x_sequencer_validate_address_effect` |
| `E-PROJECT-INDIRECT-CIRCULAR-PLANNER-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/address/indirect_circular.h` anchor `ti_c54x_indirect_circular_word16` |
| `E-PROJECT-INDIRECT-CIRCULAR-APPLICATION` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `src/sequencer/address_effect_apply.h` anchor `ti_c54x_sequencer_apply_address_result` |
| `E-PROJECT-MASKED-OPCODE-CATALOG-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W17: masked opcode catalog foundation (active kickoff)` |
| `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W18: private program-PC bridge and CPU NOP transaction (active kickoff)` |
| `E-PROJECT-P2-W19A-OPERAND-FIELD-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W19A: value-owned operand-field extraction (active kickoff)` |
| `E-PROJECT-P2-W19B-EXTENSION-FETCH-PC-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W19B: transactional extension fetch and width-aware program PC (active kickoff)` |
| `E-TI-C54X-SPRU172C-T2-16-P2-12-RPT-WIDTHS` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §Table 2‑16, `2-12` |
| `E-TI-C54X-SPRU172C-S4-RPT-P4-146-4-147` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §RPT, `4-146`–`4-147` |
| `E-TI-C54X-SPRU172C-S4-RPT-P4-146-RC-INACCESSIBLE` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §RPT, `4-146` |
| `E-TI-C54X-SPRU172C-S4-MVDK-MVKD-P4-108-4-114-RPT-RC-PREDICATE` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §MVDK and MVKD, `4-108`–`4-114` |
| `E-TI-C54X-SPRU172C-S2-5-T2-28-P2-19-2-21-RPT-BODY` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §2.5 and Table 2‑28, `2-19`–`2-21` |
| `E-TI-C54X-SPRU131G-S6-7-P6-20-6-22-RPT-IRQ` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §6.7, `6-20`–`6-22` |
| `E-TI-C54X-SPRU131G-S6-10-7-P6-34-RPT-IRQ-COMPLETION` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §6.10.7, `6-34` |
| `E-TI-C54X-SPRU131G-S6-10-3-S6-10-8-P6-31-6-36-IFR` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §6.10.3 and 6.10.8, `6-31`–`6-36` |
| `E-PROJECT-P2-W20A-RPT-TRANSACTION-ARCHITECTURE` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W20A: RPT evidence and instruction-transaction architecture (active kickoff)` |
| `E-PROJECT-P2-W20E-REPEAT-POLICY-FOUNDATION` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W20E: pure single-repeat policy foundation (active kickoff)` |
| `E-PROJECT-P2-W21A-BUS-WORD16-REQUEST-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W21A: private WORD16 bus-request contract (active kickoff)` |
| `E-PROJECT-P2-W21B-BUS-WORD16-BYTE-ORDER-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W21B: explicit WORD16 byte-order codec (implemented)` |
| `E-PROJECT-P2-W21C-BUS-WORD16-TRANSACTION-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W21C: resumable WORD16 transaction protocol (implemented)` |
| `E-TI-C54X-SPRU172C-LD-STATUS-FORMS` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §Table 2‑19, `2-14`–`2-15` |
| `E-TI-C54X-SPRU172C-S4-70-P4-70-4-71-LD-STATUS-EFFECTS` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §4.70, `4-70`–`4-71` |
| `E-TI-C54X-SPRU172C-LD-REPEATABILITY` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §2.5/Table 2‑28, `2-19`–`2-21` |
| `E-PROJECT-P2-W22-LD-STATUS-VERTICAL-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W22: immediate status-field LD vertical slice (implemented; local verification green)` |
| `E-TI-C54X-SPRU172C-S4-MAX-MIN-FORMS` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §MAX and MIN, `4-99`–`4-100` |
| `E-TI-C54X-SPRU172C-S4-MAX-MIN-EFFECTS` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §MAX and MIN, `4-99`–`4-100` |
| `E-TI-C54X-SPRU131G-S2.3-P2-8-ACCUMULATOR-40BIT` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU131G` | `C54X_CORE_COMMON_V1` | §2.3, `2-8` |
| `E-TI-C54X-SPRU172C-S2-5-T2-28-MAX-MIN-REPEATABILITY-BOUNDARY` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §2.5/Table 2‑28, `2-19`–`2-21` |
| `E-PROJECT-P2-W23-MAX-MIN-VERTICAL-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W23: MAX/MIN accumulator vertical slice (planned; implementation not started)` |
| `E-TI-C54X-SPRU172C-S4-CMPL-FORMS-P4-32` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §4 CMPL, `4-32` |
| `E-TI-C54X-SPRU172C-S4-CMPL-EFFECTS-P4-32` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §4 CMPL, `4-32` |
| `E-TI-C54X-SPRU172C-S2-5-T2-28-CMPL-REPEATABILITY-BOUNDARY` | PINNED | COMMON_TI | TI_PRIMARY | `SRC-TI-SPRU172C` | `C54X_CORE_COMMON_V1` | §2.5/Table 2‑28, `2-19`–`2-21` |
| `E-PROJECT-P2-W24-CMPL-ACCUMULATOR-WRITE-CONTRACT` | PINNED | PROJECT_CONTRACT_ONLY | PROJECT_CONTRACT_ONLY | `SRC-PROJECT-CONTRACT` | `C54X_CORE_COMMON_V1` | `docs/implementation-plan.md` anchor `P2-W24: generic accumulator-write effect and CMPL vertical slice (planned)` |

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

The long32 extension is intentionally narrow. `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-5-14-LONG32-DELTA`
records only the Table 5-4 magnitude: one data word for 16-bit access and two
for 32-bit access; the existing §5.5.3/§5.5.3.1 records remain the authority for
MOD ordering and the MOD3 write restriction. `E-TI-C54X-SPRU131G-S5.8-P5-28-5-29-LONG32-WORD-PAIRING`
records only long32 word order and parity pairing: first/MSW, second/LSW, even
first with the next higher word, and odd first with the previous lower word.
Neither record asserts bus selection, cycles, timing, or address wrap. The
implemented long-delta requirement therefore rejects unsupported overflow
endpoints under the project contract without claiming architectural wrap; the
separate `REQ-COMMON-INDIRECT-BOUNDARY` requirement remains `UNRESOLVED`.

The sequencer application slice consumes value-owned planner effects. It checks
the intrinsic NONE/PRE/POST shape and effective-address relation, then requires
the selected known AR to match the effect's expected value before performing at
most one checked write. Unknown and stale AR state have distinct statuses;
rejected calls preserve the whole private state. This is a project sequencing
contract and now validates the `CMPT0_ARP0` policy tag, requiring known CMPT=0
and ARP=0 without writing ARP back. Generic `NONE` effects do not consult mode
state. It deliberately adds no bus, memory, timing, arithmetic, or
address-boundary behavior.

The AR0-indexed slice is intentionally narrow. The new immutable TI records use
§5.5.3/Table 5-4 (printed p. 5-13) for MOD5's old-ARx then ARx−AR0 and MOD6's
old-ARx then ARx+AR0 sequence, and §5.5.3.3 (printed p. 5-15) for AR0 indexing
another auxiliary register. The common requirements and tests cover WORD16 only
over admitted safe domains (selected AR1–AR7, subtraction with ARx ≥ AR0, and
addition whose sum is ≤ 0xFFFF), using representative vectors plus complete
single-source dimensions; they do not claim a full Cartesian product. Selected
AR0 alias ordering, long32, wrap, circular, bit-reversed, offset, and other
unregistered modes remain unresolved or unsupported. The value-owned effect
copies both ARx and AR0 source snapshots, while the sequencer applies at most one
post-access write to selected ARx and never mutates AR0.

The signed long-offset MOD13 slice is similarly narrow. The new TI records pin
the signed 16-bit `lk` premodify semantics in §5.5.3/Table 5-4 (printed p.
5-14), the raw extension/update context in §5.5.3.2 (printed pp. 5-14–5-15),
and long-offset indirect long-word read/write forms in `SPRU172C` classes 9B
and 13B (printed pp. 3-21 and 3-29). They intentionally do not claim extension
fetch, repeat, bus, cycle, or timing behavior. WORD16 and LONG32 planners copy
the offset, sign-extend it explicitly, use one PRE_ACCESS selected-AR update,
and reject out-of-range unsigned endpoints under the project contract. The
application seam validates intrinsic shape and CMPT0/ARP0 plus selected-AR
state before a single write. MOD12 signedness is recorded as `UNRESOLVED` with
no test link. The MOD13 tests sweep each independent source dimension and add
representative cross-source vectors; they do not claim a full AR-by-offset
Cartesian product.

The reverse-carry slice is limited to the documented WORD16 safe domain. The
Table 5-4 record covers MOD4's old-AR then subtract-AR0 and MOD7's old-AR then
add-AR0 ordering; it does not claim width scaling or read/write restrictions.
Section 5.5.3.5 supplies the AR0 power-of-two and bit-15/MSB-toward-bit-0/LSB
carry propagation used by the foundation primitive. Table 5-9
records that `*AR0-0B` and `*AR0+0B` are legal syntax only; selected-AR0 alias
ordering is intentionally unresolved. The project contracts own input
validation, rejected non-one-hot/zero and unsafe results, and the sequencer's
single transaction application. Final propagation/wrap, selected-AR0 alias
ordering, and LONG32 applicability/update remain unresolved and have no
passing test links.

The BK state foundation is deliberately narrower than circular addressing. TI
`SPRU131G` §3.3.4.8 (printed p. 3-28) defines BK as a 16-bit circular-buffer
size register, while Table 3-3 (printed p. 3-27) maps its CPU MMR address to
`0x19`; the MMR record is mapping evidence only and belongs to future `src/bus/`
work. Section 5.5.3.4 (printed pp. 5-15–5-16) is retained for future circular
addressing design, not current arithmetic. The project BK accessor contract
keeps reset storage physically zero but unknown and establishes knownness only
through checked writes. No TI reset, circular arithmetic, boundary, timing, or
MMR behavior is claimed by this state slice.

The private state seed is narrower than the full sequencer reset requirement:
`REQ-COMMON-ARCH-STATE-RESET-SEED` proves the fixed-width PC/IFR/RC and raw
ST0/ST1/PMST reset values and known masks using
`T-COMMON-ARCH-STATE-RESET-SEED`. AR0–AR7 and SP remain physically stored but
unknown after reset. `REQ-PROJECT-ARCH-STATE-KNOWNNESS` proves that checked
accessors preserve outputs on invalid or unknown reads, reset stale storage and
knownness deterministically, and establish known values only through writes;
`REQ-PROJECT-ARCH-ADDRESS-FIELD-ACCESS` owns the ARP/CMPT-specific output,
rejection, unrelated-bit, and knownness behavior. These are project contracts
and do not claim CPU-shell reset integration.

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
| `REQ-COMMON-ARCH-STATE-ADDRESS-FIELDS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/state/` | `E-TI-C54X-SPRU131G-S4.1.1-P4-2-4-5`, `E-TI-C54X-SPRU131G-S4.1.1-P4-2-STATUS-FIELD-WRITES` | `T-COMMON-ARCH-STATE-ADDRESS-FIELDS` |
| `REQ-COMMON-RESET-UNSPECIFIED-FIELDS` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S6.9-P6-25` | — |
| `REQ-COMMON-NOP-DECODE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/decode/` | `E-TI-C54X-SPRU172C-S4.NOP-P4-121` | `T-COMMON-NOP-DECODE` |
| `REQ-COMMON-NOP-SEMANTICS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/ops/` | `E-TI-C54X-SPRU172C-S4.NOP-P4-121` | `T-COMMON-NOP-SEMANTICS` |
| `REQ-COMMON-NOP-PC-SEQUENTIAL` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S6.1-P6-3`, `E-TI-C54X-SPRU131G-S6.2-P6-4`, `E-TI-C54X-SPRU172C-S4.NOP-P4-121` | `T-COMMON-NOP-PC-SEQUENTIAL` |
| `REQ-COMMON-DIRECT-DP-ADDRESS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9-R1` | `T-COMMON-DIRECT-DP` |
| `REQ-COMMON-DIRECT-REGISTER-NONMUTATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9-R1` | `T-COMMON-DIRECT-REGISTER-NONMUTATION` |
| `REQ-COMMON-DIRECT-SP-ADDRESS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9-R1` | `T-COMMON-DIRECT-SP` |
| `REQ-COMMON-DIRECT-CPL-SOURCE-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9-R1` | `T-COMMON-DIRECT-CPL-SOURCE-APPLICATION` |
| `REQ-COMMON-DIRECT-SP-WRAP` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.4-P5-7-5-9-R1` | — |
| `REQ-COMMON-INDIRECT-MOD0-WORD` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.3-P5-13` | `T-COMMON-INDIRECT-SINGLE-WORD` |
| `REQ-COMMON-INDIRECT-MOD1-MOD2-WORD-POSTUPDATE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3-P5-13`, `E-TI-C54X-SPRU131G-S5.5.3.1-P5-14` | `T-COMMON-INDIRECT-SINGLE-WORD` |
| `REQ-COMMON-INDIRECT-MOD3-WORD-PREINCREMENT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3-P5-13`, `E-TI-C54X-SPRU131G-S5.5.3.1-P5-14` | `T-COMMON-INDIRECT-SINGLE-WORD` |
| `REQ-COMMON-INDIRECT-CMPT0-ARF-SELECTION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.5-P5-24` | `T-COMMON-INDIRECT-SINGLE-WORD` |
| `REQ-COMMON-INDIRECT-CMPT0-ARP-STATE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.5-P5-24` | `T-COMMON-INDIRECT-ARP-STATE` |
| `REQ-COMMON-INDIRECT-BOUNDARY` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.2-P5-11` | — |
| `REQ-COMMON-INDIRECT-LONG-DELTA` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3-P5-13`, `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-5-14-LONG32-DELTA` | `T-COMMON-INDIRECT-SINGLE-LONG` |
| `REQ-COMMON-DATA-OPERAND-LONG32-WORD-ORDER` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.8-P5-28-5-29-LONG32-WORD-PAIRING` | `T-COMMON-DATA-OPERAND-LONG32-WORD-ORDER` |
| `REQ-COMMON-INDIRECT-LONG32-EFFECT-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3-P5-13`, `E-TI-C54X-SPRU131G-S5.5.3.1-P5-14`, `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-5-14-LONG32-DELTA`, `E-TI-C54X-SPRU131G-S5.8-P5-28-5-29-LONG32-WORD-PAIRING` | `T-COMMON-INDIRECT-LONG32-EFFECT-APPLICATION` |
| `REQ-COMMON-INDIRECT-EFFECT-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3-P5-13`, `E-TI-C54X-SPRU131G-S5.5.3.1-P5-14` | `T-COMMON-INDIRECT-EFFECT-APPLICATION` |
| `REQ-COMMON-INDIRECT-INDEXED-MOD5-WORD` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD5-MOD6`, `E-TI-C54X-SPRU131G-S5.5.3.3-P5-15-AR0-INDEX` | `T-COMMON-INDIRECT-INDEXED-WORD` |
| `REQ-COMMON-INDIRECT-INDEXED-MOD6-WORD` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD5-MOD6`, `E-TI-C54X-SPRU131G-S5.5.3.3-P5-15-AR0-INDEX` | `T-COMMON-INDIRECT-INDEXED-WORD` |
| `REQ-COMMON-INDIRECT-INDEXED-EFFECT-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD5-MOD6`, `E-TI-C54X-SPRU131G-S5.5.3.3-P5-15-AR0-INDEX`, `E-TI-C54X-SPRU131G-S5.5.5-P5-24` | `T-COMMON-INDIRECT-INDEXED-EFFECT-APPLICATION` |
| `REQ-PROJECT-ADDRESS-EFFECT-CONTRACT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/address/` | `E-PROJECT-ADDRESS-EFFECT-CONTRACT` | `T-PROJECT-ADDRESS-EFFECT-CONTRACT` |
| `REQ-PROJECT-DIRECT-PLANNER-CONTRACT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/address/` | `E-PROJECT-DIRECT-PLANNER-CONTRACT` | `T-PROJECT-DIRECT-ADDRESS-CONTRACT` |
| `REQ-PROJECT-INDIRECT-INDEXED-PLANNER-CONTRACT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/address/` | `E-PROJECT-INDIRECT-INDEXED-PLANNER-CONTRACT` | `T-PROJECT-INDIRECT-INDEXED-PLANNER-CONTRACT` |
| `REQ-PROJECT-INDIRECT-INDEXED-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-INDIRECT-INDEXED-APPLICATION` | `T-PROJECT-INDIRECT-INDEXED-APPLICATION` |
| `REQ-PROJECT-CPU-SHELL` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `include/ti_c54x/` | `E-PROJECT-CPU-SHELL-API` | `T-PROJECT-CPU-SHELL` |
| `REQ-PROJECT-U40-FOUNDATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/foundation/` | `E-PROJECT-U40-FOUNDATION` | `T-PROJECT-U40-FOUNDATION` |
| `REQ-PROJECT-OBSERVER-NONINTERFERENCE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/debug/` | `E-PROJECT-OBSERVER-NONINTERFERENCE` | `T-PROJECT-OBSERVER-NONINTERFERENCE` |
| `REQ-PROJECT-EVIDENCE-REGISTRY` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `tools/quality/` | `E-PROJECT-EVIDENCE-REGISTRY` | `T-PROJECT-EVIDENCE-REGISTRY` |
| `REQ-PROJECT-ARCH-STATE-KNOWNNESS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/state/` | `E-PROJECT-ARCH-STATE-KNOWNNESS` | `T-PROJECT-ARCH-STATE-KNOWNNESS` |
| `REQ-PROJECT-ARCH-ADDRESS-FIELD-ACCESS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/state/` | `E-PROJECT-ARCH-ADDRESS-FIELD-ACCESS` | `T-PROJECT-ARCH-ADDRESS-FIELD-ACCESS` |
| `REQ-PROJECT-ADDRESS-STATE-POLICY` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-ADDRESS-STATE-POLICY` | `T-PROJECT-ADDRESS-STATE-POLICY` |
| `REQ-PROJECT-ADDRESS-EFFECT-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-ADDRESS-EFFECT-APPLICATION` | `T-PROJECT-ADDRESS-EFFECT-APPLICATION`, `T-PROJECT-ADDRESS-EFFECT-STATE-CONTRACT` |
| `REQ-PROJECT-DATA-OPERAND-CONTRACT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/address/` | `E-PROJECT-DATA-OPERAND-CONTRACT` | `T-PROJECT-DATA-OPERAND-CONTRACT` |
| `REQ-PROJECT-ADDRESS-OPERAND-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-DATA-OPERAND-CONTRACT`, `E-PROJECT-ADDRESS-OPERAND-APPLICATION` | `T-PROJECT-ADDRESS-OPERAND-APPLICATION` |
| `REQ-PROJECT-DP-CPL-FIELD-ACCESS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/state/` | `E-PROJECT-DP-CPL-FIELD-ACCESS` | `T-PROJECT-DP-CPL-FIELD-ACCESS` |
| `REQ-PROJECT-DIRECT-SOURCE-VALIDATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-DIRECT-SOURCE-VALIDATION` | `T-PROJECT-DIRECT-SOURCE-CONTRACT` |
| `REQ-PROJECT-DIRECT-REJECTION-PRECEDENCE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-DIRECT-SOURCE-APPLICATION` | `T-PROJECT-DIRECT-SOURCE-CONTRACT` |
| `REQ-PROJECT-DIRECT-SOURCE-STATUSES` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-DIRECT-SOURCE-APPLICATION` | `T-PROJECT-DIRECT-SOURCE-CONTRACT` |
| `REQ-PROJECT-DIRECT-TRANSACTION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-DIRECT-SOURCE-APPLICATION` | `T-PROJECT-DIRECT-SOURCE-CONTRACT` |
| `REQ-COMMON-INDIRECT-LONG-OFFSET-MOD13-WORD` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-14-MOD12-MOD13`, `E-TI-C54X-SPRU131G-S5.5.3.2-P5-14-5-15-LONG-OFFSET-CONTEXT` | `T-COMMON-INDIRECT-LONG-OFFSET-MOD13` |
| `REQ-COMMON-INDIRECT-LONG-OFFSET-MOD13-LONG32` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-14-MOD12-MOD13`, `E-TI-C54X-SPRU131G-S5.5.3.2-P5-14-5-15-LONG-OFFSET-CONTEXT`, `E-TI-C54X-SPRU172C-S3-9B-P3-21-LONG-OFFSET-READ`, `E-TI-C54X-SPRU172C-S3-13B-P3-29-LONG-OFFSET-WRITE`, `E-TI-C54X-SPRU131G-S5.8-P5-28-5-29-LONG32-WORD-PAIRING` | `T-COMMON-INDIRECT-LONG-OFFSET-MOD13` |
| `REQ-COMMON-INDIRECT-LONG-OFFSET-EFFECT-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-14-MOD12-MOD13`, `E-TI-C54X-SPRU131G-S5.5.3.2-P5-14-5-15-LONG-OFFSET-CONTEXT`, `E-TI-C54X-SPRU172C-S3-9B-P3-21-LONG-OFFSET-READ`, `E-TI-C54X-SPRU172C-S3-13B-P3-29-LONG-OFFSET-WRITE`, `E-TI-C54X-SPRU131G-S5.8-P5-28-5-29-LONG32-WORD-PAIRING` | `T-COMMON-INDIRECT-LONG-OFFSET-EFFECT-APPLICATION` |
| `REQ-COMMON-INDIRECT-LONG-OFFSET-MOD12-SIGNEDNESS` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-14-MOD12-MOD13`, `E-TI-C54X-SPRU131G-S5.5.3.2-P5-14-5-15-LONG-OFFSET-CONTEXT` | — |
| `REQ-PROJECT-INDIRECT-LONG-OFFSET-PLANNER` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/address/` | `E-PROJECT-INDIRECT-LONG-OFFSET-PLANNER-CONTRACT` | `T-PROJECT-INDIRECT-LONG-OFFSET-PLANNER` |
| `REQ-PROJECT-INDIRECT-LONG-OFFSET-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-INDIRECT-LONG-OFFSET-APPLICATION` | `T-PROJECT-INDIRECT-LONG-OFFSET-APPLICATION` |
| `REQ-COMMON-REVERSE-CARRY-PRIMITIVE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/foundation/` | `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3.5-P5-18-5-19-REVERSE-CARRY` | `T-COMMON-REVERSE-CARRY-PRIMITIVE` |
| `REQ-COMMON-INDIRECT-MOD4-REVERSE-CARRY-WORD` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD4-MOD7` | `T-COMMON-INDIRECT-REVERSE-CARRY-WORD` |
| `REQ-COMMON-INDIRECT-MOD7-REVERSE-CARRY-WORD` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD4-MOD7` | `T-COMMON-INDIRECT-REVERSE-CARRY-WORD` |
| `REQ-COMMON-INDIRECT-REVERSE-CARRY-EFFECT-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD4-MOD7`, `E-TI-C54X-SPRU131G-S5.5.5-P5-24` | `T-COMMON-INDIRECT-REVERSE-CARRY-EFFECT-APPLICATION` |
| `REQ-COMMON-INDIRECT-REVERSE-CARRY-BOUNDARY` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.2-P5-11`, `E-TI-C54X-SPRU131G-S5.5.3.5-P5-18-5-19-REVERSE-CARRY` | — |
| `REQ-COMMON-INDIRECT-REVERSE-CARRY-AR0-ALIAS` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.5-T5-9-P5-23-AR0-SYNTAX` | — |
| `REQ-COMMON-INDIRECT-REVERSE-CARRY-LONG32` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD4-MOD7` | — |
| `REQ-PROJECT-REVERSE-CARRY-PRIMITIVE-CONTRACT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/foundation/` | `E-PROJECT-REVERSE-CARRY-PRIMITIVE-CONTRACT` | `T-PROJECT-REVERSE-CARRY-PRIMITIVE-CONTRACT` |
| `REQ-PROJECT-INDIRECT-REVERSE-CARRY-PLANNER-CONTRACT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/address/` | `E-PROJECT-INDIRECT-REVERSE-CARRY-PLANNER-CONTRACT` | `T-PROJECT-INDIRECT-REVERSE-CARRY-PLANNER-CONTRACT` |
| `REQ-PROJECT-INDIRECT-REVERSE-CARRY-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-INDIRECT-REVERSE-CARRY-APPLICATION` | `T-PROJECT-INDIRECT-REVERSE-CARRY-APPLICATION` |
| `REQ-COMMON-ARCH-STATE-BK-WIDTH` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/state/` | `E-TI-C54X-SPRU131G-S3.3.4.8-P3-28-BK-WIDTH` | `T-COMMON-ARCH-STATE-BK-WIDTH` |
| `REQ-PROJECT-ARCH-STATE-BK-KNOWNNESS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/state/` | `E-PROJECT-ARCH-STATE-BK-CONTRACT` | `T-PROJECT-ARCH-STATE-BK-KNOWNNESS` |
| `REQ-COMMON-BK-MMR-ROUTING` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/bus/` | `E-TI-C54X-SPRU131G-S3.3.4-T3-3-P3-27-BK-MMR-ADDRESS` | `T-COMMON-BK-MMR-ROUTING` |
| `REQ-COMMON-INDIRECT-CIRCULAR-BK` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.3.4-P5-15-5-16-CIRCULAR-BK`, `E-TI-C54X-SPRU131G-S5.5.3.4-P5-15-5-16-CIRCULAR-ALGORITHM` | `T-COMMON-INDIRECT-CIRCULAR-BK` |
| `REQ-COMMON-INDIRECT-CIRCULAR-MOD8-MOD10-WORD16` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.1-P5-10-5-11`, `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD8-MOD10-WORD16`, `E-TI-C54X-SPRU131G-S5.5.3.4-P5-15-5-16-CIRCULAR-ALGORITHM` | `T-COMMON-INDIRECT-CIRCULAR-BK` |
| `REQ-COMMON-INDIRECT-CIRCULAR-EFFECT-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/` | `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD8-MOD10-WORD16`, `E-TI-C54X-SPRU131G-S5.5.3.4-P5-15-5-16-CIRCULAR-ALGORITHM`, `E-TI-C54X-SPRU131G-S5.5.5-P5-24` | `T-COMMON-INDIRECT-CIRCULAR-EFFECT-APPLICATION` |
| `REQ-PROJECT-INDIRECT-CIRCULAR-PLANNER-CONTRACT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/address/` | `E-PROJECT-INDIRECT-CIRCULAR-PLANNER-CONTRACT` | `T-PROJECT-INDIRECT-CIRCULAR-PLANNER-CONTRACT` |
| `REQ-PROJECT-INDIRECT-CIRCULAR-APPLICATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-INDIRECT-CIRCULAR-APPLICATION` | `T-PROJECT-INDIRECT-CIRCULAR-APPLICATION` |
| `REQ-COMMON-INDIRECT-CIRCULAR-INITIAL-AR-BOUNDARY` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.3.4-P5-15-5-16-CIRCULAR-ALGORITHM` | — |
| `REQ-COMMON-INDIRECT-CIRCULAR-LONG32-SCALING` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.3-T5-4-P5-13-MOD8-MOD10-WORD16` | — |
| `REQ-COMMON-INDIRECT-CIRCULAR-MOD9-MOD11-AR0` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.3.4-P5-15-5-16-CIRCULAR-ALGORITHM` | — |
| `REQ-COMMON-INDIRECT-CIRCULAR-MOD14-OFFSET` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.3.4-P5-15-5-16-CIRCULAR-ALGORITHM` | — |
| `REQ-COMMON-INDIRECT-CIRCULAR-CMPT1-DUAL` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/address/` | `E-TI-C54X-SPRU131G-S5.5.3.4-P5-15-5-16-CIRCULAR-ALGORITHM` | — |
| `REQ-PROJECT-ADDRESS-EFFECT-VALIDATION-SEAM` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-ADDRESS-EFFECT-VALIDATION-SEAM` | `T-PROJECT-ADDRESS-EFFECT-VALIDATION-SEAM`, `T-PROJECT-ADDRESS-EFFECT-DEPENDENCY-ORDER` |
| `REQ-PROJECT-MASKED-OPCODE-CATALOG` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/isa/catalog/` | `E-PROJECT-MASKED-OPCODE-CATALOG-CONTRACT` | `T-PROJECT-MASKED-OPCODE-CATALOG` |
| `REQ-PROJECT-OPCODE-LOOKUP-GENERATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/isa/catalog/` | `E-PROJECT-MASKED-OPCODE-CATALOG-CONTRACT` | `T-PROJECT-OPCODE-LOOKUP-GENERATION` |
| `REQ-PROJECT-OPCODE-CATALOG-CONTRACT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/isa/catalog/` | `E-PROJECT-OPCODE-CATALOG-CONTRACT` | `T-PROJECT-OPCODE-CATALOG-CONTRACT` |
| `REQ-COMMON-CPU-NOP-SEQUENTIAL-EXECUTION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/cpu/` | `E-TI-C54X-SPRU131G-S6.1-P6-3`, `E-TI-C54X-SPRU131G-S6.2-P6-4`, `E-TI-C54X-SPRU172C-S4.NOP-P4-121` | `T-COMMON-CPU-NOP-EXECUTION` |
| `REQ-PROJECT-CPU-BUDGET-ZERO` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT` | `T-PROJECT-CPU-EXECUTION-BUDGET` |
| `REQ-PROJECT-CPU-PUBLIC-PC-VALIDATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT`, `E-PROJECT-CPU-SHELL-API` | `T-PROJECT-CPU-NOP-EXECUTION-CONTRACT` |
| `REQ-PROJECT-CPU-FETCH-WORD16` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT`, `E-PROJECT-CPU-SHELL-API` | `T-PROJECT-CPU-NOP-EXECUTION-CONTRACT` |
| `REQ-PROJECT-CPU-FETCH-FAILURE-STOP` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT` | `T-PROJECT-CPU-NOP-EXECUTION-CONTRACT` |
| `REQ-PROJECT-CPU-UNKNOWN-DECODE-STOP` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT`, `E-PROJECT-OPCODE-CATALOG-CONTRACT` | `T-PROJECT-CPU-NOP-EXECUTION-CONTRACT` |
| `REQ-PROJECT-CPU-NOP-DISPATCH` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT`, `E-PROJECT-OPCODE-CATALOG-CONTRACT` | `T-PROJECT-CPU-NOP-EXECUTION-CONTRACT` |
| `REQ-PROJECT-SEQUENCER-PROGRAM-PC-BRIDGE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT` | `T-PROJECT-PROGRAM-PC-BRIDGE` |
| `REQ-PROJECT-CPU-PC-BOUNDARIES` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT` | `T-PROJECT-PROGRAM-PC-BRIDGE`, `T-PROJECT-CPU-NOP-EXECUTION-CONTRACT` |
| `REQ-PROJECT-SEQUENCER-PC-TRANSACTION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT` | `T-PROJECT-PROGRAM-PC-BRIDGE` |
| `REQ-PROJECT-CPU-PC-COMMIT-ORDER` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT` | `T-PROJECT-CPU-NOP-EXECUTION-CONTRACT` |
| `REQ-PROJECT-CPU-COUNTER-PHASES` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT` | `T-PROJECT-CPU-EXECUTION-BUDGET` |
| `REQ-PROJECT-CPU-RUN-RETIREMENT-BUDGET` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT` | `T-PROJECT-CPU-EXECUTION-BUDGET` |
| `REQ-PROJECT-CPU-COUNTER-OVERFLOW` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W18-CPU-EXECUTION-CONTRACT` | `T-PROJECT-CPU-EXECUTION-BUDGET` |
| `REQ-PROJECT-OPERAND-FIELD-EXTRACTION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/isa/decode/` | `E-PROJECT-P2-W19A-OPERAND-FIELD-CONTRACT` | `T-PROJECT-OPERAND-FIELD-EXTRACTION` |
| `REQ-PROJECT-INSTRUCTION-EXTENSION-FETCH` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W19B-EXTENSION-FETCH-PC-CONTRACT` | `T-PROJECT-INSTRUCTION-EXTENSION-FETCH` |
| `REQ-PROJECT-SEQUENCER-MULTIWORD-PC` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/` | `E-PROJECT-P2-W19B-EXTENSION-FETCH-PC-CONTRACT` | `T-PROJECT-SEQUENCER-MULTIWORD-PC` |
| `REQ-PROJECT-CANONICAL-INSTRUCTION-BUNDLE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/isa/decode/` | `E-PROJECT-P2-W20A-RPT-TRANSACTION-ARCHITECTURE` | `T-PROJECT-CANONICAL-INSTRUCTION-BUNDLE` |
| `REQ-PROJECT-CATALOG-METADATA-TOKENS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/isa/catalog/` | `E-PROJECT-P2-W20A-RPT-TRANSACTION-ARCHITECTURE` | `T-PROJECT-CATALOG-METADATA-TOKENS` |
| `REQ-PROJECT-CATALOG-OPERAND-LAYOUT-BINDING` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/isa/decode/` | `E-PROJECT-P2-W20A-RPT-TRANSACTION-ARCHITECTURE` | `T-PROJECT-CATALOG-OPERAND-LAYOUT-BINDING` |
| `REQ-PROJECT-RPT-IMMEDIATE-EFFECT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/isa/ops/repeat/` | `E-PROJECT-P2-W20A-RPT-TRANSACTION-ARCHITECTURE` | `T-PROJECT-RPT-IMMEDIATE-EFFECT` |
| `REQ-PROJECT-SINGLE-REPEAT-CARDINALITY-POLICY` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/repeat/` | `E-PROJECT-P2-W20E-REPEAT-POLICY-FOUNDATION` | `T-PROJECT-SINGLE-REPEAT-CARDINALITY-POLICY` |
| `REQ-PROJECT-SINGLE-REPEAT-BODY-ADMISSION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/repeat/` | `E-PROJECT-P2-W20E-REPEAT-POLICY-FOUNDATION` | `T-PROJECT-SINGLE-REPEAT-BODY-ADMISSION` |
| `REQ-PROJECT-SINGLE-REPEAT-REQUEST-GATE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/repeat/` | `E-PROJECT-P2-W20E-REPEAT-POLICY-FOUNDATION` | `T-PROJECT-SINGLE-REPEAT-REQUEST-GATE` |
| `REQ-PROJECT-BUS-WORD16-REQUEST-SHAPE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/bus/request/` | `E-PROJECT-P2-W21A-BUS-WORD16-REQUEST-CONTRACT` | `T-PROJECT-BUS-WORD16-REQUEST-SHAPE` |
| `REQ-PROJECT-BUS-WORD16-BYTE-ORDER-CODEC` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/bus/byte_order/` | `E-PROJECT-P2-W21B-BUS-WORD16-BYTE-ORDER-CONTRACT` | `T-PROJECT-BUS-WORD16-BYTE-ORDER-CODEC` |
| `REQ-COMMON-RPT-IMMEDIATE-DECODE` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/decode/` | `E-TI-C54X-SPRU172C-T2-16-P2-12-RPT-WIDTHS`, `E-TI-C54X-SPRU172C-S4-RPT-P4-146-4-147` | `T-COMMON-RPT-IMMEDIATE-DECODE` |
| `REQ-COMMON-RPT-IMMEDIATE-OPERATION` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/ops/repeat/` | `E-TI-C54X-SPRU172C-S4-RPT-P4-146-4-147` | `T-COMMON-RPT-IMMEDIATE-OPERATION` |
| `REQ-COMMON-SINGLE-REPEAT-CARDINALITY` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/repeat/` | `E-TI-C54X-SPRU172C-S4-RPT-P4-146-4-147` | `T-COMMON-SINGLE-REPEAT-CARDINALITY` |
| `REQ-COMMON-SINGLE-REPEAT-IRQ-GATE` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/repeat/` | `E-TI-C54X-SPRU131G-S6-7-P6-20-6-22-RPT-IRQ`, `E-TI-C54X-SPRU131G-S6-10-7-P6-34-RPT-IRQ-COMPLETION` | `T-COMMON-SINGLE-REPEAT-IRQ-GATE` |
| `REQ-COMMON-SINGLE-REPEAT-PENDING-REQUEST` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/interrupt/` | `E-TI-C54X-SPRU131G-S6-7-P6-20-6-22-RPT-IRQ`, `E-TI-C54X-SPRU131G-S6-10-3-S6-10-8-P6-31-6-36-IFR` | `T-COMMON-SINGLE-REPEAT-PENDING-REQUEST` |
| `REQ-COMMON-SINGLE-REPEAT-BODY-ELIGIBILITY` | PLANNED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/repeat/` | `E-TI-C54X-SPRU172C-S2-5-T2-28-P2-19-2-21-RPT-BODY` | `T-COMMON-SINGLE-REPEAT-BODY-ELIGIBILITY` |
| `REQ-COMMON-SINGLE-REPEAT-RC-PROGRESSION` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/repeat/` | `E-TI-C54X-SPRU172C-S4-RPT-P4-146-4-147`, `E-TI-C54X-SPRU172C-S4-RPT-P4-146-RC-INACCESSIBLE`, `E-TI-C54X-SPRU172C-S4-MVDK-MVKD-P4-108-4-114-RPT-RC-PREDICATE`, `E-TI-C54X-SPRU131G-S6-7-P6-20-6-22-RPT-IRQ` |  |
| `REQ-COMMON-SINGLE-REPEAT-PC-FLOW` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/repeat/` | `E-TI-C54X-SPRU131G-S6.1-P6-3`, `E-TI-C54X-SPRU131G-S6-7-P6-20-6-22-RPT-IRQ` |  |
| `REQ-COMMON-SINGLE-REPEAT-PREDECODE-IRQ-ARBITRATION` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/sequencer/interrupt/` | `E-TI-C54X-SPRU131G-S6-7-P6-20-6-22-RPT-IRQ`, `E-TI-C54X-SPRU131G-S6-10-3-S6-10-8-P6-31-6-36-IFR` |  |
| `REQ-PROJECT-BUS-WORD16-TRANSACTION-LIFECYCLE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/bus/transaction/` | `E-PROJECT-P2-W21C-BUS-WORD16-TRANSACTION-CONTRACT` | `T-PROJECT-BUS-WORD16-TRANSACTION-LIFECYCLE` |
| `REQ-PROJECT-BUS-WORD16-TRANSACTION-VALIDATION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/bus/transaction/` | `E-PROJECT-P2-W21C-BUS-WORD16-TRANSACTION-CONTRACT` | `T-PROJECT-BUS-WORD16-TRANSACTION-VALIDATION` |
| `REQ-PROJECT-BUS-WORD16-EXTERNAL-EXACTLY-ONCE` | PLANNED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/bus/adapter/` | `E-PROJECT-P2-W21C-BUS-WORD16-TRANSACTION-CONTRACT` | `T-PROJECT-BUS-WORD16-EXTERNAL-EXACTLY-ONCE` |
| `REQ-COMMON-LD-K9-DP-IMMEDIATE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/decode/` | `E-TI-C54X-SPRU172C-LD-STATUS-FORMS`, `E-TI-C54X-SPRU172C-S4-70-P4-70-4-71-LD-STATUS-EFFECTS`, `E-TI-C54X-SPRU172C-LD-REPEATABILITY` | `T-COMMON-LD-STATUS-FIELD-DECODE` |
| `REQ-COMMON-LD-K5-ASM-IMMEDIATE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/decode/` | `E-TI-C54X-SPRU172C-LD-STATUS-FORMS`, `E-TI-C54X-SPRU172C-S4-70-P4-70-4-71-LD-STATUS-EFFECTS`, `E-TI-C54X-SPRU172C-LD-REPEATABILITY` | `T-COMMON-LD-STATUS-FIELD-DECODE` |
| `REQ-COMMON-LD-K3-ARP-IMMEDIATE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/decode/` | `E-TI-C54X-SPRU172C-LD-STATUS-FORMS`, `E-TI-C54X-SPRU172C-S4-70-P4-70-4-71-LD-STATUS-EFFECTS`, `E-TI-C54X-SPRU172C-LD-REPEATABILITY` | `T-COMMON-LD-STATUS-FIELD-DECODE` |
| `REQ-COMMON-LD-STATUS-FIELD-EFFECTS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/ops/load/` | `E-TI-C54X-SPRU172C-S4-70-P4-70-4-71-LD-STATUS-EFFECTS` | `T-COMMON-LD-STATUS-FIELD-OPS` |
| `REQ-PROJECT-LD-STATUS-REGISTER-CONTRACT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/state/status/` | `E-PROJECT-P2-W22-LD-STATUS-VERTICAL-CONTRACT` | `T-PROJECT-STATUS-REGISTER-CONTRACT` |
| `REQ-PROJECT-LD-STATUS-FIELD-EFFECT-TRANSACTION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/status/` | `E-PROJECT-P2-W22-LD-STATUS-VERTICAL-CONTRACT` | `T-PROJECT-STATUS-FIELD-EFFECT-APPLICATION` |
| `REQ-PROJECT-LD-CPU-STATE-OBSERVATION-RETIRE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W22-LD-STATUS-VERTICAL-CONTRACT` | `T-PROJECT-CPU-LD-STATUS-FIELDS` |
| `REQ-COMMON-MAX-MIN-DECODE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/decode/` | `E-TI-C54X-SPRU172C-S4-MAX-MIN-FORMS` | `T-COMMON-MAX-MIN-DECODE` |
| `REQ-COMMON-MAX-MIN-SELECTION-EFFECTS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/ops/compare/` | `E-TI-C54X-SPRU172C-S4-MAX-MIN-EFFECTS`, `E-TI-C54X-SPRU131G-S2.3-P2-8-ACCUMULATOR-40BIT` | `T-COMMON-MAX-MIN-OPS` |
| `REQ-PROJECT-MAX-MIN-ACCUMULATOR-EFFECT-TRANSACTION` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/accumulator/` | `E-PROJECT-P2-W23-MAX-MIN-VERTICAL-CONTRACT` | `T-PROJECT-MAX-MIN-EFFECT-TRANSACTION` |
| `REQ-PROJECT-MAX-MIN-ACCUMULATOR-STATE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/state/accumulator/` | `E-PROJECT-P2-W23-MAX-MIN-VERTICAL-CONTRACT` | `T-PROJECT-MAX-MIN-ACCUMULATOR-STATE` |
| `REQ-PROJECT-MAX-MIN-CPU-RETIREMENT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W23-MAX-MIN-VERTICAL-CONTRACT` | `T-PROJECT-CPU-MAX-MIN` |
| `REQ-COMMON-MAX-MIN-REPEATABILITY` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/catalog/` | `E-TI-C54X-SPRU172C-S2-5-T2-28-MAX-MIN-REPEATABILITY-BOUNDARY` |  |
| `REQ-PROJECT-ACCUMULATOR-WRITE-EFFECT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/sequencer/accumulator/` | `E-PROJECT-P2-W24-CMPL-ACCUMULATOR-WRITE-CONTRACT` | `T-PROJECT-ACCUMULATOR-WRITE-EFFECT` |
| `REQ-COMMON-CMPL-DECODE` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/decode/` | `E-TI-C54X-SPRU172C-S4-CMPL-FORMS-P4-32` | `T-COMMON-CMPL-DECODE` |
| `REQ-COMMON-CMPL-EFFECTS` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/ops/accumulator/` | `E-TI-C54X-SPRU172C-S4-CMPL-EFFECTS-P4-32`, `E-TI-C54X-SPRU131G-S2.3-P2-8-ACCUMULATOR-40BIT` | `T-COMMON-CMPL-OPS` |
| `REQ-PROJECT-CMPL-CPU-RETIREMENT` | IMPLEMENTED | `C54X_CORE_COMMON_V1` | PROJECT_CONTRACT | `src/cpu/` | `E-PROJECT-P2-W24-CMPL-ACCUMULATOR-WRITE-CONTRACT` | `T-PROJECT-CPU-CMPL` |
| `REQ-COMMON-CMPL-REPEATABILITY` | UNRESOLVED | `C54X_CORE_COMMON_V1` | ARCHITECTURAL | `src/isa/catalog/` | `E-TI-C54X-SPRU172C-S2-5-T2-28-CMPL-REPEATABILITY-BOUNDARY` |  |

## Tests

Tests use exactly `PLANNED` or `PASSING`. Every test declares a scope and reverse
links to its requirements. Planned paths may not exist yet; passing paths must
exist in the repository.

| Test ID | State | Scope | Path | Runner | Requirement IDs |
|---|---|---|---|---|---|
| `T-COMMON-RESET-SUBSET` | PLANNED | COMMON_TI | `tests/unit/sequencer/test_reset.c` | `null` | `REQ-COMMON-RESET-DOCUMENTED-SUBSET` |
| `T-COMMON-ARCH-STATE-RESET-SEED` | PASSING | COMMON_TI | `tests/unit/state/test_arch_state_reset.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.arch_state_reset"}` | `REQ-COMMON-ARCH-STATE-RESET-SEED` |
| `T-COMMON-ARCH-STATE-ADDRESS-FIELDS` | PASSING | COMMON_TI | `tests/unit/state/test_arch_state_address_fields.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.arch_state_address_fields"}` | `REQ-COMMON-ARCH-STATE-ADDRESS-FIELDS` |
| `T-COMMON-NOP-DECODE` | PASSING | COMMON_TI | `tests/unit/isa/decode/test_nop_decode.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.nop_decode"}` | `REQ-COMMON-NOP-DECODE` |
| `T-COMMON-NOP-SEMANTICS` | PASSING | COMMON_TI | `tests/unit/isa/ops/test_nop.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.nop"}` | `REQ-COMMON-NOP-SEMANTICS` |
| `T-COMMON-NOP-PC-SEQUENTIAL` | PASSING | COMMON_TI | `tests/unit/sequencer/test_nop_flow.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.nop_flow"}` | `REQ-COMMON-NOP-PC-SEQUENTIAL` |
| `T-COMMON-DIRECT-REGISTER-NONMUTATION` | PASSING | COMMON_TI | `tests/unit/sequencer/test_direct_source_apply.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.direct_source_apply"}` | `REQ-COMMON-DIRECT-REGISTER-NONMUTATION` |
| `T-COMMON-DIRECT-SP` | PASSING | COMMON_TI | `tests/unit/address/test_direct_sp.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.direct_sp"}` | `REQ-COMMON-DIRECT-SP-ADDRESS` |
| `T-COMMON-DIRECT-CPL-SOURCE-APPLICATION` | PASSING | COMMON_TI | `tests/unit/sequencer/test_direct_source_apply.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.direct_source_apply"}` | `REQ-COMMON-DIRECT-CPL-SOURCE-APPLICATION` |
| `T-COMMON-DIRECT-DP` | PASSING | COMMON_TI | `tests/unit/address/test_direct_dp.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.direct_dp"}` | `REQ-COMMON-DIRECT-DP-ADDRESS` |
| `T-COMMON-INDIRECT-SINGLE-WORD` | PASSING | COMMON_TI | `tests/unit/address/test_indirect_single_word.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_single_word"}` | `REQ-COMMON-INDIRECT-MOD0-WORD`, `REQ-COMMON-INDIRECT-MOD1-MOD2-WORD-POSTUPDATE`, `REQ-COMMON-INDIRECT-MOD3-WORD-PREINCREMENT`, `REQ-COMMON-INDIRECT-CMPT0-ARF-SELECTION` |
| `T-COMMON-INDIRECT-EFFECT-APPLICATION` | PASSING | COMMON_TI | `tests/unit/sequencer/test_address_effect_apply.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.address_effect_apply"}` | `REQ-COMMON-INDIRECT-EFFECT-APPLICATION` |
| `T-COMMON-INDIRECT-ARP-STATE` | PASSING | COMMON_TI | `tests/unit/sequencer/test_indirect_cmpt0_state.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_cmpt0_state"}` | `REQ-COMMON-INDIRECT-CMPT0-ARP-STATE` |
| `T-COMMON-INDIRECT-SINGLE-LONG` | PASSING | COMMON_TI | `tests/unit/address/test_indirect_single_long32.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_single_long32"}` | `REQ-COMMON-INDIRECT-LONG-DELTA` |
| `T-COMMON-DATA-OPERAND-LONG32-WORD-ORDER` | PASSING | COMMON_TI | `tests/unit/address/test_data_operand.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.data_operand"}` | `REQ-COMMON-DATA-OPERAND-LONG32-WORD-ORDER` |
| `T-COMMON-INDIRECT-LONG32-EFFECT-APPLICATION` | PASSING | COMMON_TI | `tests/unit/sequencer/test_indirect_long32_effect_apply.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_long32_effect_apply"}` | `REQ-COMMON-INDIRECT-LONG32-EFFECT-APPLICATION` |
| `T-COMMON-INDIRECT-INDEXED-WORD` | PASSING | COMMON_TI | `tests/unit/address/test_indirect_indexed_word.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_indexed_word"}` | `REQ-COMMON-INDIRECT-INDEXED-MOD5-WORD`, `REQ-COMMON-INDIRECT-INDEXED-MOD6-WORD` |
| `T-COMMON-INDIRECT-INDEXED-EFFECT-APPLICATION` | PASSING | COMMON_TI | `tests/unit/sequencer/test_indirect_indexed_effect_apply.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_indexed_effect_apply"}` | `REQ-COMMON-INDIRECT-INDEXED-EFFECT-APPLICATION` |
| `T-PROJECT-ADDRESS-EFFECT-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/address/test_address_effect_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.address_effect_contract"}` | `REQ-PROJECT-ADDRESS-EFFECT-CONTRACT` |
| `T-PROJECT-DIRECT-ADDRESS-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/address/test_direct_address_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.direct_address_contract"}` | `REQ-PROJECT-DIRECT-PLANNER-CONTRACT` |
| `T-PROJECT-INDIRECT-INDEXED-PLANNER-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/address/test_indirect_indexed_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_indexed_contract"}` | `REQ-PROJECT-INDIRECT-INDEXED-PLANNER-CONTRACT` |
| `T-PROJECT-CPU-SHELL` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/cpu/test_cpu_api.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.cpu_api"}` | `REQ-PROJECT-CPU-SHELL` |
| `T-PROJECT-U40-FOUNDATION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/foundation/test_arithmetic40.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.arithmetic40"}` | `REQ-PROJECT-U40-FOUNDATION` |
| `T-PROJECT-OBSERVER-NONINTERFERENCE` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/debug/test_observer.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.observer"}` | `REQ-PROJECT-OBSERVER-NONINTERFERENCE` |
| `T-PROJECT-EVIDENCE-REGISTRY` | PASSING | PROJECT_CONTRACT_ONLY | `tests/contract/quality/test_evidence_registry.py` | `{"kind":"PYTHON_UNITTEST_DISCOVERY","make_goal":"test","ctest_name":"ti_c54x.quality.python"}` | `REQ-PROJECT-EVIDENCE-REGISTRY` |
| `T-PROJECT-ARCH-STATE-KNOWNNESS` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/state/test_arch_state_knownness.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.arch_state_knownness"}` | `REQ-PROJECT-ARCH-STATE-KNOWNNESS` |
| `T-PROJECT-ARCH-ADDRESS-FIELD-ACCESS` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/state/test_arch_state_address_field_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.arch_state_address_field_contract"}` | `REQ-PROJECT-ARCH-ADDRESS-FIELD-ACCESS` |
| `T-PROJECT-ADDRESS-STATE-POLICY` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_address_state_policy_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.address_state_policy_contract"}` | `REQ-PROJECT-ADDRESS-STATE-POLICY` |
| `T-PROJECT-ADDRESS-EFFECT-APPLICATION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_address_effect_apply_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.address_effect_apply_contract"}` | `REQ-PROJECT-ADDRESS-EFFECT-APPLICATION` |
| `T-PROJECT-ADDRESS-EFFECT-STATE-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_address_effect_state_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.address_effect_state_contract"}` | `REQ-PROJECT-ADDRESS-EFFECT-APPLICATION` |
| `T-PROJECT-DATA-OPERAND-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/address/test_data_operand_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.data_operand_contract"}` | `REQ-PROJECT-DATA-OPERAND-CONTRACT` |
| `T-PROJECT-ADDRESS-OPERAND-APPLICATION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_address_operand_apply_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.address_operand_apply_contract"}` | `REQ-PROJECT-ADDRESS-OPERAND-APPLICATION` |
| `T-PROJECT-DP-CPL-FIELD-ACCESS` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/state/test_arch_state_address_field_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.arch_state_address_field_contract"}` | `REQ-PROJECT-DP-CPL-FIELD-ACCESS` |
| `T-PROJECT-DIRECT-SOURCE-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_direct_source_apply_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.direct_source_apply_contract"}` | `REQ-PROJECT-DIRECT-SOURCE-VALIDATION`, `REQ-PROJECT-DIRECT-REJECTION-PRECEDENCE`, `REQ-PROJECT-DIRECT-SOURCE-STATUSES`, `REQ-PROJECT-DIRECT-TRANSACTION` |
| `T-PROJECT-INDIRECT-INDEXED-APPLICATION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_indirect_indexed_effect_apply_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_indexed_effect_apply_contract"}` | `REQ-PROJECT-INDIRECT-INDEXED-APPLICATION` |
| `T-COMMON-INDIRECT-LONG-OFFSET-MOD13` | PASSING | COMMON_TI | `tests/unit/address/test_indirect_long_offset_mod13.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_long_offset_mod13"}` | `REQ-COMMON-INDIRECT-LONG-OFFSET-MOD13-WORD`, `REQ-COMMON-INDIRECT-LONG-OFFSET-MOD13-LONG32` |
| `T-COMMON-INDIRECT-LONG-OFFSET-EFFECT-APPLICATION` | PASSING | COMMON_TI | `tests/unit/sequencer/test_indirect_long_offset_effect_apply.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_long_offset_effect_apply"}` | `REQ-COMMON-INDIRECT-LONG-OFFSET-EFFECT-APPLICATION` |
| `T-PROJECT-INDIRECT-LONG-OFFSET-PLANNER` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/address/test_indirect_long_offset_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_long_offset_contract"}` | `REQ-PROJECT-INDIRECT-LONG-OFFSET-PLANNER` |
| `T-PROJECT-INDIRECT-LONG-OFFSET-APPLICATION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_indirect_long_offset_effect_apply_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_long_offset_effect_apply_contract"}` | `REQ-PROJECT-INDIRECT-LONG-OFFSET-APPLICATION` |
| `T-COMMON-REVERSE-CARRY-PRIMITIVE` | PASSING | COMMON_TI | `tests/unit/foundation/test_reverse_carry.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.reverse_carry"}` | `REQ-COMMON-REVERSE-CARRY-PRIMITIVE` |
| `T-PROJECT-REVERSE-CARRY-PRIMITIVE-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/foundation/test_reverse_carry_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.reverse_carry_contract"}` | `REQ-PROJECT-REVERSE-CARRY-PRIMITIVE-CONTRACT` |
| `T-COMMON-INDIRECT-REVERSE-CARRY-WORD` | PASSING | COMMON_TI | `tests/unit/address/test_indirect_reverse_carry_word.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_reverse_carry_word"}` | `REQ-COMMON-INDIRECT-MOD4-REVERSE-CARRY-WORD`, `REQ-COMMON-INDIRECT-MOD7-REVERSE-CARRY-WORD` |
| `T-PROJECT-INDIRECT-REVERSE-CARRY-PLANNER-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/address/test_indirect_reverse_carry_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_reverse_carry_contract"}` | `REQ-PROJECT-INDIRECT-REVERSE-CARRY-PLANNER-CONTRACT` |
| `T-COMMON-INDIRECT-REVERSE-CARRY-EFFECT-APPLICATION` | PASSING | COMMON_TI | `tests/unit/sequencer/test_indirect_reverse_carry_effect_apply.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_reverse_carry_effect_apply"}` | `REQ-COMMON-INDIRECT-REVERSE-CARRY-EFFECT-APPLICATION` |
| `T-PROJECT-INDIRECT-REVERSE-CARRY-APPLICATION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_indirect_reverse_carry_effect_apply_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_reverse_carry_effect_apply_contract"}` | `REQ-PROJECT-INDIRECT-REVERSE-CARRY-APPLICATION` |
| `T-COMMON-ARCH-STATE-BK-WIDTH` | PASSING | COMMON_TI | `tests/unit/state/test_block_size_register.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.block_size_register"}` | `REQ-COMMON-ARCH-STATE-BK-WIDTH` |
| `T-PROJECT-ARCH-STATE-BK-KNOWNNESS` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/state/test_block_size_register_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.block_size_register_contract"}` | `REQ-PROJECT-ARCH-STATE-BK-KNOWNNESS` |
| `T-COMMON-BK-MMR-ROUTING` | PLANNED | COMMON_TI | `tests/unit/bus/test_bk_mmr.c` | `null` | `REQ-COMMON-BK-MMR-ROUTING` |
| `T-COMMON-INDIRECT-CIRCULAR-BK` | PASSING | COMMON_TI | `tests/unit/address/test_indirect_circular_word.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_circular_word"}` | `REQ-COMMON-INDIRECT-CIRCULAR-BK`, `REQ-COMMON-INDIRECT-CIRCULAR-MOD8-MOD10-WORD16` |
| `T-COMMON-INDIRECT-CIRCULAR-EFFECT-APPLICATION` | PASSING | COMMON_TI | `tests/unit/sequencer/test_indirect_circular_effect_apply.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_circular_effect_apply"}` | `REQ-COMMON-INDIRECT-CIRCULAR-EFFECT-APPLICATION` |
| `T-PROJECT-INDIRECT-CIRCULAR-PLANNER-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/address/test_indirect_circular_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_circular_contract"}` | `REQ-PROJECT-INDIRECT-CIRCULAR-PLANNER-CONTRACT` |
| `T-PROJECT-INDIRECT-CIRCULAR-APPLICATION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_indirect_circular_effect_apply_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.indirect_circular_effect_apply_contract"}` | `REQ-PROJECT-INDIRECT-CIRCULAR-APPLICATION` |
| `T-PROJECT-ADDRESS-EFFECT-VALIDATION-SEAM` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_address_effect_validate_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.address_effect_validate_contract"}` | `REQ-PROJECT-ADDRESS-EFFECT-VALIDATION-SEAM` |
| `T-PROJECT-ADDRESS-EFFECT-DEPENDENCY-ORDER` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_address_effect_dependency_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.address_effect_dependency_contract"}` | `REQ-PROJECT-ADDRESS-EFFECT-VALIDATION-SEAM` |
| `T-PROJECT-MASKED-OPCODE-CATALOG` | PASSING | PROJECT_CONTRACT_ONLY | `tests/contract/quality/isa/catalog/test_opcode_catalog_schema.py` | `{"kind":"PYTHON_UNITTEST_DISCOVERY","make_goal":"test","ctest_name":"ti_c54x.quality.python"}` | `REQ-PROJECT-MASKED-OPCODE-CATALOG` |
| `T-PROJECT-OPCODE-LOOKUP-GENERATION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/contract/quality/isa/catalog/test_opcode_lookup_generator.py` | `{"kind":"PYTHON_UNITTEST_DISCOVERY","make_goal":"test","ctest_name":"ti_c54x.quality.python"}` | `REQ-PROJECT-OPCODE-LOOKUP-GENERATION` |
| `T-PROJECT-OPCODE-CATALOG-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/contract/quality/test_opcode_catalog.py` | `{"kind":"PYTHON_UNITTEST_DISCOVERY","make_goal":"test","ctest_name":"ti_c54x.quality.python"}` | `REQ-PROJECT-OPCODE-CATALOG-CONTRACT` |
| `T-COMMON-CPU-NOP-EXECUTION` | PASSING | COMMON_TI | `tests/unit/cpu/test_cpu_nop_execution.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.cpu_nop_execution"}` | `REQ-COMMON-CPU-NOP-SEQUENTIAL-EXECUTION` |
| `T-PROJECT-CPU-NOP-EXECUTION-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/cpu/test_cpu_nop_execution_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.cpu_nop_execution_contract"}` | `REQ-PROJECT-CPU-PUBLIC-PC-VALIDATION`, `REQ-PROJECT-CPU-FETCH-WORD16`, `REQ-PROJECT-CPU-FETCH-FAILURE-STOP`, `REQ-PROJECT-CPU-UNKNOWN-DECODE-STOP`, `REQ-PROJECT-CPU-NOP-DISPATCH`, `REQ-PROJECT-CPU-PC-COMMIT-ORDER`, `REQ-PROJECT-CPU-PC-BOUNDARIES` |
| `T-PROJECT-CPU-EXECUTION-BUDGET` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/cpu/test_cpu_execution_budget.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.cpu_execution_budget"}` | `REQ-PROJECT-CPU-BUDGET-ZERO`, `REQ-PROJECT-CPU-COUNTER-PHASES`, `REQ-PROJECT-CPU-RUN-RETIREMENT-BUDGET`, `REQ-PROJECT-CPU-COUNTER-OVERFLOW` |
| `T-PROJECT-PROGRAM-PC-BRIDGE` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_program_pc_bridge.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.program_pc_bridge"}` | `REQ-PROJECT-SEQUENCER-PROGRAM-PC-BRIDGE`, `REQ-PROJECT-CPU-PC-BOUNDARIES`, `REQ-PROJECT-SEQUENCER-PC-TRANSACTION` |
| `T-PROJECT-OPERAND-FIELD-EXTRACTION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/isa/decode/test_operand_field.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.operand_field"}` | `REQ-PROJECT-OPERAND-FIELD-EXTRACTION` |
| `T-PROJECT-INSTRUCTION-EXTENSION-FETCH` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/cpu/test_instruction_extension_fetch.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.cpu_instruction_extension_fetch"}` | `REQ-PROJECT-INSTRUCTION-EXTENSION-FETCH` |
| `T-PROJECT-SEQUENCER-MULTIWORD-PC` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/test_program_pc_bridge_multiword.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.program_pc_bridge_multiword"}` | `REQ-PROJECT-SEQUENCER-MULTIWORD-PC` |
| `T-PROJECT-CANONICAL-INSTRUCTION-BUNDLE` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/isa/decode/test_instruction_bundle.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.instruction_bundle"}` | `REQ-PROJECT-CANONICAL-INSTRUCTION-BUNDLE` |
| `T-PROJECT-CATALOG-METADATA-TOKENS` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/isa/catalog/test_opcode_metadata.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.opcode_metadata"}` | `REQ-PROJECT-CATALOG-METADATA-TOKENS` |
| `T-PROJECT-CATALOG-OPERAND-LAYOUT-BINDING` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/isa/decode/test_operand_layout_binding.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.operand_layout_binding"}` | `REQ-PROJECT-CATALOG-OPERAND-LAYOUT-BINDING` |
| `T-PROJECT-RPT-IMMEDIATE-EFFECT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/isa/ops/repeat/test_rpt_immediate_effect.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.rpt_immediate_effect"}` | `REQ-PROJECT-RPT-IMMEDIATE-EFFECT` |
| `T-PROJECT-SINGLE-REPEAT-CARDINALITY-POLICY` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/repeat/test_repeat_cardinality_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.repeat_cardinality_contract"}` | `REQ-PROJECT-SINGLE-REPEAT-CARDINALITY-POLICY` |
| `T-PROJECT-SINGLE-REPEAT-BODY-ADMISSION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/repeat/test_repeat_body_admission_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.repeat_body_admission_contract"}` | `REQ-PROJECT-SINGLE-REPEAT-BODY-ADMISSION` |
| `T-PROJECT-SINGLE-REPEAT-REQUEST-GATE` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/repeat/test_repeat_request_gate_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.repeat_request_gate_contract"}` | `REQ-PROJECT-SINGLE-REPEAT-REQUEST-GATE` |
| `T-PROJECT-BUS-WORD16-REQUEST-SHAPE` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/bus/request/test_word16_request.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.bus_word16_request"}` | `REQ-PROJECT-BUS-WORD16-REQUEST-SHAPE` |
| `T-PROJECT-BUS-WORD16-BYTE-ORDER-CODEC` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/bus/byte_order/test_word16_codec.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.bus_word16_codec"}` | `REQ-PROJECT-BUS-WORD16-BYTE-ORDER-CODEC` |
| `T-COMMON-RPT-IMMEDIATE-DECODE` | PLANNED | COMMON_TI | `tests/unit/isa/decode/test_rpt_immediate_decode.c` | `null` | `REQ-COMMON-RPT-IMMEDIATE-DECODE` |
| `T-COMMON-RPT-IMMEDIATE-OPERATION` | PLANNED | COMMON_TI | `tests/unit/isa/ops/repeat/test_rpt_immediate.c` | `null` | `REQ-COMMON-RPT-IMMEDIATE-OPERATION` |
| `T-COMMON-SINGLE-REPEAT-CARDINALITY` | PLANNED | COMMON_TI | `tests/unit/sequencer/repeat/test_single_repeat_cardinality.c` | `null` | `REQ-COMMON-SINGLE-REPEAT-CARDINALITY` |
| `T-COMMON-SINGLE-REPEAT-IRQ-GATE` | PLANNED | COMMON_TI | `tests/unit/sequencer/repeat/test_single_repeat_irq_gate.c` | `null` | `REQ-COMMON-SINGLE-REPEAT-IRQ-GATE` |
| `T-COMMON-SINGLE-REPEAT-PENDING-REQUEST` | PLANNED | COMMON_TI | `tests/unit/sequencer/interrupt/test_repeat_pending_request.c` | `null` | `REQ-COMMON-SINGLE-REPEAT-PENDING-REQUEST` |
| `T-COMMON-SINGLE-REPEAT-BODY-ELIGIBILITY` | PLANNED | COMMON_TI | `tests/unit/sequencer/repeat/test_single_repeat_body_eligibility.c` | `null` | `REQ-COMMON-SINGLE-REPEAT-BODY-ELIGIBILITY` |
| `T-PROJECT-BUS-WORD16-TRANSACTION-LIFECYCLE` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/bus/transaction/test_word16_transaction.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.bus_word16_transaction"}` | `REQ-PROJECT-BUS-WORD16-TRANSACTION-LIFECYCLE` |
| `T-PROJECT-BUS-WORD16-TRANSACTION-VALIDATION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/bus/transaction/test_word16_transaction_contract.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.bus_word16_transaction_contract"}` | `REQ-PROJECT-BUS-WORD16-TRANSACTION-VALIDATION` |
| `T-PROJECT-BUS-WORD16-EXTERNAL-EXACTLY-ONCE` | PLANNED | PROJECT_CONTRACT_ONLY | `tests/unit/bus/adapter/test_word16_exactly_once.c` | `null` | `REQ-PROJECT-BUS-WORD16-EXTERNAL-EXACTLY-ONCE` |
| `T-COMMON-LD-STATUS-FIELD-DECODE` | PASSING | COMMON_TI | `tests/unit/isa/decode/test_ld_status_fields.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.ld_status_fields_decode"}` | `REQ-COMMON-LD-K9-DP-IMMEDIATE`, `REQ-COMMON-LD-K5-ASM-IMMEDIATE`, `REQ-COMMON-LD-K3-ARP-IMMEDIATE` |
| `T-COMMON-LD-STATUS-FIELD-OPS` | PASSING | COMMON_TI | `tests/unit/isa/ops/load/test_ld_status_fields.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.ld_status_fields_ops"}` | `REQ-COMMON-LD-STATUS-FIELD-EFFECTS` |
| `T-PROJECT-STATUS-REGISTER-CONTRACT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/state/status/test_status_registers.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.status_registers"}` | `REQ-PROJECT-LD-STATUS-REGISTER-CONTRACT` |
| `T-PROJECT-STATUS-FIELD-EFFECT-APPLICATION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/status/test_status_field_effect_apply.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.status_field_effect_apply"}` | `REQ-PROJECT-LD-STATUS-FIELD-EFFECT-TRANSACTION` |
| `T-PROJECT-CPU-LD-STATUS-FIELDS` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/cpu/test_cpu_ld_status_fields.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.cpu_ld_status_fields"}` | `REQ-PROJECT-LD-CPU-STATE-OBSERVATION-RETIRE` |
| `T-COMMON-MAX-MIN-DECODE` | PASSING | COMMON_TI | `tests/unit/isa/decode/test_max_min_decode.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.max_min_decode"}` | `REQ-COMMON-MAX-MIN-DECODE` |
| `T-COMMON-MAX-MIN-OPS` | PASSING | COMMON_TI | `tests/unit/isa/ops/compare/test_max_min.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.max_min_compare"}` | `REQ-COMMON-MAX-MIN-SELECTION-EFFECTS` |
| `T-PROJECT-MAX-MIN-EFFECT-TRANSACTION` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/accumulator/test_accumulator_write_effect.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.accumulator_write_effect"}` | `REQ-PROJECT-MAX-MIN-ACCUMULATOR-EFFECT-TRANSACTION` |
| `T-PROJECT-MAX-MIN-ACCUMULATOR-STATE` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/state/accumulator/test_accumulator_state.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.accumulator_state"}` | `REQ-PROJECT-MAX-MIN-ACCUMULATOR-STATE` |
| `T-PROJECT-CPU-MAX-MIN` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/cpu/test_cpu_max_min.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.cpu_max_min"}` | `REQ-PROJECT-MAX-MIN-CPU-RETIREMENT` |
| `T-PROJECT-ACCUMULATOR-WRITE-EFFECT` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/sequencer/accumulator/test_accumulator_write_effect.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.accumulator_write_effect"}` | `REQ-PROJECT-ACCUMULATOR-WRITE-EFFECT` |
| `T-COMMON-CMPL-DECODE` | PASSING | COMMON_TI | `tests/unit/isa/decode/test_cmpl_decode.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.cmpl_decode"}` | `REQ-COMMON-CMPL-DECODE` |
| `T-COMMON-CMPL-OPS` | PASSING | COMMON_TI | `tests/unit/isa/ops/accumulator/test_cmpl.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.cmpl"}` | `REQ-COMMON-CMPL-EFFECTS` |
| `T-PROJECT-CPU-CMPL` | PASSING | PROJECT_CONTRACT_ONLY | `tests/unit/cpu/test_cpu_cmpl.c` | `{"kind":"C_EXECUTABLE","make_goal":"test","ctest_name":"ti_c54x.cpu_cmpl"}` | `REQ-PROJECT-CMPL-CPU-RETIREMENT` |

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
