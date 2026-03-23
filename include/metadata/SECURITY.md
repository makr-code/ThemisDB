<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Metadata Module (Public Headers)

> For reporting vulnerabilities see project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Unauthorized schema access | `IMetadataSecurityProvider` enforces ACL checks before any schema operation |
| Metadata leakage via export | `IMetadataExportPolicy` masks/filters sensitive columns before export |
| Schema mutation without audit | `SchemaAuditLog` records all DDL with actor, timestamp, and diff |
| Cross-tenant schema visibility | Schema keys are tenant-namespaced; `DistributedCatalog` enforces isolation |
| Stale observer callbacks causing data confusion | `IMetadataChangeListener` delivers ordered, versioned events |

## Security Controls
- `IMetadataSecurityProvider` is a mandatory injection point; default-deny if not configured.
- `SchemaAuditLog` is append-only; no public delete API.
- Export APIs require `IMetadataExportPolicy` to be explicitly provided; no default allow-all.

## Known Limitations
- Thread-safety guarantees for `IMetadataChangeListener` implementations are caller-responsibility.
- `IMetadataEncryptionProvider` (field-level) is not yet available (planned Q3 2026).
