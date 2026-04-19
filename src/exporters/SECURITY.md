> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Exporters Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Exporters module exports ThemisDB data to external formats (JSONL, Parquet, Arrow IPC, Hugging Face Datasets) and destinations (Hugging Face Hub). Security concerns focus on: authorization enforcement before export, PII detection and redaction, AES-256-GCM encryption for sensitive exports, secure Hub upload, and preventing export of unauthorized data.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Unauthorized data export | `enforceExportPolicy()` PolicyEngine check in all 6 exporters before opening cursor; `ERR_EXPORT_POLICY_DENIED` (9310) returned on denial |
| PII exfiltration in training data | `PiiDetector` identifies and redacts PII fields before export; configurable redaction strategy |
| Sensitive training data at rest | `ExportEncryption` provides AES-256-GCM encryption for export files; key rotation supported |
| Hugging Face Hub upload of unauthorized datasets | `HuggingFaceHubClient::HubUploadConfig` requires policy engine and audit log; authorization checked and audit-logged on all return paths (EXP-002) |
| AQL predicate injection via filter expression | `AQLPredicateFilter` validates and sandboxes predicate expressions before execution; raw filter strings are not eval'd |
| Cross-tenant data export | PolicyEngine authorization is tenant-scoped; export requests cannot access another tenant's collections |
| Incremental export cursor manipulation | Watermark and checkpoint values are server-side; client cannot advance the checkpoint arbitrarily |
| Export flooding / DoS | Streaming exporter enforces configurable max export rate; long-running exports use async job API |
| Template injection in format templates | Format templates are validated via `validateTemplate()` dry-run before use; field references are validated against known schema |

## Security Controls

### Authorization Enforcement
- `enforceExportPolicy()` is called in all 6 exporters (JSONL, Parquet, Arrow IPC, Hugging Face, Streaming, Incremental) before the export cursor is opened.
- PolicyEngine evaluates access against the requesting user's RBAC roles and ABAC attributes.
- Denied exports are logged with the denial reason; no partial data is returned.

### PII Detection and Redaction
- `PiiDetector` uses configurable patterns (regex, NER) to identify PII fields in exported records.
- Redaction strategies: mask (replace with `[REDACTED]`), drop (omit field), hash (pseudonymization).
- Redaction is applied before data reaches the serialization layer.

### Export Encryption
- `ExportEncryption` (AES-256-GCM) protects export files at rest.
- Encryption key is injected via configuration; not included in export metadata.
- Authenticated encryption detects tampering of encrypted export files.

### Hugging Face Hub Upload
- All Hub upload operations go through `HuggingFaceHubClient` which enforces PolicyEngine authorization.
- Hub API tokens are injected via configuration; not logged or returned in API responses.
- Audit log records every upload attempt including outcome and requesting user.

### Template Validation
- `validateTemplate()` dry-run validates instruction-tuning templates against schema before registering.
- Template field references are validated against known field names; unknown field references are rejected.

## Data Handling

- Export files may contain PII; PII detection is applied before serialization when configured.
- AES-256-GCM encrypted exports protect data at rest; operators must manage encryption keys.
- Incremental export checkpoints are stored server-side; clients receive a checkpoint token only.
- Hugging Face Hub uploads transmit data over HTTPS; operator responsible for Hub repository access controls.
- Export progress metrics (records exported, bytes written) do not include record content.

## Known Limitations

- Cross-collection join export (Issue #1722) is not yet implemented; single-collection export only.
- PII detection accuracy depends on configured patterns; novel PII types may not be automatically detected.
- Export file encryption key management is operator responsibility; no built-in key escrow.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| Apache Arrow / Parquet | Columnar export | Keep patched |
| libcurl | Hugging Face Hub upload | TLS with certificate validation |
| OpenSSL / libcrypto | AES-256-GCM export encryption | System-provided; keep patched |
