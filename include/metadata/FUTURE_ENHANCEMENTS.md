> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/metadata/FUTURE_ENHANCEMENTS.md -->

# Metadata Module — Public Header Future Enhancements

**Module Path:** `include/metadata/`
**Canonical implementation enhancements:** [`../../src/metadata/FUTURE_ENHANCEMENTS.md`](../../src/metadata/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/metadata/`. Runtime hardening and benchmark work remain tracked in:

→ [`../../src/metadata/FUTURE_ENHANCEMENTS.md`](../../src/metadata/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Schema and consistency outcomes must remain explicit and deterministic.
- `[x]` Export and lineage headers must stay backend-neutral for external integrations.
- `[x]` Security/provider interfaces must remain swappable without changing schema core types.
- `[x]` Metadata diagnostics must remain consumable outside the metadata implementation.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `SchemaManager` | `schema_manager.h` | DDL and API layers | ✅ Stable |
| `SchemaConsistencyChecker` | `schema_consistency_checker.h` | Validation and admin tooling | ✅ Stable |
| `CatalogExporter` / `ERDiagramExporter` | `catalog_exporter.h`, `er_diagram_exporter.h` | External integrations | ✅ Stable |
| `DistributedCatalog` | `distributed_catalog.h` | Multi-node metadata consumers | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Document degraded-export and malformed-schema behavior consistently across public governance headers.
- Standardize naming for metadata audit, diff, and incident payloads.
- Clarify extension expectations for export-policy and security-provider interfaces.

### Medium-Term (Q4 2026)

- Introduce `metadata_incident.h` and `schema_capability_profile.h` for shared diagnostics/capability exchange.
- Document benchmark-reference expectations for metadata-access and catalog-cache hot paths.
- Align lineage/export headers around a shared metadata-object identity vocabulary.

### Long-Term

- Add a backend-neutral metadata event bus contract for embedders that need schema-change subscriptions.
- Unify export and lineage serialization under a common metadata snapshot envelope.
