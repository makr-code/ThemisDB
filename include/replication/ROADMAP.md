---
Author: ThemisDB Contributors
Created: 2026-06-01
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - README.md
  - ARCHITECTURE.md
  - ../../src/replication/ROADMAP.md
  - ../../include/replication/
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

<!-- Historical/backlog status entries; header inventory checked 2026-10-31, not every item revalidated. -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/replication/ROADMAP.md -->

# Replication Module — Public Header Roadmap

**Module Path:** `include/replication/`
**Canonical implementation roadmap:** [`../../src/replication/ROADMAP.md`](../../src/replication/ROADMAP.md)

---

## Overview

Tracks public replication API contract stability, header-level failover/CDC coverage, and planned public entry points. Runtime implementation work remains in:

→ [`../../src/replication/ROADMAP.md`](../../src/replication/ROADMAP.md)

---

## Current Status

The directory currently contains 18 replication headers covering orchestration, Raft-v2, placement, logical streaming, CDC, conflict resolution, lag/observability, diagnostics, and related declarations. This inventory does not imply implementation or runtime integration for every header; see `README.md` and `ARCHITECTURE.md`.

---

## Completed ✅

- [x] `replication_manager.h`, `multi_master_replication.h`, `multi_tier_replication.h` — orchestration and topology headers
- [x] `logical_replication.h`, `replication_slot.h`, `event_stream.h`, `schema_cdc.h` — propagation and CDC surfaces
- [x] `conflict_resolution.h`, `crdt_types.h` — deterministic merge contracts
- [x] `observability.h`, `policy.h` — diagnostics and policy contracts

---

## In Progress

- [ ] Clarify promotion/failover state-transition guarantees across orchestration headers (Target: 2026-Q3)
- [ ] Add stronger degraded-lag and slot-fault guidance to CDC-facing header docs (Target: 2026-Q3)

---

## Planned

- [ ] `replication_transition_event.h` — shared event object for promotion/failover subscribers (Target: 2026-Q4)
- [ ] `replication_backpressure_hint.h` — explicit DTO for lag/backpressure advisory signals (Target: 2026-Q4)
- [ ] Document benchmark-backed compatibility notes for promotion, conflict, and CDC hot paths (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Public replication headers must remain backward compatible within the active major line; contract changes require migration notes and changelog updates.

> **Status note (2026-10-31):** The dated checkboxes and planned filenames above are
> retained as roadmap/backlog history, not verified claims about current implementation
> or a promise that an item is still planned. Compare the current headers and sources
> before using them as requirements.
