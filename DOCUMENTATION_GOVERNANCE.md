# ThemisDB Documentation Governance

> Author: ThemisDB Contributors
> Created: 2026-09-09
> Status: Active
> Last Updated: 2026-09-09

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

## Markdown Metadata Minimum Gate

Changed Markdown files covered by `.github/workflows/gate-pr-doc-metadata.yml` must declare a minimum metadata block near the top of the document. The gate accepts either YAML front matter or a short header section and requires these fields:

- `Author` or `Urheber`
- `Created` / `Erstelldatum` in `YYYY-MM-DD`
- `Last Updated` / `Letzte Änderung` in `YYYY-MM-DD`
- `Status` in `draft | review | active | approved | stable | deprecated | archived`

Example header section:

```markdown
**Author:** ThemisDB Contributors
**Created:** 2026-09-09
**Last Updated:** 2026-09-09
**Status:** active
```

Example front matter:

```yaml
---
Author: ThemisDB Contributors
Created: 2026-09-09
Last Updated: 2026-09-09
Status: review
---
```

The gate intentionally excludes backlog/history/template paths that already follow dedicated governance formats, including `CHANGELOG.md`, `ROADMAP.md`, `**/FUTURE_ENHANCEMENTS.md`, archived documentation trees, issue templates, `docs/_standards/**`, `ai_working/**`, and generated developer wiki artifacts. The canonical scope and exclude list live in `.github/doc-metadata-gate.json`.
