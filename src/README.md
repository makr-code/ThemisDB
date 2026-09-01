# ThemisDB Source Root

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Purpose

This root-level src documentation describes the shared source tree directly under src, not an individual feature module. It acts as a navigation and governance layer across the 62 top-level source modules plus the root-level cross-module documents that live beside them.

## Scope

In scope:
- navigation across top-level source modules under src
- root-level cross-module documents such as ROADMAP, AUDIT, SECURITY, MODULE_FUNCTION_USAGE_MAP, UNUSED_FUNCTIONS_REPORT, and STUB_INVENTORY
- explanation of how module-local docs and root-level aggregation docs fit together

Out of scope:
- per-module implementation contracts owned by src/<module>/ docs
- public include contracts owned by include/

## Module Status Snapshot

Use [`MODULE_INDEX.md`](MODULE_INDEX.md) for the canonical per-module matrix. This file only gives the quick read:

| Status | Module groups | Interpretation |
|---|---|---|
| Production-ready / minor gaps | `server`, `storage`, `network`, `auth`, `security`, `cache`, `failover`, `maintenance`, `updates`, `process`, `execution`, `analytics` | Source and audit evidence are largely closed; remaining work is mostly hardening, diagnostics, or benchmark refresh. |
| Hardening in progress | `themis`, `transaction`, `query`, `index`, `sharding`, `replication`, `graph`, `cdc`, `llm`, `rag`, `gpu`, `acceleration`, `geo`, `voice`, `access_model`, `ethics_ai` | Code exists and is production-shaped, but the module still carries real source gaps or evidence debt. |
| Planned / externalization | `chimera`, `user_storage`, selected plugin-externalization paths | The feature surface is documented and tracked, but the implementation boundary is still being finalized. |

## Structure

- top-level source modules: 62 module directories represented in the developer docs inventory core matrix
- root-level source aggregation docs:
  - README.md
  - ARCHITECTURE.md
  - ROADMAP.md
  - FUTURE_ENHANCEMENTS.md
  - SECURITY.md
  - AUDIT.md
  - MODULE_FUNCTION_USAGE_MAP.md
  - UNUSED_FUNCTIONS_REPORT.md
  - STUB_INVENTORY.md
  - CONSOLIDATION_ANALYSIS.md

## How To Read This Area

1. Use this file for source-tree orientation.
2. Use module-local docs under src/<module>/ for implementation ownership and behavior contracts.
3. Use the root aggregation docs for cross-module planning, audit, security, and inventory context.

## Key References

- repository overview: ../README.md
- public header overview: ../include/README.md
- public header architecture: ../include/ARCHITECTURE.md
- test architecture overview: ../tests/ARCHITECTURE.md
- benchmark architecture overview: ../benchmarks/ARCHITECTURE.md
- source-wide backlog: ROADMAP.md
- source-wide future work constraints: FUTURE_ENHANCEMENTS.md
- source-wide audit aggregation: AUDIT.md
- source-wide security summary: SECURITY.md
- cross-module usage map: MODULE_FUNCTION_USAGE_MAP.md
- cross-module unused-symbol report: UNUSED_FUNCTIONS_REPORT.md

## Sourcecode Verification (Scope: src/<root>)

- Verified inventory anchor:
  - ai_working/developer_docs_inventory_report.md
- Verified root-level docs:
  - src/README.md
  - src/ARCHITECTURE.md
  - src/ROADMAP.md
  - src/FUTURE_ENHANCEMENTS.md
  - src/SECURITY.md
  - src/AUDIT.md
  - src/MODULE_FUNCTION_USAGE_MAP.md
  - src/UNUSED_FUNCTIONS_REPORT.md
  - src/STUB_INVENTORY.md
- Note:
  - root-level src docs are tracked separately from module-directory rows in the filename matrix as <root>.