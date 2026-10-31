---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "tests/exporters/CMakeLists.txt; src/exporters/ROADMAP.md"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

# Exporter tests roadmap

Exact model backend not verifiable from final document text alone.

## Current Status

The folder contains focused tests registered dynamically from top-level
`test_*.cpp` files by [`CMakeLists.txt`](./CMakeLists.txt). Runtime/API
priorities remain in [`../../src/exporters/ROADMAP.md`](../../src/exporters/ROADMAP.md);
this test-directory roadmap does not set separate delivery dates.

## Planned Features

- [ ] Add focused regression cases for newly changed exporter contracts alongside
  their owning test source; no independent delivery date is established.

## Implementation Phases

### Phase 1: Design / API-Vertrag
- [ ] Identify the changed public contract and its owning test suite before
  adding test-only work.

### Phase 2: Core-Implementierung
- [ ] Keep test helpers and fixtures within the existing focused-test target.

### Phase 3: Fehlerbehandlung & Edge Cases
- [ ] Cover applicable invalid-input, error, limit, and recovery behavior.

### Phase 4: Tests
- [ ] Register each new top-level `test_*.cpp` through the module CMake file's
  stem-based target and CTest naming.

### Phase 5: Performance/Hardening
- [ ] Add stress or hardening coverage only for a named runtime requirement or
  observed regression; this roadmap defines no performance threshold.

### Phase 6: Dokumentation & Abnahme
- [ ] Record the focused build/CTest command and actual result with the change.

## Production Readiness Checklist
- [ ] Tests cover the changed contract and relevant failure behavior.
- [ ] CTest registration and labels match the module CMake convention.
- [ ] Execution evidence records environment blockers rather than inferring success.

## Known Issues & Limitations
- [!] Test-only milestones and dated gaps require an accepted runtime roadmap item.

## Breaking Changes
- None known for the test-directory structure; runtime API changes belong in the
  canonical module roadmap.
