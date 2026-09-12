# DSP reconstruction hypothesis and evidence ledger

Status: active; no sustained, correlated downlink-speech result has been proven.

## Ownership and boundary

This append-only ledger owns hypotheses, experiments, and decisions that guide the
standalone C54x reconstruction toward executing the Nokia 5110 ROM4/ROM6 DSP
workload. It complements `docs/implementation-plan.md`; completed implementation
work remains recorded in `docs/development-ledger.md` using the required
implementation-commit/ledger-commit sequence.

Observations from the Nokia emulator or physical hardware are black-box behavioral
evidence only. They must not introduce Nokia, QEMU, Osmocom, firmware, host-app, or
legacy-core dependencies into this library, and no source, opcode table, comment,
test, or implementation structure may be copied from the legacy interpreter.
Emulator integration remains deferred until provenance and standalone contracts are
stable.

## Reconstruction invariants

- ROM4 and ROM6 behavior is reconstructed from independently stated requirements,
  TI primary documentation, physical-hardware observations where available, and
  reproducible black-box traces.
- The eventual HLE DSP is stateful and advances at a nominal 13 MHz DSP tick. Work
  is retained in instance-owned state and divided into bounded tick-sized steps;
  no tick may trigger an unbounded or monolithic decode/render operation.
- The 13 MHz target is an integration requirement, not permission to expose a
  machine-cycle API before timing behavior has requirement evidence and executable
  tests. Until then, this standalone library continues to use instruction budgets.
- BTS frame number and phase are immutable external time. The DSP may consume that
  timeline but must never offset, reset, pause, rebase, or make BTS time follow DSP
  progress.
- No acceptance result may be manufactured by forcing firmware RAM, publishing a
  synthetic firmware verdict, or injecting already-decoded PCM.
- All mutable execution state is instance-owned. Core execution performs no
  environment reads, file I/O, allocation, or logging.

## Acceptance evidence

For the speech path, completion requires sustained playback of a versioned GSM-FR
fixture through the resident receive, channel-decode, vocoder, and audio-output
path. Nonzero or audible PCM is insufficient. The captured PCM must correlate at
least 0.75 with an independently decoded reference, with an aligned silence fixture
as the negative control. The exact revision, dirty state, command, artifacts,
hashes, timing coordinates, and first divergence must be recorded.

Historical emulator runs are state-transition oracles only. The operator has never
personally verified that downlink speech decoded correctly in any revision, so no
old commit is a positive audio baseline.

## H1 — Host audio output is the primary speech failure

**Status: narrowed; not sufficient.**

Keypad tones reach host PCM, proving that the firmware-to-codec-to-host sink can
carry audio. Separately, GUI runs without host audio acquisition can hang during
audio release and may require another application to hold the host device open.
That lifecycle defect affects both C54x and HLE runs, so it must be fixed, but it
does not explain why the injected GSM-FR fixture fails correlation.

**Decision:** keep host-device acquisition/release diagnosis separate from RF
speech decode. A clean GUI exit or audible keypad tone is not speech acceptance.

## H2 — The displayed `Car` profile proves a car-kit route is selected

**Status: disproven in the emulator investigation.**

The displayed label is profile index zero rather than proof of a connected
accessory. The observed accessory ADC state represents an open circuit, the
modeled `accessory_present` state is false, and the observed DSP route cell has the
alternate-route bit clear.

**Decision:** do not force accessory or profile state in a production or acceptance
run. Reopen this hypothesis only if physical accessory input and DSP route state
contradict those observations in the same aligned trace.

## H3 — DSP cell `0x085D.bit1` is the missing RF speech gate

**Status: disproven in the emulator investigation.**

Temporary A/B forces at two different times advanced the associated buffer path but
did not produce fixture-correlated PCM. Static and dynamic observations instead
associate this cell with a generic MCU-streamed audio overlay, not the established
RF channel-decoder/vocoder route.

**Decision:** do not synthesize the related MCU audio commands or use this cell as
RF-speech acceptance. Trace the genuine receive path to the authoritative audio
output boundary.

## H4 — Fixture data is lost before equalization or deinterleaving

**Status: disproven by aligned black-box traces.**

At the same immutable RF event, fish and silence inputs produced different hashes
in both the equalizer workspace and the first observed deinterleaver entry. This
proves fixture-dependent information reaches those stages. It does not prove that a
traffic frame passes channel decode, reaches the vocoder, or reaches the DSP audio
port.

**Decision:** the next discriminator is downstream of deinterleaving. Preserve the
same BTS frame number and phase for fish and silence controls.

## H5 — The current resident path accepts and emits decoded speech

**Status: open; current evidence indicates repeated short audio-service starts, not
sustained correlated speech.**

Observed resident activity enters the audio-mode setup repeatedly across the call,
but captured PCM remains uncorrelated with the fixture. The unresolved boundary is
one of:

1. channel decode or CRC rejects the reconstructed TCH/F diagonal;
2. an accepted speech frame is not handed to or committed by the vocoder; or
3. valid vocoder output does not reach the authoritative DSP audio-port write.

**Prediction:** an aligned fish/silence trace at each boundary will show the first
stage whose accepted state or payload fails to propagate.

**Next experiment:** record bounded, read-only checkpoints for channel-decode
return/status, speech-frame handoff, vocoder entry/output commit, and the
authoritative audio-port value. Compare fish and silence using the identical call
timeline and immutable BTS time. Convert the first proven architectural mismatch
into an independent requirement and executable standalone test before changing the
library.

## Entry template

```md
## H<number> — hypothesis

**Status:** open / proven / disproven / narrowed.

**Requirement/evidence:** independent requirement identifier and provenance.

**Prediction:** one causal, falsifiable outcome.

**Experiment:** exact revision, dirty state, command, control, and one changed
variable.

**Observed:** ordered instruction/tick, architectural state, external timing,
payload metrics, and first divergence.

**Artifacts:** paths and SHA-256 hashes.

**Decision:** what this proves and explicitly does not prove.

**Next experiment:** the smallest remaining discriminator and its owner.
```
