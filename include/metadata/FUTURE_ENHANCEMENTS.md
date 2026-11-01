> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Selected public type names reconciled 2026-10-31; remaining planning status not revalidated -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/metadata/FUTURE_ENHANCEMENTS.md -->

# Metadata Module — Public Header Future Enhancements

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
**Canonical source references:** [public metadata headers](./), [runtime future enhancements](../../src/metadata/FUTURE_ENHANCEMENTS.md), [API reference](../../src/metadata/API.md)  
**Related issue / milestone:** [#7362](https://github.com/makr-code/ThemisDB/issues/7362) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

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
| `DistributedMetadataCatalog` | `distributed_catalog.h` | Multi-node metadata consumers | ✅ Implemented interface; review compatibility before contract changes |

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
