> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/metadata/ROADMAP.md -->

# Metadata Module — Public Header Roadmap

**Module Path:** `include/metadata/`
**Canonical implementation roadmap:** [`../../src/metadata/ROADMAP.md`](../../src/metadata/ROADMAP.md)

---

## Overview

Tracks public metadata API contract stability, schema/export header coverage, and planned public entry points. Runtime implementation work remains in:

→ [`../../src/metadata/ROADMAP.md`](../../src/metadata/ROADMAP.md)

---

## Current Status

All 19 metadata headers are present and cover schema management, consistency checking, versioning, lineage/export, distributed catalog access, and metadata security/provider extension points.

---

## Completed ✅

- [x] `schema_manager.h`, `schema_version_manager.h`, `information_schema.h` — schema and catalog core surfaces
- [x] `schema_constraints.h`, `schema_consistency_checker.h`, `schema_audit_log.h`, `schema_diff.h` — governance and diagnostics surfaces
- [x] `column_lineage.h`, `catalog_exporter.h`, `er_diagram_exporter.h`, `index_recommender.h` — lineage and export surfaces
- [x] provider/listener headers — integration and security extension contracts

---

## In Progress

- [ ] Clarify compatibility guidance for distributed-catalog and export-policy extension interfaces (Target: 2026-Q3)
- [ ] Add stronger malformed-schema and degraded-export notes to public governance headers (Target: 2026-Q3)

---

## Planned

- [ ] `metadata_incident.h` — shared incident/diagnostic DTO for schema and export failures (Target: 2026-Q4)
- [ ] `schema_capability_profile.h` — capability summary contract for embedders and API layers (Target: 2026-Q4)
- [ ] Document benchmark-backed compatibility notes for metadata-access hot paths (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Public metadata headers must remain backward compatible within the active major line; contract changes require migration notes and changelog updates.
