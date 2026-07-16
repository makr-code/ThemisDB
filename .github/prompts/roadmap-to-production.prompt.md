---
name: "Roadmap To Production"
description: "Implement a Roadmap or Future Enhancement item into production code with tests and documentation updates."
argument-hint: "Paste roadmap checkbox, milestone, or issue reference"
agent: "themisdb-implementer"
---

Implement the requested roadmap item using a roadmap-first delivery flow.

## Inputs

- Task reference: ${input}
- Optional constraints: target branch, non-goals, timebox

## Required Steps

1. Read the relevant sections in ROADMAP.md and FUTURE_ENHANCEMENTS.md.
2. Derive explicit acceptance criteria before coding.
3. Identify impacted files, symbols, and tests.
4. Implement production logic with minimal diff.
5. Add or update focused tests that verify behavior.
6. Update docs for public API or behavior changes.
7. Build and run focused validation, then summarize results.

## Output Requirements

Return:

1. Acceptance criteria used
2. Code changes made
3. Tests added or updated
4. Build and test commands run with outcomes
5. Open risks or follow-up items
