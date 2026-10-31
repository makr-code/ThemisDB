# PRODUCTION_REQUIREMENTS — ai_working Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot | Human_Review: false | Human_Author: unknown -->
<!-- Exact model backend not verifiable from final document text alone. -->

## Purpose and Scope

The `ai_working` module is a documentation-only artifact container. It has no C++ runtime code and no production deployment footprint. This document defines the governance requirements that apply in lieu of operational production requirements.

## Document Boundary

- **`src/ai_working/PRODUCTION_REQUIREMENTS.md` (this document):** governance and compliance requirements for the module's documentation lifecycle.
- **`src/ai_working/README.md`:** module purpose, scope, and layout.
- **`src/ai_working/ROADMAP.md`:** delivery phases and readiness state.
- **`src/ai_working/FUTURE_ENHANCEMENTS.md`:** planned extensions.

## Governance Requirements

- **MUST:** The required governance documentation set (README, ARCHITECTURE, AUDIT, CHANGELOG, FUTURE_ENHANCEMENTS, PERFORMANCE_EXPECTATIONS, PRODUCTION_REQUIREMENTS, ROADMAP, SECURITY) must be present and substantive in `src/ai_working/`.
- **MUST:** Stale or superseded wave-specific artifacts must be archived to `docs/ARCHIVED/ai-working-history/` rather than deleted.
- **MUST NOT:** Committed artifacts must not contain credentials, API keys, or sensitive deployment parameters.
- **MUST:** Secret scanning must be applied to all artifacts committed to `ai_working/` or `src/ai_working/`.
- **MUST NOT:** Placeholder-only content ("module placeholder for mirrored structure") counts as a governance documentation gap.

## Compliance Check (Audit-capable)

The following is a review checklist, not evidence that a check has already passed:

- [ ] Confirm the required governance documents exist and are substantive; `scripts/archive_ai_working.py --check-freshness` checks presence, one placeholder pattern, and minimum size only.
- [ ] Review claims against current scripts, workflows, and repository configuration; the freshness command does not check factual accuracy.
- [ ] Review `--archive-stale` candidates before movement because it selects all non-protected top-level Markdown files, not semantically stale files.
- [ ] Run the repository's configured secret-scanning check on changed working artifacts; this module's maintenance workflow does not itself run a secret scanner.
- [ ] Keep module purpose, scope, and separate root/module artifact lifecycles documented.
