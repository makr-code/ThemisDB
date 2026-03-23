<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Importers Module (Public Headers)

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Importers module public headers expose connectors to external data sources. Security concerns focus on credential management, SQL/NoSQL injection, schema validation bypass, CDC stream tampering, and audit trail integrity.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Credential leakage in connection strings | Connection strings handled via environment variables / secret store; never logged |
| SQL injection via schema inference | `schema_inference.h` uses parameterised queries; dynamic DDL sanitised before execution |
| CDC stream tampering | `postgres_cdc.h` validates LSN sequence continuity; gaps trigger alert |
| Malicious data injection via flat file | `flatfile_importer.h` enforces schema validation via `schema_validator.h` before load |
| Audit trail tampering | `audit_trail.h` writes append-only records; `blockchain_integrity.h` optionally anchors hashes on-chain |
| MDM golden record poisoning | `mdm_engine.h` requires quorum-based resolution; single-source overrides require elevated scope |
| Kafka message replay | `kafka_importer.h` tracks consumer offset; duplicate messages detected and skipped |
| Cross-tenant import leakage | Each importer operation is scoped to a tenant context; cross-tenant merge requires explicit configuration |

## Security Controls

- Credentials are never stored in header-level configuration structs; passed via injection at construction.
- Schema validation enforced before any data is written to ThemisDB storage.
- CDC stream integrity checked via LSN continuity.
- Blockchain-anchored audit trail (optional) for tamper-evidence.

## Known Limitations

- Ethereum blockchain anchor is a stub; production anchor pending smart contract deployment.
- Kafka TLS and SASL configuration is the operator's responsibility.
- Oracle and MySQL connectors require vendor client libraries; keep patched.
- Implementation-level security details: `../../src/importers/SECURITY.md`.
