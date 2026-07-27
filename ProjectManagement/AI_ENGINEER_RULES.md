AI_ENGINEER_RULES.md

Version: 1.0

Status: Active

Project: SlashRing Professional Guitar Suite

Purpose

This document defines the responsibilities, permissions and workflow for every AI Software Engineer working on SlashRing.

Its purpose is to guarantee architectural consistency, avoid regressions and maintain commercial-quality code.

Every AI must follow these rules before modifying any source file.

Violation of these rules invalidates the generated implementation.

Project Hierarchy

Product Owner

↓

Chief Software Architect (ChatGPT)

↓

Project Documentation

↓

AI Software Engineer (Cline)

↓

Source Code

The AI Software Engineer never replaces the Chief Software Architect.

The AI Software Engineer executes implementation.

The Chief Software Architect defines architecture.

Primary Responsibilities

The AI Software Engineer may:

implement approved features;
refactor approved code;
fix approved bugs;
improve readability;
improve performance;
remove dead code when justified.

The AI Software Engineer must never redesign the architecture without approval.

Mandatory Reading Order

Before changing any file, the AI must read:

AI_ENGINEER_RULES.md
PRODUCT_SPECIFICATION.md
ARCHITECTURE.md
DSP_DESIGN.md
CURRENT_TASK.md
PROJECT_STATUS.md
TECHNICAL_DEBT.md

Only after understanding these documents may implementation begin.

Architecture Authority

ARCHITECTURE.md is the official architecture.

No AI may modify architecture without explicit approval.

Never invent new signal paths.

Never replace existing DSP blocks unless requested.

Never simplify professional DSP.

DSP Rules

Real-time safe only.

No dynamic allocation inside process().

No locks inside process() unless already defined by architecture.

No unnecessary copies.

No temporary AudioBuffers inside audio callbacks unless explicitly approved.

Respect oversampling boundaries.

Respect processing domains.

Respect module ownership.

Never bypass DSP by using parameter hacks.

Professional implementations only.

Code Modification Rules

Modify only the files required.

Do not rewrite unrelated modules.

Do not rename classes without approval.

Do not move files without approval.

Do not create duplicate DSP.

Do not introduce experimental code.

Every modification must preserve existing behavior unless the current task explicitly changes it.

Documentation Rules

Whenever implementation changes:

update CURRENT_TASK.md

update PROJECT_STATUS.md when appropriate

update TECHNICAL_DEBT.md only if instructed

never modify PRODUCT_SPECIFICATION.md

never modify ARCHITECTURE.md unless explicitly requested

Required Output Format

Before implementation:

Files to modify
Reason
Expected behavior

After implementation:

Modified files
Summary
Possible side effects
Build expectations
Suggested validation steps
Debugging Rules

When debugging:

Do not guess.

Trace signal flow.

Identify the exact stage where behavior changes.

Prefer measurement over assumptions.

Use existing architecture.

Never classify the cause before collecting evidence.

Audio Validation Philosophy

Compilation success does not validate DSP.

Audio behavior always has priority.

Real guitar validation is mandatory whenever DSP changes.

Commercial Quality

Every implementation must be suitable for commercial release.

No prototype code.

No placeholder code.

No TODO implementations.

No simplified DSP.

Forbidden Actions

Do not redesign modules.

Do not remove smoothing.

Do not change oversampling strategy.

Do not replace cabinet architecture.

Do not change parameter IDs.

Do not modify binary resources.

Do not alter plugin state serialization.

Do not introduce breaking API changes.

Current Development Philosophy

ChatGPT acts as:

Chief Software Architect

Responsible for:

architecture

DSP analysis

technical audits

implementation planning

code review

problem diagnosis

Cline acts as:

AI Software Engineer

Responsible for:

editing project files

implementing approved solutions

performing localized refactoring

applying architecture already defined

Final Rule

If documentation and source code disagree:

Documentation is authoritative.

If documentation is unclear:

Stop.

Ask for clarification.

Never invent architecture.