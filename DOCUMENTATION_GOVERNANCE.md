# ThemisDB Documentation Governance

> Author: ThemisDB Contributors
> Created: 2026-09-09
> Status: Active
> Last Updated: 2026-09-20

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

## Wiki Source Classes And Weighting

For GitHub wiki generation, every source is assigned a class and weight.
The class controls ordering, conflict resolution, and reader-facing trust level.

### Source Classes

1. Primary sources (weight 100)
   - Sourcecode-near and contract-near evidence
   - Examples:
     - `include/**` API headers and public interface comments
     - `src/<module>/{ROADMAP,ARCHITECTURE,CHANGELOG,FUTURE_ENHANCEMENTS}.md`
     - generated `ai_context/developer_llm_wiki/API_REFERENCE_*.md` from Doxygen XML
     - canonical API references in `docs/api/**` and `docs/aql/**`

2. Secondary sources (weight 60)
   - Explanatory and operational synthesis
   - Examples:
     - guides, tutorials, runbooks, integration docs
     - root-level explanatory docs that summarize behavior

3. Metadata sources (weight 20)
   - Navigation and generated overview artifacts
   - Examples:
     - generated `Home`, `Module-Index`, `Wiki-Index`, `_Sidebar`, `_Footer`
     - audit snapshots and index-like summaries

### Processing Rules

1. Conflict rule
   - Primary overrides secondary and metadata.
   - Secondary may explain, but must not contradict primary.
   - Metadata must never be treated as normative behavior evidence.

2. Ordering rule for wiki build
   - Sort by source class weight first, then freshness/currency.
   - Reader outcome: high-trust technical evidence appears before explanatory pages.

3. Traceability rule
   - Generated wiki pages must carry provenance (`source`, `source_class`, `source_weight`).
   - Publish pipeline must emit a machine-readable manifest artifact.

4. Guardrail rule
   - Private-content boundary remains fail-closed for Community scope.
   - If configured, private-content blocks are release-blocking for wiki publication.

5. Branch-link rule
   - Absolute source links in generated wiki content must target the triggering branch
     (for example `develop` vs `community`) instead of a hardcoded branch.

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
