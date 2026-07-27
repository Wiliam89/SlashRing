# AI CONTEXT

Project:
SlashRing Professional Guitar Suite

Chief Software Architect:
ChatGPT

AI Implementation Engineer:
Cline

Version:
1.0

--------------------------------------------------

PROJECT PURPOSE

SlashRing is a professional commercial guitar plugin developed in JUCE.

The objective is commercial quality comparable to:

- Neural DSP
- Softube
- IK Multimedia
- STL Tones
- Mercuriall

This project is NOT a prototype.

Every implementation must be production quality.

--------------------------------------------------

DEVELOPMENT RULES

Always:

• Respect ARCHITECTURE.md
• Respect PRODUCT_SPECIFICATION.md
• Respect TECHNICAL_DEBT.md
• Respect CURRENT_STATE.md

Never:

• Change architecture without authorization.

• Modify DSP outside the requested scope.

• Refactor unrelated files.

• Remove smoothing.

• Change oversampling routing.

• Replace existing DSP with simplified code.

• Introduce temporary hacks.

--------------------------------------------------

OFFICIAL WORKFLOW

1 Audit

↓

2 Plan

↓

3 Modify only requested files

↓

4 Compile

↓

5 Stop

↓

6 Wait for real guitar validation

↓

7 Update documentation

--------------------------------------------------

OUTPUT RULES

Every implementation must contain:

Files modified

Reason

Technical explanation

Expected behavior

Possible risks

Compilation status

Never continue implementing after compilation without validation.