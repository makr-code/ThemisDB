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

### Active vs historical status artifacts

- Active status artifacts: root governance and release docs, module-local `src/<module>/ROADMAP.md`, current audit/release evidence documents, and live `ai_context/` operational summaries.
- Historical artifacts: `ai_working/` execution snapshots, planning decks, launch summaries, and older status bundles. These are evidence-only records and must never be treated as the current release/GA source of truth.
- When a derivative artifact conflicts with `ROADMAP.md`, the canonical roadmap and active governance docs override the historical text.

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
- `AI_Used` when AI generation or AI-assisted editing was used
- `AI_Models` when AI was used; if the exact backend cannot be verified, use `unknown` and add the explicit text: `Exact model backend not verifiable from final document text alone.`
- `AI_Contributor` naming the responsible AI/agent or `unknown`
- `Human_Review` for final human verification status
- `Human_Author` naming the human maintainer or author

Example header section:

```markdown
**Author:** ThemisDB Contributors
**Created:** 2026-09-09
**Last Updated:** 2026-09-09
**Status:** active
**AI_Used:** true
**AI_Models:** GitHub Copilot, Ollama/qwen2.5-coder:14b
**AI_Contributor:** GitHub Copilot
**Human_Review:** true
**Human_Author:** ThemisDB Contributors
```

Example front matter:

```yaml
---
Author: ThemisDB Contributors
Created: 2026-09-09
Last Updated: 2026-09-09
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---
```

When the exact model backend cannot be determined with certainty, the document must state:

- `Exact model backend not verifiable from final document text alone.`

This is the repository-standard AI provenance convention for generated or AI-assisted documentation and must be used for Copilot, Claude, Ollama, and any other AI agent involved in creating or materially editing documentation.

The gate intentionally excludes backlog/history/template paths that already follow dedicated governance formats, including `CHANGELOG.md`, `ROADMAP.md`, `**/FUTURE_ENHANCEMENTS.md`, archived documentation trees, issue templates, `docs/_standards/**`, `ai_working/**`, `ai_context/**`, and generated developer wiki artifacts. The canonical scope and exclude list live in `.github/doc-metadata-gate.json`.
