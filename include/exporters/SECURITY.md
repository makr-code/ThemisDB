<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Exporters Module (Public Headers)

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Exporters module public headers expose the export pipeline interface. Security concerns focus on PII leakage via exported data, memory exhaustion via unbounded join operations, injection via AQL predicates, and output encryption.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| PII leakage in exported data | `pii_detector.h` / `PolicyEngine` applies detection and redaction before serialisation |
| Memory exhaustion from large join right-side | `JoinExportConfig::right_side_memory_limit_bytes` (default 1 GiB) enforced; `ERR_EXPORT_JOIN_MEMORY_LIMIT` returned on breach |
| AQL predicate injection | `AqlPredicateFilter` validates and sandboxes AQL expressions; `ERR_EXPORT_JOIN_PREDICATE_INVALID` on invalid input |
| Plaintext output to storage | `ExportEncryptor` supports AES-256-GCM encryption of output streams |
| Ambiguous field names in join output | `ERR_EXPORT_JOIN_AMBIGUOUS_FIELD` (9313) terminates export on unresolved field ambiguity |
| Tenant data isolation | `ExportTenantContext` namespaces all export operations per tenant |

## Security Controls

- PII detection is applied on all merged/joined records before serialisation.
- Join memory budget prevents denial-of-service via large right-side collections.
- Output encryption available via `ExportEncryptor` for at-rest and in-transit protection.
- Tenant context is mandatory for all `IExporter` implementations.

## Known Limitations

- HuggingFace Hub upload (`HuggingFaceHubClient`) transmits data over HTTPS; ensure TLS certificate validation is enforced at the HTTP client layer.
- Implementation-level security details: `../../src/exporters/SECURITY.md`.
