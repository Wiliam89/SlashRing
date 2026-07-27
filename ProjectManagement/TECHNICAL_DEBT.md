#TECHNICAL DEBT

Version: 1.0

Status: Active

Purpose

This document records all known technical issues identified during code audits.

Its purpose is to prevent technical debt from being forgotten during future Sprints.

Only the Chief Software Architect may classify or reprioritize technical debt.

Resolved items must remain documented and be marked as Resolved.

Priority Levels
P0 — Critical

Must be resolved before continuing the affected Sprint.

P1 — High

Should be resolved during the current Sprint.

P2 — Medium

Can be scheduled in a future Sprint without compromising the architecture.

P3 — Low

Improvements, refactoring, style consistency or optimizations.

Current Technical Debt
P0
TD-001

Title: Oversampling signal routing

Status: Resolved

Description:

The current PluginProcessor signal routing must be audited and corrected so that the oversampled processing path complies with ARCHITECTURE.md.

Affected modules:

PluginProcessor
OverdriveModule
AmpModule

Validation:

Resolved 2026-07-12. Build result: 4 succeeded, 0 failed, 0 up-to-date, 0 skipped. Real-guitar audio validation performed via standalone build (Windows Audio, Microphone USB PnP Sound Device input, PC speakers output). Amp processing confirmed audible with Overdrive disabled (base amp tone carries audible drive/saturation); enabling Overdrive produces a clearly more aggressive, heavily saturated tone consistent with the intended signal path. Validated and closed by the Chief Software Architect.

TD-002

Title: Processing domain separation

Status: Resolved

Description:

The DSP preparation must use independent ProcessSpec configurations for:

Base sample rate:
InputStage
CabinetModule
DelayModule
ReverbModule
OutputStage
Oversampled domain:
OverdriveModule
AmpModule

Validation:

Resolved 2026-07-12. Build result: 4 succeeded, 0 failed, 0 up-to-date, 0 skipped. Real-guitar audio validation performed via standalone build (Windows Audio, Microphone USB PnP Sound Device input, PC speakers output). Delay confirmed audible via short-note/mute test with clearly present repetitions; Overdrive + Delay combination produced a tone subjectively consistent with the intended Slash-style character, supporting correct domain separation between the oversampled and base-rate processing paths. Validated and closed by the Chief Software Architect.
P1
TD-003

Title: Parameter smoothing consistency

Status: Resolved

Description:

All SmoothedValue instances must advance once per sample, never once per channel.

Affected modules:

OverdriveModule
AmpModule

Validation:

Resolved 2026-07-15. Repository-wide SmoothedValue/getNextValue audit performed across AmpModule, OverdriveModule, DelayModule, InputStage, and OutputStage. AmpModule contained a confirmed channel-dependent smoother advancement defect caused by channel-outer/sample-inner processing and was corrected to sample-outer/channel-inner processing, ensuring gainSmoothed and masterSmoothed advance exactly once per temporal sample and share the same instantaneous value across all active channels. OverdriveModule and DelayModule were verified as already compliant with sample-synchronous smoothing. InputStage and OutputStage had been previously audited and remain compliant. Post-correction Debug x64 build completed successfully with 0 warnings and 0 errors, including SharedCode, Standalone, and VST3 targets. Validated and closed by the Chief Software Architect.
TD-004

Title: DSP lifecycle completion

Status: Open

Description:

Every DSP module shall implement the complete lifecycle defined by ARCHITECTURE.md:

prepare()

reset()

process()

release()

TD-005

Title: Professional bypass implementation

Status: Open

Description:

Module bypass behavior shall follow the professional architecture specification and avoid parameter-based bypass hacks.

Disabled modules must not rely on neutral parameter values to simulate bypass.

Affected modules:

OverdriveModule
DelayModule
ReverbModule
TD-006

Title: Reverb parameter smoothing

Status: Open

TD-007

Title: Tail length implementation

Status: Open

TD-008

Title: Crossfade bypass transitions

Status: Open

TD-009

Title: Reverb denormal protection

Status: Open

Description:

ReverbModule::process() does not currently implement denormal protection.

All DSP modules must comply with the real-time processing rules defined in ARCHITECTURE.md.

Affected modules:

ReverbModule

P3

Reserved for future improvements.

New Items — Opened 2026-07-12 (Real-Guitar Validation Session)

P1
TD-010

Title: Audible background noise in Amp / Overdrive chain

Status: Open

Classification: P1, confirmed by Chief Software Architect on 2026-07-12.

Description:

Real-guitar testing identified audible background noise in the Amp processing chain, which increases significantly when Overdrive is enabled. No root cause has been identified or assigned at this time. This item must not be classified as a Cabinet defect, or attributed to any other specific module, without further technical evidence. Requires a dedicated audit.

Affected modules:

Under investigation. Observed with AmpModule and OverdriveModule active.

Correction (2026-07-12, controlled overdrive_drive sweep — supersedes any prior informal reading of this data):

- At overdrive_drive = 0, the Overdrive effectively loses its characteristic push/boost.
- At overdrive_drive = 0, a lower level of noise is still audible, associated with the Amp already producing some drive on its own.
- The noise is noticeably lower at overdrive_drive = 0 than at higher settings.
- From approximately overdrive_drive = 25 onward, the strong noise returns and is present.
- Increasing drive beyond ~25 clearly changes the power/aggressiveness of the Overdrive, but the strong noise is already present from approximately 25 and does not scale much further with it.
- The noise becomes less perceptible while actively playing, but is strongly noticeable around harmonics.
- On lower notes, the noise can contribute to a slightly fizzy / bee-like character.
- Disabling Cabinet shifts the tone toward a more fizzy / bee-like character but does not remove the underlying noise.

The previous conclusion that overdrive_drive scaling is not materially related to TD-010 is retracted. overdrive_drive is materially related to TD-010, specifically through the transition observed between 0 and ~25.

Status: remains Open, P1.

Protocol correction (2026-07-12): Future technical debt findings may be proposed by the AI Software Engineer, but classification and prioritization remain pending until approved by the Chief Software Architect, per this document's authority rule ("Only the Chief Software Architect may classify or reprioritize technical debt"). TD-010, TD-011, and TD-012 were proposed 2026-07-12 and classified/confirmed by the Chief Software Architect the same day.

P2
TD-011

Title: Reverb audibility unclear pending dedicated audit

Status: Open

Classification: P2, assigned by Chief Software Architect on 2026-07-12. Does not block Sprint 05.

Description:

A/B testing with a short-note and mute test could not clearly distinguish Reverb enabled from Reverb disabled; the audible difference is currently very subtle or unclear. This is recorded as an observation only. ReverbModule must not be modified until a dedicated audio audit is performed.

Affected modules:

ReverbModule

P2
TD-012

Title: UI knobs missing visible parameter labels

Status: Open

Classification: P2, assigned by Chief Software Architect on 2026-07-12. Definitive correction remains within the UI development scope.

Description:

Current UI knobs display numeric values but have no visible parameter labels, making manual parameter identification difficult during testing and use.

Affected modules:

PluginEditor

End of Technical Debt