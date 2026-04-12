<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/metadata/ROADMAP.md -->

# Roadmap — Metadata Module (Public Headers)

## Current Status
Stable public API at v1.6.0. All three core interfaces (`IMetadataSecurityProvider`,
`IMetadataChangeListener`, `IMetadataExportPolicy`) are production-ready.

## Completed ✅
- [x] Core schema management headers (`schema_manager.h`, `schema_version_manager.h`)
- [x] Security provider interface (`imetadata_security_provider.h`)
- [x] Change listener interface (`imetadata_change_listener.h`)
- [x] Export policy interface (`imetadata_export_policy.h`)
- [x] Column lineage, index recommender, statistics collector headers
- [x] Distributed catalog, information schema, ER diagram exporter

## Planned Features
- [x] `IMetadataEncryptionProvider` — field-level encryption policy (v1.7.0)
  (`include/metadata/imetadata_encryption_provider.h`)
- [x] `schema_diff.h` — structural diff API for schema migration previews (v1.7.0)
  (`include/metadata/schema_diff.h`)
- [x] `metadata_snapshot.h` — point-in-time metadata snapshots (v1.7.0)
  (`include/metadata/metadata_snapshot.h`)

## Production Readiness Checklist
- [x] All public headers compile without warnings (`-Wall -Wextra`)
- [x] No implementation details exposed in public headers
- [x] All interfaces documented with Doxygen
- [x] Breaking-change policy enforced (semver)
- [x] `IMetadataEncryptionProvider` header finalized
