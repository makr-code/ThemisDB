---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `include/replication/`, `src/replication/`, `tests/replication/CMakeLists.txt`
Related issues: #7093, #7167

# Replication module guide

## Audience and ownership

For engineers changing replication membership/role management, WAL shipping, logical streams, slots, placement, or conflict handling. Storage owns the local storage engine; replication owns its replication-specific state and transport workflows.

## Start with the affected component

- Manager and replica state: `replication_manager.h` / `replication_manager.cpp`.
- Cross-region shipper: `async_wal_shipper.h` / `async_wal_shipper.cpp`.
- Slots, logical replication, or placement: find the matching header and source under `include/replication/` and `src/replication/`.

## Maintainer workflow

1. Confirm the mode/configuration and return type in the public header.
2. Trace the relevant state transition and callback in its implementation.
3. For queue changes, test saturation as well as successful enqueue; for manager changes, test failure/health paths.
4. Add or adjust a focused test under `tests/replication/` and run the registered target from its `CMakeLists.txt`.
5. Update `API.md` if caller-visible behavior changes and `IMPLEMENTATION.md` if internal invariants change.

See `ARCHITECTURE.md`, `ROADMAP.md`, and `RUNBOOK.md` for broader subsystem context.
