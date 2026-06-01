[docs](../../index.md) > [en](../index.md) > [core](./index.md) > [PRIMARY_SOURCES](./PRIMARY_SOURCES.md)
**Date:** 2026-05-31
**Status:** current
**Primary Source (source of truth):**
- `src/core/README.md`
- `src/core/ARCHITECTURE.md`
- `src/core/ROADMAP.md`
- `src/core/FUTURE_ENHANCEMENTS.md`
- `src/core/MODULE_GAPS.md`
- `src/core/SECURITY.md`
- `src/core/AUDIT.md`
- `src/core/PERFORMANCE_EXPECTATIONS.md`
- `src/core/PRODUCTION_REQUIREMENTS.md`
- `src/core/CHANGELOG.md`

**Reference:**
- Inventory baseline: `ai_working/developer_docs_inventory_report.md`
- Alignment baseline: `ai_working/docs_module_alignment_report_2026-05-31.md`
- Context: module-level docs alignment where newer planning docs are prioritized over historical docs.

---

# Primary Sources - core

This page defines which core documents in `src/core/` are authoritative for planning and behavior alignment.

## Alignment Policy

- Newer documents are considered more relevant than older documents.
- `FUTURE_ENHANCEMENTS.md` and `MODULE_GAPS.md` are the main workload inputs.
- Historical implementation reports in `docs/` are secondary unless they are newer than planning docs and source-verified.

## Authoritative Planning Inputs

| File | Role |
|---|---|
| `src/core/FUTURE_ENHANCEMENTS.md` | target behavior, constraints, and planned interfaces |
| `src/core/MODULE_GAPS.md` | current gap workload and severity snapshot |
| `src/core/ROADMAP.md` | phased execution and milestone status |

## Authoritative Runtime and Governance Inputs

| File | Role |
|---|---|
| `src/core/ARCHITECTURE.md` | runtime architecture and dependency boundaries |
| `src/core/SECURITY.md` | security controls and constraints |
| `src/core/AUDIT.md` | verification and evidence status |
| `src/core/PERFORMANCE_EXPECTATIONS.md` | benchmark contracts and performance gates |
| `src/core/PRODUCTION_REQUIREMENTS.md` | production-readiness requirements |
| `src/core/CHANGELOG.md` | change traceability |

## Secondary References (non-authoritative)

- `include/core/*` markdown files
- historical docs under `docs/` (for example archived reports)

Use these for context, not as the primary source of current workload status.

---

*Updated for docs-vs-planning alignment sweep on 2026-05-31.*
