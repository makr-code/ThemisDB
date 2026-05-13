# Copilot Instructions (Repository-wide)

## Documentation tasks (MUST)
If the task involves documentation (any change under `docs/**`):

1) Use the standard:
   - Spec: `docs/_standards/doc_header.schema.yml`
   - Template: `docs/_standards/DOC_TEMPLATE.md`

2) Every doc under `docs/**` MUST start with a clickable breadcrumb link chain:
   - Use Markdown links for every breadcrumb segment (label + relative target path), e.g. `docs > <lang> > <domain?> > <module> > <doc_kind>`.

3) Every doc header MUST include:
   - **Datum** (YYYY-MM-DD)
   - **Status**
   - **Primary (Quelle der Wahrheit)** (links to `src/**`, `include/**`, `examples/**`)
   - **Bezug / Reference** (issue/PR/module context)

4) Prefer linking to Primary docs instead of duplicating canonical information.

## Root governance and release/versioning alignment (MUST)

For root governance or release/versioning updates, treat the following files as one aligned set:

- `COPILOT_INSTRUCTIONS.md` (AI-/Agent-Prozessregeln)
- `VERSIONING.md` (SemVer + release type model)
- `RELEASE_STRATEGY.md` (branch/tag/milestone flow)
- `CHANGELOG.md` (released + unreleased traceability)
- `ROADMAP.md` (feature/milestone source of truth)
- `FUTURE_ENHANCEMENTS.md` (open enhancement backlog)

Alignment rules:

1) Keep release type mapping consistent across `VERSIONING.md`, `RELEASE_STRATEGY.md`, and `CHANGELOG.md`.
2) Keep terminology consistent: shipped scope = `ROADMAP.md`; open backlog = `FUTURE_ENHANCEMENTS.md`.
3) For governance updates, include review/audit references from:
   - `docs/DOCUMENTATION_REVIEW_GUIDELINES.md`
   - `docs/SYSTEMATISCHER_REVIEWPLAN.md`
   - `docs/PR_DOCUMENTATION_CHECKLIST.md`
   - `docs/de/development/SOURCE_CODE_AUDIT.md`
   - `docs/audit-framework/AUDIT_RUNBOOK.md`
