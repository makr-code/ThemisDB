---
description: "Use when implementing, refactoring, or debugging ThemisDB features with AI. Enforces roadmap-first delivery, branch governance, build/test verification, and documentation sync."
---

# ThemisDB AI Delivery Contract

Use this instruction for production-facing implementation work in this repository.

## Scope Gate

- Treat ROADMAP.md and FUTURE_ENHANCEMENTS.md as binding acceptance context for feature work.
- If requirements are ambiguous, refine acceptance criteria before coding.
- Keep changes minimal, targeted, and reversible.

## Branch And Release Governance

- Default implementation target is develop.
- Use community or military only when explicitly requested.
- Never propose legacy branch names main or millitary.

## Execution Workflow

1. Explore
- Identify affected modules, symbols, and tests before editing.
- For C/C++ symbol-impact work, use semantic symbol tooling instead of text-only grep.

2. Plan
- For non-trivial work, write a short plan in ai_working/ with acceptance checks.

3. Execute
- Implement production logic, not TODO-only placeholders.
- Avoid introducing legacy, stub, mock, or simulation paths unless explicitly human-approved.

4. Verify
- Build and run focused tests first.
- If available, use workspace CMake/CTest tasks instead of ad-hoc command variants.
- Report exactly what was built and tested.

5. Document
- Update Doxygen/API docs for public C++ API changes in the same change.
- Keep architecture and behavior docs aligned with code changes.

## Quality Gates

- No silent behavior changes without tests.
- No broad refactors outside task scope.
- No unrelated formatting churn.
- Include risk notes for edge cases and rollback considerations when relevant.

## Response Contract

When completing work, provide:

1. What changed
2. Why this approach
3. Verification performed
4. Remaining risks or follow-ups
