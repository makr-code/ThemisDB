# Documentation Placement and Template Guidelines

This guide defines where different documentation types belong and which templates should be used so that generated output stays reproducible and easy to review.

## Canonical Buckets

| Bucket | Target locations | Primary template or guide | Audience |
|---|---|---|---|
| Developer source documentation | `src/<module>/`, `include/<module>/`, `tests/`, `benchmarks/` | [SOURCECODE_DOCUMENTATION_7PHASE_TEMPLATE.md](SOURCECODE_DOCUMENTATION_7PHASE_TEMPLATE.md) | Developers and maintainers |
| Public / admin / user documentation | `docs/` | [PUBLIC_DOCUMENTATION_7PHASE_TEMPLATE.md](../docs/_standards/PUBLIC_DOCUMENTATION_7PHASE_TEMPLATE.md) plus [DOC_TEMPLATE.md](../docs/_standards/DOC_TEMPLATE.md) | Users, admins, operators |
| Research and evidence documentation | `research/` | [RESEARCH_GUIDE.md](../research/RESEARCH_GUIDE.md) and the specific research templates in `research/papers/`, `research/best_practices/`, and `research/architecture_decisions/` | Maintainers, reviewers, researchers |
| Module gap documentation | `src/<module>/MODULE_GAPS.md` and `ai_working/module_gaps/` | [MODULE_DOCUMENTATION_GUIDE.md](MODULE_DOCUMENTATION_GUIDE.md) | Developers and gap-analysis workflows |

## Placement Rules

1. If the document explains code behavior, API contracts, ownership, error handling, or build integration, place it in the developer source documentation bucket.
2. If the document explains how to use, operate, or administer the product, place it in `docs/` and start from the public 7-phase template.
3. If the document is based on an external source, paper, benchmark study, or architecture decision, place it in `research/` and use the research-specific templates.
4. If the document lists implementation gaps, progress, or remediation tasks, keep it in the module gap documentation bucket.
5. Do not duplicate the same content across buckets without a clear source-of-truth statement and a reason for the duplication.

## Required Output Properties

Every generated document should include, where applicable:

- Purpose
- Audience
- Source of truth
- Scope and non-goals
- Current status
- Validation notes
- Known limitations
- Links to related docs, code, issues, or research

## Writing Rules

- Keep source-code docs aligned with the actual code, tests, and build flags.
- Keep public docs readable without source-code context.
- Keep research docs citation-driven and traceable to sources or experiments.
- State uncertainty explicitly instead of guessing.
- Update the owning index or README when a new document is added.

## Template Selection

- Use [SOURCECODE_DOCUMENTATION_7PHASE_TEMPLATE.md](SOURCECODE_DOCUMENTATION_7PHASE_TEMPLATE.md) for docs generated from source files.
- Use [PUBLIC_DOCUMENTATION_7PHASE_TEMPLATE.md](../docs/_standards/PUBLIC_DOCUMENTATION_7PHASE_TEMPLATE.md) for user, admin, and operational docs in `docs/`.
- Use the templates and guide from `research/RESEARCH_GUIDE.md` for research content.

## Notes on Special Folders

- `docs/analysis/` is for analysis artifacts and investigative summaries.
- `docs/_standards/` is for maintained documentation templates and schemas.
- `research/` is the canonical location for research, evidence, and source-backed design rationale.
- If a future `compendium/` area is added, treat it as a documentation mirror only after a clear ownership rule has been written.

## Template Quick Reference

- Source-code documentation: [SOURCECODE_DOCUMENTATION_7PHASE_TEMPLATE.md](SOURCECODE_DOCUMENTATION_7PHASE_TEMPLATE.md)
- Developer consolidation workload: [DEVELOPER_DOCS_PHASE_WORKLOAD.md](DEVELOPER_DOCS_PHASE_WORKLOAD.md)
- Public/admin/user documentation: [PUBLIC_DOCUMENTATION_7PHASE_TEMPLATE.md](../docs/_standards/PUBLIC_DOCUMENTATION_7PHASE_TEMPLATE.md)
- Public doc structure reference: [DOC_TEMPLATE.md](../docs/_standards/DOC_TEMPLATE.md)
- Research documentation: [RESEARCH_GUIDE.md](../research/RESEARCH_GUIDE.md)
