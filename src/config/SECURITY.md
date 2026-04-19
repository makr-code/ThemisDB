> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Config Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Config module manages configuration path resolution, schema validation, encrypted configuration storage, and audit logging. Security concerns focus on: preventing path traversal via configuration keys, securing encrypted config values at rest, preventing SSRF via schema `$ref` resolution, and protecting the audit log from tampering.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Path traversal via config key | `..` normalization and absolute-path rejection in `ConfigPathResolver`; all resolved paths validated against config root |
| Symlink escape outside config root | Symlink target checked against config root; symlinks resolving outside are rejected |
| SSRF via JSON Schema `$ref` | SSRF guard in `ConfigSchemaValidator` rejects `$ref` values containing external URIs (HTTP/HTTPS/FTP); only document-internal JSON Pointer refs are allowed |
| Encrypted config key leakage | AES-256-GCM encryption in `ConfigEncryptedStore`; key rotation supported; keys injected via environment or secrets manager |
| Audit log tampering | `ConfigAuditLog` is a bounded in-memory ring-buffer; entries cannot be modified after insertion; exportable to persistent append-only audit log |
| LRU cache poisoning via malicious config paths | Cache key includes full normalized path; only successfully resolved and validated paths are cached |
| Environment variable injection | `THEMIS_CONFIG_CACHE_SIZE` and `THEMIS_CONFIG_CACHE_TTL` values are validated as positive integers before use |
| Schema validation bypass via `$ref` cycles | Cycle detection in `ConfigSchemaValidator` prevents infinite recursion during `$ref` resolution |
| Legacy path exposure in Prometheus metrics | Metrics expose counts only (hit rate, miss rate, legacy fallback rate); no config key values or resolved paths are exposed |

## Security Controls

### Path Traversal Prevention
- All config key lookups are passed through `ConfigPathResolver` which normalizes paths and rejects `..` components.
- Absolute path requests are rejected.
- Symlink targets are validated against the config root using `realpath()` before the file is accessed.

### Encrypted Configuration Storage
- `ConfigEncryptedStore` provides AES-256-GCM encryption for sensitive configuration values (API keys, passwords, secrets).
- Key rotation is supported without service restart.
- Encrypted store serializes with authenticated encryption; tampered ciphertext is rejected.

### Schema Validation Security
- `ConfigSchemaValidator` enforces a JSON Schema Draft 7 subset including `format` keyword validation.
- `$ref` resolution is restricted to document-internal JSON Pointers; external URI refs trigger an SSRF guard error.
- Schema cycle detection prevents stack overflow attacks via deeply nested `$ref` chains.

### Audit Trail
- `ConfigAuditLog` records every config path access with timestamp and operation type.
- Ring-buffer design prevents unbounded memory growth; oldest entries are dropped when capacity is reached.
- Audit entries are not modifiable after insertion.

## Data Handling

- Config values resolved by this module may contain sensitive data (connection strings, API keys); these are never logged.
- `ConfigEncryptedStore` ciphertexts are stored on disk; plaintext values exist only in memory during the decryption call.
- Audit log entries contain config path names and timestamps — not config values; safe to export to external audit systems.
- Prometheus metrics export counts only; no config values or paths are included in metric labels.

## Known Limitations

- `ConfigEncryptedStore` key management relies on the operator to protect the encryption key; no key escrow is provided.
- Deprecation warning aggregation background thread logs deprecated path names to the application logger; ensure log shipping is secured.
- Removal of all deprecated legacy paths (Issue #1665) is pending; legacy paths remain accessible until migration is complete.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| OpenSSL / libcrypto | AES-256-GCM for `ConfigEncryptedStore` | System-provided; keep patched |
| yaml-cpp / nlohmann-json | YAML/JSON parsing in `ConfigSchemaValidator` | Input size limits recommended |
