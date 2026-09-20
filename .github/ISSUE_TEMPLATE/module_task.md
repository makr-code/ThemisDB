---
name: Module Task
about: Create a focused module implementation or documentation task with the repository's governance and evidence contract.
title: "[module][task] <module-name>: <short task summary>"
labels: ["module", "ai-task"]
assignees: []
---

## Scope
- Module: <module-name>
- Files: <paths>
- Status: <active|in-progress|blocked>
- Labels: maintenance, soll-ist
- Milestone: backlog

## Source Evidence
- Module docs: <README/ARCHITECTURE/ROADMAP path>
- Source graph / symbol evidence: <path>
- Doxygen artifact: <path>
- Compliance report: <path>

## What needs to be done
- <clear implementation task>
- <clear documentation task, if relevant>

## Validation
- Repro command: <command>
- Gate check: <command>
- Success condition: <one sentence>

## Acceptance criteria
- [ ] <criterion 1>
- [ ] <criterion 2>
- [ ] <criterion 3>

## Notes
- Risk/constraint: <brief note>
- Ownership/path rule: <include/src/tests/benchmarks/external>
- Follow the module quick rules in [.github/copilot/module-quick-rules.md](../copilot/module-quick-rules.md) and the detailed guidance in [.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md](../MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md).
- Use the same module issue labels and milestone as the automated Soll-Ist sync, unless a maintainer explicitly overrides them.
