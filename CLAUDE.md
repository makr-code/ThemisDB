# CLAUDE.md

Project: ThemisDB

This file defines the working contract for Claude Code in this repository.

## Wiki Usage (MUST)

- Use the repository wiki as required context before implementation or refactoring.
- Review relevant wiki pages first (architecture, operations, module, governance).
- Keep corresponding wiki content synchronized when architecture/process/governance behavior changes.
- If wiki coverage is missing, report the gap explicitly in working notes or PR context.

## Core Workflow (Explore -> Plan -> Execute -> Commit)

1. Explore
- Read relevant headers, architecture docs, and roadmap entries before changing code.
- Use semantic symbol navigation for C/C++ impact checks.
- Identify call sites, overloads, and ABI-sensitive touch points before edits.

2. Plan
- Write a short implementation plan in ai_working/ before non-trivial changes.
- Include affected files, acceptance criteria, and test scope.
- Do not start implementation until the plan is coherent.

3. Execute
- Implement the smallest safe change set.
- Follow RAII and modern C++ practices; avoid raw pointers in public APIs.
- Update public API documentation when behavior or signatures change.

4. Commit / PR
- Summarize what changed, why this approach was selected, and what was tested.
- Open AI-driven pull requests as Draft by default.
- Require human review before merge.

## Build, Test, Docs

Preferred Windows preset flow:
- Configure: cmake --preset windows-release
- Build: cmake --build --preset windows-release --parallel 16
- Test: ctest --preset windows-release --output-on-failure -j 1 --timeout 60
- Docs: doxygen Doxyfile.audit

## C++ Rules

- Use RAII and ownership-safe interfaces.
- Prefer const-correctness and clear, boring code over clever abstractions.
- Keep functions focused; avoid hidden side effects.
- Do not introduce stubs as final production output.
- If temporary simulation/stub paths are needed, document purpose, activation, and removal plan.
- Do not introduce legacy/compatibility paths by default.
- Legacy paths are only allowed with explicit human approval and must be clearly marked with reason, activation conditions, behavior delta, approver reference, and removal target.
- Simulation/Stub/Mockup paths are forbidden without explicit human approval and explicit human marking.

## Documentation Rules

- Every new or modified public C++ API requires Doxygen comments.
- Doxygen must cover purpose, parameters, return behavior, and failure/edge cases.
- Keep docs aligned with implementation changes in the same PR.

## Symbol-First Refactoring Rule

For C/C++ refactoring, always use semantic symbol tools (references/call hierarchy) instead of text-only search.
This is mandatory for signature changes, renames, and cross-module edits.

## Simplicity Contract

- Reject unnecessary architecture expansion.
- Prefer explicit, readable control flow.
- Only optimize after evidence or profiling.

---
Zuletzt geprueft (Root-Sync): 2026-05-26
