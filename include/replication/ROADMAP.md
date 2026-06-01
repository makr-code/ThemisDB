> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
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

All 13 replication headers are present and cover orchestration, multi-writer variants, logical streaming, CDC, conflict resolution, and lag/topology observability.

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
