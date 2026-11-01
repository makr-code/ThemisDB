> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Header inventory and stale-contract note reconciled 2026-10-31; remaining planning status not revalidated -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/metadata/ROADMAP.md -->

# Metadata Module — Public Header Roadmap

**Author:** ThemisDB Contributors  
**Created:** 2026-10-31  
**Last Updated:** 2026-10-31  
**Status:** review  
**AI_Used:** true  
**AI_Models:** unknown  
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator  
**Human_Review:** false  
**Human_Author:** ThemisDB Contributors  
**Source level:** level1 — module-local  
**SOT domain:** api-contract  
**Canonical source references:** [metadata public headers](./), [runtime roadmap](../../src/metadata/ROADMAP.md), [API reference](../../src/metadata/API.md)  
**Related issue / milestone:** [#7362](https://github.com/makr-code/ThemisDB/issues/7362) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

**Module Path:** `include/metadata/`
**Canonical implementation roadmap:** [`../../src/metadata/ROADMAP.md`](../../src/metadata/ROADMAP.md)

---

## Overview

Tracks public metadata API contract stability, schema/export header coverage, and planned public entry points. Runtime implementation work remains in:

→ [`../../src/metadata/ROADMAP.md`](../../src/metadata/ROADMAP.md)

---

## Current Status

All 20 metadata headers are present. They cover schema management,
consistency checking, versioning, lineage/export, distributed catalog access,
snapshots, and provider interfaces. `metadata_api_contract.h` is a frozen
artifact whose method names and behavioral text do not match the current runtime
API; see [`../../src/metadata/API.md`](../../src/metadata/API.md).

---

## Completed ✅

- [x] `schema_manager.h`, `schema_version_manager.h`, `information_schema.h` — schema and catalog core surfaces
- [x] `schema_constraints.h`, `schema_consistency_checker.h`, `schema_audit_log.h`, `schema_diff.h` — governance and diagnostics surfaces
- [x] `column_lineage.h`, `catalog_exporter.h`, `er_diagram_exporter.h`, `index_recommender.h` — lineage and export surfaces
- [x] provider/listener headers — integration and security extension contracts

---

## In Progress

- [?] Reconfirm compatibility guidance for distributed-catalog and export-policy extension interfaces (Target: 2026-Q3; status not revalidated)
- [?] Reconfirm malformed-schema and degraded-export notes in public governance headers (Target: 2026-Q3; status not revalidated)

---

## Planned

- [ ] `metadata_incident.h` — shared incident/diagnostic DTO for schema and export failures (Target: 2026-Q4)
- [ ] `schema_capability_profile.h` — capability summary contract for embedders and API layers (Target: 2026-Q4)
- [ ] Document benchmark-backed compatibility notes for metadata-access hot paths (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Public metadata headers must remain backward compatible within the active major line; contract changes require migration notes and changelog updates.
