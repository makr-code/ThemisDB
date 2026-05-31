# Architecture - Source Root

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The root of src is a documentation and aggregation boundary above individual modules. It does not own one runtime subsystem; instead, it organizes cross-module source structure, source-wide plans, aggregated audit/security context, and cross-module static-analysis artifacts.

## Main Planes

1. Module plane
- 62 top-level source modules own feature-local implementation and behavior contracts under src/<module>/

2. Aggregation plane
- source-wide ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, SECURITY, and inventory-oriented reports summarize cross-module state

3. Analysis plane
- MODULE_FUNCTION_USAGE_MAP, UNUSED_FUNCTIONS_REPORT, STUB_INVENTORY, and related artifacts support review, refactoring, and governance workflows

## Contracts

| Contract | Behavior |
|---|---|
| module contract | implementation ownership stays in src/<module>/ docs |
| aggregation contract | root docs summarize cross-module state without replacing module-local contracts |
| analysis contract | root reports inform review and planning but do not override source truth in owning modules |

## Failure Semantics

- stale root aggregation docs create governance and navigation drift, not direct runtime behavior changes.
- module-local docs remain the source of truth for feature-local behavior.
- inventory and aggregation docs must be regenerated when root or module summaries materially change.

## Sourcecode Verification (Scope: src/<root>/architecture)

- Verified files:
  - src/ROADMAP.md
  - src/FUTURE_ENHANCEMENTS.md
  - src/AUDIT.md
  - src/SECURITY.md
  - src/MODULE_FUNCTION_USAGE_MAP.md
  - src/UNUSED_FUNCTIONS_REPORT.md
  - ai_working/developer_docs_inventory_report.md
- Verified architecture claims:
  - root docs sit above module-local docs as aggregation artifacts
  - the inventory distinguishes module rows from the <root> filename-matrix row
  - ownership of runtime behavior remains module-local