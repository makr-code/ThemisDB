# ROADMAP — ai_working Module

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot | Human_Review: false | Human_Author: unknown -->
<!-- Exact model backend not verifiable from final document text alone. -->

## Current Status

The `ai_working` module is a documentation-only context and planning artifact container. No C++ runtime implementation, public header, test target, or benchmark target exists. The historical 10% compliance score and restoration status are retained only as prior audit context and are not current measurements.

## In Progress

- None. The current module documentation was validated against implementation and workflow sources on 2026-10-31.

## Planned Features

- [x] Add an archival command for non-protected `src/ai_working/*.md` (`scripts/archive_ai_working.py --archive-stale`; it does not determine semantic staleness)
- [x] Add a required-document check (`scripts/archive_ai_working.py --check-freshness`) to `maintenance-ai-working.yml`; it checks existence, a placeholder pattern, and minimum content length, not date or factual freshness
- [ ] Structured artifact index for agent discovery (Target: Q1 2027)

## Implementation Phases

### Phase 1 — Design / Module Contract
- [x] Define module purpose and scope as documentation-only container
- [x] Align module with MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md
- [x] Identify required governance document set

### Phase 2 — Core Documentation Delivery
- [x] Create README.md with module purpose, scope, and layout
- [x] Create ARCHITECTURE.md with structural overview and lifecycle
- [x] Create CHANGELOG.md with governance history
- [x] Create AUDIT.md with compliance snapshot
- [x] Create SECURITY.md with security scope and controls
- [x] Create FUTURE_ENHANCEMENTS.md with extension areas
- [x] Create PERFORMANCE_EXPECTATIONS.md
- [x] Create PRODUCTION_REQUIREMENTS.md
- [x] Update ROADMAP.md (this document)

### Phase 3 — Archival and Cleanup
- [x] Archive BATCH1_ANALYTICS_HARDENING.md to `docs/ARCHIVED/ai-working-history/`
- [x] Archive BATCH1_STATUS.md to `docs/ARCHIVED/ai-working-history/`
- [x] Update MODULE_GAPS.md to reflect resolved documentation gap
- [x] Update docs/ai_working/ placeholder docs

### Phase 4 — Tests
- [x] Confirm no focused C++ tests are present or registered; `tests/ai_working/CMakeLists.txt` defines no target
- [ ] Add Python unit tests if the archival/freshness helper receives behavior changes

### Phase 5 — Performance / Hardening
- [x] No runtime performance gate applies; the proposed <10-second script target has not been measured

### Phase 6 — Documentation and Acceptance
- [x] Full governance doc set present and substantive
- [ ] Maintainer merge gate completed

## Production Readiness Checklist

- [x] Module purpose and scope documented
- [x] Architecture and artifact lifecycle documented
- [x] Changelog maintained
- [x] Security scope defined
- [x] Audit compliance snapshot present
- [x] Archival and required-document check commands exist and are invoked by `maintenance-ai-working.yml`
- [ ] Review semantic archive candidates and current factual claims; neither helper mode verifies semantic staleness

## Known Issues and Limitations

- No C++ implementation: governance tooling counts this as a documentation-only module.
- Doxygen coverage is N/A (no public C++ API symbols exist).
- Root `ai_working/` files are compacted and age-rotated by a different script from the `src/ai_working/` archival helper.
- The module-specific “freshness” check is structural and can pass stale factual claims.

## Breaking Changes

None. This module has no public API or runtime behavior.
