> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Storage Module

> Report vulnerabilities via the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Unauthorized data access | Auth checks at `StorageEngine` API boundary; RBAC enforced by auth module |
| Data at rest exposure | AES-256-GCM field-level encryption via `SecuritySignature`; `THEMIS_PRODUCTION_MODE` enforces encryption |
| Data tampering | HMAC-SHA256 tamper detection via `SecuritySignatureManager` |
| WAL replay attack | WAL entries authenticated; sequence numbers validated on replay |
| Blob backend credential exposure | Credentials supplied via environment variables or HSM; never stored in config files |
| Backup data exposure | Backups encrypted before upload to cloud storage |
| Path traversal in blob filesystem backend | Paths normalized and restricted to configured blob root directory |
| RocksDB SSTable poisoning | Checksums verified on compaction and read; BlobDB integrity checked |
| Tiered storage data leakage | Tier migration preserves encryption; data deleted from source only after verified copy |

## Security Controls

- `THEMIS_PRODUCTION_MODE` compile-time flag: fail-closed if encryption key provider is null
- Field-level AES-256-GCM encryption via `SecuritySignature`
- HMAC-SHA256 for tamper detection on all stored records
- `StorageAuditLogger` records all write operations with caller identity
- Blob backends use HTTPS/TLS for all cloud communications
- RocksDB checksum verification enabled by default

## Data Handling

- Encryption keys are never stored alongside data; managed by `IKeyProviderPtr`
- HSM integration available via `IKeyProviderPtr` → `vault_key_provider.cpp`
- PII fields can be encrypted at different key granularity than non-PII fields

## Known Limitations

- Erasure coding in `BlobRedundancyManager` is implemented (PARITY mode via `ErasureCodingBackend`, e.g., Reed-Solomon); default redundancy for critical blobs remains MIRROR unless PARITY is configured
- `NLPMetadataExtractor` may log document excerpts at DEBUG level — disable in production

## Dependency Security

- RocksDB: storage engine — version pinned; CVE monitoring active
- OpenSSL: AES-GCM and HMAC — version pinned via vcpkg
