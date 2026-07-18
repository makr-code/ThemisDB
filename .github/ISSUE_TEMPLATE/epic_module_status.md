---
name: Epic - Module Development Status
about: Track development status per module with linked sub-issues
title: "[EPIC][STATUS][MODULES] "
labels: ["epic", "status:open", "priority:medium"]
assignees: []
---

## Epic Summary

Document and track the current development status for each ThemisDB module.

## Goal

- Establish one source of tracking for module-level progress.
- Link one sub-issue per module.
- Keep roadmap, implementation status, and verification evidence aligned.

## Epic Metadata

- Tracking Date: `<YYYY-MM-DD>`
- Release Lane: `<develop | edition-specific>`
- Epic Branch (optional): `<epic/...>`
- Owner: `<team or handle>`

## Scope

- Branch: `develop` (unless explicitly edition-specific).
- Source files per module:
  - `ROADMAP.md`
  - `FUTURE_ENHANCEMENTS.md`
  - Relevant module tests and build targets

## Sub-Issue Conventions

- One module status issue per module in this epic.
- A sub-issue should have exactly one parent epic.
- If work is reused across epics, cross-link instead of re-parenting.
- Recommended sub-issue title:
  - `[module:<module-name>] Development Status <YYYY-MM-DD>`
- Recommended labels for each sub-issue:
  - `area:<module>`
  - `type:enhancement` or `type:feature`
  - `status:open` (or current status)
  - `priority:<level>`

## Definition of Done (Epic)

- [ ] Every module has exactly one linked sub-issue.
- [ ] Each sub-issue documents: current status, in-progress work, blockers, next milestone.
- [ ] Each sub-issue contains verification evidence (build/tests) or an explicit gap note.
- [ ] Each sub-issue references module-specific roadmap and future-enhancements paths.
- [ ] Status labels and checklist state are synchronized before closing.
- [ ] Epic task list is fully linked and up to date.

## Module Status Matrix

<!--
Replace rows with real module data.
Keep module naming stable and path-based where possible.
-->

| Module | Canonical Paths | Sub-Issue | Status | Last Validation |
|---|---|---|---|---|
| api | `src/api/ROADMAP.md` + `src/api/FUTURE_ENHANCEMENTS.md` | #<issue-number> | `[ ] / [~] / [x] / [?]` | `<YYYY-MM-DD>` |
| storage | `src/storage/ROADMAP.md` + `src/storage/FUTURE_ENHANCEMENTS.md` | #<issue-number> | `[ ] / [~] / [x] / [?]` | `<YYYY-MM-DD>` |
| sharding | `src/sharding/ROADMAP.md` + `src/sharding/FUTURE_ENHANCEMENTS.md` | #<issue-number> | `[ ] / [~] / [x] / [?]` | `<YYYY-MM-DD>` |
| llm | `src/llm/ROADMAP.md` + `src/llm/FUTURE_ENHANCEMENTS.md` | #<issue-number> | `[ ] / [~] / [x] / [?]` | `<YYYY-MM-DD>` |
| server | `src/server/ROADMAP.md` + `src/server/FUTURE_ENHANCEMENTS.md` | #<issue-number> | `[ ] / [~] / [x] / [?]` | `<YYYY-MM-DD>` |
| replication | `src/replication/ROADMAP.md` + `src/replication/FUTURE_ENHANCEMENTS.md` | #<issue-number> | `[ ] / [~] / [x] / [?]` | `<YYYY-MM-DD>` |

## Sub-Issue Template (copy into each module sub-issue)

### Module Identity

- Module: `<module-name>`
- Area Label: `area:<module-name>`
- Roadmap Path: `<path/to/ROADMAP.md>`
- Future Path: `<path/to/FUTURE_ENHANCEMENTS.md>`

### Current Status

- Status: `[ ] open` `[~] in progress` `[x] done` `[?] blocked`
- Last validated: `<YYYY-MM-DD>`
- Roadmap progress: `<short summary>`
- Implementation coverage: `<short summary>`

### Implementation Phases Snapshot

- [ ] Phase 1: Design / API Contract
- [ ] Phase 2: Core Implementation
- [ ] Phase 3: Error Handling and Edge Cases
- [ ] Phase 4: Tests
- [ ] Phase 5: Performance and Hardening
- [ ] Phase 6: Documentation and Acceptance

### Evidence

- Build preset: `<e.g. windows-release>`
- Build target(s): `<target names>`
- Test target(s): `<target names>`
- Latest run/result: `<command + pass/fail summary + date>`

### Open Work

- [ ] `<task 1>` (Target: `<milestone/quarter>`)
- [ ] `<task 2>` (Target: `<milestone/quarter>`)

### Risks / Blockers

- `<risk or blocker, if any>`

### Next Milestone

- `<next concrete step>`

### Closure Criteria

- [ ] All module acceptance criteria updated and traceable.
- [ ] Evidence updated (build/tests) or explicit justified gap.
- [ ] Parent epic task entry checked.
- [ ] Status labels updated before close (`status:open` removed/updated).
- [ ] Close reason documented (`completed` or `not planned`).

## Epic Checklist (Operational)

- [ ] Sub-issue created for each module row.
- [ ] Sub-issues linked in matrix with issue numbers.
- [ ] Area labels applied consistently.
- [ ] Priority labels normalized across sub-issues.
- [ ] Closure checks run before epic closure.

## Notes

- Keep updates concise and factual.
- If an item is blocked, capture the blocking dependency and owner.
- If verification is pending, state what is missing and why.
- Avoid duplicate or superseded epics; prefer one active parent and cross-links.