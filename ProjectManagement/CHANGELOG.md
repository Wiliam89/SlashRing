# CHANGELOG

Version 2.0

Project migrated to AI Assisted Engineering.

New documentation created.

Sprint workflow adopted.

Commercial development process established.

---

2026-07-12

TD-001 (Oversampling signal routing) resolved and validated via build verification and real-guitar audio testing.

TD-002 (Processing domain separation) resolved and validated via build verification and real-guitar audio testing.

New open items recorded pending dedicated audits: TD-010 (Amp/Overdrive chain noise, root cause unassigned), TD-011 (Reverb audibility unclear), TD-012 (UI knobs missing parameter labels).

Confirmed TD-005 and TD-009 preserved unchanged (both remain Open) from the authoritative baseline; no prior approved records were overwritten.

Chief Software Architect classified: TD-010 = P1, TD-011 = P2 (does not block Sprint 05), TD-012 = P2 (UI development scope).

---

2026-07-12 (structure correction)

TD-009 relocated inside the TECHNICAL_DEBT.md document body (after TD-008, before the P3 section); content and Open status unchanged. Removed the resulting duplicate "End of Technical Debt" marker so it appears once, at the true end of the file. No record removed or duplicated.

---

2026-07-12 (TD-010 evidence correction)

Product Owner corrected the controlled overdrive_drive sweep interpretation for TD-010. Prior conclusion that overdrive_drive scaling is not materially related to TD-010 is retracted. Corrected evidence: noise is lower at overdrive_drive = 0 (residual noise attributed to Amp's own drive), strong noise returns from approximately overdrive_drive = 25 onward and does not scale materially further; noise is most noticeable around harmonics and contributes to fizzy/bee-like character on lower notes; disabling Cabinet changes tone but does not remove the underlying noise. TD-010 remains Open, P1. TECHNICAL_DEBT.md and SESSIONS2.md updated accordingly. A revised read-only root-cause audit was performed; no source code, DSP, or Noise Gate changes were made.