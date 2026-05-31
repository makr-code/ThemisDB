# Security - Source Root

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md -->

Report vulnerabilities via the repository-level SECURITY.md.

## Scope

This document summarizes source-root security concerns that span multiple modules under src. It does not replace module-local SECURITY.md files; it links cross-module patterns, audit aggregation, and security-sensitive review expectations.

## Root-Level Security Concerns

| Concern | Root-Level Surface |
|---|---|
| cross-module auth and authorization drift | source-root audit aggregation and module audits |
| unsafe cross-module integration paths | source-wide roadmap and future-enhancement aggregation |
| stale or misleading security summaries | root aggregation docs and inventory regeneration |
| hidden non-production or weakly governed paths | STUB_INVENTORY and UNUSED_FUNCTIONS_REPORT review context |

## Controls

- module-local SECURITY.md remains the owning document for feature-local controls.
- src/AUDIT.md aggregates historically important cross-module findings and status references.
- root analysis artifacts support review of unused symbols, stub paths, and cross-module coupling.
- inventory regeneration is required when root-level summaries materially change.

## Follow-Ups

- keep cross-module security summaries synchronized with owning module audits.
- avoid treating root-level aggregation docs as substitutes for module-local security contracts.
- re-run inventory and cross-check aggregation artifacts after material source-root documentation changes.

## Sourcecode Verification (Scope: src/<root>/security)

- Verified files:
  - src/AUDIT.md
  - src/ROADMAP.md
  - src/FUTURE_ENHANCEMENTS.md
  - src/STUB_INVENTORY.md
  - src/UNUSED_FUNCTIONS_REPORT.md
  - ai_working/developer_docs_inventory_report.md
- Verified controls:
  - module-local ownership of feature security docs
  - root-level aggregation and analysis support for cross-module review