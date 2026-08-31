# ThemisDB Documentation Governance

> Status: Active
> Last Updated: 2026-08-28

This file defines source-of-truth precedence and synchronization expectations for root and module documentation.

## Source Of Truth (SOT) Precedence

When documentation sources conflict, use this order:

1. Root governance and release documents:
   - `ROADMAP.md`
   - `CHANGELOG.md`
   - `RELEASE_STRATEGY.md`
   - `VERSIONING.md`
   - `BRANCHING_STRATEGY.md`
2. Canonical audit domain:
   - `audit/AUDIT.md`
   - `audit/README.md`
3. Module-local documentation near code:
   - `src/<module>/ROADMAP.md`
   - `src/<module>/README.md`
   - `include/<module>/*.h` API comments
4. Historical and archived material:
   - `docs/ARCHIVED/**`
   - `docs/archive/**`

Archived or ai_working artifacts are evidence/history by default and are not normative unless explicitly promoted.

## Mandatory Sync Rules

1. Code behavior changes must update affected docs in the same change.
2. Public API and contract changes must update API-facing docs and related runbooks.
3. Root onboarding links (`README.md`, `QUICKSTART.md`, `SETUP.md`, `INDEX.md`) must remain valid.
4. Broken local links in root docs are treated as documentation defects.
5. Branch and release naming must stay consistent with `BRANCHING_STRATEGY.md`.

## Naming And Scope

- Keep root docs concise and navigational.
- Keep module implementation detail in module-local docs.
- Use one canonical file per topic in a scope; avoid semantic duplicates.
