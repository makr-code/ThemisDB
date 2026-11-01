---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - README.md
  - ../../include/replication/
  - ../../tests/CMakeLists.txt
  - ../../tests/replication/CMakeLists.txt
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"

# Replication module guide

## Audience and ownership

For engineers changing replication membership/role management, WAL shipping, logical streams, slots, placement, or conflict handling. Storage owns the local storage engine; replication owns its replication-specific state and transport workflows.

## Start with the affected component

- Manager and replica state: `replication_manager.h` / `replication_manager.cpp`.
- Cross-region shipper: `async_wal_shipper.h` / `async_wal_shipper.cpp`.
- Slots, logical replication, or placement: find the matching header and source under `include/replication/` and `src/replication/`.

## Maintainer workflow

1. Confirm the mode/configuration and return type in the public header.
2. Trace the relevant state transition and callback in its implementation. Check the
   public header first; a header declaration alone does not establish runtime wiring.
3. For queue changes, test saturation as well as successful enqueue; for manager changes, test failure/health paths.
4. Add or adjust a focused test under `tests/replication/`. Confirm its actual
   registration in the active root `tests/CMakeLists.txt` or module-local
   `tests/replication/CMakeLists.txt` before selecting a CTest target.
5. Update `API.md` if caller-visible behavior changes and `IMPLEMENTATION.md` if internal invariants change.

See `ARCHITECTURE.md`, `ROADMAP.md`, and `RUNBOOK.md` for broader subsystem context.

The root test CMake contains explicit replication registrations. The module-local
CMake file also contains target-generation logic, but this page does not assert that
the latter is included by every configure path. No test execution is claimed here.
