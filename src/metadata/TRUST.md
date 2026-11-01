# Metadata Trust and Security Boundaries

**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-11-01
**Status:** review
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Source level:** level1 — module-local
**SOT domain:** security
**Canonical source references:** [security provider](../../include/metadata/imetadata_security_provider.h), [encryption provider](../../include/metadata/imetadata_encryption_provider.h), [metadata security tests](../../tests/metadata/test_metadata_security_provider.cpp)
**Related issue / milestone:** [#7133](https://github.com/makr-code/ThemisDB/issues/7133) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

## Trust boundary

Metadata can reveal schema names, field names/types, statistics, audit history,
lineage, and external-catalog payloads. Treat this information as potentially
sensitive even when it contains no row values. This page records source-visible
controls and limits; it is not a security certification or evidence that every
caller enforces those controls.

## Implemented provider behavior

### Authorization

- `IMetadataSecurityProvider` defines `hasPermission()` and
  `assertPermission()` decisions for named principals, operations, and
  resources.
- `InMemoryRbacMetadataSecurityProvider` supports exact grants, `"*"` resource
  grants, and `ADMIN` grants; its state is in-memory and protected by a mutex.
- `NoOpMetadataSecurityProvider` permits every operation. It is not an
  authorization policy for deployments that require access control.
- These provider implementations are available building blocks and are not
  enabled automatically. `HttpServer::Config` has an optional
  `metadata_security_provider`; when supplied, the server injects it and its
  `AuthMiddleware` into
  [`SchemaApiHandler`](../../include/server/schema_api_handler.h).
- Each schema/metadata handler checks the configured provider before reading or
  mutating state. The handler validates a bearer token, uses the authenticated
  `user_id` as the principal, and maps the HTTP verb/path to a
  `MetadataOperation` and table resource. Missing/disabled authentication
  returns 401; provider denial returns 403. Bulk/schema-wide requests use `"*"`
  as the resource and therefore need a matching wildcard grant.
- A null provider preserves the prior behavior; this is an opt-in gate, not a
  claim that deployments are protected by default. `NoOpMetadataSecurityProvider`
  permits every operation if a caller explicitly configures it.
- The gate covers calls through `SchemaApiHandler`. Direct C++ use of metadata
  classes does not pass through this HTTP authorization boundary and must be
  secured by its caller.

Tests for allow/deny, wildcard, admin, revocation, exception details, and
concurrent use are in
[`test_metadata_security_provider.cpp`](../../tests/metadata/test_metadata_security_provider.cpp).
HTTP identity, resource-specific denial, write-operation enforcement, and
default compatibility are covered by
[`test_schema_api_metadata_security.cpp`](../../tests/metadata/test_schema_api_metadata_security.cpp).

### Encryption

- `NoOpMetadataEncryptionProvider` passes values through unchanged.
- `FieldSetMetadataEncryptionProvider` uses repeating-key XOR and the interface
  explicitly warns it is demonstration/test/development only, not
  cryptographically secure.
- `AES_GCM_256` is an algorithm identifier for an external implementation; the
  header does not implement AES-GCM or key management.
- A field-selection policy alone does not encrypt data: callers must check and
  use the provider. Confirm that boundary for any production use.

See [`imetadata_encryption_provider.h`](../../include/metadata/imetadata_encryption_provider.h)
and [`test_metadata_encryption_provider.cpp`](../../tests/metadata/test_metadata_encryption_provider.cpp).

## Data integrity and provenance limits

- Schema validation and explicit error results are available; callers must
  handle them and must not silently accept failed updates.
- `SchemaAuditLog` is documented as an append-only RocksDB audit history, but
  the existence of the log API does not prove every security-sensitive action
  is recorded.
- `ColumnLineageTracker` entries are append-only in its in-memory tracker, but
  the tracker has no persistence dependency. Durability requires a caller-owned
  store.
- `IMetadataExportPolicy` exists as a standalone interface, but
  [`CatalogExporter`](../../include/metadata/catalog_exporter.h) does not accept
  it in its constructor or publish API. Do not assume filtering or delay
  behavior is applied to publish calls; an integration must explicitly enforce
  any required policy.
- External catalog publication may disclose names/types and uses configured
  credentials. Verify endpoint and credentials at the integration boundary.
- Never infer production security from a build maturity banner, a no-op default,
  or unit tests alone.

## Requirements versus runtime evidence

[`SECURITY.md`](./SECURITY.md) and
[`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) state module
security requirements and expectations. They do not prove that runtime HTTP
handlers or every caller enforce those requirements. Establish enforcement from
the actual handler/caller wiring and tests for each access path.

## Review checklist

1. Trace authorization and encryption from the caller to the sensitive
   operation; record any bypass path.
2. Confirm that denied permission produces an explicit failure and does not
   leak schema, statistics, audit, or export payloads.
3. Confirm deployment uses a real approved encryption provider and secure key
   management; do not use the XOR implementation for production secrets.
4. Verify lineage/audit persistence and retention requirements at the system
   integration boundary.
5. Use [`SECURITY.md`](./SECURITY.md) for module-level context and the
   repository [`SECURITY.md`](../../SECURITY.md) for vulnerability reporting.
